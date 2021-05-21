#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "simplefs.h"
#define MAX_FILE_COUNT 16
#define FREE 0      /* 0 means the block is free */
#define OCCUPIED 1  /* 1 means the block is occupied */

// Global Variables =======================================
int vdisk_fd; // Global virtual disk file descriptor. Global within the library.
              // Will be assigned with the vsfs_mount call.
              // Any function in this file can use this.
              // Applications will not use  this directly. 

char disk_name[110];

//Open file entry structure
typedef struct open_file_table_entry {
    char file_name[110];
    int32_t position_offset;       //which byte of file bookmark
    int mode;
    int32_t FCB_index;
    int32_t is_available;
    int32_t file_size; // byte
} open_file_table_entry;

//Directory Structure (128 bytes)
typedef struct directory_entry {
    char file_name[110];
    int32_t FCB_index;
    int32_t is_available;
	char pads[14 - 2*sizeof(int32_t)];
} directory_entry;

//Superblock
typedef struct superblock{
    int32_t no_blocks;
    int32_t current_file_count;
    int32_t free_block_count;
    //might need more attr
} superblock; 

//FCB struct (size 128 bytes)
typedef struct fcb{
    int32_t is_available;
    int32_t file_size; //byte
    int32_t index_block_no;
    char pads[128-3*sizeof(int32_t)];
} fcb;

//FCB table
fcb * fcb_table;

//Directory table
directory_entry *directory_table;

//Open file table
open_file_table_entry *open_file_table;
int opened_file_count;

// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk. 
int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = read (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf ("read error\n");
        return -1;
    }
    return (0); 
}

// write block k into the virtual disk. 
int write_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = write (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
        printf ("write error\n");
        return (-1);
    }
    return 0; 
}

// Bitvector functions
// k is desired bit to set
void  SetBit( int32_t *A, int k)
{
    int i = k/32;        //gives the corresponding index in the array A
    int pos = k%32;      //gives the corresponding bit position in A[i]

    unsigned int flag = 1;   // flag = 0000.....00001

    flag = flag << pos;      // flag = 0000...010...000   (shifted k positions)

    A[i] = A[i] | flag;      // Set the bit at the k-th position in A[i]
}

int TestBit( int32_t *A,  int k )
{
    return ( (A[k/32] & (1 << (k%32) )) != 0 );     
}

void  ClearBit( int32_t *A,  int k )                
{
    A[k/32] &= ~(1 << (k%32));
}

// Find first free block from bitvector, returns block number
int findFirst(){
    superblock *super_block_buffer = (superblock*) calloc(1, BLOCKSIZE);
    read_block((void*) super_block_buffer, 0);

    int block_count = super_block_buffer -> no_blocks;
    
    // Find how many blocks to check for bitvector
    double bit_blocks = ceil((double) block_count / (double) BLOCKSIZE);
    int block_rem = block_count;
    int * buffer;

    for (int i = 0; i < bit_blocks; i++){
        buffer = (int32_t *) calloc(1, BLOCKSIZE);
        read_block((void* ) buffer, i+1);
        
        for (int j = 0; j < BLOCKSIZE && block_rem > 0; j++){
            if (TestBit(buffer, j) == 0) {
                free(super_block_buffer);
                free(buffer);
                return j + (i * BLOCKSIZE);
            }
            block_rem--; 
        }
        free(buffer);
    }
    free(super_block_buffer);
    return -1;
}

int findAndSetFirst(){
    superblock *super_block_buffer = (superblock*) calloc(1, BLOCKSIZE);
    read_block((void*) super_block_buffer, 0);

    int block_count = super_block_buffer -> no_blocks;
    
    // Find how many blocks to check for bitvector
    double bit_blocks = ceil((double) block_count / (double) BLOCKSIZE);
    int block_rem = block_count;
    int * buffer;

    for (int i = 0; i < bit_blocks; i++){
        buffer = (int32_t *) calloc(1, BLOCKSIZE);
        read_block((void* ) buffer, i+1);
        
        for (int j = 0; j < BLOCKSIZE && block_rem > 0; j++){
            if (TestBit(buffer, j) == 0) {
                SetBit(buffer, j);
                write_block(buffer, i+1);
                free(super_block_buffer);
                free(buffer);
                return j + (i * BLOCKSIZE);
            }
            block_rem--; 
        }
        free(buffer);
    }
    free(super_block_buffer);
    return -1;
}

