
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include <limits.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "dt.h"
#include "dberror.h"


#define maxBufferPoolNum 10
BM_PageHandle *pageHandle;
SM_FileHandle fh;

int currentBufferPoolNum=0;

//the global variable bufferRecord, which holds all the bufferpool information including readnum,writenum,etc 
typedef struct BufferRecord{ 
	int bufferpoolNumber;
	int readNum;	//read number
	int writeNum; //write number
	int pageAge; //a buffer bool level counter
	int lru_k;
	//int BufferNum;//BUfferPool Num
} BufferRecord; 
BufferRecord bufferRecord[maxBufferPoolNum];

typedef struct PageFrame{
	int pageTimeRecord; // record the time record of a page,this value would be refreshed when the page was pinned into the buffer / or used by user(in LRU/LRU-K/CLOCK)
	int fixCount; //fix Count  : evict the page only when fixcount==0
	bool dirty; //store the state of the "dirty"
	bool pinFlag; //record the state of the page pinned or not
	int lru_k;
	int clockwise;
	BM_PageHandle pageHandle;
} PageFrame;

// Buffer Manager Interface Pool Handling
void setBuffer(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData)
{
	bm->pageFile = (char*)pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;
}

void freePageFrame(BM_BufferPool *const bm)
{									
	free(bm->mgmtData);
}


void initBufferRecord(int i)	/* initialze the global variable bufferRecord, which holds all the bufferpool information including readnum,writenum,etc */
{
	bufferRecord[i].readNum=0;
	bufferRecord[i].writeNum=0;
	bufferRecord[i].pageAge=0;
	bufferRecord[i].lru_k=1;

}

PageFrame* initPageFrame(const int numPages)
{
		
		PageFrame *pf;
		pf= (PageFrame*) malloc(sizeof(PageFrame)*numPages);
		int i;
		for(i=0;i<numPages;i++)
		{
			pf[i].dirty=false;
			pf[i].fixCount=0;
			pf[i].pageTimeRecord=0;
			pf[i].pinFlag=false;
			pf[i].pageHandle.pageNum=NO_PAGE;
			pf[i].pageHandle.data=(char*) malloc(PAGE_SIZE*sizeof(char));
			pf[i].lru_k=1;

		}
		return pf;
}

//creates a new buffer pool 
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData )
{
	
	
	int p,*k;
	p=1;
	k=&p;
	

	if ((stratData)!=NULL)
		k=(int*) stratData;
	if(pageFileName==NULL)	
		return RC_FILE_NOT_FOUND; 

	//set new buffer pool info
	setBuffer(bm,pageFileName,numPages,strategy,stratData);
	//allocate the spaces for strategy parameters
	//lru_k=(int*)malloc(sizeof(int)*numPages);
	//clockwise=(int*)malloc(sizeof(int)*numPages);
	// initialze the global variable bufferRecord, which holds all the bufferpool information including readnum,writenum,etc
	PageFrame *pf;
	pf=initPageFrame(numPages);
	bm->mgmtData=pf;
	if (currentBufferPoolNum<maxBufferPoolNum)
    	{	currentBufferPoolNum++;
    		initBufferRecord(currentBufferPoolNum);
    	}	
    else 
  //if exceed max pool, print error message
  	printf("currentBufferPoolNum> maxBufferPoolNum, BufferPoolis full\n");
  
  	bufferRecord[currentBufferPoolNum].lru_k=*k;
	fh.totalNumPages=numPages;
	printf("in initBufferPool \n fh.totalNumPages==%d \n",fh.totalNumPages);
	openPageFile(bm->pageFile, &fh);
	printf("Initialize finished!\n");
	return RC_OK;
}

