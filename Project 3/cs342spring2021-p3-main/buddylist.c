#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>

#define MIN 7   //minimum block size
#define SHARED_MEM_NAME "/shared_mem_cm"

enum flag {Free , Taken };  //indicates whether a block is free or taken

//block representation
typedef struct Block{
    enum flag status;   //indicates status of block
    int level;  //size of block is indicated by its level, i.e. level 0 = 128 bytes of blocks
    struct Block * next;  //link to next block
    struct Block * prev; //link to prev block
} Block;

int levels = 0;

//gets buddy of a block
Block *buddyfinder(Block* block){
    int index = block->level;   //find the level
    long int mask = 0x1 << (index + MIN);
    return (Block*) ((long int) block ^ mask); 
}

//divides larger block and gets the second part of the block
Block *split(Block* block){
    int index = block->level-1;
    int mask = 0x1 << (index + MIN);
    return (Block*)((long int) block | mask);
}

//merge two buddies if they are free
//first block in a pair = primary block
//returns primary block
Block *merge_and_get_primary(Block* block, int segmentsize){
    int index = block->level;
    int f_size = log2(segmentsize) + MIN - 1;
    char hex[24 + 3] = "0x";
    for (int i = 0; i<f_size; i++){
        strcat(hex,"f");
    }
    
    long int mask = atoi(hex) << (1 + index + MIN);
    return (Block*)((long int) block & mask);
}

//hide the block info from user helper functions
void *hide(Block* block){
    return (void*)(block+1);
}

Block *magic(void*memory){
    return ((Block*) memory - 1);
}

//which block to allocate?
//we need slightly larger block than requested to hide block info struct
int level(int requested){
    int total = requested + sizeof(struct Block);

    int i = 0;
    int size = 1 << MIN;
    while(total > size){
        size <<= 1;
        i+=1;
    }
    return i;
}

int main() {

    int segmentsize;    //max block space = segment size
    printf("Enter segment size: \n");
    scanf("%d", &segmentsize);

    //find number of levels
    levels = log2(segmentsize / 128) + 1;
    printf("The number of levels required is: %d\n", levels);

    int fd = shm_open(SHARED_MEM_NAME, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    ftruncate(fd, segmentsize);

    Block * new = (Block*) mmap(0, segmentsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); //get the mem as a whole block
    //also give the shared mem base to it.

    //lower bit hex construction
    int f_num = log2(segmentsize);
    char hex[18 + 3] = "0x";
    for (int i = 0; i<f_num; i++){
        strcat(hex,"f");
    }
    
    printf("Hexadecimal is: %s\n", hex);

    assert(( (long int)new & atoi(hex) ) == 0 ) ;   //ensure the lower bits are 0
    new->status = Free;
    new->level = levels - 1;

    //Block* flists[levels] = {NULL} ; //global that stores free lists of each layer
    //give the one large initial block to flists
    //flists[levels-1] = new;
    
    //tests
    int testa = level(255);
    printf("The supposed level must be: %d\n", testa);
    return 0;
    
}

void * balloc(size_t size) {
    if ( size == 0 ){
        return NULL;
    }
    
    int index = level(size);
    Block * taken = find(index);
    return hide(taken);
}

void bfree(void *memory) {
    if(memory != NULL) {
        Block *block = magic(memory);
        insert(block);
    }
    return;
}