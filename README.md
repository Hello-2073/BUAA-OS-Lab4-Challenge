# LAB4 挑战性任务实现报告

## 1 线程实现

### 1.1 线程机制

我采用的线程实现机制是“轻量级进程”，即共享其它进程的内存的进程，均使用`struct Env`控制。

为了实现这一点，我进行了以下设置：

- 为`struct Env`增加`env_tid`成员。`env_tid`的含义是线程组ID，对于同一进程的若干线程是相同的，均为该进程第一个线程的ID；

- 为`struct Env`增加`env_stack_map`成员。`env_stack_map`有两种含义：

  - 对于`env_id == env_tid`的，表示该进程用户栈的分配情况位图，其本身的栈位于用户栈顶；
  - 对于`env_id != env_tid`的，表示该线程所分配到的用户栈的位置；

  栈空间布局如下。用户空间自顶开始共有8个栈，每个栈大小为一页。每个栈之下附带一页无效内存，一来是安全起见，二来是为了在某些时候作该线程的私有内存使用（例如IPC时用作该线程的共享页，详见信号量部分）。

  ```c
   o   +----------------------------+------------0x7f40 0000    |
   o   |     user exception stack   |     BY2PG                 |
   o   +----------------------------+------------0x7f3f f000    |
   o   |       Invalid memory       |     BY2PG                 |
   o   +----------------------------+------------0x7f3f e000    |
   o   |     normal user stack0     |     BY2PG                 |
   o   +----------------------------+------------0x7f3f d000    |
   o   |       Invalid memory       |     BY2PG                 |
   o   +----------------------------+------------0x7f3f c000    |
   o   |     normal user stack1     |     BY2PG                 |
   o   +----------------------------+------------0x7f3f b000    |
   o   |       Invalid memory       |     BY2PG                 |
   o   +----------------------------+------------0x7f3f a000    |
   o   |                            |                           |
   o   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                           |
   o   .                            .                           |
   o   .                            .                           |
   o   .                            .                           |
   o   |~~~~~~~~~~~~~~~~~~~~~~~~~~~~|                           |
   o   |                            |                           |
   o   +----------------------------+------------0x7f3f 0000    |
   o   |     normal user stack7     |     BY2PG                 |
   o   +----------------------------+------------0x7f3e f000    |
   o   |       Invalid memory       |     BY2PG                 |
   o   +----------------------------+------------0x7f3e e000    |
   o   |                            |                           |
  ```

- 为`struct Env`增加`env_join_id`与`env_retval`成员。
- 为`env_status`增加`ENV_JOINT`状态。此状态下该进程只有等待目标线程结束才可以改变状态为`ENV_RUNNABLE`。

### 1.2 函数实现

- `pthread_create()`

  - **内核函数修改：`env_alloc()`**

    增加`int alloc_mem`参数，指定是否为新创建的进程/线程分配内存。

    若分配内存，则为创建一个新的进程：

    - 初始化`env_tid = env_id`，表示该线程代表了该进程，是该进程的主进程；
    - 初始化`env_stack_map = 1`，表示分配到的是最顶部的栈；

    若不分配内存，则为创建一个新的线程：

    - 要求参数`parent_id`有效，并由其找到父线程的`env_tid`，再找到`env_tid`对应的主线程；
    - 初始化`env_stack_map`，从主线程的`env_stack_map`找到一空闲位，设置二者该位为1；
    - 初始化`env_tf.regs[29]`为对应栈顶；
    - 初始化`env_pgdir`为主线程的页目录，并将对应物理页的`pp_ref`加一；

  - **用户接口实现：**

    使用相应系统调用调用以上内核函数（系统调用函数也做了相应修改）。

    ```C
    int 
    pthread_create(pthread_t *thread, 
                   const pthread_attr_t *attr,
                   void *(*start_routine)(void *), 
                   void *arg)
    {
        u_int newenvid = syscall_env_alloc(0);
        if (newenvid == 0) {
            // 为创建的新线程，进入入口函数
            (*start_routine)(arg);
            exit(0);
        }
        // 为父线程，启动子线程
        syscall_set_env_status(newenvid, ENV_RUNNABLE);
        *thread = newenvid; 
        return 0;
    }
    ```

