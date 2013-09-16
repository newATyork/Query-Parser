#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <string.h>
#include <malloc.h>
#include <algorithm>
#include <vector>  
#include <queue>  // priority queue
#include <functional>  
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <time.h>
#include "vbyte.h"
#include <tr1/unordered_map>

#define MAX_GLOBAL_DOCID 1145865
#define AVG_DOC_LENGTH 6065 

using namespace std;
using std::tr1::unordered_map;


unordered_map < int, string > urlHashMap;                // < urlId, url >
unordered_map < string, pair<int,int> > lexHashMap;           // < word ,pair<wordid,maxDocID> >


struct Node
{
	int DocID;
	float Rank;

};

class Compare {
	public:
    		bool operator()(Node& n1, Node& n2)
		{
			if (n1.Rank > n2.Rank) return true;
			else return false;
    		}
};


struct lp
{
	FILE* f;
	int startPosition;
	int length;
	int maxdocid;
	int prevdocid;
	int ft; // the number of Documents which contains this term


	lp()
	{
		f = NULL;	
		startPosition = -10;
		length = -10;
		maxdocid = -10;
		prevdocid = -10;  //previous DocID;
		ft = -10;

	}

	lp(FILE* file, int sp,int len,int mid,int freqTerm)
	{
		f = file;	
		startPosition = sp;
		length = len;
		maxdocid = mid;
		ft = freqTerm;
		
	}

	void get()
	{
		cout<<"term startpoint:  "<<startPosition <<endl;
		cout<<"posting list length of this term:  "<<length <<endl;
		cout<<"maximum DocID of this term:  "<<maxdocid <<endl;
		cout<<"the number of Docs which contains this term:  "<<ft <<endl;
		cout<<endl;
	}
};


vector<Node> VecNodes;  //  a vector of urlnodes




lp openLIST(string term)
{

	lp pointer;

	char aLine2[2000],word[1000];
	int wordID,FT,maxDocID,offset,lenSum=0;

	string lexfilepath = "/home/alex/mycode/newindex/results/lexiconnew3";;
	FILE* lexfile;  

	if( ( lexfile = fopen(lexfilepath.data(),"r") ) == 0 )
	{
		cout<<"Lex file fails to be open!"<<endl;
		exit(1);
	}
	
	string compressfilepath = "/home/alex/mycode/newindex/mergefile/compressfile3";
	FILE* compressfile;

	if( ( compressfile = fopen(compressfilepath.data(),"r") ) == 0 )
	{
		cout<<"Final file fails to be open!"<<endl;
		exit(1);
	}

	
	unordered_map < string, pair<int,int> >::const_iterator got = lexHashMap.find (term);

		
	if ( got == lexHashMap.end() )
	{
		cout<<endl<<"the word: '"<< term << "' not found! "<<endl;
		//pointer.get();
		return pointer;
	}
	else
	{

		int tempWordID = got->second.first;

		for(;tempWordID> 0;tempWordID--)
		{
			fscanf(lexfile,"%d %s %d %d %d\n",&wordID,word,&FT,&maxDocID,&offset);
			lenSum = lenSum + offset;
		}

		fscanf(lexfile,"%d %s %d %d %d\n",&wordID,word,&FT,&maxDocID,&offset);


		pointer = lp(compressfile,lenSum,offset,maxDocID,FT);

		//pointer.get();

		return pointer;
	
	}	

}

void closeLIST(lp* pt)
{
	if(pt->f != NULL)
		fclose(pt->f);
}


int nextGEQ(lp* pter , int k)
{
	int number;
	int DocID=-1;

	
	int lenOfNum, lenSum=0;
	unsigned char temp[30]; // it's enough to hold the numbers, DocID, Freq

	if( k > pter->maxdocid )
	{
		
		return MAX_GLOBAL_DOCID+20;
		
	}
	else
	{
		
		FILE* tempFilePointer=pter->f;

	
		while(DocID<k)
		{
			
			lenSum = 0;
		
			fseek(tempFilePointer, pter->startPosition, SEEK_SET);
			fread(temp,30,1,tempFilePointer);

			lenOfNum = vbyte_decompress(temp,&number);
			
			lenSum= lenSum + lenOfNum;

			DocID = number;
			
			lenOfNum = vbyte_decompress(temp+lenOfNum,&number);
			lenSum= lenSum + lenOfNum;

			pter->startPosition = pter->startPosition + lenSum;
			
		}
	
		if(DocID==k) pter->startPosition = pter->startPosition - lenSum;
		return DocID;
	}
}


int getFreq(lp* pter)  // get the frequency of a term in one doc   
{
	int number;
	int lenOfNum;
	unsigned char temp[30]; // it's enough to hold the numbers, DocID, Freq

	FILE* tempFilePointer=pter->f;

	fseek(tempFilePointer, pter->startPosition, SEEK_SET);
	fread(temp,30,1,tempFilePointer);

	lenOfNum = vbyte_decompress(temp,&number);
	lenOfNum = vbyte_decompress(temp+lenOfNum,&number);

	return number;
}


//==================================Get TOP 10==============================================

float K_Func(int lenOfDoc)
{
	float b = 0.75;
	float k1 = 1.2;
	
	return k1*(1-b) + ( b*lenOfDoc/AVG_DOC_LENGTH );
}


