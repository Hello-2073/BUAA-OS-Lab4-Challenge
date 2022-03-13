PATH_INC := ./include
PATH_SRC := ./code
PATH_OBJ := ./code
PATH_BIN := .

TARGET := fibo

TARGET : $(PATH_SRC)/main.c $(PATH_SRC)/fibo.c $(PATH_INC)/fibo.h
	@cd $(PATH_SRC) && make
	@gcc $(PATH_OBJ)/main.o $(PATH_OBJ)/fibo.o -o $(PATH_BIN)/$(TARGET) 

.PHONY:clean
clean : 
	@cd $(PATH_OBJ) && make clean
