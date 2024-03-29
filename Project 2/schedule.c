/* Mustafa Yasar CS-342 Operating Systems Project 2 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <math.h>
#include <stdbool.h>
#include <sys/time.h>

double My_random(unsigned seed);

/* The node that the threads will add to the runqueue */
struct runqueue_elem {
	struct runqueue_elem *next;
	int thread_index;
	int burst_index;
	double length;	/* in ms */
	long int arrival_time;
	long int exe_start_time;
};

/* The queue (linked list) that the threads will add the runqueue_elems into */
struct runqueue {
	struct runqueue_elem *head;
	struct runqueue_elem *tail;
	int count; /* Number of runqueue elements in the queue */
};

/* Constructor of the runqueue */
void runqueue_init(struct runqueue *rq) {
	rq->count = 0;
	rq->head = NULL;
	rq->tail = NULL;
}

/* Runqueue insertion, adds the element to the tail of the queue */
void runqueue_insert(struct runqueue *q, struct runqueue_elem *qe) {
	if (q -> count == 0) {
		q->head = qe;
		q->tail = qe;
	} else {
		q->tail->next = qe;
		q->tail = qe;
	}

	q->count++;
}

/* Delete a specific burst node from the runqueue */
void runqueue_remove(struct runqueue *rq, struct runqueue_elem *qe) {
    if ( rq->count == 0 ) return;

    if ( rq->count == 1 ) {
        rq -> head = NULL;
        rq -> tail = NULL;
        rq -> count--;
        free(qe);
        return;
    }

    if ( qe == rq -> head ) {
        rq -> head = qe -> next;
        free(qe);
        rq -> count--;
        return;
    }

    struct runqueue_elem *previous = rq -> head;

    while ( previous -> next != qe ) {
        previous = previous -> next;
    }

    if ( qe == rq -> tail ) {
        rq -> tail = previous;
        rq -> tail -> next = NULL;
        rq -> count--;
        free(qe);
        return;
    }

    previous -> next = qe -> next;
    free(qe);
    rq -> count--;
}

/* Runqueue retrieval, retrieves the head node and deletes it from the queue */
struct runqueue_elem * runqueue_retrieve(struct runqueue *rq) {
	struct runqueue_elem *qe;

	if ( rq->count == 0 ) return NULL;

	qe = rq->head;

	rq->head = rq->head->next;
	rq->count--;

	return qe;
}

/* The object that is shared by the threads */
struct shared_buffer {
	struct runqueue *rq;
	pthread_mutex_t mutex_queue;	/* mutex to ensure synchronization among threads */
	pthread_cond_t	cond_hasburst;	/* will cause the S Thread to wait */
};

struct shared_buffer *buf;
char algorithm[15];
int total_burst_count;
int Bcount;
int N;
double vruntime_info[10];
int mode;
char file_name[20];
struct timeval start;
struct runqueue *waiting_times;

void shared_buffer_add(struct shared_buffer *sbp, struct runqueue_elem *elem) {
    pthread_mutex_lock( &(sbp -> mutex_queue) );

    /* Critical section, put the burst into the queue */
    printf("Thread index:%d is in critical section\n", elem->thread_index);

    struct timeval end;
    gettimeofday(&end, NULL);
    long seconds = (end.tv_sec - start.tv_sec);
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

    elem->arrival_time = micros / 1000;

    runqueue_insert(sbp->rq, elem);

    if (sbp->rq->count > 0) {
        pthread_cond_signal(&(sbp->cond_hasburst));
    }

    printf("Thread index:%d EXITTED the critical section\n", elem->thread_index);
    pthread_mutex_unlock( &(sbp->mutex_queue) );
}

struct thread_args {
	int i;		/* Thread index */
	int avgB;
	int avgA;
	int Bcount;
	int minA;
	int minB;
};

struct s_thread_args {
    char alg[15];
};