// Find first free block from bitvector, returns block number
int findSecondFirst(){
    int firstFound = 0;
    superblock *super_block_buffer = (superblock*) calloc(1, BLOCKSIZE);
    read_block((void*) super_block_buffer, 0);

    int block_count = super_block_buffer -> no_blocks;
    
    // Find how many blocks to check for bitvector
    double bit_blocks = ceil((double) block_count / (double) BLOCKSIZE);
    int block_rem = block_count;
    int * buffer;

    for (int i = 0; i < bit_blocks; i++){
        buffer = (int32_t *) calloc(1, BLOCKSIZE);
        read_block((void* ) buffer, i+1);
        
        for (int j = 0; j < BLOCKSIZE && block_rem > 0; j++){
            if (TestBit(buffer, j) == 0) {
                if (firstFound == 0) {
                    firstFound = 1;
                    continue;
                } else {
                    free(super_block_buffer);
                    free(buffer);
                    return j + (i * BLOCKSIZE);
                }
            }
            block_rem--; 
        }
        free(buffer);
    }
    free(super_block_buffer);
    return -1;
}

void ClearBitVector(int block_index) {
    int vectors_block = block_index / BLOCKSIZE;
    int vectors_offset = block_index % BLOCKSIZE;

    int * buffer = (int32_t *) calloc(1, BLOCKSIZE);
    read_block( (void *) buffer, vectors_block + 1 );
    ClearBit(buffer, vectors_offset);
    write_block( (void *) buffer, vectors_block + 1);
    free(buffer);
}


/**********************************************************************
   The following functions are to be called by applications directly. 
***********************************************************************/

// this function is partially implemented.
int create_format_vdisk (char *vdiskname, unsigned int m)
{
    char command[1000];
    int size;
    int num = 1;
    int count;
    size  = num << m;
    count = size / BLOCKSIZE; // block amount
    sprintf (command, "dd if=/dev/zero of=%s bs=%d count=%d",
             vdiskname, BLOCKSIZE, count);
    system (command);

    // now write the code to format the disk below.
    // .. your code...
    strcpy(disk_name, vdiskname);
	vdisk_fd = open(disk_name, O_RDWR);

    //create bitvector and write them to blocks
    int32_t * bitvector;
    for(int i = 0; i < 4; i++){
        bitvector = (int32_t *) calloc(1, BLOCKSIZE);
        write_block((void *) bitvector, i+1);
        free(bitvector);
    }

    /* Read block 1, set first 13 bits to 1 (OCCUPIED) */
    int32_t * buffer = (int32_t *) calloc(1, BLOCKSIZE);
    read_block((void* ) buffer, 1);

    for (int i = 0; i < 13; i++) {
        SetBit(buffer, i);
    }

    write_block((void *) buffer, 1);
    free(buffer);

    //create superblock and write it to first block
    superblock *super_block = (superblock*) calloc(1, BLOCKSIZE);
    super_block->no_blocks = count;
    super_block->free_block_count = count - 13;
    write_block((void*) super_block, 0);
    free(super_block);
    
    directory_entry *directoryEntry;
    for(int i = 5; i < 9; i++){
        directoryEntry = (directory_entry *) calloc(32, sizeof(directory_entry));
        write_block((void*) directoryEntry, i);
        free(directoryEntry);
    }

    fcb *fcb_element;
    for(int i = 9; i < 13; i++){
        fcb_element = (fcb *) calloc(32, sizeof(fcb));
        write_block( (void*) fcb_element, i);
        free(fcb_element);
    }

    
    return (0);
}


