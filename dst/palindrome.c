#include<stdio.h>
int main()
{
	int n;
	scanf("%d",&n);

	int rev = 0;
	
	for (int tmp = n; tmp > 0; tmp /= 10) {
		rev *= 10;
		rev += tmp % 10;
	}

	if(rev == n){
		printf("Y");
	}else{
		printf("N");
	}
	return 0;
}
