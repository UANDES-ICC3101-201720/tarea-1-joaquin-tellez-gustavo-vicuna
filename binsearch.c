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
	int R = n - 1;
	int m;
	while (L <= R){
	    	m = ((L + R) / 2);
		if (A[m] < T)
		    L = m + 1;
		else if (A[m] > T)
		    R = m - 1;
		else		
		    return m;
	}
	return -1;    
}

void *first(void * ft){
	struct final *fs = ft;
	pthread_exit((void *) (intptr_t) serial_binsearch(fs->z, fs->x, fs->y)); //verificar T
}

// TODO: implement
int parallel_binsearch(int A[], int n, int T) {
	int B[n/3];
	//B = malloc((n/3)*sizeof(char));
	int C[n/3];
	//C = malloc((n/3)*sizeof(char));
	int D[n/3];
	//D = malloc((n/3)*sizeof(char));
	
	for(int i=0; i<=n/3; i++){B[i] = A[i]; } 
	for(int i=n/3; i<=2*(n/3); i++){C[i-(n/3)] = A[i]; } 
	for(int i=2*(n/3); i<=n; i++){D[i-(2*n/3)] = A[i]; } 	//quizas se salga de la lista

	//struct final fi = {pow(10, T)/3, A[T], B}; //cambiar el t por pvalue
	struct final *fi = malloc(sizeof(struct final)+(n/3)*sizeof(int));
	fi->x = pow(10, T)/3;
	fi->y = A[T];
	memcpy(fi->z, B, (n/3)*sizeof(int));
	//struct final se = {pow(10, T)/3, A[T], C}; //verificar largo de los arreglos, puede q sean distintos
	struct final *se = malloc(sizeof(struct final)+(n/3)*sizeof(int));
	fi->x = pow(10, T)/3;
	fi->y = A[T];
	memcpy(fi->z, C, (n/3)*sizeof(int));	
	//struct final th = {pow(10, T)/3, A[T], D};
	struct final *th = malloc(sizeof(struct final)+(n/3)*sizeof(int));
	fi->x = pow(10, T)/3;
	fi->y = A[T];
	memcpy(fi->z, D, (n/3)*sizeof(int));

	pthread_t first_thread;
	pthread_t second_thread;
	pthread_t third_thread;
	//void *first();
	//void *second();
	//void *third();

	pthread_create(&first_thread, NULL, first, &fi);
	void * ft;
	pthread_join(first_thread, &ft);
	pthread_create(&second_thread, NULL, first, &se);
	pthread_create(&third_thread, NULL, first, &th);
	return 0;
}

int main(int argc, char** argv) {
	/* TODO: move this time measurement to right before the execution of each binsearch algorithms
	* in your experiment code. It now stands here just for demonstrating time measurement. */

	
	clock_t cbegin = clock();
	
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


	printf ("E = %d, T = %s, P = %d\n",
	  Evalue, Tvalue, Pvalue);

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

	/* TODO: implement code for your experiments using data provided by datagen and your
	* serial and parallel versions of binsearch.
	* */
	/* TODO: connect to datagen and ask for the necessary data in each experiment round.
	* Create a Unix domain socket with DSOCKET_PATH (see const.h).
	* Talk to datagen using the messages specified in the assignment description document.
	* Read the values generated by datagen from the socket and use them to run your
	* experiments
	* */

	struct sockaddr_un addr;
  	int fd,rc;
	char buf[100];
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
	if ((rc = write(fd, buf, sizeof(buf))) == -1){
		perror("write error\n");
		exit(-1);
	}
	printf("Enviando info: %d bytes enviados.\n", rc);
	if ((rc=read(fd,buf,sizeof(buf)))== -1) {
	        perror("read");
	        exit(-1);
	}
	if ((rc = write(fd, "END", sizeof(buf))) == -1){
		perror("write error\n");
		exit(-1);
	}



	/* Probe time elapsed. */
	clock_t cend = clock();

	// Time elapsed in miliseconds.
	double time_elapsed = ((double) (cend - cbegin) / CLOCKS_PER_SEC) * 1000;

	printf("Time elapsed '%lf' [ms].\n", time_elapsed);

	exit(0);
}
