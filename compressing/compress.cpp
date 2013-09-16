#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <string>
#include <malloc.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include <time.h>
#include "vbyte.h"

using namespace std;

vector< unsigned int > FT;    // the number of documents that contain term t
vector< pair<string, vector< pair<int,int> > > > Lex;  // vector subtitile is wordId, word in charactors, vector <maxdocid,len>

int main(int argc, char* argv[])
{
	unsigned int id;
	char word[1000];
	string lexiconFilePath = "/home/alex/mycode/newindex/results/lexi2" ;
	string finalFilePath = "/home/alex/mycode/newindex/mergefile/finalfile20" ;
	string compressFilePath = "/home/alex/mycode/newindex/mergefile/compressfile2" ;
	

	FILE* lexiconFile = fopen(lexiconFilePath.data(),"r");

	clock_t start_time=clock();


/************************************************ generate default new lexicon ***************/

	vector< pair<int,int> > tempv;
	tempv.push_back(make_pair(-1,-1));

	while( !feof(lexiconFile) )
	{
		fscanf(lexiconFile,"%d %s\n",&id,word);
		
		Lex.push_back( make_pair( string(word), tempv ));
	}

	cout<<"the size of lex is :"<<Lex.size()<<endl;
	fclose(lexiconFile);

/*******************************************************************************************/


	string newLexFilePath = "/home/alex/mycode/newindex/results/lexiconnew" ;
	FILE* newLexFile;

	if ( (newLexFile = fopen(newLexFilePath.data(),"w")) == 0 )
	{
		cout<<"New Lexicon file fails to be open!"<<endl;
		exit(1);
	}


/******************************* compression by variable bytes coding******************************/

	FILE* finalfile;
	FILE* compressfile;
	FILE* termfreqfile;

	char temp[12],compressNum[10];

	unsigned int wordid,docid,tf;
	unsigned int recordit=0;

	int lenOfCompressNum;
	char* chunkbuf = (char*) malloc(8000000); // 8MB buffer for a term word's postinglist maximum size
	char* pointer = chunkbuf;
	int frRet,fwRet;
	int lenOfPostingList=0;
	int maxDocID = -2 ;

	if( ( finalfile = fopen(finalFilePath.data(),"r") ) == 0 )
	{
		cout<<"Final file fails to be open!"<<endl;
		exit(1);
	}


	if( ( compressfile = fopen(compressFilePath.data(),"w") ) == 0 )
	{
		cout<<"Compressed file fails to be open!"<<endl;
		exit(1);
	}


	if( chunkbuf == NULL)
	{
		cout <<"Chunk Buffer fails to be allocated!" << endl;
		exit(1); 
	}
	
	

	while(!feof(finalfile))
	{
		
		frRet = fread(temp,12,1,finalfile);

		wordid =*(unsigned int *)temp;
		docid = *(unsigned int *)(temp+4);
		tf = *(unsigned int *)(temp+8);


		if(FT.size() < wordid+1)
			FT.push_back(1);
		else 
		{
			FT.at(wordid)++;
		}
		
		if(recordit == wordid)
		{
			lenOfCompressNum = vbyte_compress(compressNum,docid);
			memcpy(pointer, compressNum, lenOfCompressNum);
			pointer = pointer + lenOfCompressNum;
			lenOfPostingList = lenOfPostingList + lenOfCompressNum;

			lenOfCompressNum = vbyte_compress(compressNum,tf);
			memcpy(pointer, compressNum, lenOfCompressNum);
			pointer = pointer + lenOfCompressNum;
			lenOfPostingList = lenOfPostingList + lenOfCompressNum;
			maxDocID = docid;

		}
		else 
		{	
			/*************************** new word, so split*******************/
			fwRet = fwrite(chunkbuf,lenOfPostingList,1,compressfile);

			vector< pair<int,int> >::iterator  iterr = Lex.at(wordid-1).second.begin(), endd = Lex.at(wordid-1).second.end();
			
			for(; iterr != endd; ++iterr)
			{
				iterr->first = maxDocID;
				iterr->second = lenOfPostingList;
			}
			/******************************************************************/

			if(feof(finalfile))
			{
				vector< pair<int,int> >::iterator  iterr2 = Lex.at(wordid).second.begin(), endd2 = Lex.at(wordid).second.end();
			
				for(; iterr2 != endd2; ++iterr2)
				{
					iterr2->first = maxDocID;
					iterr2->second = lenOfPostingList;
				}
			}

			recordit = wordid;
			lenOfPostingList = 0;
			pointer = chunkbuf;
			

			lenOfCompressNum = vbyte_compress(compressNum,docid);
			memcpy(pointer, compressNum, lenOfCompressNum);
			pointer = pointer + lenOfCompressNum;
			lenOfPostingList = lenOfPostingList + lenOfCompressNum;

			lenOfCompressNum = vbyte_compress(compressNum,tf);
			memcpy(pointer, compressNum, lenOfCompressNum);
			pointer = pointer + lenOfCompressNum;
			lenOfPostingList = lenOfPostingList + lenOfCompressNum;

		}
		
	}

	fwRet = fwrite(chunkbuf,lenOfPostingList,1,compressfile);


	vector< pair<int,int> >::iterator  iterr = Lex.at(wordid).second.begin(), endd = Lex.at(wordid).second.end();
			
	for(; iterr != endd; ++iterr)
	{
		iterr->first = maxDocID;
		iterr->second = lenOfPostingList;
	}


/*******************************************Compression by variable bytes coding********************************************************/
	
	vector< pair<string, vector< pair<int,int> > > >::iterator itr = Lex.begin(), endr = Lex.end();
	vector< unsigned int >::iterator it = FT.begin(), end = FT.end();

	for(;itr != endr; ++itr,++it)
	{	
		fprintf(newLexFile,"%d %s",itr - Lex.begin(), itr->first.data() );

		fprintf(newLexFile," %d", *it );

		vector< pair<int,int> >::iterator it2 = itr->second.begin(), end2 =  itr->second.end();

		for(;it2!=end2;++it2)
			fprintf(newLexFile," %d %d", it2->first, it2->second);

		fprintf(newLexFile,"\n");
	}

	fclose(newLexFile);
	fclose(compressfile);
	fclose(finalfile);
	free(chunkbuf);
	FT.clear();
	Lex.clear();

	clock_t end_time=clock();

	cout<< "Running time of variable bytes coding compression is: "<<static_cast<double>(end_time-start_time)/CLOCKS_PER_SEC*1000<<"ms"<<endl; 
	cout<<"OK"<<endl;

	return 0; 
}
