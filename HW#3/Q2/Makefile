all: producer consumer

consumer: consumer.c
	gcc -Wall -g -o consumer consumer.c -lrt


producer: producer.c
	gcc -Wall -g -o producer producer.c -lrt

clean:
	rm -fr producer consumer *~ *.o   core*