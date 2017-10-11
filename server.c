


/*Swap Simulation main program
 *  *  * Author: Chun TAO
 *   *   * student ID: 879010
 To compile: gcc -o server server.c sha256.c -lpthread -std=gnu99
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>//multithread
#include <unistd.h>
#include <stddef.h>//split msg
#include <assert.h>

#include "uint256.h"// uint 256
#include "sha256.h"//sha 256 hash function
#include "byte_typedef.h"//typedef for BYTE
#include <poll.h>
#define MAX_CONN 100//max client connection
void *connection_handler(void *thread_arg); //deal with cient connection
int SOLN(char *smsg); //deal with verification SOLN msgs
char *WORK(char *smsg); //deal with prof of work requrests
void write_log(char *cli_ip, char *sever_ip,int sock, char *msg); //write down the interaction between server and client

pthread_mutex_t lock; //secure the concurrent editing

/* input struct for thread */
typedef struct {
	int client_sockfd;
	struct sockaddr_in cli_addr, serv_addr;
//  struct pollfd pfds;
} thread_args_t;

int main(int argc, char **argv) {
	int sockfd, newsockfd, portno, clilen;
 //struct pollfd pfds;
	struct sockaddr_in serv_addr, cli_addr;
	// initialize mutex lock
	if (pthread_mutex_init(&lock, NULL) != 0) {
		fprintf(stderr, "Can't initialize mutex lock\n");
		exit(EXIT_FAILURE);
	}
  //make sure the port number is provided
	if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}

	/* Create TCP socket */
  
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		perror("ERROR opening socket");
		exit(1);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);

	/* Create address we're going to listen on (given port number)
	 - converted to network byte order & any IP address for 
	 this machine */

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);  // store in machine-neutral format

	/* Bind address to the socket */

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
		exit(1);
	}

	/* Listen on socket - means we're ready to accept connections - 
	 incoming connection requests will be queued */

	listen(sockfd, MAX_CONN);

	clilen = sizeof(cli_addr);

	// keep listen to the socket to check connection requests
  
	while (1) {
 /* Accept a connection - block until a connection is ready to
	 be accepted. Get back a new file descriptor to communicate on. */
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
		if (newsockfd < 0) {
			perror("ERROR on accept");
			continue;
     // exit(1);
		}
   
   
		puts("connection accepts");
		//   printf(" client IP address is: %s\n", inet_ntoa(cli_addr.sin_addr));
		//   printf(" server IP address is: %s\n", inet_ntoa(serv_addr.sin_addr));
    //pfds.fd =newsockfd;
		//pfds.events = POLLIN;
		//Create a new thread for this client
    pthread_t sniffer_thread;
		thread_args_t *thread_arg;//set up the thread parameter
		thread_arg = (thread_args_t *) malloc(sizeof(thread_args_t));
		if (thread_arg == NULL) {
			fprintf(stderr,
					"Oh no! Not enough memory to create \
                    new thread\nSkipping request\n");
			continue;
		}
		thread_arg->client_sockfd = newsockfd;
		thread_arg->cli_addr = cli_addr;
		thread_arg->serv_addr = serv_addr;
  // thread_arg->pfds = pfds;

		int pth;
		pth = pthread_create(&sniffer_thread, NULL, connection_handler,
				(void*) thread_arg);
		if (pth < 0) {
			perror("could not create thread");
      
			continue;
     // exit(1);
		}
		//pthread_join(sniffer_thread, NULL);

	}
	/* close socket */
	close(sockfd);
	return 0;

}

