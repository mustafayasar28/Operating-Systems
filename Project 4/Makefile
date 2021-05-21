


all: libsimplefs.a create_format app

libsimplefs.a: 	simplefs.c
	gcc -Wall -c simplefs.c -lm
	ar -cvq libsimplefs.a simplefs.o
	ranlib libsimplefs.a

create_format: create_format.c
	gcc -Wall -o create_format  create_format.c   -L. -lsimplefs -lm
	
test: test.c
	gcc -Wall -o test test.c -L. -lsimplefs -lm

app: 	app.c
	gcc -Wall -o app app.c  -L. -lsimplefs -lm

clean: 
	rm -fr *.o *.a *~ a.out app  vdisk create_format

