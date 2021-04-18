#include <stdio.h>
#include <unistd.h>
#include <math.h>


typedef struct Block_Item {
    int start_addr;
    int finish_addr;
} Block_Item;

typedef struct Block_List {
    int size;
    Block_Item block_items[1024];
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
    int block_size;
    int process_id;
} Alloc_Info;

typedef struct Alloc_Info_List
{
    int size;
    Alloc_Info info_list[100];
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
    printf("init n = %d, size = %d\n", n, size);
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

        printf("block.block_list[n].size = %d\n", block.block_list[n].size);

        /* Remove block from free list */
        for (int k = 0; k < block.block_list[n].size - 1; k++) {
            block.block_list[n].block_items[k] = block.block_list[n].block_items[k + 1];
        }
        block.block_list[n].size--;

        printf("Memory from %d to %d allocated\n temp.first = %d\n", temp.start_addr, temp.finish_addr, temp.start_addr);

        mp.info_list[ mp.size ].start_addr = temp.start_addr;
        mp.info_list[ mp.size ].block_size = temp.finish_addr - temp.start_addr + 1;
        mp.size++;
        // mp[temp.start_addr] = temp.finish_addr - temp.start_addr + 1;
    }
    else
    {
        int i;

        for (i = n + 1; i < size; i++) {
            if ( block.block_list[i].size != 0 )
                break;
        }

        printf("After searching i = %d, size = %d\n", i, size);

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
            printf("Memory from %d to %d allocated\n", temp.start_addr, temp.finish_addr);

            mp.info_list[ mp.size ].start_addr = temp.start_addr;
            mp.info_list[ mp.size ].block_size = temp.finish_addr - temp.start_addr + 1;
            mp.size++;
            //mp[temp.start_addr] = temp.finish_addr - temp.start_addr + 1;
        }
    }
}

int main() {
    int sz = 32768;

    initialize(128);

    allocate(32);
    allocate(13);
    allocate(64);
    allocate(56);


    for (int i = 0; i < mp.size; i++) {
        printf("start = %d, block size = %d\n", mp.info_list[i].start_addr, mp.info_list[i].block_size);
    }

}

/*
*/