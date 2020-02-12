CC      = gcc
CFLAGS  = -g
TARGET  = cfs
OBJS    = simulator.o

$(TARGET):	$(OBJS)
		gcc -pthread -o cfs simulator.c
