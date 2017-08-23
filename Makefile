# 该项目旨在开发一款在linux上运行的多线程服务器。
# 该服务器使用TCP协议与多个客户端通过SSTP方式通讯,包含基于SHA-256算法的工作量证明求解器，
# 以实现可用于比特币的哈希现金挖掘算法

# Makefile for project 2
# COMP30023 Computer Systems Project 2
# Name: Yitao Deng
# User ID: yitaod
# Student ID: 711645

## CC  = Compiler.
## CFLAGS = Compiler flags.
CC	= gcc
CFLAGS =	-Wall -Wextra -std=gnu99


## OBJ = Object files.
## SRC = Source files.
## EXE = Executable name.

SRC =		server.c hash.c sha256.c
OBJ =		server.o hash.o sha256.o
EXE = 		server

## Top level target is executable.
$(EXE):	$(OBJ)
		$(CC) $(CFLAGS) -o $(EXE) $(OBJ) -lpthread


## Clean: Remove object files and core dump files.
clean:
		/bin/rm $(OBJ) 

## Clobber: Performs Clean and removes executable file.

clobber: clean
		/bin/rm $(EXE) 

## Dependencies

server.o: uint256.h sha256.h server.h
hash.o:	uint256.h sha256.h server.h
sha256.o: sha256.h

