#include <stdio.h>
#include <unistd.h>
#include <math.h>


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

int size;
Block block;
Alloc_Info_List mp;

void initialize(int sz) {
    int n = ceil(log(sz) / log(2));

    size = n + 1;

    mp.size = 0;

    for (int i = 0; i <= n; i++) {
        block.block_list[i].size = 0;
    }
    block.block_list[n].block_items[ block.block_list[n].size ].start_addr = 0;
    block.block_list[n].block_items[ block.block_list[n].size ].finish_addr = sz - 1;

    block.block_list[n].size++;
}

void allocate(int sz) {
    int n = ceil( log(sz) / log(2) );

    if ( block.block_list[n].size > 0)
    {
        Block_Item temp;
        temp.start_addr = block.block_list[n].block_items[0].start_addr;
        temp.finish_addr = block.block_list[n].block_items[0].finish_addr;

        /* Remove block from free list */
        for (int k = 0; k < block.block_list[n].size - 1; k++) {
            block.block_list[n].block_items[k] = block.block_list[n].block_items[k + 1];
        }
        block.block_list[n].size--;

        mp.info_list[ mp.size ].start_addr = temp.start_addr;
        mp.info_list[ mp.size ].block_size = temp.finish_addr - temp.start_addr + 1;
        mp.info_list[ mp.size ].allocated_size = sz;
        mp.info_list[ mp.size ].process_id = getpid();

        mp.size++;
    }
    else
    {
        int i;

        for (i = n + 1; i < size; i++) {
            if ( block.block_list[i].size != 0 )
                break;
        }

        if ( i == size ) {
            printf("Sorry, failed to allocate memory\n");
        } else {
            /* If found */
            Block_Item temp;
            temp.start_addr = block.block_list[i].block_items[0].start_addr;
            temp.finish_addr = block.block_list[i].block_items[0].finish_addr;

            for (int k = 0; k < block.block_list[i].size - 1; k++) {
                block.block_list[i].block_items[k] = block.block_list[i].block_items[k + 1];
            }
            block.block_list[i].size--;
            i--;

            for (; i >= n; i--) {
                Block_Item temp1, temp2;
                temp1.start_addr = temp.start_addr;
                temp1.finish_addr = temp.start_addr + (temp.finish_addr - temp.start_addr) / 2;

                temp2.start_addr = temp.start_addr + (temp.finish_addr - temp.start_addr + 1) / 2;
                temp2.finish_addr = temp.finish_addr;

                block.block_list[i].block_items[ block.block_list[i].size ].start_addr = temp1.start_addr;
                block.block_list[i].block_items[ block.block_list[i].size ].finish_addr = temp1.finish_addr;

                block.block_list[i].size++;

                block.block_list[i].block_items[ block.block_list[i].size ].start_addr = temp2.start_addr;
                block.block_list[i].block_items[ block.block_list[i].size ].finish_addr = temp2.finish_addr;

                block.block_list[i].size++;

                temp.start_addr = block.block_list[i].block_items[0].start_addr;
                temp.finish_addr = block.block_list[i].block_items[0].finish_addr;

                for (int k = 0; k < block.block_list[i].size - 1; k++) {
                    block.block_list[i].block_items[k] = block.block_list[i].block_items[k + 1];
                }
                block.block_list[i].size--;
            }

            mp.info_list[ mp.size ].start_addr = temp.start_addr;
            mp.info_list[ mp.size ].block_size = temp.finish_addr - temp.start_addr + 1;
            mp.info_list[ mp.size ].process_id = getpid();
            mp.info_list[ mp.size ].allocated_size = sz;
            mp.size++;
            //mp[temp.start_addr] = temp.finish_addr - temp.start_addr + 1;
        }
    }
}

int deallocate(int id) {
    int found = 0;
    int j;
    for (j = 0; j < mp.size; j++ ) {
        if (mp.info_list[j].start_addr == id) {
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("Invalid free request\n");
        return -1;
    }

    int block_size = mp.info_list[j].block_size; /* mp[id] = block_size */
    int n = ceil(log(block_size) / log(2));

    int i, buddyNumber, buddyAddress;

    block.block_list[n].block_items[ block.block_list[n].size ].start_addr = id;
    block.block_list[n].block_items[ block.block_list[n].size ].finish_addr = id + (int) pow(2, n) - 1;
    block.block_list[n].size++;

    printf("Memory block from %d to %f freed\n", id, id + pow(2, n) - 1 );

    buddyNumber = id / block_size;

    if (buddyNumber % 2 != 0)
        buddyAddress = id - (int) pow(2, n);
    else
        buddyAddress = id + (int) pow(2, n);

    for (i = 0; i < block.block_list[n].size; i++) {
        if (block.block_list[n].block_items[i].start_addr == buddyAddress) {
            if (buddyNumber % 2 == 0)
            {
                block.block_list[n + 1].block_items[ block.block_list[n + 1].size ].start_addr = id;
                block.block_list[n + 1].block_items[ block.block_list[n + 1].size ].finish_addr = id + (int) (2 * (pow(2, n) -1 ));
                block.block_list[n + 1].size++;

                printf("Coalescing of blocks starting at %d and %d was done\n", id, buddyAddress);
            }
            else
            {
                block.block_list[n + 1].block_items[ block.block_list[n + 1].size ].start_addr = buddyAddress;
                block.block_list[n + 1].block_items[ block.block_list[n + 1].size ].finish_addr = buddyAddress +
                                                                                                  (int) (2 * (pow(2,n)));
                block.block_list[n + 1].size++;

                printf("Coalescing of blocks starting at %d and %d was done\n", buddyAddress, id);
            }

            for (int k = i; k < block.block_list[n].size - 1; k++) {
                block.block_list[n].block_items[k] = block.block_list[n].block_items[k + 1];
            }
            
            block.block_list[n].size--;

            /* Çalısmazsa direkt block.block_list[n].size-- dene*/
            for (int k = block.block_list[n].size - 1; k < block.block_list[n].size - 1; k++) {
                block.block_list[n].block_items[k] = block.block_list[n].block_items[k + 1];
            }
            block.block_list[n].size--;
            break;
        }
    }

    /* Remove the existence from map */
    /* Remove whose start_addr is equal to id */
    int m;
    for (m = 0; m < mp.size; ++m) {
        if (mp.info_list[m].start_addr == id)
            break;
    }

    for (int k = m; k < mp.size - 1; k++) {
        mp.info_list[k] = mp.info_list[k + 1];
    }

    mp.size--;

}

int main() {
    int sz = 32768;

    initialize(2048);


    allocate(150);
    allocate(260);
    allocate(1000);
    deallocate(563);
    allocate(129);
    allocate(500);

    for (int i = 0; i < mp.size; i++) {
        printf("start = %d, block size = %d, process id = %d, allocation size = %d\n",
               mp.info_list[i].start_addr,
               mp.info_list[i].block_size,
               mp.info_list[i].process_id,
               mp.info_list[i].allocated_size);
    }
}