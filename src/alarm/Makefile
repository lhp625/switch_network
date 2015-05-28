PATH1="."

CC=gcc
LIBS=-lpthread
OBJDIR ?= ../compiled

alarm_deps=alarm_pro.h
alarm_obj = alarm_pro.o

all: lib

lib: $(alarm_obj) $(alarm_deps)

clean:
	rm -f $(alarm_obj)

%.o: %.c $(alarm_deps)
	$(CC) -c $(OPTS) $< -o $@