//destroys a buffer pool, write dirty pages back to disk
RC shutdownBufferPool(BM_BufferPool *const bm){
	int i;
	PageFrame *pf;
	pf=bm->mgmtData;
	for(i=0;i<bm->numPages;i++){
		if(pf[i].fixCount!=0){
		printf("file write failed\n");
		return RC_WRITE_FAILED;
	}
	}
	//write the dirty pages back to disk 
	forceFlushPool(bm);
	//reset buffer
	setBuffer(bm,NULL,0,0,NULL);
	//reset buffer record
	bufferRecord[currentBufferPoolNum].readNum=0;
	bufferRecord[currentBufferPoolNum].writeNum=0;
	//free the spaces of the buffer
	freePageFrame(bm);
	//reduce buffer pool number
	currentBufferPoolNum--;
	return RC_OK;
}

//if the page is unpin and dirty,write it back 
RC forceFlushPool(BM_BufferPool *const bm){

	int i;
	PageFrame *pf;
	pf=bm->mgmtData;
	//open the file to be flushed to
	openPageFile(bm->pageFile, &fh);		
	for(i=0;i<bm->numPages;i++){
		//judge dirty pages
		if(pf[i].dirty==true&&pf[i].fixCount==0 )			
		{
			//ensure capacity before writing back to disk
			ensureCapacity(pf[i].pageHandle.pageNum+1,&fh);                    
			//write back dirty page
			writeBlock(pf[i].pageHandle.pageNum, &fh, pf[i].pageHandle.data);   
			pf[i].dirty=false;
		}
	}
	printf("forceFlushPool Success\n");
	return RC_OK;
}


// Buffer Manager Interface Access Pages
RC checkTargetPage (BM_BufferPool *const bm, BM_PageHandle *const page){
	PageFrame *pf;
	pf=bm->mgmtData;
// check whether the target page is in the buffer pool
	bool check = false;
	int i;
	for(i = 0; i < bm->numPages; i++)
	{
		if(pf[i].pageHandle.pageNum == page->pageNum)
		{
			check = true;
		}
	}
	
	if(check == false)
		return NO_PAGE;
	else
		return RC_OK;
}

RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){
	RC result = checkTargetPage(bm, page);
    PageFrame *pf;
	pf=bm->mgmtData;
	if(result == RC_OK){
// find and mark the dirty page
	int current = 0;	// start from the first pageHadle
	while(current < bm->numPages)
	{
		if(pf[current].pageHandle.pageNum == page->pageNum)	// find the dirty page
		{
			pf[current].dirty= true;	// mark the page dirty
			bufferRecord[currentBufferPoolNum].writeNum++;	// modify the times of writing pages to the disk
			break;
		}
		current++;	// go to the next pageHandle if the dirty page is not found
	}

	return RC_OK;
	}
	else
		return result;
}
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page){
	RC result = checkTargetPage(bm, page);
	PageFrame *pf;
	pf=bm->mgmtData;
	if(result == RC_OK){
// modify the fix count of the to-be-unpinned page
	int current = 0;	// start from the first pageHandle
	while(current < bm->numPages)
	{
		if(pf[current].pageHandle.pageNum == page->pageNum)	// find the to-be-unpinned page
		{
			if(pf[current].fixCount > 0)	// if there are clients who pinned the page before
				pf[current].fixCount--;	// deduct the number of clients by 1
			else				// if no client pinned the page before
				pf[current].fixCount = 0;
			break;
		}
		current++;	// go to the next pageHandle if the to-be-unpinned page is not found

	}
	return RC_OK;
	}
	else
		return result;
}

RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){
	RC result = checkTargetPage(bm, page);
	PageFrame *pf;
	pf=bm->mgmtData;
	if(result == RC_OK){
// write back to the page file in disk
	int current;	// start from the first pageHandle
	for(current = 0; current < bm->numPages; current++)
	{
		if(pf[current].pageHandle.pageNum == page->pageNum ){
           
		if(pf[current].dirty && (pf[current].fixCount == 0))
		{
			openPageFile(bm->pageFile, &fh);            //write the page back to file
			ensureCapacity(page->pageNum, &fh);
			writeBlock(page->pageNum, &fh, page->data);
			closePageFile(&fh);
			printf("forcePage Success\n");

			pf[current].dirty = false;		//the current page is no longer considered dirty

			return RC_OK;
		}
		else if(pf[current].dirty && (pf[current].fixCount != 0))
		{
			openPageFile(bm->pageFile, &fh);            //write the page back to file
			ensureCapacity(page->pageNum, &fh);
			writeBlock(page->pageNum, &fh, page->data);
			closePageFile(&fh);
			printf("forcePage Success\n");

			return RC_OK;
		}
		else
			return RC_WRITE_FAILED;
		}

		}
		}
		else
			return result;

}



RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
	int i=0;
	PageFrame *pf;
	pf=bm->mgmtData;
	page->data=(char*)malloc(sizeof(char)*bm->numPages);
	sprintf(page->data,"%s-%i","Page",pageNum);
	//check if the page is in the buffer pool	
	
	for(i=0;i<bm->numPages;i++){
		//if already in the buffer pool, pin it
		if(pf[i].pageHandle.pageNum==pageNum){	
			pf[i].pinFlag=true;
			page->pageNum=pageNum;
			pf[i].fixCount++;
			switch(bm->strategy)			
			{
				case RS_FIFO:
					//do nothing
				case RS_LRU:
					bufferRecord[currentBufferPoolNum].pageAge++;
					pf[i].pageTimeRecord=bufferRecord[currentBufferPoolNum].pageAge;
				case RS_LRU_K:
					pf[i].lru_k++;
					if (bufferRecord[currentBufferPoolNum].lru_k==pf[i].lru_k)
					bufferRecord[currentBufferPoolNum].pageAge++;
					pf[i].pageTimeRecord=bufferRecord[currentBufferPoolNum].pageAge;

				case RS_CLOCK:
					pf[i].clockwise=1;

			}
			return RC_OK;
		}		

	}

	//if not in the buffer pool, check if in the memory	

	for(i=0;i<bm->numPages;i++){
		
		if(pf[i].pinFlag==false){//if there is a empty frame, fill it
			pf[i].pinFlag=true;
			page->pageNum=pageNum;
			pf[i].pageHandle.pageNum=pageNum;

			if (bm->strategy==RS_CLOCK)
			{
				pf[i].clockwise=1;
			}
			

			pf[i].fixCount++;
			bufferRecord[currentBufferPoolNum].pageAge++;
			pf[i].pageTimeRecord=bufferRecord[currentBufferPoolNum].pageAge;
			bufferRecord[currentBufferPoolNum].readNum++;
			return RC_OK;
		}
	
	}
	
	replaceStrategy(bm,page,pageNum);
}


void pageInfo(BM_BufferPool *const bm,int i)
{	
	PageFrame *pf;
	pf=bm->mgmtData;
	bufferRecord[currentBufferPoolNum].pageAge++;
	pf[i].pageTimeRecord=bufferRecord[currentBufferPoolNum].pageAge;
}

