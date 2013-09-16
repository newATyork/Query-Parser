#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "merge.h"
#include "merge.c"



int main (int argc, char * argv[])
{


	recSize = 12;
	int memSize = 100000000;
	int maxDegree = 730;  // 730 can be faster here.
	char* basepath = "/home/alex/mycode/newindex/results/";
	char* fileInPath = "/home/alex/mycode/newindex/results/inputlist";
	char* fileOutPath = "/home/alex/mycode/newindex/results/outlist";
	char* outfileprefix = "/home/alex/mycode/newindex/mergefile/finalfile2";


	clock_t start_time=clock();
	
	naiveMerge(recSize, memSize, maxDegree, basepath, fileInPath, outfileprefix, fileOutPath);

	clock_t end_time=clock();

	printf( "Running time is: %f ms\n", (double)(end_time-start_time)/CLOCKS_PER_SEC*1000.0);

}