// already implemented
int sfs_mount (char *vdiskname)
{
    // simply open the Linux file vdiskname and in this
    // way make it ready to be used for other operations.
    // vdisk_fd is global; hence other function can use it. 
    open_file_table = (open_file_table_entry *) calloc(MAX_FILE_COUNT, sizeof(open_file_table_entry));
    opened_file_count = 0;
    vdisk_fd = open(vdiskname, O_RDWR); 
    return(0);
}


// already implemented
int sfs_umount ()
{   
    free(open_file_table);
    fsync (vdisk_fd); // copy everything in memory to disk
    close (vdisk_fd);
    return (0); 
}


int sfs_create(char *filename)
{
    int32_t free_block_index_datablock;
    int32_t index_block_index;

    // There should be at least 2 free blocks in the system to allow creation
    // This is because a file needs at least one data block and one index block
    superblock *super_block_buffer = (superblock*) calloc(1, BLOCKSIZE);
    read_block((void*) super_block_buffer, 0);

    int free_count = super_block_buffer -> free_block_count;

    if (free_count < 2){
        free(super_block_buffer);
        return -1;
    }
    
    // Find free block for file's first data block
    free_block_index_datablock = findFirst();
    
    // Find free block for file's index block
    index_block_index = findSecondFirst();

    //Find suitable fcb block
    int fcb_index = -1;
    int fcb_block, fcb_offset;
    fcb *fcb_buffer;
    int flag = 0;
    for (int i = 0; i < 4; i++){
        fcb_buffer = (fcb *) calloc(32, sizeof(fcb));
        read_block(fcb_buffer, i + 9);

        for (int j = 0; j < 32; j++){
            if (fcb_buffer[j].is_available == 0){
                fcb_index = j + i * 32;
                fcb_offset = j;
                fcb_block = i + 9;
                free(fcb_buffer);
                flag = 1;
                break;
            }
        }
        if(flag == 1){
            break;
        }
        free(fcb_buffer);
    }
    

    // No space for new file, file limit exceeded
    if (fcb_index == -1){
        return -1;
    }

    fcb_buffer = (fcb *) calloc(32, sizeof(fcb));
    read_block(fcb_buffer, fcb_block);
    fcb_buffer[fcb_offset].is_available = 1;
    fcb_buffer[fcb_offset].file_size = 0;
    fcb_buffer[fcb_offset].index_block_no = index_block_index;
    write_block(fcb_buffer, fcb_block);

    //Find first empty entry from directory
    int dir_index = -1;
    int dir_block, dir_offset;
    directory_entry *directoryEntry;
    flag = 0;
    for (int i = 0; i < 4; i++){
        directoryEntry = (directory_entry *) calloc(32, sizeof(directory_entry));
        read_block(directoryEntry, i + 5);

        for (int j = 0; j < 32; j++){
            if (directoryEntry[j].is_available == 0){
                dir_index = j + i * 32;
                dir_offset = j;
                dir_block = i + 5;
                free(directoryEntry);
                flag = 1;
                break;
            }
        }
        if (flag == 1){
            break;
        }
        free(directoryEntry);
    }

    // No space for new file, file limit exceeded
    if (dir_index == -1){
        free(fcb_buffer);
        return -1;
    }

    directoryEntry = (directory_entry *) calloc(32, sizeof(directory_entry));
    read_block(directoryEntry, dir_block);


    strcpy(directoryEntry[dir_offset].file_name, filename);
    directoryEntry[dir_offset].FCB_index = fcb_offset;
    directoryEntry[dir_offset].is_available = 1;
    write_block(directoryEntry, dir_block);

    //Set bitvectors to 1 for index block and file block
    findAndSetFirst();  // set for file block
    findAndSetFirst(); // set for index block

    //write file to block
    void * file = calloc(BLOCKSIZE, sizeof(char));
    write_block(file, free_block_index_datablock);

    //write index block
    int32_t *index_table = calloc(1024, sizeof(int32_t));
    index_table[0] = free_block_index_datablock;
    write_block(index_table, index_block_index);

    super_block_buffer -> free_block_count = (super_block_buffer -> free_block_count) - 2;
    super_block_buffer -> current_file_count = super_block_buffer -> current_file_count + 1;
    write_block(super_block_buffer,0);

    // Free allocated space
    free(super_block_buffer);
    free(fcb_buffer);
    free(directoryEntry);
    free(file);
    free(index_table);
    
    return (0);
}


