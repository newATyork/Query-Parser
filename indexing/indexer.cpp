#include <zlib.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <malloc.h>
#include "parser-revised-again.h"
#include <algorithm>
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <locale>
#include "trim.h"
#include <sstream>
#include <time.h>
#include <assert.h>



#define INDEX_CHUNK 1000000
#define DATA_CHUNK 5097150


using namespace std;


map < string, unsigned int > Lexicon;  // < Word, WordID >   TreeMap
map < string,  pair<unsigned int,unsigned int> > URLMap;  // < URL, pair<DocID,lenOfDoc > >   TreeMap
map <unsigned int, vector< pair<unsigned int,unsigned int > > >  TempIndex; // Temporary invered index,  <    WordID,  <pair1<DocID, TF>,pair2<DocID,TF>,...>     >   
vector < unsigned int > FT;          // <the number of documents that contain term t>, FT's subtitle is wordID; FT also means the length of each postinglist.
vector< pair <string,unsigned int> > LexiconByValue;     // Lexicon sorted by value (WordID).
vector< pair <string,pair<unsigned int,unsigned int> > > URLMapByValue;      // URL sorted by value (DocID).



vector<pair<string,string> > getFileList()
{
	int N = 730;
	string indexlist[N];
	string datalist[N];

	string basepath = "/home/alex/Downloads/NZ/dataset/" ;	
	string index_end = "_index";
	string data_end = "_data";
	
	
	vector<pair<string,string> > filepair;

	for(int i=0; i<N ; i++)
	{
		stringstream ss;
		string num;
		ss<<i;
		ss>>num;
		indexlist[i] = basepath + num + index_end;
		datalist[i] = basepath + num + data_end;
		
		filepair.push_back(make_pair(indexlist[i],datalist[i]));
	}
	return filepair;
}


char *memAlloc(gzFile fileName, int size)    
{
    char *buffer=(char *)malloc(size);
    int flen;
    int oldSize=size;

    int count=0;             //The number of bytes that already read
    while (!gzeof(fileName))
    { 
        flen = gzread(fileName,buffer+count,oldSize);  
	  
		if(flen == -1)
		{
            cout<<"gzread failure! Some compressed file is broken! =====>> ";  
			// 2406_data can not be decompressed, bad file!
			free(buffer);
			return NULL;
		}
		count+= flen;
	   
		if (count==size)                    // Reallocate when buffer is full
		{
			oldSize=size;
			size*=2;
			buffer=(char *)realloc(buffer,size);
			if(buffer == NULL) cout<< "NULL buffer"<<endl;
		}
    }
    return buffer;
}



unsigned int readLineMem(char* buffer, unsigned int start)   // read a line into memory
{
	unsigned int i = start;
	while(buffer[i] != '\0')
	{
		if(buffer[i] == '\n')
		{
			return i-start+1 ;
		}
		i++;
		
	}
	
}


char* getLineCol(char* aString, unsigned int column)
{
    int j, i; 
    char *token[7]; //hold 7 tokens

    token[0] = strtok(aString, " "); //get pointer to first token

    if(column == 0)
	return token[0];
    else
    {
	    i =1;
	    for(;i <= column ;i++ )
	    {
		token[i] = strtok(NULL," ");
	    }

            return token[column];
    }
}


void substr(char *dest, const char* src, unsigned int start, unsigned int count) 
{
   strncpy(dest, src + start, count);
   dest[count] = '\0';
}


///////////////////////////////////  Sort URLs by value (ID) rather than key(string)  //////////////////////////////////////////////////

int cmp(const pair<string,pair<unsigned int,unsigned int> >& x,const pair<string,pair<unsigned int,unsigned int> >& y)  
{  
	return x.second.first<y.second.first;  
}
  
void sortMapByValue(map < string,pair<unsigned int,unsigned int> >& tMap,vector< pair< string,pair<unsigned int,unsigned int> > >& tVector)  
{  
	map<string, pair<unsigned int,unsigned int> >::iterator curr=tMap.begin(), end = tMap.end();
	for(;curr!= end; ++curr)  
	{  
		tVector.push_back(   make_pair(curr->first,make_pair(curr->second.first,curr->second.second) )  );  
	}  

	sort(tVector.begin(),tVector.end(),cmp);  
}  

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////  Sort Lexicon by value (ID) rather than key(string)  //////////////////////////////////////////////////

int cmp2(const pair<string,unsigned int>& x,const pair<string,unsigned int>& y)  
{  
    return x.second<y.second;  
}
  
void sortMapByValue2(map < string,unsigned int >& tMap,vector< pair< string,unsigned int> >& tVector)  
{  
      for(map<string, unsigned int>::iterator curr=tMap.begin();curr!=tMap.end();curr++)  
      {  
         tVector.push_back(make_pair(curr->first,curr->second));  
      }  

      sort(tVector.begin(),tVector.end(),cmp2);  
}  

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



