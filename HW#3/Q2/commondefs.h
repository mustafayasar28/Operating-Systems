#include <stdbool.h>

#define BUFFER_SIZE 3

struct Student {
	int id;
	char name[64];
	char lastname[64];
	int age;
	double cgpa;
};

struct shared_data {
	int in;
	bool finished;
	struct Student buffer[3];
};

#define SHARED_MEM_NAME "/sharedmem"
#define SHARED_MEM_SIZE 4096