int sfs_open(char *file, int mode)
{
    char file_name[110];
    strcpy(file_name, file);

    // Find currently opened file count, if max reached, give error.
    if (opened_file_count == 16) {
        return -1;
    }
    
    // Find file_name from directory
    int fcb_index = -1;

    directory_entry *directoryEntry;
    int flag = 0;
    for (int i = 0; i < 4; i++){
        directoryEntry = (directory_entry *) calloc(32, sizeof(directory_entry));
        read_block(directoryEntry, i + 5);

        for (int j = 0; j < 32; j++){
            if (strcmp(directoryEntry[j].file_name, file_name) == 0){
                fcb_index = directoryEntry[j].FCB_index;
                free(directoryEntry);
                flag = 1;
                break;
            }
        }
        if (flag == 1){
            break;
        }
        free(directoryEntry);
    }

    // If file_name does not exist, return -1
    if (flag == 0) {
        printf("file_name not found in directory\n");
        return -1;
    }

    // Find file_size from FCB
    int fcb_block = fcb_index / 32;
    int fcb_offset = fcb_index % 32;
    fcb *fcb_buffer = (fcb*) calloc(32, sizeof(fcb));
    read_block(fcb_buffer, fcb_block+9);
    int filesize = fcb_buffer[fcb_offset].file_size;
    free(fcb_buffer);

    // Find suitable index from open file table
    int fd = -1;
    for(int i = 0; i < MAX_FILE_COUNT; i++){
        if(open_file_table[i].is_available == 0){
         //   printf("The opened file is put to index: %d\n", i);
            open_file_table[i].is_available = 1;
            open_file_table[i].position_offset = 0;
            strcpy(open_file_table[i].file_name, file_name);
            open_file_table[i].mode = mode;
            open_file_table[i].FCB_index = fcb_index;
            open_file_table[i].file_size = filesize;
            fd = i;
            break;
        }
    }
    if (fd == -1) {
        printf("could not found suitable index from open file table\n");
        return -1;
    }

    return (fd);
}

int sfs_close(int fd){

    if(fd >= MAX_FILE_COUNT || fd < 0) {
        printf("fd error in sfs_close %d", fd);
        return -1;
    }
    
    // Clear entry from open table
    open_file_table[fd].is_available = 0;
    open_file_table[fd].position_offset = 0;
    strcpy(open_file_table[fd].file_name, "");
    open_file_table[fd].FCB_index = -1;
    open_file_table[fd].mode = -1;
    open_file_table[fd].file_size = 0;

    opened_file_count = opened_file_count - 1;

    return (0); 
}

int sfs_getsize (int fd)
{
    if(fd >= MAX_FILE_COUNT || fd < 0) {
        printf("fd error in sfs_close %d\n", fd);
        return -1;
    }

    return (open_file_table[fd].file_size); 
}