- `pthread_exit()`

  - **内核函数修改：`env_destroy()`**

    增加`void *retval`参数，作为线程结束的返回值。

    - `Env`的`env_joint_id`，`env_retval`初始化为0，在`pthread_join()`时被设置；
    - 若`env_joint_id`有效，修改对应`Env`的状态为就绪；
    - 若`env_retval`不为空，将其所指向内容设为参数`retval`；

  - **内核函数修改`env_free()`**

    增加释放资源时机的检测。

    - 当仅有当前释放的线程引用页目录时，可以将页目录释放；
    - 无论何时都要在其对应主线程(tid对应线程)除释放`env_stack_map`;
    - 若当前释放的线程是进程的代表(id = tid)，且还有子线程未释放，则暂不释放其控制块，而将其状态设置为`ENV_JOINT`;
    - 若当前释放的线程不是进程的代表，则获得其主线程Env，若其`env_stack_map`为0，则说明该进程所有线程已释放，释放其主线程Env；

  - **用户接口实现**

    使用相应系统调用调用以上内核函数（系统调用函数也做了相应修改）。

    ```c
    void 
    pthread_exit(void *retval) 
    {
        syscall_env_destroy(0, retval);
    }
    ```

- `pthread_cancel()`

  - **用户接口实现**

    类似`pthread_exit()`，但指定退出线程不是自己。并且设定退出返回值为宏` PTHREAD_CANCELED`。

    ```c
    int
    pthread_cancel(pthread_t thread) {
        return syscall_env_destroy(thread, PTHREAD_CANCELED);
    }
    ```

- `pthread_join()`

  - **内核函数修改：sys_env_join()**

    增加了一个新的系统调用。

    ```c
    int 
    sys_env_join(int sysno, u_int envid, void **retval)
    {
        struct Env *e;
        int r;
    
        do {								
        	r = envid2env(envid, &e, 0);  // 这个循环的意义是：
        	if (r < 0) return r;		  //	若目标线程已设置了后继
            envid = e->env_joint_id;	  //  则当前进程顺延到其之后
        } while(e->env_joint_id != 0);		 
            
        e->env_joint_id = curenv->env_id; // 设置为后继
        e->env_retval = retval;			  // 设置返回值地址
    
        curenv->env_status = ENV_JOINT;	  // 阻塞当前进程
    
        return 0;
    }
    ```

  - **用户接口实现**

    ```c
    int 
    pthread_join(pthread_t thread, void **retval) {
        int r;
        r = syscall_env_join(thread, retval);
        if (r) return r;	// 没有找到对应线程，异常退出
        
        syscall_yield();	// 成功阻塞，放弃cpu
        return 0;
    }
    ```

    

## 2 信号量实现

### 2.1 信号量机制

我模仿Lab5中的文件系统，使用一个用户进程来提供信号量服务。

为了实现这一点，我进行了以下工作：

- 扩展IPC功能至多线程：

  - 由于此时还未实现互斥机制，所以每个进程需要有专用的IPC共享页。这里参见栈分配部分的 Invalid Memory；

- 实现信号量服务进程：

  - **信号量数据结构定义**

    ```c
    typedef struct Sem {
        int value;					// 信号量的值
        int shared;					// 是否进程间共享
        struct Env_list queue;		// 进程阻塞队列
        LIST_ENTRY(Sem) sem_link;	// 链表项
    } Sem;
    
    // 信号量引用：
    //  这是信号量服务进程中信号量的ID，在其它进程里仅仅是一个整数
    typedef u_int sem_t;			
    ```

  - **内存分配**

    在信号量服务进程的用户空间中开辟一块作为信号量资源。

    ```c
    #define NSEM 32
    #define SEMMAP 0x10000000
    #define SEMMAX (SEMMAP + NSEM * sizeof(Sem))
    
    Sem *sems = (Sem *)SEMMAP;
    static struct Sem_list sem_free_list;
    
    void 
    ss_init()
    {
        int i;
        syscall_mem_alloc(0, sems, PTE_R | PTE_V);
        LIST_INIT(&sem_free_list);
        for (i = NSEM - 1; i >= 0; i--) {
            LIST_INSERT_HEAD(&sem_free_list, &sems[i], sem_link);
        }
    }
    ```

  - **服务框架**

    模仿Lab5文件系统的实现，我建立了类似的服务框架。

    ```c
    /* 服务端 */
    void
    serve()
    {
        u_int req, whom, perm;
    
        for (;;) {
            req = ipc_recv(&whom, REQVA, &perm);
            
            if (!(perm & PTE_V)) continue;
    
            switch(req) {
                case SEMREQ_*:
                    serve_sem_*(whom, (struct Semreq *)REQVA)
                    break;
                default:
                    break;
            }
            syscall_mem_unmap(0, REQVA);
        }
    }
    
    
    /* 用户端 */
    int
    semipc(u_int type, void *semreq, u_int dstva, u_int *perm)  
    {
        u_int whom;
        ipc_send(envs[0].env_id, type, (u_int)semreq, PTE_V | PTE_R);
        return ipc_recv(&whom, dstva, perm);
    }
    ```