void * generate_burst(void * _arg) {
	/* Generate a sequence of cpu bursts one by by
		and add them to rq.

		Length of each burst is random.

		avgA >= 100 and avgB >= 100

		f(x) = lambda * exp(-lambda * x).

		Here, the lambda is the rate parameter. It is 1/avgA (for interarrival times).
		It is 1/avgB (for burst times)

		We will generate a random number between 0 and 1 UNTIL the generated
		random number is greater than minA / 1000 (For inter-arrival time and sleeping time)

		We will generate a random number between 0 and 1 UNTIL the generated
		random number is greater than minB / 1000 (For Burst time)

	*/
	if (mode == 1) {
        struct thread_args *th_args = (struct thread_args *) _arg;

        int index = th_args -> i + 1;
        int avgB = th_args -> avgB;
        int avgA = th_args -> avgA;
        int Bcount = th_args -> Bcount;
        int minA = th_args -> minA;
        int minB = th_args -> minB;

        My_random(index); /* Different seeds for different threads */
        double lambda = (double) 1 / (double) avgA;
        double rand_num = (double) rand() / (double) RAND_MAX;
        double arrival_sleep_time = (double) -1/lambda * log(1 - rand_num);

        while (arrival_sleep_time < minA) {
            rand_num = (double) rand() / (double) RAND_MAX;
            arrival_sleep_time = (double) -1/lambda * log(1 - rand_num);
        }

        printf("Thread %d is created, sleeping for %lf ms\n", index, arrival_sleep_time);

        /* arrival_sleep_time is in ms */
        usleep( arrival_sleep_time * 1000 ); /* Convert mikroseconds to milliseconds */


        for (int i = 0; i < Bcount; i++) {
            double b_lambda = (double) 1 / (double) avgB;
            double b_rand_num = (double) rand() / (double) RAND_MAX;
            double burst_length = (double) -1/b_lambda * log(1 - b_rand_num);

            while (burst_length < minB) {
                b_rand_num = (double) rand() / (double) RAND_MAX;
                burst_length = (double) -1/b_lambda * log(1 - b_rand_num);
            }

            struct runqueue_elem *burst;
            burst = (struct runqueue_elem *) malloc(sizeof(struct runqueue_elem));

            burst->thread_index = index;
            burst->burst_index = i + 1;
            burst->length = burst_length;
            burst->next = NULL;

            /* Try adding the burst into the runqueue */
            shared_buffer_add(buf, burst);

            printf("Thread index:%d, has added burst #%d, length=%f "
                    "At time: %ld\n", index, i + 1, burst->length, burst -> arrival_time);

            /* Sleep again before creating the other burst */
            double after_burst = (double) 1 / (double) avgA;
            double after_burst_rand_num = (double) rand() / (double) RAND_MAX;
            double after_burst_sleeping_time = (double) -1 / after_burst * log(1 - after_burst_rand_num);

            while(after_burst_sleeping_time < minA) {
                after_burst_rand_num = (double) rand() / (double) RAND_MAX;
                after_burst_sleeping_time = (double) -1 / after_burst * log(1 - after_burst_rand_num);
            }

            printf("Thread index: %d, after adding, sleeping for %f ms\n", index, after_burst_sleeping_time);
            usleep(after_burst_sleeping_time * 1000);
        }
        free(th_args);
    } else if ( mode == 2 ) {
        struct thread_args *th_args = (struct thread_args *) _arg;
        int index = th_args -> i + 1;

        FILE *fp;

        My_random(index); /* Different seeds for different threads */

        char fileName[30];
        char new_file[40];

        strcpy(fileName, file_name);

        /* Prepare the name of the file to be read */
        strcat(fileName, "-");
        sprintf(new_file, "%s%d", fileName, index);
        strcat(new_file, ".txt");
        printf("%s\n", new_file);

        // Open the file
        fp = fopen(new_file, "r");
        if ( fp == NULL ) {
            printf("Could not be opened file %s\n", new_file);
            exit(1);
        }

        int burst_count = 0;

        for (char c = getc(fp); c != EOF; c = getc(fp) ) {
            if ( c == '\n' ) {
                burst_count = burst_count + 1;
            }
        }

        burst_count--;
        fclose(fp);

        fp = fopen(new_file, "r");

        if ( fp == NULL ) {
            printf("Could not be opened file %s\n", new_file);
            exit(1);
        }

        double sleeping_length;
        fscanf(fp, "%lf", &sleeping_length);

        printf("Thread %d is created, sleeping for %lf ms\n", index, sleeping_length);

        usleep(sleeping_length * 1000);

        for (int i = 1; i <= burst_count; i++) {


            double burst_time;
            double arrival_time;


            fscanf(fp, "%lf %lf", &burst_time, &arrival_time);

            printf("Burst index: %d, burst time: %f, inter time: %f\n", i, arrival_time, burst_time);

            struct runqueue_elem *burst;
            /*	buf = (struct shared_buffer *) malloc(sizeof(struct shared_buffer));*/
            burst = (struct runqueue_elem *) malloc(sizeof(struct runqueue_elem));

            burst->thread_index = index;
            burst->burst_index = i;
            burst->length = burst_time;
            burst->next = NULL;

            /* Try adding the burst into the runqueue */
            shared_buffer_add(buf, burst);
            printf("Thread index:%d, has added burst #%d, length=%f, at time: %ld\n", index, i, burst->length, burst->arrival_time);
            printf("Thread index:%d, after adding, sleeping for %f\n", index, arrival_time);

            /* Sleep again before creating the other burst */
            usleep(arrival_time * 1000);
        }
        fclose(fp);
        free(th_args);
    }
	pthread_exit(NULL);
}