int sfs_read(int fd, void *buf, int n){
    char * casted_buf = buf;

    // Check validity of fd
    if(fd >= MAX_FILE_COUNT || fd < 0) {
        printf("fd error in sfs_read %d\n", fd);
        return -1;
    }

    // Check validity of n
    if (open_file_table[fd].file_size == open_file_table[fd].position_offset){
        printf("Invalid read request size\n");
        return -1;
    }

    // Check validity of mode
    if (open_file_table[fd].mode == MODE_APPEND){
        printf("Mode is incorrect, file is write-only\n");
        return -1;
    }

    if ( n > open_file_table[fd].file_size - open_file_table[fd].position_offset) {
        n = open_file_table[fd].file_size - open_file_table[fd].position_offset;
    }

    if (open_file_table[fd].file_size < n) {
        n = open_file_table[fd].file_size - open_file_table[fd].position_offset;
    }

    // Find the file's index table
    int fcb_block = open_file_table[fd].FCB_index / 32;
    int fcb_offset = open_file_table[fd].FCB_index % 32;
    fcb *fcb_buffer = (fcb*) calloc(32, sizeof(fcb));
    read_block(fcb_buffer, fcb_block+9);
    int32_t index = fcb_buffer[fcb_offset].index_block_no;
    free(fcb_buffer);

    int32_t * index_buffer = calloc(1024, sizeof(int32_t));
    read_block(index_buffer, index);

    int read_counter = 0;
    int position_offset_block = open_file_table[fd].position_offset / BLOCKSIZE;
    int position_offset_off = open_file_table[fd].position_offset % BLOCKSIZE;

    char * file;
    file = calloc(BLOCKSIZE, sizeof(char));
    read_block(file, index_buffer[position_offset_block]);
    while (read_counter < n) 
    {
        if (position_offset_off == BLOCKSIZE) {
            // Go to the next block
            position_offset_off = 0;
            position_offset_block++;
            free(file);
            file = calloc(BLOCKSIZE, sizeof(char));
            read_block(file, index_buffer[position_offset_block]);
            casted_buf[read_counter] = file[position_offset_off];
        } else {
            casted_buf[read_counter] = file[position_offset_off];
        }
        read_counter++;
        position_offset_off++;
    }
    open_file_table[fd].position_offset = open_file_table[fd].position_offset + n;
    free(file);
    free(index_buffer);
    return (read_counter);
}

int sfs_append(int fd, void *buf, int n)
{
    char * casted_buf = buf;

    // Check validity of fd
    if(fd >= MAX_FILE_COUNT || fd < 0) {
        printf("fd error in sfs_read %d\n", fd);
        return -1;
    }

    // Check validity of mode
    if (open_file_table[fd].mode == MODE_READ){
        printf("Mode is incorrect, file is read-only\n");
        return -1;
    }

    if (open_file_table[fd].file_size + n > 4194304) {
        n = 4194304 - open_file_table[fd].file_size;
    }

    int file_size = open_file_table[fd].file_size;

    //Is no of free block enough for new allocated no of blocks? 
    superblock *super_block_buffer = (superblock*) calloc(1, BLOCKSIZE);
    read_block((void*) super_block_buffer, 0);

    int total_req = ((file_size + n) / BLOCKSIZE) + 1;
    int req_no = total_req - ((file_size / BLOCKSIZE) + 1); 

    if(req_no > super_block_buffer -> free_block_count){
        printf("Not enough block to allocate new data.\n");
        return -1;
    }

    int write_count = 0;
    int32_t free_block;

    // Find the file's index table
    int fcb_block = open_file_table[fd].FCB_index / 32;
    int fcb_offset = open_file_table[fd].FCB_index % 32;
    fcb *fcb_buffer = (fcb*) calloc(32, sizeof(fcb));

    read_block(fcb_buffer, fcb_block+9);
    int32_t index = fcb_buffer[fcb_offset].index_block_no;

    int32_t * index_buffer = calloc(1024, sizeof(int32_t));
    read_block(index_buffer, index);

    int f_block;
    char * file;
    int f_off = (open_file_table[fd].file_size) % BLOCKSIZE;

    f_block = (open_file_table[fd].file_size) / BLOCKSIZE;
    
    file = calloc(BLOCKSIZE, sizeof(char));
    read_block(file, index_buffer[f_block]);

    while(write_count < n){
        if (file_size % BLOCKSIZE  == 0) {
            free(file);
            // Find free block for file's data block
            free_block = findAndSetFirst();
            super_block_buffer -> free_block_count = super_block_buffer -> free_block_count - 1;
            
            //write file to block
            file = calloc(BLOCKSIZE, sizeof(char));
            write_block(file, free_block);
            
            free(file);

            file = calloc(BLOCKSIZE, sizeof(char));
            read_block(file, free_block);
            
            // read and write to index block
            index_buffer[(file_size / BLOCKSIZE)] = free_block;
            write_block(index_buffer, index);
            
            f_off = 0;
        }

        file[f_off] = casted_buf[write_count];
        f_off++;
        write_count++;
        write_block(file, index_buffer[file_size / BLOCKSIZE]);
        file_size++;
    }
    open_file_table[fd].file_size = file_size;
    fcb_buffer[fcb_offset].file_size = file_size;
    write_block(fcb_buffer, fcb_block+9);

    free(file);
    free(super_block_buffer);
    free(fcb_buffer);
    free(index_buffer);
    return write_count;
}