### 2.2 函数实现

- `sem_init()` 

  - **信号量服务进程函数**

  ```c
  int
  ss_sem_init(u_int value)
  {
      Sem *psem;
      if (LIST_EMPTY(&sem_free_list)) {   
          user_panic("no free sem\n\n");
          return 0;
      }
      psem = LIST_FIRST(&sem_free_list);
      LIST_REMOVE(psem, sem_link); // 获取空闲链表第一个信号量
      psem->value = value;
      LIST_INIT(&((psem)->queue));
      return (psem - sems) | (1 << 7);
  }
  ```

  - **用户端接口函数**

  ```c
  int 
  sem_init(sem_t *sem, int pshared, u_int value)
  {
      // 获得Invalid Memory
      struct Semreq_init *req = 
          (struct Semreq_init *)syscall_get_thread_page();
      req->pshared = pshared;
      req->value = value;
      *sem = semipc(SEMREQ_INIT, req, 0, 0);   
      return sem ? 0 : -1;
  }
  ```

- `sem_destroy()` 

  - **信号量服务进程函数**

  ```c
  void
  ss_sem_destroy(sem_t semid)
  {
      Sem *psem = sems + (semid & 0x7f);
      LIST_INSERT_HEAD(&sem_free_list, psem, sem_link);
  }
  ```

  - **用户端接口函数**

  ```c
  int 
  sem_destroy(sem_t *sem)
  {
      struct Semreq_destroy *req = 
          (struct Semreq_destroy *)syscall_get_thread_page();
      req->semid = *sem;
      return semipc(SEMREQ_DESTROY, req, 0, 0);
  }
  ```

- **sem_wait()**

  - **信号量服务进程函数**

  ```c
  int
  ss_sem_wait(sem_t semid, u_int envid)
  {
      Sem *psem = sems + (semid & 0x7f);
      psem->value--;
      if (psem->value >= 0) {
          return 0;
      } else {
          LIST_INSERT_TAIL(&(psem->queue), 
                &envs[ENVX(envid)], env_link);	// 进入阻塞队列
          return -1;
      }
  }
  ```

  - **用户端接口函数**

  ```c
  int
  sem_wait(sem_t *sem)
  {
      struct Semreq_wait *req = 
          (struct Semreq_wait *)syscall_get_thread_page();
      req->semid = *sem;
      int r;
      r = semipc(SEMREQ_WAIT, req, 0, 0);
      if (r < 0) {
          syscall_set_env_status(0, ENV_NOT_RUNNABLE);
          syscall_yield(); // 阻塞
      }
      return 0;
  }
  ```

- **sem_trywait()**

  - **信号量服务进程函数**

  ```c
  int
  ss_sem_trywait(sem_t semid, u_int envid)
  {
      Sem *psem = sems + (semid & 0x7f);
      if (psem->value > 0) {
          psem->value--;
          return 0;
      } else {
          return -1;	// 没有资源时不阻塞
      }
  }
  ```

  - **用户端接口函数**

  ```c
  int sem_trywait(sem_t *sem)
  {
      struct Semreq_wait *req = 
          (struct Semreq_wait *)syscall_get_thread_page();
      req->semid = *sem;
      return semipc(SEMREQ_TRYWAIT, req, 0, 0); // 不阻塞，返回错误
  }
  ```

