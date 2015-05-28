PATH1="."

CC=gcc
LIBS=-lpthread 
LIBS+=-lm
OBJDIR ?= ../compiled

api_comm_deps=def_devstr.h api_login.h api_command.h api_shell_terminal.h api_monitor.h
api_comm_obj = api_login.o api_command.o api_shell_terminal.o api_monitor.o

all: lib

lib: $(api_comm_obj) $(api_comm_deps)

clean:
	rm -f $(api_comm_obj)

%.o: %.c $(api_comm_deps)
	$(CC) -c $(OPTS) $< -o $@
