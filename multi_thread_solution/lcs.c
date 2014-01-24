#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

/**********
  Globals
**********/
int NUM_THREADS = 4;
int mode, block_x, block_y;
int str1size, str2size;
char *str1, *str2;
int **left_edge;

/***********
  Functions
************/
int** calculate_submatrix();
void *thread_main();
int print_matrix();
char* get_lcs();


int main(int argc, char *argv[])
{

	/***************************
	  Arguments + File Parsing 
	***************************/

	if(argc<3)
	{
		printf("Usage: $s file1 file2 [mode] [block_x] [block_y]\n");
		return 0;

	}


	//Lazy argument parsing, instead of -v, etc.
	mode = 2; //Default
	if(argc>3)
	{
		mode = atoi(argv[3]);	
		printf("\nMode = %i\n", mode);
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

	if(argc>6)
	{
		NUM_THREADS = atoi(argv[6]);
	}



	printf("Size of block_x, block_y: %i, %i\n", block_x, block_y);
	
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

	int j;

	left_edge = (int **) malloc ((NUM_THREADS+1) * sizeof(int *));

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

	
	/******************
	  Thread Creation
	******************/

	int thread_num;
	long k;
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

	//Get end time and print time elapsed.
	long int timesec2 = time(NULL);
	gettimeofday(&tv, NULL);
	float timems2 = tv.tv_usec*0.001;
	printf("\nRuntime: %f ms", (timesec2-timesec1)*1000 + (timems2-timems1)); 
	
	printf("\n\n");

	return 0;
}

void *thread_main(void *threadID)
{
	long tnum = (long) threadID;
	int thread_num = (int) tnum;
	int i;
	long spinlocks = 0;

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
	int *left = (int *) malloc (block_x * sizeof(int *));
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
	int k, n;
	for(i=0; i<str1size/block_x; i++)
	{
		
		//Reset left:
		for(k=0; k<block_x; k++)
		{
			while(left_edge[thread_num][i*block_x+k] == -1)
			{
				printf(".\r");
				spinlocks++;
			}
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
	
		//TODO:
		//add left[] to edge[i * str1size/block_x + n] for n = 0; n < block_x.
		
		for(n = 0; n<block_x; n++)
		{
			left_edge[thread_num+1][i * block_x + n] = left[n];	
		} 

	}

	printf("\nThread %i spinlocked %li times.", thread_num, spinlocks);

	if (thread_num == NUM_THREADS-1)
	{
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


//	for(i=0;i<str1size;i++)
//	{
//		free(matrix[i]);
//	}


	//free(matrix);

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
