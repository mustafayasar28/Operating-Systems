#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdbool.h>

#include "commondefs.h"

int main() {

	char sharedmem_name[100];
	int fd;
	
	void *shm_start;
	struct shared_data *sdata_ptr;
	
	struct stat sbuf;
	
	strcpy(sharedmem_name, SHARED_MEM_NAME);
	
	fd = shm_open(sharedmem_name, O_RDWR, 0660);
	
	if (fd < 0) {
		perror("consumer can't open shared the shared mem");
		exit(1);
	} else {
		printf("shared mem open success, fd=%d\n", fd);
	}
	
	fstat(fd, &sbuf);
	
	printf("size of shared mem = %d\n", (int) sbuf.st_size);
	
	shm_start = mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	if (shm_start < 0) {
		perror("cannot map shared memory\n");
		exit(1);
	} else {
		printf("mapping ok, start address = %lu\n", (unsigned long) shm_start);
	}
	
	close(fd);
	
	
	sdata_ptr = (struct shared_data *) shm_start;
	
	int i = 0;
	while(1) {
		while( sdata_ptr -> in != 3)
			;
		
		printf("Student %d info: \n", i+1);
		printf("ID: %d\n", sdata_ptr -> buffer[i].id);
		printf("Name: %s\n", sdata_ptr -> buffer[i].name);
		printf("Last Name: %s\n", sdata_ptr -> buffer[i].lastname);
		printf("Age: %d\n", sdata_ptr -> buffer[i].age);
		printf("CGPA: %f\n\n", sdata_ptr -> buffer[i].cgpa);
		
		i++;
		sleep(1);
		if (i == 3) {
			sdata_ptr -> finished = true;
			break;
		}
	}
	shm_unlink(sharedmem_name);
	exit(0);
}
	
	
	
	
	
	
	
	