
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <math.h>

mqd_t qdes;
mqd_t mq1;
mqd_t mq2;

struct timeval tv;
double t0, t1, t2;
int N, P, B, C;
int numConsumed;
int counter_c;

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

void producer (int pid)
{
	
	int msg;
	int counter_p;

	for (counter_p = pid; counter_p < N; counter_p = counter_p + P){

		//Check if ready to produce
		if (mq_receive(mq1, (char*)&msg, sizeof(int), 0) == -1){
			perror("mq_receive() failed");
		}
		else{
			//Data for consumer to receive
			msg = counter_p;
			mq_send(mq2, (char *)&msg, sizeof(int), 0);
			//}
		}
	}	
}

void consumer (int cid)
{
	int c_msg;
	int root;
	int empty = 0;
	for (counter_c = cid; counter_c < N; counter_c = counter_c + C){

		//Check if ready to consume
		if ( mq_receive(mq2, (char *) &c_msg, sizeof(int), 0) == -1) {
			perror("mq_receive() failed\n");
		}
		root = get_sqrt(c_msg);
		if (root >= 0){
			printf("%d %d %d \n", cid, c_msg, root );
		}

		//Tells producer to produce more data
		mq_send(mq1, (char *)&empty, sizeof(int), 0);
	}
}

int main(int argc, char *argv[])
{
	gettimeofday(&tv, NULL);
	t0 = tv.tv_sec + tv.tv_usec/1000000.0;
	int child_status;
	N = atoi(argv[1]);
	B = atoi(argv[2]);
	P = atoi(argv[3]);
	C = atoi(argv[4]);

	struct mq_attr attr;
	attr.mq_maxmsg  = B;
	attr.mq_msgsize = sizeof(int);
	attr.mq_flags   = 0;	/* a blocking queue  */
	char mayProduce[] = "/s3daveMayProduceP";	
	char mayConsume[] = "/s3daveMayConsumeC";
	mode_t mode = S_IRUSR | S_IWUSR;
	printf ("N: %d, B: %d, P: %d, C: %d\n", N, B, P, C);

	//Initializes POSIX queues:
	mq1  = mq_open(mayProduce, O_RDWR | O_CREAT, mode, &attr);
	mq2  = mq_open(mayConsume, O_RDWR | O_CREAT, mode, &attr);

	if (mq1 == -1 || mq2 == -1) {
		perror("mq_open() failed");
	}
	int i;
	int msg = 0;
	
	//Fill queue with empty
	for(i = 0; i < B; i++){
		if(mq_send(mq1, (char *)&msg, sizeof(int), 0) == -1 ){
			perror("mq_send() failed\n");
		}
	}

	int j, k, forkValue;
	
	//Creates P producers and C consumer processes
	for(j = 0; j < C+P; j++){
		forkValue = fork();
		//Only considers child processes
		if(forkValue == 0){ 
			if(j >= P){
				consumer(j-P);
			}
			else{
				producer(j);
			}
			break;
		}
	}
	
	//Closes POSIX queue and terminates the child processes.
	if(forkValue != 0){
		for(k = 0; k < (P+C); k++){
			wait(&child_status);
			if(WIFEXITED(child_status) != 0){
				gettimeofday(&tv, NULL);
				t1 = tv.tv_sec + tv.tv_usec/1000000.0;
				mq_close(mq1);
				mq_close(mq2);
				mq_unlink(mayProduce);
				mq_unlink(mayConsume);
			}
		}
		printf("Execution time: %f\n", t1-t0);
	}
	return 0;
}