void *connection_handler(void *thread_arg) {
	
  int n;
	thread_args_t arg = *((thread_args_t *) thread_arg);
	int newsock = arg.client_sockfd;
	struct sockaddr_in cli_addr, serv_addr;
	cli_addr = arg.cli_addr;
	serv_addr = arg.serv_addr;	
  //create the buffers for read() and write()
	char rbuffer[256];
	char wbuffer[256];
 // struct pollfd pfds=arg.pfds;
	//loop for continuously check to the socket 
	puts("Handler processing");
	while (1) {
		puts("\n waiting for requrest reading");
   
   /* Read characters from the connection,
	   then process */ 
		bzero(rbuffer, 256);                        		
		int j = 0;		
		n = read(newsock, &rbuffer[j], 1);
    if(n==0){
    //printf("nothing in buffer, exsiting");    
    pthread_exit(NULL);
    }
    
    if (n <0) {
				perror("ERROR reading from socket");
			  pthread_exit(NULL);
      	//exit(1);
			}
    if (n < 0) {
				perror("ERROR reading from socket");
			  pthread_exit(NULL);
      	//exit(1);
			}
		char eor = rbuffer[j];    
		j = j + 1;
   //read the data in the socket
		while (eor != '\n') {

				n = read(newsock, &rbuffer[j], 1);
        if (n < 0) {
				perror("ERROR reading from socket");
			  pthread_exit(NULL);
      	//exit(1);
			}
				eor = rbuffer[j];
				j = j + 1;
			}
			int i = 0;
			
			printf("Data in buffer is %s, size is %d\n", rbuffer,
					strlen(rbuffer));
        //get the client and server IP
				char cli_ip[20];
				strcpy(cli_ip, inet_ntoa(cli_addr.sin_addr));
				puts(cli_ip);
				char serv_ip[20];
				strcpy(serv_ip, inet_ntoa(serv_addr.sin_addr));
				puts(serv_ip);
        //write to the log file
				write_log(cli_ip, serv_ip, newsock, rbuffer);
				char header[5];
				bzero(header, 5);
				strncpy(header, rbuffer, 4);
				puts(header);
				                                    
        bzero(wbuffer, 256);
     
				if (strcmp(header, "PING") == 0 && strlen(rbuffer) == 6) {					
					strcpy(wbuffer, "PONG\r\n");                            
				} else if (strcmp(header, "PONG") == 0
						&& strlen(rbuffer) == 6) {            
					strcpy(wbuffer,
							"ERRO: ERRO messages are reserved\r\n");
				} else if (strcmp(header, "OKAY") == 0
						&& strlen(rbuffer) == 6) {
					strcpy(wbuffer,
							"ERRO: OKAY messages are reserved\r\n");
				} else if (strcmp(header, "ERRO") == 0
						&& strlen(rbuffer) == 6) {
					strcpy(wbuffer,
							"ERRO: ERROR messages are reserved\r\n");
				} else if (strcmp(header, "SOLN") == 0
						&& strlen(rbuffer) == 97) {
					int sol = 0;
					sol = SOLN(rbuffer);//call SOLN function to process SOLN msg
					printf("sol=%d\n", sol);
					if (sol == 1) {
						strcpy(wbuffer, "OKAY\r\n");
					} else {
						strcpy(wbuffer, "ERRO: invalid SOLN messages\r\n");
					}
				} else if (strcmp(header, "WORK") == 0
						&& strlen(rbuffer) == 100) {
					char w[97];
					strcpy(w, WORK(rbuffer));//cal WORK function to process WORK msg
					if (w != "e") {
						strcpy(wbuffer, w);
					} else {
						strcpy(wbuffer, "ERRO: invalid messages\r\n");
					}
				} else if (strcmp(header, "ABRT") == 0
						&& strlen(rbuffer) == 6) {
					puts("Abort thread.Disconnected.");
          strcpy(wbuffer, "OKAY\r\n");
          n = write(newsock, wbuffer, strlen(wbuffer));
				  write_log(cli_ip, serv_ip,newsock, wbuffer);
					pthread_exit(NULL);
				} else {
					strcpy(wbuffer, "ERRO: Invalid messages\r\n");
				}
				n = write(newsock, wbuffer, strlen(wbuffer));
        if (n < 0) {
					perror("ERROR writing to socket");
						pthread_exit(NULL);
				}
				write_log(cli_ip, serv_ip,newsock, wbuffer);
								
			}
	
	puts("Time out. Disconnected.");
	pthread_exit(NULL);
}

