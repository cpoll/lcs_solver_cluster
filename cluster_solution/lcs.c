#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/**********
  Globals
**********/
int NUM_THREADS;
int mode, block_x, block_y;
int str1size, str2size;
char *str1, *str2;
int **left_edge;
char *server_IP;
int prev_socket;
int next_socket;
int server_socket;
int debug_flag = 0;

/***********
  Functions
************/
int** calculate_submatrix();
int thread_main();
int print_matrix();
char* get_lcs();


int main(int argc, char *argv[])
{
	
	mode = 0; //Default

	/***************************
	  Arguments + File Parsing 
	***************************/

	if(argc<3)
	{
		printf("Usage: %s file1 file2 ServerIP [block_x] [block_y]\n", argv[0]);
		return 0;

	}


	//Lazy argument parsing, instead of -v, etc.
	if(argc>3)
	{
		server_IP = argv[3];	
	}


	//Get start time.
	struct timeval tv;
	long int timesec1 = time(NULL);
	gettimeofday(&tv, NULL);
	float timems1 = tv.tv_usec*0.001;



	//Parse arguments and get data.
	FILE *file = fopen(argv[1], "r");
	FILE *file2 = fopen(argv[2], "r");

	//Get size of files by seeking to the end, getting position.	
	fseek(file, 0L, SEEK_END);
	str1size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	
	fseek(file2, 0L, SEEK_END);
	str2size = ftell(file2);
	fseek(file2, 0L, SEEK_SET);

	printf("Size of strings: %i, %i\n", str1size, str2size);

	block_x = str1size;
	block_y = str2size;

	if(argc>5)
	{
		block_x = atoi(argv[4]);
		block_y = atoi(argv[5]);
	}

	if(mode==2||mode==3)
	{
		block_x = str1size;
		block_y = str2size;
	}
	
	printf("Size of block_x, block_y: %i, %i\n", block_x, block_y);

	/*********************/
	/* Connect to Server */
	/*********************/

        struct sockaddr_in server_address;
        memset(&server_address, 0, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = inet_addr(server_IP);
        server_address.sin_port = htons(8000);

        server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

        if( (connect(server_socket, (struct sockaddr *) &server_address,
                                        sizeof(server_address)))<0)
                printf("Failed to connect to server\n");
	
	/************************/
	/* Get Info from Server */
	/************************/

	int thread_num;
	int buffer_size = 30;
	char buffer[buffer_size];
	in_addr_t next_IP;

	int recieve_size;

        if((recieve_size = recv(server_socket, buffer, buffer_size, 0)) <0)
        {
                printf("Recieve from server failed.\n");
                return -1;
        }

        printf("Recieved from server: %s\n", buffer);

	thread_num = atoi(strtok(buffer, "+"));
	NUM_THREADS = atoi(strtok(NULL, "+"));
	next_IP = atoi(strtok(NULL, "+"));

	printf("thread_num = %i, NUM_THREADS = %i, next_IP = %i\n", thread_num, NUM_THREADS, next_IP);


	/**********************************/
	/* Connect to neighbouring clients*/
	/**********************************/

	/* Note:
		We avoid deadlocking by first waiting for a client to connect
		to us, then connecting to the next client. Since client 0 does
		not wait for a previous client (there is none), it will
		satisfy client 1, who satisfies client 2, etc.

		Also, client NUM_THREADS-1 does not connect to any client, as
		it is the last in line. 
	*/

	//Wait for connection from previous client.
	//We essentially become a server for the previous client.
	//Client 0 does not do this.
	
	if(thread_num!=0)
	{
		int max_connections = 1;
		printf("Waiting for connection from previous client.\n");	
	
		struct sockaddr_in self_address;
		memset(&self_address, 0, sizeof(self_address));
		self_address.sin_family = AF_INET;
		self_address.sin_addr.s_addr = htonl(INADDR_ANY);
		self_address.sin_port = htons(8000);

		int self_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(bind(self_socket, (const struct sockaddr *) &self_address,
							sizeof(self_address))<0)
			printf("Failed to bind self as server\n");

		if(listen(self_socket, max_connections)<0)
			printf("Failed to set self server socket to listen\n");

		
		struct sockaddr_in prev_address;
		socklen_t sizeof_prev_address = sizeof(prev_address);

		if ((prev_socket = accept(self_socket,
			(struct sockaddr *) &prev_address, &sizeof_prev_address))<0)
		{
			printf("Failed to accept prev client\n");
			return -1;
		}

		printf("prev_client has reported in.\n");
	}

	//Connect to next_IP
	//Client NUM_THREADS-1 does not do this.
	if(thread_num<NUM_THREADS-1)
	{	
		printf("Establishing connection with next client.\n");		

		struct sockaddr_in next_address;
		memset(&next_address, 0, sizeof(next_address));
		next_address.sin_family = AF_INET;
		next_address.sin_addr.s_addr = next_IP;
		next_address.sin_port = htons(8000);

		next_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

		int tries = 5;
		int success = 0;
		while(!success && tries>0)
		{
			printf("Attempting connection\n");
			if( (connect(next_socket, (struct sockaddr *) &next_address,
							sizeof(next_address)))<0)
			{
				printf("Failed to connect to next client, retrying...\n");
				tries--;
				sleep(2);
			}	
			else
			{
				success = 1;
			}

		}
		
		if(tries == 0)
		{
			printf("Failed to connect to next client\n");
			return -1;
		}

		printf("Connection to next_IP established.\n");
	}

	

	/* Try out next_socket and prev_socket */
	//TODO: Get rid of this once we see that it works.
	
        //int recv_buffer_size = 50;
	int send_buffer_size = 50;
        //char recv_buffer[recv_buffer_size];
	char send_buffer[send_buffer_size];

	/*
	if(thread_num!=0)
	{
		//Recieve and print a message.
		if((recieve_size = recv(prev_socket, recv_buffer, recv_buffer_size, 0)) <0)
      		{
                	printf("Recieve from prev_client failed.\n");
                	return -1;
        	}

	        printf("Recieved greeting from prev_client: %s\n", recv_buffer);
	}

	if(thread_num<NUM_THREADS-1)
	{
		//Send a message.
                sprintf(send_buffer, "Hi from Client %i", thread_num);
		printf("Sending to next_client: %s\n", send_buffer);
                if (send(next_socket, send_buffer, send_buffer_size, 0) != send_buffer_size)
                {
                        printf("Sending info to next_client failed.\n");
                        return -1;
                }
        }
	*/

	/****************/
	/* Parse Inputs */
	/****************/

	str1 = (char *) malloc ((str1size-1) * sizeof(char *));
	str2 = (char *) malloc ((str2size-1) * sizeof(char *));	

	char c;
	int i=0;
	while((c=fgetc(file))!=EOF)
	{
		//printf("%c", c);
		str1[i]=c;
		i++;
	}


	i=0;
	while((c=fgetc(file2))!=EOF)
	{
		//printf("%c", c);	
		str2[i]=c;
		i++;
	}

	/*************************
	  Pre-thread Preparation
	*************************/

	//Create the edge array.

	/*left_edge[n] represents the left-edge of thread n's submatrix.
 	  left_edge[0] should be all 0s, as it is the leftmost edge.
	  All others should be all -1s, as they are to be filled by the threads. */
	
	/*left_edge[NUM_THREADS+1] exists to avoid lots of if statements,
	  but it is not filled with 0s, as no-one reads it */


	left_edge = (int **) malloc ((NUM_THREADS+1) * sizeof(int *));

	if (1)
	{
		int j;		

		for(i=0; i<NUM_THREADS+1; i++)
		{
			left_edge[i] = (int *) malloc (str1size * sizeof(int));
		}
		
		for(j=0; j<str1size; j++)
		{
			left_edge[0][j] = 0;
		}

		for(i=1; i<NUM_THREADS; i++) //Skips left_edge[NUM_THREADS]
		{
			for(j=0; j<str1size; j++)
			{
				left_edge[i][j] = -1;
			}
		}	
	}
	
	/******************
	  Thread Creation
	******************/

	/* For now, each client will be single threaded. */

	/*long k;
	pthread_t thread[NUM_THREADS];

	for(k=0; k<NUM_THREADS; k++)
	{
		if((thread_num = pthread_create(&thread[k], NULL, &thread_main, (void *) k)))
		{
			printf("Creation failed on thread %i.\n", thread_num);
			return 1;
		}
	}
	
	for(i=0; i<NUM_THREADS; i++)
	{
		pthread_join(thread[i], NULL);
	}
	*/

	thread_main(thread_num);

	/***********/
	/* Wrap-up */
	/***********/

	//Get end time and print time elapsed.
	long int timesec2 = time(NULL);
	gettimeofday(&tv, NULL);
	float timems2 = tv.tv_usec*0.001;
	float runtime = (timesec2-timesec1)*1000 + (timems2-timems1); 
	printf("\nRuntime: %f ms", runtime); 
	
	printf("\n\n");

	//Give the server our runtime.
	memset(send_buffer, '\0', send_buffer_size);
        sprintf(send_buffer, "%f", runtime);
        if (send(server_socket, send_buffer, send_buffer_size, 0) != send_buffer_size)
        {
       		printf("Sending runtime info to server failed.\n");
                return -1;
        }



	return 0;
}

int thread_main(int threadID)
{
	int thread_num = threadID;
	int i;

	//Make our buffers.	
	int recv_buffer_size = block_x * sizeof(int);
	int send_buffer_size = block_x * sizeof(int);
        int *recv_buffer = malloc(recv_buffer_size);
	//int *send_buffer = malloc(send_buffer_size);
	int recieve_size;


	printf("Thread %i starting.\n", thread_num);

	/*****************
	  Splitting Work
	*****************/

	int thread_str2size = str2size/NUM_THREADS;	



	//Take 1/NUM_THREADS of str2
	char *thread_str2 = (char *) malloc ((thread_str2size) * sizeof(char));
	for(i=0; i<thread_str2size; i++)
	{
		thread_str2[i] = str2[thread_num * thread_str2size + i];		
	}
	
	/*****************
	  Matrix Solving
	*****************/

	//Each submatrix is of size block_x and block_y. 

	//TODO:
	//For now, we don't check for leftover, so make sure block_x and block_y
	//are factors of str1size and str2size, respectively.
	
	//Create top and left, two multidimensional arrays containing top and
	//left edges of submatrices, initially filled with 0s.

	int **top = (int **) malloc ((thread_str2size/block_y) * sizeof(int *));
	int *left = malloc (block_x * sizeof(int));
	int **diag = (int **) malloc ((str1size/block_x) * sizeof(int *));

	//Because we're going left-to-right, top to bottom, we need the top
	//of every block above, but only the left of the block direclty left.
	int *subarray;
	char *substring;
	int j;
	for(i=0; i<thread_str2size/block_y; i++)
	{
		subarray = (int *) malloc (block_y * sizeof(int));		
		
		for(j=0; j<block_y; j++)
		{
			subarray[j] = 0;
		}
	
		top[i] = subarray;
		
	}

	for(i=0; i<str1size/block_x; i++)
	{	
		diag[i] = (int *) malloc ((thread_str2size/block_y) * sizeof(int));
	}	
	for(i=0; i<thread_str2size/block_y; i++)
	{
		diag[0][i] = 0;
	}

	for(i=0; i<block_x; i++)
	{
		left[i] = 0;
	}


	//Split str1 and str2 into blocks for use in calculate_submatrix

	char **substr2 = (char **) malloc ((thread_str2size/block_y) * sizeof(char *));
	for(i=0; i<thread_str2size/block_y; i++)
	{
		substring = (char *) malloc (block_y * sizeof(char));
		
		for(j=0; j<block_y; j++)
		{
			substring[j] = thread_str2[(i*block_y)+j];
		}
		
		substr2[i] = substring;
	
	}
	
	
	char **substr1 = (char **) malloc ((str1size/block_x) * sizeof(char *));
	for(i=0; i<str1size/block_x; i++)
	{
		substring = (char *) malloc (block_x * sizeof(char));
		
		for(j=0; j<block_x; j++)
		{
			substring[j] = str1[(i*block_x)+j];
		}
		
		substr1[i] = substring;
	
	}


	//Create the submatrix we'll be filling out/reusing.
	int **matrix = (int **) malloc (block_x * sizeof(int *));

	for(i = 0; i < block_x; i++)
		matrix[i] = (int *) malloc (block_y * sizeof(int));



	//Solve subarrays, row by row, column by column, until arriving at subarray
	//(str1size/block_x, str2size/block_y).
	int** result = (int **) malloc (1 * sizeof(int *));
	int k;
	for(i=0; i<str1size/block_x; i++)
	{
		
		//Reset left:
		//TODO: left_edge[thread_num][i*block_x onward] is obtained from prev_socket.

		if(thread_num > 0)
		{	
			//Recieve and print a message.
			if((recieve_size = recv(prev_socket, recv_buffer, recv_buffer_size,
					MSG_WAITALL))!= recv_buffer_size)
			{
				printf("Recieve left edge from prev_client failed.\n");
				return -1;
			}
			
			

			//For debugging
			if(debug_flag)
			{	
				int m;
				for(m=0; m<block_x; m++)
					printf("Recieved from prev_client: %i\n", recv_buffer[m]);
				printf(" --- \n");
			}

			
			//printf("Recieve_size = %i.\n", recieve_size);
			//printf("CHECK: %i.\n", recv_buffer[block_x/2]);

			for(k=0; k<block_x; k++)
			{
				left_edge[thread_num][i*block_x+k] = recv_buffer[k];
			}
	
		}

		for(k=0; k<block_x; k++)
		{
			left[k] = left_edge[thread_num][i*block_x+k];
		}	
	
		
		if(i>0)
		{
			diag[i][0] = left_edge[thread_num][i*block_x-1];
		}


		for(j=0; j<thread_str2size/block_y; j++)
		{
			free(result);
			result = calculate_submatrix(top[j], left, diag[i][j],
					substr1[i], substr2[j], block_x, block_y, matrix); 



			free(left);
			left = result[0];

			//free(top[j]);
			top[j] = result[1];


			if(j<thread_str2size/block_y-1) //Have not reached end.
			{	
				if(i<str1size/block_x-1) //Have not reached bottom.
				{
					diag[i+1][j+1] = left[block_x-1]; 
				}


			}
		}

		//TODO: Send left_edge[thread_num+1] to next_socket.
		/*	
		for(n = 0; n<block_x; n++)
		{
			left_edge[thread_num+1][i * block_x + n] = left[n];	
		} */

		if(thread_num<NUM_THREADS-1)
		{
			if (send(next_socket, left, send_buffer_size, 0) != send_buffer_size)
			{
				printf("Sending left_edge to next client failed.\n");
				return -1;
			}

			//For debugging
			if(debug_flag)
			{	
				int l;
				for(l=0;l<block_x;l++)
					printf("Sent to next client: %i\n", left[l]);
				printf(" --- \n");
			}
		}


	}

	if (thread_num == NUM_THREADS-1)
	{
		//Send our result to the server

		char result_buffer[50];
		memset(result_buffer, '\0', 50);
                sprintf(result_buffer, "%i", left[block_x-1]);
                if (send(server_socket, result_buffer, 50, 0) != 50)
                {
                        printf("Sending LCS result to server failed.\n");
                        return -1;
                }

		printf("\n\nSize of LCS = %i\n", left[block_x-1]);
		
	}

	return 0;

}

int** calculate_submatrix(int* top, int* left, int diag, char* str1, char* str2, int str1size, int str2size, int** matrix)
{
	/* Given four arrays: two substrings and two edges, as well as two ints
	containing size of the substrings, returns a multidimensional array 
	containing the right and bottom edges of the calculated submatrix.*/

	//Pass this in instead:
	//int **matrix = (int **) malloc (str1size * sizeof(int *));

	//int i;
	//for(i = 0; i < str1size; i++)
	//	matrix[i] = (int *) malloc (str2size * sizeof(int));


	//Fill the matrix out.
	//str1 goes along the side (row) of the matrix, str2 on the top (column).
	int x,y,i;

	x=0;
	y=0;


	//Exception case: First row, first column, depends on diag or left.
	if(str1[0] == str2[0])
	{
		matrix[0][0] = diag+1;
	}
	else
	{
		if(top[0]>left[0])
		{
			matrix[0][0] = top[0];
		}
		else
		{
			matrix[0][0] = left[0];
		}
	}

	//Exception case: First row, may need to check values in top.
	for(y=1; y<str2size; y++)
	{
		if(str1[0] == str2[y])
		{
			matrix[0][y] = top[y-1]+1;
		}
		else
		{
			if(top[y]>matrix[0][y-1])
			{
				matrix[0][y] = top[y];
			}
			else
			{
				matrix[0][y] = matrix[0][y-1];
			}
		}		
	}

	//Row 2-N
	for(x=1; x<str1size; x++)
	{
		
		//Exception case: First column, may need to check values in left.
		if(str1[x] == str2[0])
		{
			matrix[x][0] = left[x-1]+1;
		}		
		else
		{	
			if(matrix[x-1][0]>left[x])
			{
				matrix[x][0] = matrix[x-1][0];
			}
			else
			{
				matrix[x][0] = left[x];
			}
		}

		//Column 2-N in Row
		for(y=1; y<str2size; y++)
		{
			if(str1[x] == str2[y])
			{
				matrix[x][y] = matrix[x-1][y-1]+1;
			}
			else
			{
				if(matrix[x-1][y]>matrix[x][y-1])
				{
					matrix[x][y] = matrix[x-1][y];
				}
				else
				{
					matrix[x][y] = matrix[x][y-1];
				}
			}
			
		}

	}

	
	//Print out the matrix if specified to do so in args.
	if(mode==1||mode==3){ print_matrix(str1, str2, str1size, str2size, matrix); }	

		
	int **result = (int **) malloc (2 * sizeof(int *));
	result[0] = (int *) malloc (str1size * sizeof(int));
	result[1] = (int *) malloc (str2size * sizeof(int));
	
	for(i=0;i<str1size;i++)
	{
		result[0][i] = matrix[i][str2size-1];
	}
	
	for(i=0;i<str2size;i++)
	{
		result[1][i] = matrix[str1size-1][i];
	}

	return result;

}

char* get_lcs(int** matrix, char* str1, char* str2, int i, int j)
{
	/* Recurse through the matrix, finally returning the LCS.*/

	//Currently based on the pseudo-code on Wikipedia.

	char *s = (char *) malloc ((matrix[i][j]) * sizeof(char *));

	//Note: Wiki pseudo-code assumes for all i==0 or j==0, the matrix
	//value is 0, but for this implementation this is not the case.
	if(i==0||j==0)
	{
		if(matrix[i][j]==1)
		{
			sprintf(s, "%c", (char)str1[i]);
			return s;
		}
	}

	if(str1[i] == str2[j])
	{
		sprintf(s,"%s%c", get_lcs(matrix, str1, str2, i-1, j-1), (char)str1[i]);
		return s;
		//return strcat(get_lcs(matrix, str1, str2, i-1, j-1), str1[i]);
	}

	if(matrix[i][j-1] > matrix[i-1][j])
	{

		return get_lcs(matrix, str1, str2, i, j-1);
	}
	
	return get_lcs(matrix, str1, str2, i-1, j);

}

int print_matrix(char* str1, char* str2, int str1size, int str2size, int** matrix)
{
	/* Given the row/column values str1 and str2, their sizes, and the
	   filled in matrix, prints a visual representation of the matrix
	   to stdout, for testing and examination purposes. */
	
	int x, y;

	//Print the first row
	printf("\n\n   | ");
	for(y=0; y<str2size; y++)
	{
		printf("%c ", str2[y]);
	}
	printf("\n---+-");
	for(y=0; y<str2size; y++)
	{
		printf("--");
	}

	for(x=0; x<str1size; x++)
	{
		printf("\n %c | ", str1[x]);

		for(y=0; y<str2size; y++)
		{
			printf("%i ", matrix[x][y]);
		}
	}


	return 0;
}
