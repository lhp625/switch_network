PATH1="."

CC=gcc
LIBS=-lpthread
OBJDIR ?= ../compiled

hashlist_deps=uthash.h my_uthash.h
hashlist_obj = my_uthash.o

all: lib

lib: $(hashlist_obj) $(hashlist_deps)

clean:
	rm -f $(hashlist_obj)

%.o: %.c $(hashlist_deps)
	$(CC) -c $(OPTS) $< -o $@