int SOLN(char *smsg) {
	puts("SOLN processing:");
	puts(smsg);

	char head[5];
	char diff[9];
	char seed[65];
	char sol[17];

	char string[97];
	strcpy(string, smsg);
	const char delimiters[] = " \r\n";
	puts(string);
  //split the smg into head, dificulty,seed and sol
	strcpy(head, strtok(string, delimiters));

	strcpy(diff, strtok(NULL, delimiters));
	strcpy(seed, strtok(NULL, delimiters));
	strcpy(sol, strtok(NULL, delimiters));
	//strcpy(head,h);
	printf("%d %d %d %d\n", strlen(head), strlen(diff), strlen(seed),
			strlen(sol));
	if (strlen(diff) != 8 || strlen(seed) != 64 || strlen(sol) != 16) {
   //check the msg format
		return -1;
	}

	puts(head);
	puts(diff);
	puts(seed);
	puts(sol);

	uint32_t diff_hex = strtoul(diff, NULL, 16);
	//unsigned long long seed_hex = strtoul(seed, NULL, 16);
	uint64_t sol_hex = strtoul(sol, NULL, 16);

//	printf(" diff value: %lx, %d\n", diff_hex, sizeof(diff_hex));
// printf(" seed value: %lx, %d\n", seed_hex,sizeof(seed_hex));
//	printf(" sol value: %lx, %d\n", sol_hex, sizeof(sol_hex));
  //tranms hex number into uint256
	BYTE uint256_diff[32];
	BYTE uint256_sol[32];
	BYTE uint256_seed[32];
	uint256_init(uint256_diff);
	uint256_init(uint256_sol);
  //convert string diff into hex number
	for (int i = 0; i < sizeof(diff_hex); i = i + 1) {
		uint256_diff[31 - i] = (diff_hex >> (8 * i)) & 0x000000fful;
		print_uint256(uint256_diff);
	}
 //extract alpha and beta
	BYTE uint256_alpha[32];
	BYTE uint256_beta[32];
	uint256_init(uint256_alpha);
	uint256_init(uint256_beta);
	uint256_alpha[31] = uint256_diff[28];
	print_uint256(uint256_alpha); 
	for (int i = 0; i < 3; i = i + 1) {
		uint256_beta[31 - i] = uint256_diff[31 - i];
		print_uint256(uint256_beta);
	}
  //calculate target value
	BYTE aa[32], bb[32], cc[32], target[32];
	uint256_init(aa);
	uint256_init(bb);
	uint256_init(cc);
	uint256_init(target);
	uint32_t aaa, bbb, ccc;
	aaa = uint256_alpha[31] - 0x03;
	bbb = 0x8;
	ccc = aaa * bbb;
	bb[31] = 0x2;
	uint256_exp(aa, bb, ccc);
	print_uint256(aa);
	uint256_mul(target, uint256_beta, aa);

  
	for (int i = 0; i < sizeof(sol_hex); i = i + 1) {
		uint256_sol[31 - i] = (sol_hex >> (8 * i)) & 0x00000000000000fful;
			}
  //combine seed and sol to get the input x of hash function
	BYTE x[40];
	char temp[2];
	for (int i = 0; i <= 31; i++) {
		temp[0] = seed[i * 2];
		temp[1] = seed[i * 2 + 1];
		x[i] = strtoul(temp, NULL, 16);
	}
	for (int i = 0; i <= 7; i++) {

		x[39 - i] = uint256_sol[31 - i];
	}	
  //check the sol with the hash function
	BYTE buf[SHA256_BLOCK_SIZE];
	BYTE buf2[SHA256_BLOCK_SIZE];
	SHA256_CTX ctx;
	int idx;
	int pass = 1;
	sha256_init(&ctx);
	sha256_update(&ctx, x, 40);
	sha256_final(&ctx, buf);	
	sha256_init(&ctx);
	sha256_update(&ctx, buf, 32);
	sha256_final(&ctx, buf2);
	puts("buf2=");
	print_uint256(buf2);
	pass = memcmp(target, buf2, SHA256_BLOCK_SIZE);//compare the result with target
	printf("%d\n", pass);
  //return 1 if sol is passed, otherwise return -1
	if (pass > 0) {
		return 1;
	} else {
		return -1;
	}
}

