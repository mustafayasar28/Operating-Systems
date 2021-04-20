all: libsbmemlib.a  app create_memory_sb destroy_memory_sb

libsbmemlib.a:  sbmemlib.c
	gcc -Wall -c sbmemlib.c -lrt -pthread -lm
	ar -cvq libsbmemlib.a sbmemlib.o
	ranlib libsbmemlib.a

app: app.c
	gcc -Wall -o app app.c -L. -lsbmemlib -lrt -pthread -lm

create_memory_sb: create_memory_sb.c
	gcc -Wall -o create_memory_sb create_memory_sb.c -L. -lsbmemlib -lrt -pthread -lm

destroy_memory_sb: destroy_memory_sb.c
	gcc -Wall -o destroy_memory_sb destroy_memory_sb.c -L. -lsbmemlib -lrt -pthread -lm

clean: 
	rm -fr *.o *.a *~ a.out  app sbmemlib.o sbmemlib.a libsbmemlib.a  create_memory_sb destroy_memory_sb
