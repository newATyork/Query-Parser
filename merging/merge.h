#ifndef _MERGE_H
#define _MERGE_H



#ifdef __cplusplus
extern "C"{
#endif

#include<stdio.h>

/* data structure for one input/output buffer */


typedef struct {
	unsigned int WordID;
	unsigned int DocID;
	unsigned int TF;
} posting;

//here I define a new struct to hold the triple <WordID, DocID,TF>

typedef struct {
	FILE *f; 
	char* buf; 
	int curRec; 
	int numRec;
} buffer;

typedef struct {
	int *arr; 
	char *cache; 
	int size; 
} heapStruct;

#define KEY(z) (*(posting *)(&(heap.cache[heap.arr[(z)]*recSize])))

extern buffer *ioBufs;          /* array of structures for in/output buffers */
extern heapStruct heap;         /* heap structure */
extern int recSize;             /* size of record (in bytes) */
extern int bufSize;             /* # of records that fit in each buffer */ 


/*************************************************************************/
/*      naiveMerge implements merge phase of an I/O-efficient mergesort  */
/*      It is assumed that the first 4 bytes of each record contain an   */
/*      integer key by which sorting occurs, and that records are of a   */
/*      fixed size that is a multiple of 4 bytes. It reads the names of  */
/*      the files that have to be merged from a file, and then merges    */
/*      each group of up to d consecutive files into one, where d is     */
/*      given as part of the command line input. Output files again have */
/*      filenames created by adding a running number to a given prefix,  */
/*      and the list of these filenames is written to another file.      */
/*                                                                       */
/* usage:  ./mergephase recsize memsize finlist outfileprefix foutlist   */
/*             where                                                     */
/*               recsize:  size of a record in bytes - must be mult(4)   */
/*               memsize:  size of available memory in bytes             */
/*               degree:   merge degree d: max # files merged into one   */
/*               finlist:  name of file containing a list of input files */
/*               outfileprefix:  prefix (including path and name) used   */
/*                               to generate numbered temp output files  */
/*               foutlist:  file to which names of temp files written    */
/*************************************************************************/


/* standard heapify on node i. Note that minimum is node 1. */
void heapify(int i);


/* get next record from input file into heap cache; return -1 if EOF */
int nextRecord(int i);

/* copy i-th record from heap cache to out buffer; write to disk if full */
/* If i==-1, then out buffer is just flushed to disk (must be last call) */
void writeRecord(buffer *b, int i);

void naiveMerge(int recSize, int memsize, int maxDegree, char* basepath, char* fileInPath, char* outfileprefix, char* fileOutPath);

void naiveMerge2(int recSize, int memSize, int maxDegree, char* fileInPath, char* outfileprefix, char* fileOutPath);


#ifdef __cplusplus
}
#endif

#endif