- **sem_post()**

  - **信号量服务进程函数**

  ```c
  void
  ss_sem_post(sem_t semid)
  {
      Sem *psem = sems + (semid & 0x7f);
      struct Env *e;
      psem->value++;
      if (psem->value <= 0) {
          e = LIST_FIRST(&(psem->queue));
          LIST_REMOVE(e, env_link);
          e->env_status = ENV_RUNNABLE; // 唤醒阻塞队列队首
      }
  }
  ```

  - **用户端接口函数**

  ```c
  int sem_post(sem_t *sem)
  {
      struct Semreq_post *req = 
          (struct Semreq_post *)syscall_get_thread_page();
      req->semid = *sem;
      return semipc(SEMREQ_POST, req, 0, 0);
  }
  ```

- **sem_getvalue()**

  - **信号量服务进程函数**

  ```c
  int
  ss_sem_getvalue(sem_t semid)
  {
      return sems[semid & 0x7f].value;
  }
  ```

  - **用户端接口函数**

  ```c
  int sem_getvalue(sem_t *sem)
  {
      struct Semreq_getvalue *req = 
          (struct Semreq_getvalue *)syscall_get_thread_page();
      req->semid = *sem;
      return semipc(SEMREQ_GETVALUE, req, 0, 0);
  }
  ```

## 3 测试

### 3.1 线程测试

- 共享资源访问测试：Dog()与Cat()轮流计数，使用带`syscall_yield()`的轮询互斥。

```c
int turn = 0;
int times = 0;

void dog() { 
    while(1) {
        while(turn) syscall_yield();
        writef("Wang : %d\n", times++);
        if (times > 3) {
        	turn = 1;
            break;
        } else { 
            turn = 1;
        }
    }
}

void cat() { 
    while(1) {
        while(!turn) syscall_yield();
        writef("Miao : %d\n", times++);
        if (times > 3) {
            turn = 0;
            break;
        } else {
            turn = 0;
        }
    }
}
```

- 线程返回值测试：Counter()将计数结果返回给Umain()

```c
void counter(void *arg) {
    int num = *(int *)arg, i, j;
    writef("Thread Counter() begins to count: ");
    for (i = 0; i < num; i++) {
        writef("%d, ", i);
        syscall_yield();
    }
    writef("... \n");
    writef("Thread Counter() trys to return %d\n", i);
    pthread_exit(&i);
}
```

- 线程撤销测试：Umain()撤销死循环线程Speaker()

```c
void speaker() {
    while(1) {
        writef("Speaker() will never dead!!!\n");
        syscall_yield();
    }
}
```

- 主程序Umain()

```c
void umain() {
    writef("\n---------------- thread test begin ----------------\n");
    pthread_t thread1, thread2, thread3, thread4;
    int num = 10,  *retval;
    pthread_create(&thread1, 0, dog, NULL);
    pthread_create(&thread2, 0, cat, NULL);
    if (pthread_join(thread1, NULL) == 0) {
        writef("Umain() recovers after thread Dog() exited.\n");
    } else {
        writef("Thread Dog() exited before Umain() try to join.\n");
    }
    if (pthread_join(thread2, NULL)) {
        writef("Umain()recovers after thread Cat() exited.\n");
    } else {
        writef("Thread Cat() exited before Umain() try to join.\n");
    }
    pthread_create(&thread3, 0, counter, &num);
    if (pthread_join(thread3, &retval) == 0) {
        writef("Umain() gets %d from thread counter, and %d is expected.\n", *retval, num);
    }
    pthread_create(&thread4, 0, speaker, NULL);
    syscall_yield();
    if (pthread_cancel(thread4) == 0) {
        writef("Umain() shut down thread Speaker()!!!");
    }
    writef("\n---------------- thread test end ------------------\n");
}
```

- 运行结果
  - 【Cat()与Dog()轮流计数】：完成
  - 【Umain()使用pthread_join()阻塞到Dog()结束】：完成
  - 【Umain()使用pthread_join()阻塞到Dog()结束】：完成
  - 【Counter()使用pthread_exit()结束并返回10】：完成
  - 【Umain()使用pthread_join()获得Counter()返回值】：完成
  - 【Umain()使用pthread_cancel()结束Speaker()】：完成