RC replaceStrategy (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
	int i=0;
	PageFrame *pf;
	pf=bm->mgmtData;
	int target = INT_MIN;
	int min=INT_MAX;
	switch (bm->strategy)
	{
			//implementation of the FIFO
			case RS_FIFO:
				for(i=0;i<bm->numPages;i++)
				{
					if(pf[i].pageTimeRecord<=min&&pf[i].fixCount==0)
					{
						min=pf[i].pageTimeRecord;
						target=i;
					}											
				}
				//BM_PageHandle *oldPage;
				//oldPage=&(pf[target].pageHandle);           //write the page back to file
				//ensureCapacity(oldPage->pageNum, &fh);
				//writeBlock(oldPage->pageNum, &fh, oldPage->data);
				
				
				

				pf[target].pinFlag=true;
				page->pageNum=pageNum;
				pf[target].pageHandle.pageNum=pageNum;
				pf[target].fixCount++;
				pageInfo(bm,target);

//				readPage(page,pageNum);
//sprintf(page->data,"%s-%i","Page",pageNum);
				if(pf[target].dirty==true)
				{
					pf[target].dirty=false;
				}	
				bufferRecord[currentBufferPoolNum].readNum++;
				return RC_OK;
			//implementation of the LRU
			case RS_LRU:	
				for(i=0;i<bm->numPages;i++)
				{
					if(pf[i].pageTimeRecord<=min&&pf[i].fixCount==0)
					{
						min=pf[i].pageTimeRecord;
						target=i;
					}											
				}
				pf[target].pinFlag=true;
				page->pageNum=pageNum;
				pf[target].pageHandle.pageNum=pageNum;
				pf[target].fixCount++;
				pageInfo(bm,target);
//				readPage(page,pageNum);
//sprintf(page->data,"%s-%i","Page",pageNum);

				if(pf[target].dirty==true)
				{
					pf[target].dirty=false;
				}	
				bufferRecord[currentBufferPoolNum].readNum++;
				return RC_OK;
			case RS_LRU_K:	
				for(i=0;i<bm->numPages;i++)
				{
					if(pf[i].pageTimeRecord<=min&&pf[i].fixCount==0)
					{
						min=pf[i].pageTimeRecord;
						target=i;
					}											
				}
				pf[target].pinFlag=true;
				page->pageNum=pageNum;
				pf[target].pageHandle.pageNum=pageNum;
				pf[target].fixCount++;
				pageInfo(bm,target);
//				readPage(page,pageNum);
//sprintf(page->data,"%s-%i","Page",pageNum);

				if(pf[target].dirty==true)
				{
					pf[target].dirty=false;
				}	
				bufferRecord[currentBufferPoolNum].readNum++;
				return RC_OK;
			case RS_CLOCK:
				for(i=0;i<bm->numPages;i++)
				{
					if(pf[i].clockwise==0&&pf[i].fixCount==0)
					{
						target=i;
					}

				}
				if (target=-1)//return no suitable position
				{
					for(i=0;i<bm->numPages;i++)
					pf[i].clockwise=0;
				}


				pf[target].pinFlag=true;
				page->pageNum=pageNum;
				pf[target].pageHandle.pageNum=pageNum;
				pf[target].fixCount++;
				pageInfo(bm,target);
				
				if(pf[target].dirty==true)
				{
					pf[target].dirty=false;
				}	
				bufferRecord[currentBufferPoolNum].readNum++;
				return RC_OK;


	}	
	
}

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm){
	PageFrame *pf;
	pf=bm->mgmtData;
	PageNumber *frameContent;
	frameContent = (int *)malloc(sizeof(int)*bm->numPages);
	int i;
	for(i = 0; i < bm->numPages; i++)
	{
		frameContent[i] = pf[i].pageHandle.pageNum;
	}
	return frameContent;
}
bool *getDirtyFlags (BM_BufferPool *const bm){
	PageFrame *pf;
	pf=bm->mgmtData;
	bool *dirtyFlag;
	dirtyFlag = (bool*)malloc(sizeof(bool)*bm->numPages);
	int i;
	for(i = 0; i < bm->numPages; i++)
	{
		dirtyFlag[i] = pf[i].dirty;
	}
	return dirtyFlag;
}
int *getFixCounts (BM_BufferPool *const bm){
	int *fixCount;
	PageFrame *pf;
	pf=bm->mgmtData;
	fixCount = (int*)malloc(sizeof(int)*bm->numPages);
	int i;
	for(i = 0; i < bm->numPages; i++)
	{
		fixCount[i] = pf[i].fixCount;
	}
	return fixCount;
}
int getNumReadIO (BM_BufferPool *const bm){
	return bufferRecord[currentBufferPoolNum].readNum++;
}
int getNumWriteIO (BM_BufferPool *const bm){
	return bufferRecord[currentBufferPoolNum].writeNum;
}

