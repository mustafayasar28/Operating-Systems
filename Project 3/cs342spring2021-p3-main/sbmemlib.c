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
#define KEEPING_SEM "/keeping_sem"

// Define your structures and variables.
typedef struct Block_Item {
    int start_addr;
    int finish_addr;
} Block_Item;

typedef struct Block_List {
    int size;
    Block_Item block_items[256];
} Block_List;

//struct to be kept in freelist
typedef struct Block
{
    int size;
    Block_List block_list[32];
} Block;

//struct for allocation info
typedef struct Alloc_Info
{
    unsigned int start_addr;    //key
    int allocated_size;
    int block_size;
    int process_id;
} Alloc_Info;

typedef struct Alloc_Info_List
{
    int size;
    Alloc_Info info_list[2048];
} Alloc_Info_List;

typedef struct SegmentBase_Table {
    int process_id;
    void *segmentbase;
} SegmentBase_Table;

typedef struct keeping {
    int segment_size;
    int process_count;
    int size;

    Block block;
    Alloc_Info_List mp;
    
    int segmentbase_table_size;
    SegmentBase_Table segmentbase_table[10];

    sem_t * pc_sem;
    sem_t * alloc_sem;
    sem_t * keeping_sem;
} Keeping;

// Shared structurelar burda yaratılacak
int sbmem_init(int segmentsize)
{
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
    if ( ftruncate(fd, segmentsize + sizeof(Keeping)) == -1 ) return -1;

    //put size info to beginning
    Keeping *sizeptr = mmap(0, sizeof(Keeping), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	printf("Size of keeping: %ld\n", sizeof(Keeping));

    /* Initialize the semaphore(s) */
    sizeptr -> pc_sem = sem_open(PROCESS_COUNT_SEM_NAME, O_CREAT, 0644, 1);
    sizeptr -> alloc_sem = sem_open(ALLOCATION_SEM, O_CREAT, 0644, 1);
    sizeptr -> keeping_sem = sem_open(KEEPING_SEM, O_CREAT, 0644, 1);

    sizeptr -> process_count = 0;
    sizeptr -> segment_size = segmentsize;
    
    int n = ceil( log(segmentsize) / log(2) );
    sizeptr -> size = n + 1;
    (sizeptr -> mp).size = 0;

    for(int i = 0; i <= n; i++) 
        (sizeptr -> block).block_list[i].size = 0;

    (sizeptr -> block).block_list[n].block_items[ (sizeptr -> block).block_list[n].size ].start_addr = 0;
    (sizeptr -> block).block_list[n].block_items[ (sizeptr -> block).block_list[n].size ].finish_addr = segmentsize - 1;
    (sizeptr -> block).block_list[n].size++;

    return (0);
}

// semaphorelar burda kapatılıcak
int sbmem_remove()
{
    int fd = shm_open(SHARED_MEM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    Keeping *segment_ptr = mmap(0, sizeof(Keeping), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
   
    //pc_sem = sem_open(PROCESS_COUNT_SEM_NAME, 0);
    //alloc = sem_open(ALLOCATION_SEM, 0);
    //keeping_sem = sem_open(KEEPING_SEM, 0);
    
    //close & unlink semaphores
    sem_unlink(PROCESS_COUNT_SEM_NAME);
    sem_unlink(ALLOCATION_SEM);
    sem_unlink(KEEPING_SEM);

    sem_close(segment_ptr->pc_sem);
    sem_close(segment_ptr->alloc_sem);
    sem_close(segment_ptr->keeping_sem);
    
    return shm_unlink(SHARED_MEM_NAME); 
}

// ne kadar process bekliyo bilebilmeliler
int sbmem_open()
{
    //sem_t *pc_sem = sem_open(PROCESS_COUNT_SEM_NAME, 0);
    //sem_t *keeping_sem = sem_open(KEEPING_SEM, 0);
    
    int fd = shm_open(SHARED_MEM_NAME, O_RDWR, S_IRUSR | S_IWUSR);

    //access keeping info to read segment size
    Keeping *segment_ptr = mmap(0, sizeof(Keeping), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    sem_wait(segment_ptr->keeping_sem);

    //segment_ptr -> pc_sem = sem_open(PROCESS_COUNT_SEM_NAME, 0);
    //segment_ptr -> keeping_sem = sem_open(KEEPING_SEM, 0);

    sem_wait(segment_ptr -> pc_sem);
    if (segment_ptr -> process_count >= 10) 
        return -1;
    
    segment_ptr -> process_count++;
    sem_post(segment_ptr -> pc_sem);

    //get segment size from keeping
    int segmentsize = segment_ptr -> segment_size;
    //mmap again just to include allocatable segment sized memory, so keeping is hidden from process
    void *segmentbase = mmap(0, sizeof(Keeping) + segmentsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    //save base address to keeping for the caller process

    /* segment_ptr->savesegmentbase = segmentbase; */
    segment_ptr -> segmentbase_table[ segment_ptr -> segmentbase_table_size ].process_id = getpid();
    segment_ptr -> segmentbase_table[ segment_ptr -> segmentbase_table_size ].segmentbase = segmentbase;
    segment_ptr -> segmentbase_table_size++;
    
    sem_post(segment_ptr -> keeping_sem); 
    return 0;
}

void *sbmem_alloc (int size)
{
    //sem_t *keeping_sem = sem_open(KEEPING_SEM, 0);
    

    /* sz = size */
    int n = ceil( log(size) / log(2) );

    int fd = shm_open(SHARED_MEM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    
    //keeping_ptr holds to base of keeping info
    Keeping *keeping_ptr = mmap(0, sizeof(Keeping), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    sem_wait(keeping_ptr -> keeping_sem);
    

    //find segmentbase address of a process from table with its pid and hide keeping from memory
    /* void *segmentbase = (keeping_ptr->savesegmentbase) + sizeof(Keeping); */
    int pid = getpid();
    void* segmentbase;
    for (int i = 0; i < (keeping_ptr -> segmentbase_table_size); i++) {
        if ( (keeping_ptr -> segmentbase_table)[i].process_id == pid ) {
            segmentbase = (keeping_ptr -> segmentbase_table[i]).segmentbase + sizeof(Keeping);
        }
    }

    if ((keeping_ptr->block).block_list[n].size > 0) {
        Block_Item temp;
        temp.start_addr = (keeping_ptr -> block).block_list[n].block_items[0].start_addr;
        temp.finish_addr = (keeping_ptr -> block).block_list[n].block_items[0].finish_addr;

        for (int k = 0; k < (keeping_ptr -> block).block_list[n].size - 1; k++)
            (keeping_ptr -> block).block_list[n].block_items[k] = (keeping_ptr -> block).block_list[n].block_items[k + 1];
        
        (keeping_ptr -> block).block_list[n].size--;

        (keeping_ptr->mp).info_list[ (keeping_ptr->mp).size ].start_addr = temp.start_addr;
        (keeping_ptr->mp).info_list[ (keeping_ptr->mp).size ].block_size = temp.finish_addr - temp.start_addr + 1;
        (keeping_ptr->mp).info_list[ (keeping_ptr->mp).size ].allocated_size = size;
        (keeping_ptr->mp).info_list[ (keeping_ptr->mp).size ].process_id = getpid();

        (keeping_ptr -> mp).size++;
		
		printf("MAP STATUS:\n");
			for (int i = 0; i < (keeping_ptr -> mp).size; i++) {
                printf("Allocated size = %d\n", keeping_ptr -> mp.info_list[i].allocated_size);
                printf("Block size = %d\n", keeping_ptr -> mp.info_list[i].block_size);
                printf("Process id = %d\n", keeping_ptr -> mp.info_list[i].process_id);
                printf("Start address = %d\n", keeping_ptr -> mp.info_list[i].start_addr);
                			printf("\n");
			}
		
		
        sem_post(keeping_ptr -> keeping_sem);
        return temp.start_addr + segmentbase;
    }
    else
    {
        int i;

        for (i = n + 1; i < keeping_ptr -> size; i++) {
            if ( (keeping_ptr -> block).block_list[i].size != 0)
                break;
        }

        if (i == keeping_ptr -> size) {
            printf("Can not allocate\n");
            sem_post(keeping_ptr -> keeping_sem);
            return NULL;
        } else {
            Block_Item temp;
            temp.start_addr = (keeping_ptr -> block).block_list[i].block_items[0].start_addr;
            temp.finish_addr = (keeping_ptr -> block).block_list[i].block_items[0].finish_addr;

            for (int k = 0; k < (keeping_ptr -> block).block_list[i].size - 1; k++) {
                (keeping_ptr -> block).block_list[i].block_items[k] = (keeping_ptr -> block).block_list[i].block_items[k + 1];
            }

            (keeping_ptr -> block).block_list[i].size--;
            i--;

            for (; i >= n; i--) {
                Block_Item temp1, temp2;
                temp1.start_addr = temp.start_addr;
                temp1.finish_addr = temp.start_addr + (temp.finish_addr - temp.start_addr) / 2;

                temp2.start_addr = temp.start_addr + (temp.finish_addr - temp.start_addr + 1) / 2;
                temp2.finish_addr = temp.finish_addr;

                (keeping_ptr->block).block_list[i].block_items[ (keeping_ptr->block).block_list[i].size ].start_addr = temp1.start_addr;
                (keeping_ptr->block).block_list[i].block_items[ (keeping_ptr->block).block_list[i].size ].finish_addr = temp1.finish_addr;
                (keeping_ptr->block).block_list[i].size++;

                (keeping_ptr->block).block_list[i].block_items[ (keeping_ptr->block).block_list[i].size ].start_addr = temp2.start_addr;
                (keeping_ptr->block).block_list[i].block_items[ (keeping_ptr->block).block_list[i].size ].finish_addr = temp2.finish_addr;
                (keeping_ptr->block).block_list[i].size++;

                temp.start_addr = (keeping_ptr->block).block_list[i].block_items[0].start_addr;
                temp.finish_addr = (keeping_ptr->block).block_list[i].block_items[0].finish_addr;

                for(int k = 0; k < (keeping_ptr->block).block_list[i].size - 1; k++) {
                    (keeping_ptr->block).block_list[i].block_items[k] = (keeping_ptr->block).block_list[i].block_items[k + 1];
                }

                (keeping_ptr -> block).block_list[i].size--;
            }

            (keeping_ptr->mp).info_list[ (keeping_ptr->mp).size ].start_addr = temp.start_addr;
            (keeping_ptr->mp).info_list[ (keeping_ptr->mp).size ].block_size = temp.finish_addr - temp.start_addr + 1;
            (keeping_ptr->mp).info_list[ (keeping_ptr->mp).size ].process_id = getpid();
            (keeping_ptr->mp).info_list[ (keeping_ptr->mp).size ].allocated_size = size;

            (keeping_ptr->mp).size++;

			
			printf("MAP STATUS:\n");
			for (int i = 0; i < (keeping_ptr -> mp).size; i++) {
                printf("Allocated size = %d\n", keeping_ptr -> mp.info_list[i].allocated_size);
                printf("Block size = %d\n", keeping_ptr -> mp.info_list[i].block_size);
                printf("Process id = %d\n", keeping_ptr -> mp.info_list[i].process_id);
                printf("Start address = %d\n", keeping_ptr -> mp.info_list[i].start_addr);
                			printf("\n");
			}

            sem_post(keeping_ptr -> keeping_sem);
            
            return temp.start_addr + segmentbase;
        }
    }
}

void sbmem_free (void *p)
{
    //get semaphores
    //sem_t *keeping_sem = sem_open(KEEPING_SEM, 0);

    int fd = shm_open(SHARED_MEM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    
    //keeping_ptr holds to base of keeping info
    Keeping *keeping_ptr = mmap(0, sizeof(Keeping), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    sem_wait(keeping_ptr -> keeping_sem);

    //segmentbase points to allocatable segment sized memory
    /* void *segmentbase = (keeping_ptr->savesegmentbase) + sizeof(Keeping); */

    //find the process' start address to segment from keeping
    int pid = getpid();
    void* segmentbase;
    for (int i = 0; i < (keeping_ptr -> segmentbase_table_size); i++) {
        if ( (keeping_ptr -> segmentbase_table)[i].process_id == pid ) {
            segmentbase = (keeping_ptr -> segmentbase_table[i]).segmentbase + sizeof(Keeping);
        }
    }

    //check validity of dealloc request
    int id = p - segmentbase;
    printf("id = %d\n", id);
    int found = 0;
    int j;

    for (j = 0; j < (keeping_ptr->mp).size; j++){
        if( (keeping_ptr->mp).info_list[j].start_addr == id){
            found = 1;
            break;
        }
    }

    if(!found){
        printf("Invalid free request.\n");
        return;
    }

    //request is valid
    int block_size = (keeping_ptr->mp).info_list[j].block_size;
    int n = ceil(log(block_size) / log(2));

    int i, buddyNumber, buddyAddress;

    (keeping_ptr->block).block_list[n].block_items[ (keeping_ptr->block).block_list[n].size ].start_addr = id;
    (keeping_ptr->block).block_list[n].block_items[ (keeping_ptr->block).block_list[n].size ].finish_addr = id + (int) pow(2, n) - 1;
    (keeping_ptr->block).block_list[n].size++;

    printf("Memory block from %d to %f freed\n", id, id + pow(2, n) - 1 );

    buddyNumber = id / block_size;

    if (buddyNumber % 2 != 0)
        buddyAddress = id - (int) pow(2, n);
    else
        buddyAddress = id + (int) pow(2, n);

    printf("buddyNumber = %d, buddyAddress = %d\n", buddyNumber, buddyAddress);


    for (i = 0; i < (keeping_ptr->block).block_list[n].size; i++) {
        printf("start_addr = %d\n", (keeping_ptr->block).block_list[n].block_items[i].start_addr);

        if ((keeping_ptr->block).block_list[n].block_items[i].start_addr == buddyAddress) {
            if (buddyNumber % 2 == 0)
            {
                (keeping_ptr->block).block_list[n + 1].block_items[ (keeping_ptr->block).block_list[n + 1].size ].start_addr = id;
                (keeping_ptr->block).block_list[n + 1].block_items[ (keeping_ptr->block).block_list[n + 1].size ].finish_addr = id + (int) (2 * (pow(2, n) -1 ));
                (keeping_ptr->block).block_list[n + 1].size++;

                printf("Coalescing of blocks starting at %d and %d was done\n", id, buddyAddress);
            }
            else
            {
                (keeping_ptr->block).block_list[n + 1].block_items[ (keeping_ptr->block).block_list[n + 1].size ].start_addr = buddyAddress;
                (keeping_ptr->block).block_list[n + 1].block_items[ (keeping_ptr->block).block_list[n + 1].size ].finish_addr = buddyAddress + (int) (2 * (pow(2,n)));
                (keeping_ptr->block).block_list[n + 1].size++;

                printf("Coalescing of blocks starting at %d and %d was done\n", buddyAddress, id);
            }

            for (int k = i; k < (keeping_ptr->block).block_list[n].size - 1; k++) {
                (keeping_ptr->block).block_list[n].block_items[k] = (keeping_ptr->block).block_list[n].block_items[k + 1];
            }

            (keeping_ptr->block).block_list[n].size--;

            /* Çalısmazsa direkt block.block_list[n].size-- dene*/
            for (int k = (keeping_ptr->block).block_list[n].size - 1; k < (keeping_ptr->block).block_list[n].size - 1; k++) {
                (keeping_ptr->block).block_list[n].block_items[k] = (keeping_ptr->block).block_list[n].block_items[k + 1];
            }
            (keeping_ptr->block).block_list[n].size--;
            break;
        }
    }

    /* Remove the existence from map */
    /* Remove whose start_addr is equal to id */
    int m;
    for (m = 0; m < (keeping_ptr->mp).size; ++m) {
        if ((keeping_ptr->mp).info_list[m].start_addr == id)
            break;
    }

    for (int k = m; k < (keeping_ptr->mp).size - 1; k++) {
        (keeping_ptr->mp).info_list[k] = (keeping_ptr->mp).info_list[k + 1];
    }

    (keeping_ptr->mp).size--;
    sem_post(keeping_ptr -> keeping_sem);
}

int sbmem_close()
{
    //sem_t *pc_sem = sem_open(PROCESS_COUNT_SEM_NAME, 0);
    //sem_t *keeping_sem = sem_open(KEEPING_SEM, 0);

    int fd = shm_open(SHARED_MEM_NAME, O_RDWR, S_IRUSR | S_IWUSR);
    
    //keeping_ptr holds to base of keeping info
    Keeping *keeping_ptr = mmap(0, sizeof(Keeping), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    sem_wait(keeping_ptr -> keeping_sem);
    keeping_ptr -> process_count--;
    int segmentsize = keeping_ptr->segment_size;
    
    //remove process info from segmentbase table as it will not use it anymore)
    int index = -1;
    int pid = getpid();
    for (int i = 0; i < keeping_ptr ->segmentbase_table_size; i++) {
        if ((keeping_ptr -> segmentbase_table)[ i ].process_id == pid ) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        printf("Not closed\n");
        return -1;
    }

    void* segmentbase = (keeping_ptr -> segmentbase_table)[index].segmentbase;

    for (int i = index; i < keeping_ptr -> segmentbase_table_size; i++) {
        (keeping_ptr -> segmentbase_table)[i] = (keeping_ptr -> segmentbase_table)[i + 1];
    }

    (keeping_ptr -> segmentbase_table_size)--;

    //decrease process count
    sem_wait(keeping_ptr -> pc_sem);
    keeping_ptr -> process_count--;
    sem_post(keeping_ptr -> pc_sem);

    //unmap from virtual address space
    int succ = munmap(segmentbase, sizeof(Keeping) + segmentsize);
    sem_post(keeping_ptr -> keeping_sem);

    return (succ); 
}
