PATH1="."

CC=gcc
LIBS=-lpthread
OBJDIR ?= ../compiled

list_deps=list.h
list_obj = list.o
list_obj += list_iterator.o
list_obj += list_node.o

all: lib

lib: $(list_obj) $(list_deps)
	make -C uthash $@ OPTS="$(OPTS)"

clean:
	rm -f $(list_obj)
	make -C uthash clean

%.o: %.c $(list_deps)
	$(CC) -c $(OPTS) $< -o $@
