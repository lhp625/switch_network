PATH1="."

CC=gcc
LIBS=-lpthread
OBJDIR ?= ../compiled

cJSON_deps=cJSON.h
cJSON_obj = cJSON.o create_array.o

all: lib

lib: $(cJSON_obj) $(cJSON_deps)

clean:
	rm -f $(cJSON_obj)

%.o: %.c $(cJSON_deps)
	$(CC) -c $(OPTS) $< -o $@