char *WORK(char *smsg) {
	puts("WORK processing:");
	puts(smsg);

	char head[5];
	char diff[9];
	char seed[65];
	char start[17];
	char work[3];

	char string[99];
	strcpy(string, smsg);
	const char delimiters[] = " \r\n";
	//puts(string);
  //split the data into head, difficulty and so on
	strcpy(head, strtok(string, delimiters));
	strcpy(diff, strtok(NULL, delimiters));
	strcpy(seed, strtok(NULL, delimiters));
	strcpy(start, strtok(NULL, delimiters));
	strcpy(work, strtok(NULL, delimiters));
//	//strcpy(head,h);
	printf("%d %d %d %d %d\n", strlen(head), strlen(diff), strlen(seed),
			strlen(start), strlen(work));
	if (strlen(diff) != 8 || strlen(seed) != 64 || strlen(start) != 16
			|| strlen(work) != 2) {//check format

		return "e";
	}

  //convert string into hex
	uint32_t diff_hex = strtoul(diff, NULL, 16);
	//unsigned long long seed_hex = strtoul(seed, NULL, 16);
	uint64_t start_hex = strtoul(start, NULL, 16);
	uint8_t work_hex = strtoul(work, NULL, 16);

//	printf(" diff value: %lx, %d\n", diff_hex, sizeof(diff_hex));
//	//  printf(" seed value: %lx, %d\n", seed_hex,sizeof(seed_hex));
//	printf(" sol value: %lx, %d\n", sol_hex, sizeof(sol_hex));
//convert hex into uint256
	BYTE uint256_diff[32];
	BYTE uint256_start[32];
	BYTE uint256_seed[32];
	uint256_init(uint256_diff);
	uint256_init(uint256_start);
	for (int i = 0; i < sizeof(diff_hex); i = i + 1) {
		uint256_diff[31 - i] = (diff_hex >> (8 * i)) & 0x000000fful;
	//	print_uint256(uint256_diff);
	}
	BYTE uint256_alpha[32];
	BYTE uint256_beta[32];
	uint256_init(uint256_alpha);
	uint256_init(uint256_beta);
	uint256_alpha[31] = uint256_diff[28];
	print_uint256(uint256_alpha);
	for (int i = 0; i < 3; i = i + 1) {
		uint256_beta[31 - i] = uint256_diff[31 - i];
	//	print_uint256(uint256_beta);
	}
 //calculate target
	BYTE aa[32], bb[32], cc[32], target[32];
	uint256_init(aa);
	uint256_init(bb);
	uint256_init(cc);
	uint256_init(target);
	uint32_t aaa, bbb, ccc;
	aaa = uint256_alpha[31] - 0x03;
	bbb = 0x8;
	ccc = aaa * bbb;
//	printf(" power value: %08x, %d\n", ccc, sizeof(ccc));
	//uint256_mul (cc, aa, bb);
	//uint256_init (aa);
	//uint256_init (bb);

	bb[31] = 0x2;
	uint256_exp(aa, bb, ccc);
//	print_uint256(aa);

	uint256_mul(target, uint256_beta, aa);

	print_uint256(target);

	for (int i = 0; i < sizeof(start_hex); i = i + 1) {
		uint256_start[31 - i] = (start_hex >> (8 * i)) & 0x00000000000000fful;

	}
//	puts("uint256_start=");
	//print_uint256(uint256_start);
	BYTE x[40];
	char temp[2];

//	printf("\n");
//	printf("%d\n", sizeof(x));
//
	BYTE buf[SHA256_BLOCK_SIZE];
	BYTE buf2[SHA256_BLOCK_SIZE];
	SHA256_CTX ctx;
	int idx;
	int pass = 0;
	for (int i = 0; i <= 31; i++) {
		temp[0] = seed[i * 2];
		temp[1] = seed[i * 2 + 1];
		x[i] = strtoul(temp, NULL, 16);
	}

	BYTE uint256a[32], uint256b[32];
	uint256_init(uint256a);
	uint256_init(uint256b);
	uint256b[31] = 0x1;
 //loop to find the right x that H(H(x))<target
	while (pass <= 0) {

		for (int i = 0; i <= 7; i++) {

			x[39 - i] = uint256_start[31 - i];
		}
		sha256_init(&ctx);
		sha256_update(&ctx, x, sizeof(x));
		sha256_final(&ctx, buf);
		sha256_init(&ctx);
		sha256_update(&ctx, buf, sizeof(buf));
		sha256_final(&ctx, buf2);
		uint256_init(uint256a);
		for (int i = 0; i < 32; i++) {
			uint256a[i] = uint256_start[i];
		}
		uint256_add(uint256_start, uint256a, uint256b);
		pass = memcmp(target, buf2, SHA256_BLOCK_SIZE);
	//	print_uint256(uint256a);
	}
//	puts("SOLN=");
//	print_uint256(uint256a);
  //
	char sol[17];
	char temp_sol[2];
	uint64_t sol_64;

	for (int i = 0; i < 8; i = i + 1) {

		sprintf(temp_sol, "%02x", uint256a[24 + i]);
		sol[2 * i] = temp_sol[0];
		sol[(2 * i) + 1] = temp_sol[1];
		puts(temp_sol);
		puts(sol);
	}

	static char sl[97];
	sprintf(sl, "%s %s %s %s\r\n", "SOLN", diff, seed, sol);
	return sl;
}

void write_log(char *cli_ip, char *serv_ip, int sock, char *msg) {
	FILE *fp;
	time_t current_time;
	char *time_string;
  int nsock;
  nsock =sock;
	// get current time
	current_time = time(NULL);

	if (NULL == (time_string = ctime(&current_time))) {
		fprintf(stderr, "Unable to convert current time.\n");
		time_string = "N/A";
	}

	// remove new line appended by ctime
	time_string[strlen(time_string) - 1] = '\0';

	// apply mutex lock
	pthread_mutex_lock(&lock);

	fp = fopen("log.txt", "a");
	if (fp == NULL) {
		printf("Error opening file!\n");
		exit(1);
	}
	puts(cli_ip);
	puts(serv_ip);
	fprintf(fp, "%s, %s, %s, %d, %s\n", time_string, cli_ip, serv_ip,nsock, msg);
	fprintf(stderr, "%s, %s, %s, %d, %s\n", time_string, cli_ip, serv_ip,nsock, msg);

	fclose(fp);

	// unlock mutex
	pthread_mutex_unlock(&lock);
}