void * s_thread_func(void * arg) {

    printf("S-THREAD Created.\n");

    pthread_mutex_lock(&(buf->mutex_queue));
    printf("S Thread is waiting for an item to be added to the runqueue\n");
    while ( buf->rq->count == 0 ) {
        pthread_cond_wait( &(buf -> cond_hasburst), &(buf->mutex_queue) );
    } /* S Thread sleeps as long as the runqueue is empty */

    pthread_mutex_unlock(&(buf->mutex_queue));
    printf("An item is added to the runqueue, S Thread is awaken\n");

    int executed_burst_count = 0;

    if (strcmp(algorithm, "FCFS") == 0) {
        do {
            pthread_mutex_lock(&(buf->mutex_queue));
            while (buf -> rq -> count == 0 ) {
                pthread_cond_wait( &(buf -> cond_hasburst), &(buf -> mutex_queue));
            }

            struct runqueue_elem *elem = runqueue_retrieve(buf -> rq);

            pthread_mutex_unlock(&(buf->mutex_queue));

            struct timeval end;
            gettimeofday(&end, NULL);
            long seconds = (end.tv_sec - start.tv_sec);
            long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

            elem->exe_start_time = micros / 1000;
            printf("S Thread is executing the burst of thread #%d, at time: %ld. It will sleep for %f ms\n", elem->thread_index, elem->exe_start_time, elem->length);
            usleep( elem -> length * 1000);

            printf("S Thread has executed the burst %d of thread #%d.\n", elem->burst_index, elem -> thread_index);

            executed_burst_count++;

            struct runqueue_elem *new_elem;
            new_elem = (struct runqueue_elem *) malloc(sizeof(struct runqueue_elem));

            new_elem->arrival_time=elem->arrival_time;
            new_elem->exe_start_time=elem->exe_start_time;
            new_elem->burst_index=elem->burst_index;
            new_elem->thread_index=elem->thread_index;
            new_elem->next = NULL;

            runqueue_insert(waiting_times, new_elem);

            free(elem);
        } while(executed_burst_count < total_burst_count );
    } else if (strcmp(algorithm, "SJF") == 0) {

        /* First, determine which burst has the smallest burst length, */
        /* Select the burst having shortest burst_length whose burst_index is i
            i will be 1...Bcount
        */

        struct runqueue_elem *copy;
        struct runqueue_elem *next_burst;


        while(true) {
            /* Lock the queue, so that while searching for a burst to execute,
            there will be no adding to the queue. */

            pthread_mutex_lock(&(buf->mutex_queue));
            while(buf -> rq -> count == 0) {
                pthread_cond_wait(&(buf->cond_hasburst), &(buf->mutex_queue));
            }

            copy = buf -> rq -> head;
            next_burst = copy;
            double shortest = copy -> length;

            while( copy != NULL ) {
                if ( (copy -> thread_index != next_burst -> thread_index) ) {
                    if ((copy -> length < shortest)) {
                        next_burst = copy;
                        shortest = next_burst -> length;
                    }
                }
                copy = copy -> next;
            }

            printf("\nNext burst info: thid: %d, bid: %d\n", next_burst ->thread_index, next_burst->burst_index);

            pthread_mutex_unlock(&(buf -> mutex_queue));

            /* Burst having shortest length has been found found, executing. */
            struct timeval end;
            gettimeofday(&end, NULL);
            long seconds = (end.tv_sec - start.tv_sec);
            long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

            next_burst->exe_start_time = micros / 1000;

            printf("S Thread is executing Thread index:%d , Burst index:%d, Length:%f, at time %ld\n", next_burst->thread_index,
                                    next_burst->burst_index, next_burst->length, next_burst->exe_start_time);

            usleep(next_burst -> length * 1000);

            printf("S Thread has executed Thread index:%d , Burst index:%d, Length:%f\n", next_burst->thread_index,
                                    next_burst->burst_index, next_burst->length);


            struct runqueue_elem *new_elem;
            new_elem = (struct runqueue_elem *) malloc(sizeof(struct runqueue_elem));

            new_elem->arrival_time=next_burst->arrival_time;
            new_elem->exe_start_time=next_burst->exe_start_time;
            new_elem->burst_index=next_burst->burst_index;
            new_elem->thread_index=next_burst->thread_index;
            new_elem->next = NULL;

            runqueue_insert(waiting_times, new_elem);

            runqueue_remove(buf -> rq, next_burst);
            executed_burst_count++;
            if ( executed_burst_count == total_burst_count ) break;
        }
    } else if (strcmp(algorithm, "PRIO") == 0) {
        struct runqueue_elem *copy;
        struct runqueue_elem *next_burst;

        do {
            pthread_mutex_lock(&(buf->mutex_queue));
            while(buf -> rq -> count == 0) {
                pthread_cond_wait(&(buf->cond_hasburst), &(buf->mutex_queue));
            }

            copy = buf -> rq -> head;
            next_burst = copy;

            while ( copy != NULL ) {
                if ( copy -> thread_index < next_burst -> thread_index ) {
                    next_burst = copy;
                }
                copy = copy -> next;
            }

            pthread_mutex_unlock(&(buf -> mutex_queue));

            printf("\nNext burst info: thid: %d, bid: %d\n", next_burst ->thread_index, next_burst->burst_index);

            /* Burst having shortest length has been found found, executing. */
            struct timeval end;
            gettimeofday(&end, NULL);
            long seconds = (end.tv_sec - start.tv_sec);
            long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

            next_burst->exe_start_time = micros / 1000;

            printf("S Thread is executing Thread index:%d , Burst index:%d, Length:%f, at time: %ld\n", next_burst->thread_index,
                                    next_burst->burst_index, next_burst->length, next_burst->exe_start_time);

            usleep( next_burst -> length * 1000);

            printf("S Thread has executed Thread index:%d , Burst index:%d, Length:%f\n", next_burst->thread_index,
                                    next_burst->burst_index, next_burst->length);

            struct runqueue_elem *new_elem;
            new_elem = (struct runqueue_elem *) malloc(sizeof(struct runqueue_elem));

            new_elem->arrival_time=next_burst->arrival_time;
            new_elem->exe_start_time=next_burst->exe_start_time;
            new_elem->burst_index=next_burst->burst_index;
            new_elem->thread_index=next_burst->thread_index;
            new_elem->next = NULL;

            runqueue_insert(waiting_times, new_elem);

            runqueue_remove(buf -> rq, next_burst);
            executed_burst_count++;

        } while(executed_burst_count < total_burst_count);
    } else if (strcmp(algorithm, "VRUNTIME") == 0 ) {
        struct runqueue_elem *copy;
        struct runqueue_elem *next_burst;

        do {

            int smallest_index = 0;
            double smallest_vruntime = vruntime_info[0];
            for (int i = 0; i < N; i++) {
                if ( vruntime_info[i] < smallest_vruntime ) {
                    smallest_vruntime = vruntime_info[i];
                    smallest_index = i;
                }
            }

            smallest_index++;

            pthread_mutex_lock( &(buf -> mutex_queue ));
            while (buf -> rq -> count == 0) {
                pthread_cond_wait(&(buf -> cond_hasburst), &(buf -> mutex_queue));
            }
            copy = buf -> rq -> head;
            next_burst = copy;

            while ( copy != NULL ) {
                // int vruntime = vruntime_info[ copy -> thread_index - 1];

                if ( copy -> thread_index != smallest_index ) {
                    copy = copy -> next;
                } else {
                    next_burst = copy;
                    break;
                }
            }

            pthread_mutex_unlock(&(buf -> mutex_queue));

            struct timeval end;
            gettimeofday(&end, NULL);
            long seconds = (end.tv_sec - start.tv_sec);
            long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

            next_burst->exe_start_time = micros / 1000;

            printf("\nNext burst info: thid: %d, bid: %d\n", next_burst ->thread_index, next_burst->burst_index);
            printf("S Thread is executing Thread index:%d , Burst index:%d, Length:%f, At time: %ld\n", next_burst->thread_index,
                                    next_burst->burst_index, next_burst->length, next_burst->exe_start_time);

            usleep(next_burst -> length * 1000);

            printf("S Thread has executed Thread index:%d , Burst index:%d, Length:%f\n", next_burst->thread_index,
                                    next_burst->burst_index, next_burst->length);


            vruntime_info[ next_burst -> thread_index -1 ] += next_burst->length * (0.7 + 0.3 * next_burst -> thread_index);

            for (int i = 0; i < N; i++) {
                printf("vruntime[%d] = %f \n", i, vruntime_info[i]);
            }

            struct runqueue_elem *new_elem;
            new_elem = (struct runqueue_elem *) malloc(sizeof(struct runqueue_elem));

            new_elem->arrival_time=next_burst->arrival_time;
            new_elem->exe_start_time=next_burst->exe_start_time;
            new_elem->burst_index=next_burst->burst_index;
            new_elem->thread_index=next_burst->thread_index;
            new_elem->next = NULL;

            runqueue_insert(waiting_times, new_elem);

            runqueue_remove(buf -> rq, next_burst);
            executed_burst_count++;
        } while (executed_burst_count < total_burst_count);
    }
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {


    gettimeofday(&start, NULL);

    srand(time(NULL));

	if ( argc != 8 && argc != 5 ) {
		printf("usage: schedule <N><Bcount><minB><avgB><minA><avgA><ALG>\n");
		printf("or\n");
		printf("usage: schedule <N><ALG> -f <inprefix>-i.txt\n");
		exit(1);
	}

	if ( argc == 8 )
        mode = 1;
    else if (argc == 5)
        mode = 2;

    N = atoi(argv[1]);

    /* Construct the buffer and runqueue */
    buf = (struct shared_buffer *) malloc(sizeof(struct shared_buffer));
    buf -> rq = (struct runqueue *) malloc(sizeof(struct runqueue));
    waiting_times = (struct runqueue *) malloc(sizeof(struct runqueue));

    /* Initializations */
    runqueue_init(buf -> rq);
    runqueue_init(waiting_times);
    pthread_mutex_init(&buf -> mutex_queue, NULL);
    pthread_cond_init(&buf -> cond_hasburst, NULL);

    if ( mode == 1 ) {
        int avgA = atoi(argv[6]);
        int avgB = atoi(argv[4]);
        int minB = atoi(argv[3]);
        int minA = atoi(argv[5]);
        strcpy(algorithm, argv[7]);
        Bcount = atoi(argv[2]);
        total_burst_count = N * Bcount;

        if (    (strcmp(algorithm, "PRIO") != 0) &&
                (strcmp(algorithm, "FCFS") != 0) &&
                (strcmp(algorithm, "SJF") != 0)  &&
                (strcmp(algorithm, "VRUNTIME") != 0) )
        {
            printf("Algorithm must be one of the 'PRIO', 'FCFS', 'SJF', 'VRUNTIME' ");
	    free(buf -> rq);
            free(buf);
            free(waiting_times);
            exit(1);
        }


        pthread_t w_thread_pids[N];
        pthread_t s_thread_id;
        int ret;
        int i;


        printf("Creating S_thread.\n");
        ret = pthread_create(&s_thread_id, NULL, s_thread_func, (void *) NULL);
        if (ret < 0) {
            perror("s_thread could not be created\n");
            exit(1);
        }

        /* Create the W threads */
        for (i = 0; i < N; ++i) {
            printf("Creating W_thread_%d.\n", i + 1);

            /* Adjust thread parameters */
            struct thread_args *th_args = calloc( sizeof(struct thread_args), 1);

            th_args -> avgB = avgB;
            th_args -> avgA = avgA;
            th_args -> Bcount = Bcount;
            th_args -> i = i;
            th_args -> minA = minA;
            th_args -> minB = minB;

            ret = pthread_create(&w_thread_pids[i], NULL, generate_burst, th_args);

            if (ret < 0) {
                perror("thread could not be created\n");
                exit(1);
            }
        }

        /* Wait for the W_threads to terminate */
        for (i = 0; i < N; ++i)  {
            pthread_join(w_thread_pids[i], NULL);
        }

        pthread_join(s_thread_id, NULL);	/* Wait for the S_thread to terminate */

        printf("\n\nProgram Finished. Runqueue information \n\n");

        while( buf->rq->count > 0 ) {
            printf("Queue element %d, length=%f, thread_index=%d \n", i, buf->rq->head->length, buf->rq->head->thread_index);
            struct runqueue_elem *elem = runqueue_retrieve(buf->rq);
            free(elem);
        }
    } else {


        strcpy(algorithm, argv[2]);

        if (    (strcmp(algorithm, "PRIO") != 0) &&
                (strcmp(algorithm, "FCFS") != 0) &&
                (strcmp(algorithm, "SJF") != 0)  &&
                (strcmp(algorithm, "VRUNTIME") != 0) )
        {
            printf("Algorithm must be one of the 'PRIO', 'FCFS', 'SJF', 'VRUNTIME' ");
            exit(1);
        }

        printf("Reading burst information from files\n"
                "Every thread will generate 10 bursts by reading these information from files.\n"
                "There will be total %d threads\n", N);

        pthread_t w_thread_pids[N];
        pthread_t s_thread_id;
        int ret;
        int i;
        strcpy(file_name, argv[4]);

        /* First calculate the total burst count that will be generated in the program */
        for (int i = 1; i <= N; i++) {
            FILE *fp;
            char fileName[30];
            char new_file[40];
            strcpy(fileName, file_name);

            /* Prepare the name of the file to be read */
            strcat(fileName, "-");
            sprintf(new_file, "%s%d", fileName, i);
            strcat(new_file, ".txt");
            printf("%s\n", new_file);

            // Open the file
            fp = fopen(new_file, "r");
            if ( fp == NULL ) {
                printf("Could not be opened file %s\n", new_file);
                exit(1);
            }

            int burst_count = -1;

            for (char c = getc(fp); c != EOF; c = getc(fp) ) {
                if ( c == '\n' ) {
                    burst_count = burst_count + 1;
                }
            }

            total_burst_count += burst_count;

            fclose(fp);
        }
        printf("Creating S_thread.\n");
        ret = pthread_create(&s_thread_id, NULL, s_thread_func, (void *) NULL);
        if (ret < 0) {
            perror("s_thread could not be created\n");
            exit(1);
        }


        /* Create the W threads */
        for (i = 0; i < N; ++i) {
            printf("Creating W_thread_%d.\n", i + 1);

            /* Adjust thread parameters */
            struct thread_args *th_args = calloc( sizeof(struct thread_args), 1);

            th_args -> i = i;

            ret = pthread_create(&w_thread_pids[i], NULL, generate_burst, th_args);

            if (ret < 0) {
                perror("thread could not be created\n");
                exit(1);
            }
        }

        /* Wait for the W_threads to terminate */
        for (i = 0; i < N; ++i)  {
            pthread_join(w_thread_pids[i], NULL);
        }

        pthread_join(s_thread_id, NULL);	/* Wait for the S_thread to terminate */

	while( buf->rq->count > 0 ) {
            printf("Queue element %d, length=%f, thread_index=%d \n", i, buf->rq->head->length, buf->rq->head->thread_index);
            struct runqueue_elem *elem = runqueue_retrieve(buf->rq);
            free(elem);
        }

    }

    double sum = 0;
    double count = 0;
    struct runqueue_elem *elem;
    while(waiting_times->count != 0) {
        elem = runqueue_retrieve(waiting_times);

        printf("Thread Index: %d, Burst index: %d, Arrival Time: %ld, Exe Start time: %ld, Waiting Time: %ld\n",
                    elem -> thread_index, elem -> burst_index, elem -> arrival_time, elem -> exe_start_time,
                    elem->exe_start_time - elem->arrival_time
                );
        sum += elem->exe_start_time - elem->arrival_time;
        count++;

        free(elem);
    }

    printf("Average waiting time: %f\n", sum / count);

    free(waiting_times);
	free(buf -> rq);
	free(buf);
	return 0;
}


double My_random(unsigned seed) {
	static unsigned z;
	unsigned long tmp;

	if (seed != 0) z = seed;

	tmp = z * 279470273;
	z = tmp % 4294967291U;
	return z;
}


























