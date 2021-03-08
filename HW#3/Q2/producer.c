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
	int i;
	int fd;
	char sharedmem_name[100];
	
	void *shm_start;
	struct shared_data *sdata_ptr;
	struct stat sbuf;
	
	strcpy(sharedmem_name, SHARED_MEM_NAME);
	
	fd = shm_open(sharedmem_name, O_RDWR | O_CREAT, 0660);
	if (fd < 0) {
		perror("CAnnot create shared memory\n");
		exit(1);
	} else {
		printf("sharedmem create success, fd = %d\n", fd);
	}
	
	ftruncate(fd, SHARED_MEM_SIZE);
	fstat(fd, &sbuf);
	printf("size = %d\n", (int) sbuf.st_size);
	
	shm_start = mmap(NULL, sbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	if (shm_start < 0) {
		perror("cannot map the shared memory\n");
		exit(1);
	} else {
		printf("mapping ok, start adress = %lu\n",
			(unsigned long) shm_start);
	}
	
	close(fd);
	
	sdata_ptr = (struct shared_data *) shm_start;
	sdata_ptr -> in = 0;
	
	i = 0;
	sdata_ptr -> finished = false;
	while(1) {
		
		if (i == 0) {
			sdata_ptr -> buffer[i].id = 1000;
			strcpy(sdata_ptr -> buffer[i].name, "Mustafa");
			strcpy(sdata_ptr -> buffer[i].lastname, "Yasar");
			sdata_ptr -> buffer[i].age = 21;
			sdata_ptr -> buffer[i].cgpa = 3.27;
		} else if (i == 1) {
			sdata_ptr -> buffer[i].id = 2000;
			strcpy(sdata_ptr -> buffer[i].name, "Ahmet");
			strcpy(sdata_ptr -> buffer[i].lastname, "Selam");
			sdata_ptr -> buffer[i].age = 20;
			sdata_ptr -> buffer[i].cgpa = 3.30;
		} else if (i == 2) {
			sdata_ptr -> buffer[i].id = 3000;
			strcpy(sdata_ptr -> buffer[i].name, "Ali");
			strcpy(sdata_ptr -> buffer[i].lastname, "Merhaba");
			sdata_ptr -> buffer[i].age = 19;
			sdata_ptr -> buffer[i].cgpa = 3.40;
		}
		printf("Student %d has been put into buffer\n", i + 1);
		printf("Student %d info: \n", i+1);
		printf("ID: %d\n", sdata_ptr -> buffer[i].id);
		printf("Name: %s\n", sdata_ptr -> buffer[i].name);
		printf("Last Name: %s\n", sdata_ptr -> buffer[i].lastname);
		printf("Age: %d\n", sdata_ptr -> buffer[i].age);
		printf("CGPA: %f\n\n", sdata_ptr -> buffer[i].cgpa);
		
		
		i++;
		sdata_ptr -> in = i;
		sleep(1);
		if (sdata_ptr -> in == 3)
			break;
	}
	
	exit(0);
}