PATH1="."

CC=gcc
LIBS=-lpthread
OBJDIR ?= ../compiled

api_comm_deps=download.h
api_comm_obj = download.o

all: lib

lib: $(api_comm_obj) $(api_comm_deps)

clean:
	rm -f $(api_comm_obj)

%.o: %.c $(api_comm_deps)
	$(CC) -c $(OPTS) $< -o $@
