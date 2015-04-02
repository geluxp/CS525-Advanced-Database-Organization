CS 525 Programming Assign 2: Buffer Manager

Group member: 
KUN MEI
FAN ZHANG
XIAOPEI ZHANG
JUN QIAN


Design: 

This project is to finish a buffer manager of which the main function is to copy desired pages from file to the buffer pool and then user can do reading/writing operation through the buffer pool. We write a .c file called buffer_mgr.c and implement all the interface given by buffer_mgr.h. Also we have written a extra test file call test_assign2_me.c and finish a more completed test for the function of storage manager. 

We made a makefile called Makefile for the testing by file test_assign2_1.c.
We also made a makefile called Makefile_ex for the testing purpose by file test_assign2_11.c


We define new structures called PageFrame and Buffer record and use them to store the page and buffer pool parameters.The details are as following

BufferRecord: in this data structure, we store those variables in the level of buffer pool.
	We initialize the BufferRecord in a format of  array. Each slot of the array represents one bufferpool
	Those variables are

	int bufferpoolNumber;  //record the No of the bufferpool.In each call of initBufferPool, this No would plus one. In turn, in each call of shutdownBufferPool, this No would minus one.
	int readNum;	      //record the number of pages that have been read from disk since a buffer pool has been initialized
	int writeNum; 	      //record the number of pages written to the page file since the buffer pool has been initialized
	int pageAge;         //a bufferpool level counter. We use this variable to record the timeseries of pages.This variable would be incremented each time a page is called/push into the buffer.
	int lru_k;			//record k value in the LRU-K

PageFrame: in this data structure, we store those variables in the level of page frame.
	We initialize the PageFrame in a format of  array of the size of num of pages. Each slot of the array represents one pageFrame.			
	Those variables are:
      	int pageTimeRecord; // record the time record of a page,this value would be refreshed when the page was pinned into the buffer / or used by user(in LRU/LRU-K/CLOCK)
	int fixCount; //fix Count  : evict the page only when fixcount==0
	bool dirty; //store the state of the "dirty"
	bool pinFlag; //record the state of the page pinned or not
	BM_PageHandle pageHandle; //record the pageHandle of the page in the corresponding pageFrame.
	int clockwise; // record an auxiliary array for clockwise algorithm
	int lru_k;			//record k value of the page. Once this k value is equal to the k value in the bufferRecord, the "age" of the page would be refreshed.

We initialize the PageFrame in a format of  array of the size of num of pasges. Each slot of the array represents one pageFrame.
			


Design of initBufferPool: 
	1.Check and set buffer manager parameters
	2.Initialze the global strcut bufferRecord, which holds all the bufferpool information including readnum,writenum,etc
	3.Open the pageFile

Design of shutdownBufferPool:
	1.Check whether pages in the buffer pool are unpinned, if so return error;
	2.write the dirty pages to the disk by forceFlushPool();
	3.Free adn reset all the parameters;

Design of forceFlushPool:
	1.Open the file to be flushed to
	2.Ensure capacity before writing back to disk and write  back to the disk

Design of markDirty:
	1.check whether the target page in the buffer pool, if not, return a NO_PAGE error
	2.use while loop to fing the target page in the buffer pool
	3.mark the page dirty (set true), and add writeNum by 1 to make sure that the ouput time will be increased accordingly

Design of forcePage:
	1.check whether the target page in the buffer pool, if not, return a NO_PAGE error
	2.use for loop to find the target page in the buffer pool
	3.1if the target page is dirty and the fixnumber is 0, write the page back to file and mark the page not dirty (set false)
	3.2if the target page is dirty and the fixnumber is not 0, write the page back to file
	3.3otherwise, return RC_WRITE_FAILED error

The mainly process of pinPage:
	1.load the pageframe
	2 check if the page is already  in the buffer pool,if so,go to 2.1
	2.1.find the corresponding pageNum and pin the page with this pageNum.
	2.2. check the replacementStrategy that would be used
 	2.3.set the pageAge and other info in related to the selected replacementStratgegy
	3 if the page is not the buffer pool, check if there is an empty pageframe.if so ,go to 3.1
	3.1 find the corresponding pageNum and pin the page with this pageNum.
	3.2  check the replacementStrategy that would be used
	3.3  set the pageAge and other info in related to the selected replacementStratgegy
	4 if all the pageFrame is full, we used the replacementStretegy selected to replace an exisiting page,go to 4.1
	4.1 replace the corresponding page in the pageFrame.
		
	

The mainly process of unpinPage:
	1.load the pageframe
	2 check if the page is already  in the buffer pool,if so,go to 2.1
	2.1.find the corresponding pageNum and pin the page with this pageNum.
	2.2. check the replacementStrategy that would be used
 	2.3.set the pageAge and other info in related to the selected replacementStratgegy
	3 if the page is not the buffer pool, check if there is an empty pageframe.if so ,go to 3.1
	3.1 find the corresponding pageNum and pin the page with this pageNum.
	3.2  check the replacementStrategy that would be used
	3.3  set the pageAge and other info in related to the selected replacementStratgegy
	4 if all the pageFrame is full, we used the replacementStretegy selected to replace an exisiting page,go to 4.1
	4.1 replace the corresponding page in the pageFrame.
		



Additional functions we added: 

The following function are called in initBufferPool()
	void initiBufferRecord(int i) :initialze the strcut bufferRecord, which holds all the bufferpool information including readnum,writenum,etc

	PageFrame* initPageFrame(const int numPages): initialize the strcut PageFrame, which holds all the pageFrame info including PageHandle,dirty,fixCount,etc
	
	void setBuffer(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData): 
	set the BM_BufferPool info

The following function are called in the shutdownBufferPool()
	freePageFrame(BM_BufferPool *const bm): free the space of the pageFrame
	
	setBuffer(): set the BM_BufferPool info

The following are called in the pinPage()	
	void pageInfo(BM_BufferPool *const bm,int i) : simple helper function. set the pageAge of the page that is called/pushed into the buffer.

	RC replaceStrategy (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum): a helper function called by the pinPage(). Implement different replacementStrategy.



************ Test Comment ************

-Test file 1: test_assign2_1.c
*Default test.
*Test all passed

-Test file 2: test_assign2_11.c
1.Buffer Manager has more than one buffer pool, ****(not realized thread safe)****in order to let several clients operate on differnt pool at the same time.  
2.The buffer pool is full and all frame has positive fix count. Test pin an existed page and pin a new page. 
3.Test LRU
-K.Test CLOCK

* All test cases are passed
 
 

*************************************************BONUS***********

-Robust: in our implementation, our code can handle different types of errors and print the error information for the user, such as the unsuccessful pinPage because that every page's fixcount>0, read/write error and so on.

-LRU-K and Clock algorithm: we realize the clock replacement strategy in our design as well.
