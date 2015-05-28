PATH1="."
CC=gcc

LIBS=-lpthread
LIBS+=-lm
LIBS+=-lmysqlclient
LIBS+=-lrt

distrooptsx32=-m32 -mtune=i686 -Wall -O2
distrooptsx64=-m64 -mtune=generic -Wall -O2
distroheaders=ap_log.h ap_str.h ap_utils.h 
distrotexts=README LICENSE

optsdebug=-Wall -ggdb -O2
optsrelease=-Wall -O2

outname=libapstoolkit

obj=ap_log.o ap_str.o ap_utils.o

# by default we make debug compile
all: OPTS=$(optsdebug)
all: lib compiletests

release: OPTS=$(optsrelease)
release: lib compiletests

doxygen:
	rm -rf doxydoc
	doxygen Doxyfile

prepdistro:
	rm -rf $(outname)
	mkdir $(outname)
	cp $(distroheaders) $(outname)/
	cp -R doc $(outname)/
#cp -R doxydoc/html $(outname)/doc/
	cp $(distrotexts) $(outname)/
	make clean lib OPTS="$(distroopts$(arch))"
	strip $(outname).a
	mv $(outname).a $(outname)/
	tar cf - $(outname) | bzip2 > $(outname)_$(arch)_`git tag`_`git log -1 --pretty --format=%h`.tar.bz2
	rm -rf $(outname)

lib32:
	make prepdistro arch=x32

lib64:
	make prepdistro arch=x64

distro:
	make doxygen lib32 lib64

lib: $(obj)
	rm -f $(outname).a
	make -C api_net $@ OPTS="$(OPTS)"
	make -C ap_error $@ OPTS="$(OPTS)"
	make -C list $@ OPTS="$(OPTS)"
	make -C api_comm $@ OPTS="$(OPTS)"
	make -C fifo_queue $@ OPTS="$(OPTS)"
	make -C app $@ OPTS="$(OPTS)"
	make -C download $@ OPTS="$(OPTS)"
	make -C alarm $@ OPTS="$(OPTS)"
	make -C db_op $@ OPTS="$(OPTS)"
	make -C cJSON $@ OPTS="$(OPTS)"
	make -C md5 $@ OPTS="$(OPTS)"
	ar rcs $(outname).a *.o ap_error/*.o api_net/*.o api_net/connection_pool/*.o list/*.o list/uthash/*.o\
	       api_comm/*.o fifo_queue/*.o app/*.o download/*.o alarm/*.o db_op/*.o cJSON/*.o md5/*.o

%.o: %.c
	$(CC) -c $(OPTS) $< -o $@

compiletests: $(obj) ./$(libname)
	$(CC) $(OPTS) main.c -o switch_pro -l:./$(outname).a $(LIBS)

clean:
	rm -f *.o $(outname)*.a switch_pro
	make -C ap_error clean
	make -C api_net clean
	make -C list clean
	make -C api_comm clean
	make -C fifo_queue clean
	make -C app clean
	make -C download clean
	make -C alarm clean
	make -C db_op clean
	make -C cJSON clean
	make -C md5 clean
