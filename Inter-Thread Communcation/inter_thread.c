
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <unistd.h>
#include<sys/time.h>
#include<pthread.h>
#include<math.h>
#include<semaphore.h>

mqd_t qdes;
struct timeval tv;
double t0, t1, t2;
int front, back;
sem_t mutex;
sem_t empty;
sem_t full;
int N, P, B, C;
int *circular_queue;

 struct t_id
{
	int thread_id;
}; 

//Only returns perfect squares
int get_sqrt( int number){
	
	double root = sqrt((double)number);
	
	if ((root - (int) root) != 0){
		return -1;
	}
	else{
		return (int) root;
	}
}

void *producer (void *arg) //Produce data
{
	struct t_id a = *(struct t_id*) arg;

	int pid = a.thread_id;
	int counter_p;
	//Starts msg with pid in order to get the appropriate produced numbers with respect to the process id.
	for (counter_p = pid; counter_p < N; counter_p = counter_p + P){
		/* only block for a limited time if the queue is empty */
		sem_wait(&empty);
		sem_wait(&mutex); //Critical section
		circular_queue[back] = counter_p;
		back = (back + 1) % B;
		sem_post(&mutex);
		sem_post(&full);	

	}
	return NULL;
}

void *consumer (void *arg) //Consume data
{	
	struct t_id* a = (struct t_id*) arg;
	int cid = a->thread_id;	
	int msg, root, counter_c;
	for (counter_c = cid; counter_c < N; counter_c = counter_c + C){
		/* only block for a limited time if the queue is empty */
		sem_wait(&full);
		sem_wait(&mutex); //Critical section
		msg = circular_queue[front];
		front = (front + 1) % B;
		sem_post(&mutex);
		sem_post(&empty);
		root = get_sqrt(msg);
		if (root >= 0){
			printf("%d %d %d \n", cid, msg, root ); //only displays perfect roots
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	gettimeofday(&tv, NULL);
	t0 = tv.tv_sec + tv.tv_usec/1000000.0;
	front = 0;
	back = 0;
	N = atoi(argv[1]);
	B = atoi(argv[2]);
	P = atoi(argv[3]);
	C = atoi(argv[4]);
	circular_queue =(int*) malloc(sizeof(int) * B);

	sem_init(&full, 0, 0);
	sem_init(&empty, 0, B);
	sem_init(&mutex, 0, 1);

	//	
	pthread_t thread_P[P];
	pthread_t thread_C[C];

	int i;
	struct t_id c_id[P];

	//Creates P producers
	for(i = 0; i < P; i++){
		c_id[i].thread_id = i;	
		if(pthread_create(&thread_P[i], NULL, producer, &c_id[i]) != 0) {
			printf ("Create pthread error!\n");
			exit (1);
		}
	}
	
	int j;
	struct t_id tc_id[C];

	//Creates C consumers: 
	for(j = 0; j < C; j++){
		
		tc_id[j].thread_id = j;		
		if(pthread_create(&thread_C[j], NULL, consumer, &tc_id[j]) != 0) {
			printf ("Create pthread error!\n");
			exit (1);
		}
	}
	
	for(i = 0; i < P; i++){
		pthread_join(thread_P[i], NULL);
	}
	
	for(i = 0; i < C; i++){
		pthread_join(thread_C[i], NULL);
	}

	gettimeofday(&tv, NULL);
	t1 = tv.tv_sec + tv.tv_usec/1000000.0;
	
	printf("Execution time: %f\n", t1-t0);
	free(circular_queue);

	//return 0;
}


