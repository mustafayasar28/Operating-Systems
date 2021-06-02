#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>


static inline unsigned myrandomlt52() {
   long l;
   do { l = random(); } while (l>=(RAND_MAX/52)*52);
   return (unsigned)(l % 52);
}

int main(int argc, char *argv[])
{	
	int M = atoi(argv[1]);
	for (int i = 0; i < M; i++) {
		char randomletter = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"[myrandomlt52 () % 52];
		printf("%c\n", randomletter);
	} 
	return 0;
}
