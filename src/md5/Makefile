PATH1="."

CC=gcc
LIBS=-lpthread 
OBJDIR ?= ../compiled

db_deps = md5.h
db_obj = md5.o

all: lib

lib: $(db_obj) $(db_deps)

clean:
	rm -f $(db_obj)

%.o: %.c $(db_deps)
	$(CC) -c $(OPTS) $< -o $@
