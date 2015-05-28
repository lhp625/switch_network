PATH1="."

CC=gcc
LIBS=-lpthread 
LIBS+=-lmysqlclient
OBJDIR ?= ../compiled

db_deps = db_mysql.h
db_obj = db_mysql.o

all: lib

lib: $(db_obj) $(db_deps)

clean:
	rm -f $(db_obj)

%.o: %.c $(db_deps)
	$(CC) -c $(OPTS) $< -o $@