int sfs_delete(char *filename)
{
    char file_name[110];
    strcpy(file_name, filename);
    
    /* Search directory entries */
    // Find file_name from directory
    int fcb_index = -1;
    
    directory_entry *directoryEntry;
    int flag = 0;
    int dir_offset;
    int dir_block;
    for (int i = 0; i < 4; i++){
        directoryEntry = (directory_entry *) calloc(32, sizeof(directory_entry));
        read_block(directoryEntry, i + 5);

        for (int j = 0; j < 32; j++){
            if (strcmp(directoryEntry[j].file_name, file_name) == 0){
                dir_block = i + 5;
                dir_offset = j;
                fcb_index = directoryEntry[j].FCB_index;
                free(directoryEntry);
                flag = 1;
                break;
            }
        }
        if (flag == 1){
            break;
        }
        free(directoryEntry);
    }

    // If file_name does not exist, return -1
    if (flag == 0) {
        printf("file_name not found in directory\n");
        return -1;
    }

    // Find FCB
    int fcb_block = fcb_index / 32;
    int fcb_offset = fcb_index % 32;
    fcb *fcb_buffer = (fcb*) calloc(32, sizeof(fcb));
    read_block(fcb_buffer, fcb_block+9);

    int32_t *index_table = calloc(1024, sizeof(int32_t));
    read_block(index_table, fcb_buffer[fcb_offset].index_block_no);
    int index_count = ((fcb_buffer[fcb_offset].file_size -1) / BLOCKSIZE) + 1;

    void *file;

    // Delete file content
    for (int i = 0; i < index_count; i++){
        
        file = calloc(BLOCKSIZE, sizeof(char));
        write_block(file, index_table[i]);

        free(file);

        // Clear the file's data block's corresponding bitvector
        ClearBitVector(index_table[i]);
    }

    // Delete index block
    ClearBitVector(fcb_buffer[fcb_offset].index_block_no);
    free(index_table);
    index_table = calloc(1024, sizeof(int32_t));
    write_block(index_table, fcb_buffer[fcb_offset].index_block_no);
    
    // Delete FCB
    fcb_buffer[fcb_offset].index_block_no = -1;
    fcb_buffer[fcb_offset].is_available = 0;
    fcb_buffer[fcb_offset].file_size = -1;
    write_block(fcb_buffer, fcb_block + 9);
    
    // Delete dir entry
    directoryEntry = (directory_entry *) calloc(32, sizeof(directory_entry));
    read_block(directoryEntry, dir_block);

    strcpy(directoryEntry[dir_offset].file_name, "");
    
    directoryEntry[dir_offset].FCB_index = -1;
    directoryEntry[dir_offset].is_available = 0;
    write_block(directoryEntry, dir_block);

    // Free memory
    free(fcb_buffer);
    free(directoryEntry);
    free(index_table);

    return (0);
}
