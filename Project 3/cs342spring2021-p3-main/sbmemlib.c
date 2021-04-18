/* ÖNEMLİ !! 

    Lütfen RUN'larken -lrt, -lm, -pthread  en sona eklemeyi unutmayınız

    gcc create_memory_sb.c sbmemlib.c -o main -lrt -pthread -lm

*/

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

// Define a name for your shared memory; you can give any name that start with a slash character; it will be like a filename.
#define SHARED_MEM_NAME "/shared_mem_cm"

// Define semaphore(s)
#define PROCESS_COUNT_SEM_NAME "/process_count_sem"
#define ALLOCATION_SEM "/sem_open_allocation_sem"

// Define your structures and variables.
typedef struct keeping {
    int segment_size;
    int process_count;


} Keeping;

//shared structurelar burda yaratılıcak
int sbmem_init(int segmentsize)
{
    int process_count = 0;
    
    if ( ((segmentsize != 0) && ((segmentsize & (segmentsize -1)) == 0)) == 0 ) {
        perror("The segment size is not a power of 2\n");
        exit(1);
    }

    /* Create shared memory object 
        O_TRUNC:  If the shared memory object already exists, truncate it to
              zero bytes.
    */
    int fd = shm_open(SHARED_MEM_NAME, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    // Shared segment fail
    if ( fd == -1 ) return -1;
    
    // Shared segment fail
    if ( ftruncate(fd, segmentsize) == -1 ) return -1;

    //put size info to beginning
    Keeping *sizeptr = mmap(0, sizeof(Keeping), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
    sizeptr -> process_count = 0;
    sizeptr -> segment_size = segmentsize;

    printf("process_count: %d\n", sizeptr -> process_count);
    printf("segment_size: %d\n", sizeptr -> segment_size);


    /* Initialize the semaphore(s) */
    sem_t * pc_sem = sem_open(PROCESS_COUNT_SEM_NAME, O_CREAT, 0644, 1);
    sem_t * alloc_sem = sem_open(ALLOCATION_SEM, O_CREAT, 0644, 1);

    return (0);
}

// semaphorelar burda kapatılıcak
int sbmem_remove()
{
    sem_t *pc_sem = sem_open(PROCESS_COUNT_SEM_NAME, 0);
    sem_t *alloc = sem_open(ALLOCATION_SEM, 0);
    
    //close & unlink semaphores
    sem_unlink(PROCESS_COUNT_SEM_NAME);
    sem_unlink(ALLOCATION_SEM);

    sem_close(pc_sem);
    sem_close(alloc);
    
    return shm_unlink(SHARED_MEM_NAME); 
}

// ne kadar process bekliyo bilebilmeliler
int sbmem_open()
{
    sem_t *pc_sem = sem_open(PROCESS_COUNT_SEM_NAME, 0);
    int fd = shm_open(SHARED_MEM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    Keeping *segment_ptr = mmap(0, sizeof(Keeping), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    sem_wait(pc_sem);

    if (segment_ptr -> process_count >= 10) {
        return -1;
    }

    printf("process_count: %d\n", segment_ptr -> process_count);
    segment_ptr -> process_count++;
    printf("process_count: %d\n", segment_ptr -> process_count);
    sem_post(pc_sem);
}


void *sbmem_alloc (int size)
{
    int next = pow(2, ceil(log(size) / log(2)));
    printf("Next power of 2 is = %d\n", next);




    return (NULL);
}


void sbmem_free (void *p)
{

 
}

int sbmem_close()
{
    sem_t *pc_sem = sem_open(PROCESS_COUNT_SEM_NAME, 0);

    sem_wait(pc_sem);
    //process_count--;
    sem_post(pc_sem);

    return (0); 
}
