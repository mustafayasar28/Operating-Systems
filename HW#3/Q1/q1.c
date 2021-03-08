#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>

/* Global variables that threads can reach */
/* Since the data of the arr will not be changed,
	there is no need for synchronisation
*/

const int ARRAY_SIZE = 1000;

int arr[1000];
int min;
int max;
double average;

void* max_function();
void* min_function();
void* average_function();

int main() {
	srand(time(NULL));

	/* Fill the array with random values */
	for (int i = 0; i < ARRAY_SIZE; i++) {
		arr[i] = rand() % 10000; /* Random int between 0 and 9999 */
	}

	/* Create 3 threads */
	pthread_t thread1, thread2, thread3;
	int t1, t2, t3;

	t1 = pthread_create(&thread1, NULL, (void*)max_function, NULL);
	if (t1) {
		fprintf(stderr, "Error creating first thread \n");
		exit(EXIT_FAILURE);
	}

	t2 = pthread_create(&thread2, NULL, (void*)min_function, NULL);
	if (t2) {
		fprintf(stderr, "Error creating second thread \n");
		exit(EXIT_FAILURE);
	}

	t3 = pthread_create(&thread3, NULL, (void*)average_function, NULL);
	if (t3) {
		fprintf(stderr, "Error creating third thread \n");
		exit(EXIT_FAILURE);
	}

	/* Wait for the threads to complete */

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	pthread_join(thread3, NULL);

	printf("The max is: %d\n", max);
	printf("The min is: %d\n", min);
	printf("The average is: %f\n", average);

	return 0;
}

void* max_function() {
	max = arr[0];

	for (int i = 0; i < ARRAY_SIZE; i++) {
		if (max < arr[i]) {
			max = arr[i];
		}
	}
}

void* min_function() {
	min = arr[0];

	for (int i = 0; i < ARRAY_SIZE; i++) {
		if (min > arr[i]) {
			min = arr[i];
		}
	}
}

void* average_function() {
	int sum = 0;

	for (int i = 0; i < ARRAY_SIZE; i++) {
		sum += arr[i];
	}

	average = sum / ARRAY_SIZE;
}

