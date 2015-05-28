PATH1="."

CC=gcc
LIBS=-lpthread
OBJDIR ?= ../compiled

fifo_queue_deps=fifo_queue.h circular_queue.h
fifo_queue_obj = fifo_queue.o
fifo_queue_obj += circular_queue.o

all: lib

lib: $(fifo_queue_obj) $(fifo_queue_deps)

clean:
	rm -f $(fifo_queue_obj)

%.o: %.c $(fifo_queue_deps)
	$(CC) -c $(OPTS) $< -o $@
