PATH1="."

CC=gcc
LIBS=-lpthread
OBJDIR ?= ../compiled

api_comm_deps=api_net.h
api_comm_obj = api_net.o

all: lib

lib: $(api_comm_obj) $(api_comm_deps)
	make -C connection_pool $@ OPTS="$(OPTS)"

clean:
	rm -f $(api_comm_obj)
	make -C connection_pool clean

%.o: %.c $(api_comm_deps)
	$(CC) -c $(OPTS) $< -o $@
