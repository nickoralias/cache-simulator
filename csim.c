#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <strings.h>

#include "cachelab.h"

/* Always use a 64-bit variable to hold memory addresses*/
typedef unsigned long long int mem_addr_t;

/* a struct that groups cache parameters together */
typedef struct {
	int s; /* 2**s cache sets */
	int b; /* cacheline block size 2**b bytes */
	int E; /* number of cachelines per set */
	int S; /* number of sets, derived from S = 2**s */
	int B; /* cacheline block size (bytes), derived from B = 2**b */
} cache_param_t;

int verbosity;

/*
 * printUsage - Print usage info
 */
void printUsage(char* argv[])
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

typedef struct 
{
       	int valid;
       	int timestamp;
       	long long tag;
}set_line;

/*checks the tag to see if it's in the cache and loads it if it's not*/
void checkTag(long long tag, cache_param_t par, set_line * cache, int * cachemiss, int * cachehit, int * eviction) 
{

	int i;
	int lru = cache[0].timestamp;
	int index = 0;
	for(i=0; i< par.E; i++)
	{	
		if (cache[i].timestamp > lru) 
		{
			lru = cache[i].timestamp;
			index = i;
		}		

		if (cache[i].valid == 1 && cache[i].tag == tag) 
		{
			cache[i].timestamp=0;
			(*cachehit)++;
			return;
		}
	}
	
	for(i=0; i< par.E; i++)
	{
		if (cache[i].valid == 0) 
		{
			cache[i].valid = 1;
			cache[i].tag = tag;
			cache[i].timestamp=0;
			(*cachemiss)++;
			return;	
		}
	}
	cache[index].tag = tag;
	cache[index].timestamp = 0;
	(*cachemiss)++;
	(*eviction)++;
	return;
}

int main(int argc, char **argv)
{

	cache_param_t par;
	bzero(&par, sizeof(par));

	char *trace_file;
	char c;
	while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1){
	  switch(c){
	  case 's':
            par.s = atoi(optarg);
            break;
	  case 'E':
            par.E = atoi(optarg);
            break;
	  case 'b':
            par.b = atoi(optarg);
            break;
	  case 't':
            trace_file = optarg;
            break;
	  case 'v':
            verbosity = 1;
            break;
	  case 'h':
            printUsage(argv);
            exit(0);
        default:
            printUsage(argv);
            exit(1);
	  }
	}

    if (par.s == 0 || par.E == 0 || par.b == 0 || trace_file == NULL) 
      {
	printf("%s: Missing required command line argument\n", argv[0]);
        	printUsage(argv);
        	exit(1);
      }

	par.S = 1 << par.s;
	par.B = 1 << par.b;

	int cachehit = 0, cachemiss = 0, eviction = 0;

	/*initialize fields of each line*/
	set_line cache[par.S][par.E];
	int i, j;
	for (i = 0; i < par.S; i++) 
	{
		for (j = 0; j < par.E; j++){
 	   		cache[i][j].valid = 0;
			cache[i][j].timestamp = 0;
			cache[i][j].tag = 0;
 	 	}
	}
	
	/*scan the trace file and call getTag for each line*/
	FILE * fp = fopen (trace_file, "r");
	char line [1024];
	char command;
	int size;
	mem_addr_t address;

	while (fgets(line, sizeof(line), fp)) 
	{
		sscanf (line, " %c %llx,%d", &command, &address, &size);
		
    		long long tag = (address&((~0x0)<<(par.s + par.b)))>>(par.s+par.b);
		int set = ((address>>par.b)&((1<<par.s)-1));
		if (line[0] == 'I') 
		{
			continue;
		}
		else 
		  {
			switch (command) {
			case ('L'):
				checkTag(tag, par, cache[set], &cachemiss, &cachehit, &eviction);
				break;
			case ('S'):
				checkTag(tag, par, cache[set], &cachemiss, &cachehit, &eviction);
				break;
			case ('M'):
				checkTag(tag, par, cache[set], &cachemiss, &cachehit, &eviction);
				checkTag(tag, par, cache[set], &cachemiss, &cachehit, &eviction);
				break;
			} 
			for (i = 0; i < par.E; i++) 
			  {
			    cache[set][i].timestamp++;
			  }
		}
	}
    	printSummary(cachehit, cachemiss, eviction);
	fclose(fp);
    	return 0;
}