int QueryProcess(vector<string>  terms)
{
	
	
	int num = terms.size();
	int d,i;
	int N = MAX_GLOBAL_DOCID + 1;
	int conjuctive_maxdocid=-1;
	
	lp lpArray[num];

	vector<string>::iterator it=  terms.begin();

	for(i=0; i<num; ++i, ++it)
	{
		lpArray[i] = openLIST(*it);
		
		int temp_maxdocid=lpArray[i].maxdocid;
		if(temp_maxdocid>conjuctive_maxdocid)
			conjuctive_maxdocid=temp_maxdocid;
	}

	
	int did = 0;
	
	

	while(did <= MAX_GLOBAL_DOCID)
	{
		
		if(num==1)
		{
			did = nextGEQ(&lpArray[0],did);
			
			float BM25RANK=0.0;

			
			if(did>conjuctive_maxdocid) break;
		
			for(int j=0; j<num; j++)
			{
				int fdt = getFreq(&lpArray[j]);
				int ft = lpArray[j].ft;
				BM25RANK = BM25RANK + log10( (N - ft +0.5)/(ft +0.5) ) * (2.2*fdt)/(fdt+K_Func(lpArray[j].length));
			}
		
			Node tempnode;
			tempnode.DocID = did;
			tempnode.Rank = BM25RANK;
			
		
			VecNodes.push_back(tempnode);

			did++;
		}
		else
		{

			did = nextGEQ(&lpArray[0],did);
			
			if(did>conjuctive_maxdocid) break;

			for (i=1; (i<num) && ((d=nextGEQ(&lpArray[i], did)) == did); i++);

			if (d > did) 
			{
				did = d;
			}
			else   // BM25
			{
				float BM25RANK=0.0;
		
				for(int j=0; j<num; j++)
				{
					int fdt = getFreq(&lpArray[j]);
					int ft = lpArray[j].ft;
					BM25RANK = BM25RANK + log10( (N - ft +0.5)/(ft +0.5) ) * (2.2*fdt)/(fdt+K_Func(lpArray[j].length));
				}
		
				Node tempnode;
				tempnode.DocID = did;
				tempnode.Rank = BM25RANK;

				VecNodes.push_back(tempnode);

				did++;
		
			}
		}
	}

	
	
	for (i = 0; i < num; i++) 
		closeLIST(&lpArray[i]);

	return 0;

}


//==========================================================================================

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

//==========================================================================================

int main(int argc, char* argv[])
{


	string urlfilepath = "/home/alex/mycode/newindex/results/url2";
	FILE* urlfile = fopen(urlfilepath.data(),"r");

	char aLine1[2500],url[2000];
	int urlID, size;

	urlHashMap.rehash(ceil(1200000/ urlHashMap.max_load_factor()));  
	lexHashMap.rehash(ceil(1200000/ lexHashMap.max_load_factor()));   

	while(!feof(urlfile))
	{
		fscanf(urlfile,"%d %s %d\n",&urlID,url,&size);

		urlHashMap.insert( make_pair(urlID,string(url)) ); 
	}

	//cout<<"urlHashMap is OK"<<endl<<endl;

	string lexfilepath = "/home/alex/mycode/newindex/results/lexiconnew3";
	FILE* lexfile = fopen(lexfilepath.data(),"r");

	char aLine2[2000],word[1000];
	int wordID,FT,maxDocID,offset;

	while(!feof(lexfile))
	{
		fscanf(lexfile,"%d %s %d %d %d\n",&wordID,word,&FT,&maxDocID,&offset);
		lexHashMap.insert( make_pair( string(word), make_pair(wordID,maxDocID) ) );   
	}
	
	//cout<<"lexHashMap is OK"<<endl<<endl;


	string seperate = " ";
	string input="";

	while(1)
	{
		
		cout<<endl;
		cout<<"========================================================================================"<<endl;

		int numOfTerms=0;
		cout<<"please input your query:"<<endl;
		getline(cin, input);

		if(input=="exit;")
			break;
		

		vector<string> terms = split(input,seperate);
	
		vector<string >::iterator it = terms.begin(), end = terms.end();

		cout<<endl<<"Terms are:"<<endl;
	
		for(;it != end; ++it, numOfTerms++)
			cout<<(*it)<<endl;

		cout<<endl;
	
		cout<<"numOfTerms is :"<<numOfTerms<<endl<<endl;

		//========================generate Priority Queue=========================================
		Node A;
		A.DocID = -2;
		A.Rank = -10000.0;

		priority_queue<Node, vector<Node>, Compare> nodevec;

	
		for(int i=0; i<10; i++)
		{
			nodevec.push(A);
		}

		clock_t start_time=clock();

		QueryProcess(terms);


		cout<<"VecNodes.size()   "<<VecNodes.size()<<endl;

	
		vector<Node>::iterator its = VecNodes.begin(), ends = VecNodes.end();

		for(;its!=ends;++its)
		{
			nodevec.push(*its);
			nodevec.pop();
		}

		vector<Node> sortarray;

		while( !nodevec.empty() )
		{
			Node tempNode;
			tempNode = nodevec.top();
			nodevec.pop();

			sortarray.push_back(tempNode);
		}

		//===============================================================================

		cout<<endl;

		vector<Node>::reverse_iterator itr1=sortarray.rbegin(), end1=sortarray.rend();   // output in descending order

		for(;itr1!=end1;++itr1)
		{
			if(itr1->DocID != -2)
				cout<<setw(-8)<<itr1->DocID<<"     "<<setw(100)<<urlHashMap[itr1->DocID]<<"     "<<setw(-11)<<itr1->Rank<<endl;
			
		}
			


		clock_t end_time=clock();
		cout<<endl<<"Running time of querying is: "<<static_cast<double>(end_time-start_time)/CLOCKS_PER_SEC*1000<<"ms"<<endl;

		terms.clear();
		VecNodes.clear();
		sortarray.clear();
		
	}

	return 0;
}
