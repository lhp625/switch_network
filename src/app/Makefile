PATH1="."

CC=gcc
LIBS=-lpthread
OBJDIR ?= ../compiled

app_deps=debug.h
debug_obj = debug.o paramdefs.o

all: lib

lib: $(debug_obj) $(app_deps)

clean:
	rm -f $(debug_obj)

%.o: %.c $(app_deps)
	$(CC) -c $(OPTS) $< -o $@