unsigned int Build_Lexicon(string word)
{
	unsigned int WordID; 
        map <string, unsigned int >::iterator idx;
	idx = Lexicon.find(word);

	if(idx == Lexicon.end())    // this is a new word.
	{
		WordID = Lexicon.size();  // Stored numbers are from 0 to URLMap.size-1 ; so new WordID can be the value of URLMap.size.
		Lexicon.insert(pair<string, unsigned int>(word, WordID));
		return WordID;
	}
	else
	{
		return idx->second;
	}
}


unsigned int Build_URLMap(string URL, unsigned int lenOfDocument)
{
	unsigned int DocID; 
        map <string, pair<unsigned int,unsigned int> >::iterator idx;
	idx = URLMap.find(URL);

	if(idx == URLMap.end())   // this is a new URL.
	{
		
		DocID = URLMap.size();    // Stored numbers are from 0 to URLMap.size-1 ; so new DocID can be the value of URLMap.size.  

		pair<unsigned int,unsigned int>  temppair;		
		temppair.first = DocID;
		temppair.second = lenOfDocument;

		URLMap.insert(pair<string, pair<unsigned int,unsigned int> >(URL, temppair));
		return DocID;
	}
	else
	{
		return idx->second.first;
	}
}


void Build_Temp_Index(unsigned int WordID, unsigned int DocID)
{

    map <unsigned int, vector< pair<unsigned int,unsigned int > > >::iterator iter;

    iter = TempIndex.find(WordID); 
    if(  iter != TempIndex.end() )
    {
		vector< pair<unsigned int,unsigned int> >::iterator cur = iter->second.end();
		--cur;  // ==> iter->second.back();

	if( cur->first  == DocID )
		cur->second++;
	else
	{
		pair< unsigned int,unsigned int > posting;
		posting = make_pair(DocID,1);
		iter->second.push_back(posting);
	}
	
    }
    else 
    {
	vector< pair<unsigned int,unsigned int > > postinglist;
	pair< unsigned int,unsigned int > posting;
	posting = make_pair(DocID,1);
	postinglist.push_back(posting);
	TempIndex.insert(make_pair(WordID, postinglist));
    }
}


int saveTempIndex(string tempIndexFilepath)
{
	FILE* f = fopen(tempIndexFilepath.data(), "w");
	if(f == NULL) return 1;


	map <unsigned int, vector< pair<unsigned int,unsigned int > > >::iterator  it = TempIndex.begin();


        for (; it != TempIndex.end(); ++it )
        {
		fprintf(f, "%d:", it->first);
		vector< pair<unsigned int,unsigned int> >::iterator iter = it->second.begin(), end = it->second.end();

		for( ; iter != end; ++iter)
		{
			fprintf(f, " %d %d", iter->first, iter->second);
		}
		fprintf(f, "\n");
        }

	fclose(f);
	return 0;
}



int saveTempIndex2(string tempIndexFilepath)                           // <1, 300, 10> <2,400,2>  ..... stored in the way rather than "saveTempIndex"
{
	FILE* f = fopen(tempIndexFilepath.data(), "w");
	if(f == NULL) return 1;
	unsigned int tempbuf[3];

	map <unsigned int, vector< pair<unsigned int,unsigned int > > >::iterator  it = TempIndex.begin();

        for (; it != TempIndex.end(); ++it )
        {

		vector< pair<unsigned int,unsigned int> >::iterator iter = it->second.begin(), end = it->second.end();

                for( ; iter != end; ++iter)
                {
			tempbuf[0]=it->first;
			tempbuf[1]=iter->first;
			tempbuf[2]=iter->second;
			fwrite(tempbuf,12,1,f);
                }
        }

	fclose(f);
	return 0;
}


int saveURLMapByID(string URLMapFilepath)
{

	unsigned int sum=0;
	FILE* f = fopen(URLMapFilepath.data(), "w");
	if(f == NULL) return 1;

	sortMapByValue(URLMap,URLMapByValue); 

	vector< pair <string, pair<unsigned int, unsigned int> > > ::iterator itr = URLMapByValue.begin();

	for(; itr != URLMapByValue.end(); ++itr) 
	{
		fprintf(f, "%d %s %d\n", itr->second.first, itr->first.data(), itr->second.second);
		sum = sum + itr->second.second;
	}

	fclose(f);
	cout<<"# of URLs  is : "<<URLMapByValue.size()<<endl;
	cout<<"URL average length is : "<< ( (double)sum/(double)URLMapByValue.size() )<<endl;
	cout<<"saveURLMapByID OK"<<endl;
	return 0;
}


