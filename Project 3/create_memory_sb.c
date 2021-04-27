#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "sbmem.h"

int main(int args, char *argv[])
{
    if (args != 2) {
    	printf("create_memory_sb.c usage: ./create_memory_sb <SEGMENT SIZE>\n");
    	exit(1);
    }
    
    int segmentsize = atoi(argv[1]);
    
    sbmem_init(segmentsize); 

    printf ("memory segment is created and initialized \n");

    return (0);
}

