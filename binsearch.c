#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include "types.h"
#include "const.h"
#include "util.h"

pthread_mutex_t region_mutex = PTHREAD_MUTEX_INITIALIZER;

struct final{
	int x;
	int y;
	int z[];
};

int serial_binsearch(int A[], int n, int T) {
	int L = 0;
	int R = n-1;
	int m;
	while (L <= R){
	    	m = ((L + R) / 2);
		//printf("%d\n", m);
		if (A[m] < T)
		    L = m + 1;
		else if (A[m] > T)
		    R = m - 1;
		else		
		    return m;
	}
	return -1;    
}

void *first(void * fv){
	struct final *fs = fv;
	pthread_exit((void *) (intptr_t) serial_binsearch(fs->z, fs->x, fs->y)); 
}
void *second(void * fv){
	struct final *fs = fv;
	pthread_exit((void *) (intptr_t) serial_binsearch(fs->z, fs->x, fs->y)); 
}
void *third(void * fv){
	struct final *fs = fv;
	pthread_exit((void *) (intptr_t) serial_binsearch(fs->z, fs->x, fs->y)); 
}

// TODO: implement
int parallel_binsearch(int A[], int n, int T) {
	int B[n/3];
	
	int C[n/3];
	
	int D[n/3];

	for(int i=0; i<n/3; i++){B[i] = A[i]; } 
	for(int i=n/3; i<2*(n/3); i++){C[i-(n/3)] = A[i]; } 
	for(int i=2*(n/3); i<n; i++){D[i-(2*n/3)] = A[i]; } 	

	struct final *fi = malloc(sizeof(struct final)+(n/3)*sizeof(int));
	fi->x = n/3;
	fi->y = A[T];
	memcpy(fi->z, B, (n/3)*sizeof(int));
	
	struct final *se = malloc(sizeof(struct final)+(n/3)*sizeof(int));
	se->x = n/3;
	se->y = A[T];
	memcpy(se->z, C, (n/3)*sizeof(int));	

	struct final *th = malloc(sizeof(struct final)+(n/3)*sizeof(int));
	th->x = n/3;
	th->y = A[T];
	memcpy(th->z, D, (n/3)*sizeof(int));

	pthread_t first_thread;
	pthread_t second_thread;
	pthread_t third_thread;
	
	pthread_create(&first_thread, NULL, first, fi);
	void * fv;
	pthread_join(first_thread, &fv);
	int q = (intptr_t) fv;	

	pthread_create(&second_thread, NULL, second, se);
	void * fq;
	pthread_join(second_thread, &fq);
	int w = (intptr_t) fq;

	pthread_create(&third_thread, NULL, third, th);
	void * fr;
	pthread_join(third_thread, &fr);
	int f = (intptr_t) fr;	

	if(q >= 0){return q;}
	else if(w >= 0){return (w+333);}
	else if(f >= 0){return (f+666);}
	return -1;
}

int main(int argc, char** argv) {
	/* TODO: move this time measurement to right before the execution of each binsearch algorithms
	* in your experiment code. It now stands here just for demonstrating time measurement. */

	
	//clock_t cbegin = clock();
	
	printf("[binsearch] Starting up...\n");

	/* Get the number of CPU cores available */
	printf("[binsearch] Number of cores available: '%ld'\n",
	   sysconf(_SC_NPROCESSORS_ONLN));

	/* parse arguments with getopt */

	char* Tvalue;
	int Evalue = 0;
	int Pvalue = 0;
	int index;
	int c;

	opterr = 0;


	while ((c = getopt (argc, argv, "E:T:P:")) != -1)
		switch (c){
			case 'E':
				Evalue = atoi(optarg);
				break;
			case 'T':
				Tvalue = optarg;
				break;
			case 'P':
				Pvalue = atoi(optarg);
				break;
			case '?':
				if (optopt == 'P' || optopt == 'T' || optopt == 'E')
				  fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				else if (isprint(optopt))
				  fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
				  fprintf (stderr,
					   "Unknown option character `\\x%x'.\n",
					   optopt);
				return 1;
			default:
				abort ();
		}
	for (index = optind; index < argc; index++)
		printf ("Non-option argument %s\n", argv[index]);

	/* start datagen here as a child process. */

	pid_t pid;
	pid = fork();
	if (pid == -1){   
		fprintf(stderr, "Error en fork\n");
		exit(EXIT_FAILURE);
	}
	if (pid == 0){
		execlp("./datagen","./datagen",NULL);
	}
	//else {wait(NULL); }

	/* connect to datagen and ask for the necessary data in each experiment round.
	* Create a Unix domain socket with DSOCKET_PATH (see const.h).
	* Talk to datagen using the messages specified in the assignment description document.
	* Read the values generated by datagen from the socket and use them to run your
	* experiments
	* */

	struct sockaddr_un addr;
  	int fd,rc;
	char buf[10];
	strcpy(buf,"BEGIN S");
	strcat(buf, Tvalue);
	
	if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, DSOCKET_PATH, sizeof(addr.sun_path)-1);

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("connect error");
		exit(-1);
	}
	//Sending T flag:
	if ((rc = write(fd, buf, sizeof(buf))) == -1){
		perror("write error\n");
		exit(-1);
	}

	//Reading data and saving data:
	int buf2[1000];
	int linea=0;
	int len=pow(10,atoi(Tvalue));
	int arreglo[len];
	while ((rc = read(fd, buf2, sizeof(buf2))) >= 1000){
		if (linea==0){
			for (int i=0;i<1000;i++){
				arreglo[i+linea*1000]=buf2[i+1];
			}
		}
		else{
			for (int i=0;i<1000;i++){
				arreglo[i+linea*1000]=buf2[i];
			}
		}
		linea++;
	}
	//Ending datagen process:
	if ((rc = write(fd, "END", sizeof(buf))) == -1){
		perror("write error\n");
		exit(-1);
	}
	clock_t cbegin = clock();
	/* TODO: implement code for your experiments using data provided by datagen and your
	* serial and parallel versions of binsearch.
	* */

	struct timespec start, finish;
	double elapsed1 = 0;
	double elapsed2 = 0;
	printf("E, T, TIEMPO_SERIAL (ms), TIEMPO_PARALELO(ms)\n");
	//int av_serial;
	for (int i=0; i<Evalue; i++){

		clock_gettime(CLOCK_MONOTONIC, &start);
		serial_binsearch(arreglo,len, arreglo[Pvalue]);
		clock_gettime(CLOCK_MONOTONIC, &finish);
		elapsed1 = (finish.tv_sec - start.tv_sec);
		elapsed1 += (finish.tv_nsec - start.tv_nsec) / 1000000.0;

		clock_gettime(CLOCK_MONOTONIC, &start);
		parallel_binsearch(arreglo, len, Pvalue);
		clock_gettime(CLOCK_MONOTONIC, &finish);
		elapsed2 = (finish.tv_sec - start.tv_sec);
		elapsed2 += (finish.tv_nsec - start.tv_nsec) / 1000000.0;
		printf("%d %d %lf %lf\n", i, atoi(Tvalue), elapsed1, elapsed2);	
	}

	/* Probe time elapsed. */
	clock_t cend = clock();

	// Time elapsed in miliseconds.
	double time_elapsed = ((double) (cend - cbegin) / CLOCKS_PER_SEC) * 1000;

	printf("Time elapsed '%lf' [ms].\n", time_elapsed);

	exit(0);
}
