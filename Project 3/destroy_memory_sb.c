

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "sbmem.h"

int main()
{

    sbmem_remove(); 

    printf ("memory segment is removed from the system. System is clean now.\n");
    printf ("you can no longer run processes to allocate memory using the library\n"); 

    return (0); 
}