```sh
---------------- thread test begin ---------------- 

Wang : 0

Miao : 1

Wang : 2

Miao : 3

Wang : 4

Umain() recovers after thread Dog() exited.	

Umain() recovers after thread Cat() exited.	

Thread Counter() begins to count: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ... 

Thread Counter() trys to return 10	

Umain() gets 10 from thread counter, and 10 is expected.

Speaker() will never dead!!!

Umain() shut down thread Speaker()!!!

---------------- thread test end ------------------
```

### 3.2 信号量测试

- 信号量集与共享资源

```c
sem_t mutex_rn;		// 用于rn的互斥访问
sem_t mutex_rw;		// 用于读者-写者互斥访问
sem_t mutex_io;		// 用于IO资源的互斥访问
sem_t mutex_zero;	// 用于退出控制
int rn = 0;			// 缓冲区读者数
char buf[100] = {'\0'}; // 缓冲区
```

- 写者

```c
void writer(char name[])
{
    int num = 10, i, j, t;
    while (num < 20) {
        sem_wait(&mutex_rw); // 读写锁
        i = num++;
        j = 0;
        do {
            buf[j++] = '0' + i % 10;
            i /= 10;
        } while(i);
        buf[j] = '\0';
        sem_wait(&mutex_io); // IO互斥
        writef("[%s] writes [%s] to buf.\n", name, buf);
        sem_post(&mutex_io);
        sem_post(&mutex_rw);
    }
}
```

- 读者

```c#
void reader(char name[])
{
    while(1) {
        sem_wait(&mutex_rn);
        if (rn == 0)
            sem_wait(&mutex_rw); // 首个进入的读者，取读写锁
        rn++;
        sem_post(&mutex_rn); // 写者数量互斥
        sem_wait(&mutex_io); // IO互斥
        writef("[%s] reads [%s] from buf.\n", name, buf);
        sem_post(&mutex_io);
        sem_wait(&mutex_rn);
        rn--;
        if (rn == 0)
            sem_post(&mutex_rw); // 所有读者离开，还读写锁
        sem_post(&mutex_rn);
        if (sem_trywait(&mutex_zero) == 0) {
            break;	// 由umain()控制的退出
        }
    }
    sem_post(&mutex_zero);
}
```

- 主程序

```c
void umain()
{
    writef("---------------- semaphore test begin ------------------");
    pthread_t thread1, thread2, thread3;
    sem_init(&mutex_rw, 0, 1);
    sem_init(&mutex_rn, 0, 1);
    sem_init(&mutex_io, 0, 1);
    sem_init(&mutex_zero, 0, 0);

    pthread_create(&thread1, 0, writer, "writer1");
    pthread_create(&thread2, 0, reader, "reader2");
    pthread_create(&thread3, 0, reader, "reader3");

    pthread_join(thread1, 0); // 等待写者结束

    sem_post(&mutex_zero);	  // 允许读者退出
    
    sem_wait(&mutex_io);
    writef("----------------- semaphore test end -------------------");
    sem_post(&mutex_io);
}
```

- 运行结果
  - 【读写互斥】完成
  - 【IO互斥】完成
  - 【非阻塞wait(mutex_zero)】完成

```c
---------------- semaphore test begin ------------------

[writer1] writes [01] to buf.

[writer1] writes [11] to buf.

[writer1] writes [21] to buf.

[writer1] writes [31] to buf.

[writer1] writes [41] to buf.

[reader2] reads [41] from buf.

[reader3] reads [41] from buf.
    
 ...

[writer1] writes [51] to buf.

[writer1] writes [61] to buf.

[writer1] writes [71] to buf.

[writer1] writes [81] to buf.

[writer1] writes [91] to buf.

[reader2] reads [91] from buf.

[reader2] reads [91] from buf.

[reader2] reads [91] from buf.

[reader3] reads [91] from buf.

 ...

----------------- semaphore test end -------------------
```

## 4 遇到的困难及解决

- **线程退出会直接释放进程的内存**

  通过页目录的`pp_ref`判断内存释放的合适时机；

- **进程主线程结束将失去进程的栈位图**

​		主线程结束时不释放Env，而是由进程线程组最后一个线程退出时释放；

- **没有合适的内存来为信号量分配**

  使用用户进程实现信号量服务，有大量自由的堆空间；

- **多线程使用IPC与信号量进程通信时无法互斥**

​		在每个线程的栈下设置一页Invalid Memory，作为其独有的内存。