int saveLexiconByID(string LexFilepath)   // Lexicon sorted by value (WordID).
{


	FILE* f = fopen(LexFilepath.data(), "w");
	if(f == NULL) return 1;

	sortMapByValue2(Lexicon,LexiconByValue); 

	vector< pair <string,unsigned int> > ::iterator itr = LexiconByValue.begin();

	for(; itr != LexiconByValue.end(); ++itr) 
	{
		fprintf(f, "%d %s\n", itr->second, itr->first.data());
	}

	fclose(f);
	
	cout<<"saveLexiconBYID OK"<<endl;
	return 0;

}


vector<string> split(string& src, string separate_character)  
{  
    vector<string> strs;  
      
    int separate_characterLen = separate_character.size();
    int lastPosition = 0,index = -1;  
    while (-1 != (index = src.find(separate_character,lastPosition)))  
    {  
        strs.push_back(src.substr(lastPosition,index - lastPosition));  
        lastPosition = index + separate_characterLen;  
    }  
    string lastString = src.substr(lastPosition);
    if (!lastString.empty())  
        strs.push_back(lastString);
    return strs;  
}



// To process one pair of index and data gzip files.

void runOnce(string indexfile, string datafile, int number)
{
    gzFile cData,cIndex;
    char *indexBuffer, *dataBuffer, *pool, *TempIndexBuffer;
    char textStr[3000000];
    char indexStr[1000];
    unsigned int indexStartPoint,dataStartPoint,len,offset,lengOfIndex, lengOfPage, ret,lengOfData;
    unsigned int LexWordID ;
    unsigned int URLDocID ;

    

    cIndex=gzopen(indexfile.data(),"r");

    if(cIndex == NULL) 
    {    
         cout<<indexfile.data()<<" error!"; 
	 exit(1);
    }


    cData=gzopen(datafile.data(),"r");

    if(cData == NULL) 
    {    
         cout<<datafile.data()<<" error!"; 
	 exit(1);
    }

    indexBuffer = memAlloc(cIndex, INDEX_CHUNK);

    if(indexBuffer == NULL)
    {
	return ;
    }

    dataBuffer = memAlloc(cData, DATA_CHUNK);

    if(dataBuffer == NULL)
    {
	return ;
    }

    lengOfIndex = strlen(indexBuffer);

    lengOfData = strlen(dataBuffer);

    indexStartPoint = 0;
    dataStartPoint = 0;

    while(1)
    {
	if(indexStartPoint >= lengOfIndex)
		break;
	len = readLineMem(indexBuffer, indexStartPoint);

	substr(indexStr,indexBuffer,indexStartPoint,len);
		

	offset = atoi( getLineCol(indexStr,3) );

	indexStartPoint = indexStartPoint + len;

	substr(textStr,dataBuffer,dataStartPoint,offset);

	dataStartPoint = dataStartPoint + offset;

	lengOfPage = offset;

	pool = (char*)malloc(2*lengOfPage+1);

	URLDocID = Build_URLMap(getLineCol(indexStr,0),offset);

	//ret = parser(getLineCol(indexStr,0), textStr, pool, 2*lengOfPage+1);
	ret = parser(getLineCol(indexStr,0), textStr,pool, 2*lengOfPage+1, offset);  //use "parser-revised-again.h"

			
	if (ret > 0)
	{
		string s(pool);
		string aWord;

		vector<string >  strs = split( s, "   ");

		vector<string >::iterator it = strs.begin(), end = strs.end();

		for(; it != end ; ++it)
		{
			aWord =  *it ;
			trim(aWord);
			if(aWord.empty())
			{ 
				continue;
			}
			else 
			{
				LexWordID = Build_Lexicon( aWord ); 
				Build_Temp_Index(LexWordID, URLDocID);
			}
		}

		s.clear();
	}
	free(pool);
    }

    stringstream ss;
    string numStr;
    ss<<number;
    ss>>numStr;

    saveTempIndex2("/home/alex/mycode/newindex/results/tempindex"+numStr);
  

    TempIndex.clear();

    free(indexBuffer);
    free(dataBuffer);

    gzclose(cIndex);
    gzclose(cData);
	
}

int main (int argc, char * argv[])
{
	time_t start_time,end_time ;
	start_time = time(NULL);

	vector<pair<string,string> >  filelist;
	filelist = getFileList();
	
	vector<pair<string,string> >::iterator itr = filelist.begin(), end = filelist.end();

	for(;itr != end; ++itr)
	{	
		runOnce(itr->first,itr->second,itr-filelist.begin());
		cout<<itr->first<<"   "<<itr->second<<endl;
	}


	cout<<"temindex finished!"<<endl;
	
  	saveLexiconByID("/home/alex/mycode/newindex/results/lexi2");

    saveURLMapByID("/home/alex/mycode/newindex/results/url2");


	end_time = time(NULL);

	printf("Elapsed Time :  %f  \n", difftime(end_time,start_time) );


	Lexicon.clear();
	URLMap.clear();
	LexiconByValue.clear();
	URLMapByValue.clear();


	return 0;
}




