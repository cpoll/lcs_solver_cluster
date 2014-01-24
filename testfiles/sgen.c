#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	
	//Random DNA strand, for piping to file.
	char *dna = (char *) malloc(4*sizeof(char *));
	dna[0] = 'A';
	dna[1] = 'C';
	dna[2] = 'G';
	dna[3] = 'T';

	if(argc<3)
	{
		printf("Usage: $s characters seed");
		return 0;
	}
	
	int chars = atoi(argv[1]);
	int seed = atoi(argv[2]);
	
	int i;
	srand(seed);
	for (i=0;i<chars;i++)
	{
		printf("%c", dna[rand()%4]);
	}

	return 0;
}
