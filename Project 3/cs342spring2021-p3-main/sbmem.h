#ifndef SBMEM_H
#define SBMEM_H


int sbmem_init(int segmentsize); 
int sbmem_remove(); 

int sbmem_open(); 
void *sbmem_alloc (int size);
void sbmem_free(void *p);
int sbmem_close(); 

#endif
    
