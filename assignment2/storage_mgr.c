#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include "storage_mgr.h"

/* run this program using the console pauser or add your own getch, system("pause") or input loop */

/************************************************************
*                    interface                             *
************************************************************/
/* manipulating page files */

/*..........
.....
....
.....]
.........................qianjun's code......................*/

int creatFile;

void initStorageManager(void){    //extern void initStorageManager (void);
	printf("We are now initialzing the StorageManager\nthe StorageManager version 1.00 \n");   // to let user know enter the StorageManager
	//return;
}


RC createPageFile (char *fileName){
	FILE *fp;   //get a file 
	char * str;   //use the str as a char document

	if(fileName==NULL){  //check the fileName if it exists
		printf("pleas enter a valid fileName!!\n");
		return RC_FILE_NOT_FOUND; 
	}

	fp = fopen(fileName,"w+");   //make sure the fp file readable, and writable
	str = (char *)malloc(PAGE_SIZE);   //allocate the space for the string
	memset(str,'\0',PAGE_SIZE);	//initialize the string
	fwrite(str,PAGE_SIZE,1,fp); // write the string to the file
	fclose(fp); // close it
	free(str); // free the space, bye!
	return RC_OK;

}


    int fileDescriptor; 
RC openPageFile (char *fileName, SM_FileHandle *fHandle)
{
	long fileSize;
	fileDescriptor=open(fileName,O_RDWR);				//get the file descriptor, if get -1, return error
	if(fileDescriptor==-1){
		printf("file not found %s\n",fileName);
		return RC_FILE_NOT_FOUND;
	}
	struct stat buffer;	
	fstat(fileDescriptor, &buffer);                          //acquire the attributes of the file
	fileSize = buffer.st_size%PAGE_SIZE? buffer.st_size/PAGE_SIZE+1: buffer.st_size/PAGE_SIZE; //get the length of the file	
		//set the fhandle structure
	fHandle->fileName = fileName;
	fHandle->totalNumPages = (int)fileSize;  
    fHandle->curPagePos = 0;                 
	fHandle->mgmtInfo = &fileDescriptor;                
	return 	RC_OK;
}



RC closePageFile(SM_FileHandle *fHandle){    // extern RC closePageFile (SM_FileHandle *fHandle)

	if (fHandle == NULL){                         //if the fHandle is null, then return not init
		return RC_FILE_HANDLE_NOT_INIT;
	}

	//set the fhandle structure
	fHandle->fileName = NULL;                //make the file name of fhandle is null;

	fHandle->totalNumPages = 0;              //make the file totalnumPages of fhandle is 0;
	
	fHandle->curPagePos = 0;                 //make the file curpagepos of fhandle is 0;
	
	fHandle->mgmtInfo = NULL;                ////make the file mgmtInfo of fhandle is null;

	return RC_OK;

}




RC destroyPageFile(char *fileName){  //extern RC destroyPageFile (char *fileName)


	if (fileName == NULL){   //if file is null, then filename is null

		return RC_FILE_NOT_FOUND;
	}

	int temp;
	temp = remove(fileName);   //use temp to store the result after removing fileName

	if (temp == -1){                //if the result is -1, means destory fail failed

		printf("destory file failed %s\n", fileName);
		return RC_FILE_NOT_FOUND;

	}

	//perroe("remove"); the error exist in the errno 	
	return RC_OK;

}

/*...............................below is xiaopei's code....................*/
/* reading blocks from disc */
extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if(fHandle != NULL)
	{
		if( 0 <= pageNum && pageNum < fHandle->totalNumPages) // if the specified pageNum is within the range
		{
			int *fileDescriptor = (int *) fHandle->mgmtInfo; // extract the fileDescriptor
			int OFFSET = pageNum * PAGE_SIZE; // get the offset read position for lseek()
			
			long lseekResult = lseek(*fileDescriptor, OFFSET, SEEK_SET); // change the file offset to the start position of the pageNumth page
			if (lseekResult == -1) // check whether lseek worked
			{
				printf("The file offset remains unchanged"); // file offset is not changed
				return lseekResult;
			}
			
			long readResult = read(*fileDescriptor, memPage, PAGE_SIZE); // read the pageNumth page into buffer memPage
			if (readResult == -1) // check whether read worked
			{
				printf("The page is not read into the buffer"); // page is not read into buffer
				return readResult;
			}
			
			fHandle->curPagePos = pageNum; // assign pageNum to curPagePos
			return RC_OK;
			
		}
		else
		{
			printError(RC_READ_NON_EXISTING_PAGE); // printout the message for the error
			return RC_READ_NON_EXISTING_PAGE; // error: the specified pageNum does not exist
		}
	}
	else
	{
		printError(RC_FILE_HANDLE_NOT_INIT); // printout the message for the error
		return RC_FILE_HANDLE_NOT_INIT; // error: fHandle is null
	}
}

extern int getBlockPos (SM_FileHandle *fHandle)
{
	if(fHandle != NULL)
		return fHandle->curPagePos; // return the current page position in the file
	else
	{
		printError(RC_FILE_HANDLE_NOT_INIT); // printout the message for the error
		return RC_FILE_HANDLE_NOT_INIT; // error: fHandle is null
	}
}

extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{
	if(fHandle != NULL)
	{
		int *fileDescriptor = (int *) fHandle->mgmtInfo; // extract the fileDescriptor
		
		long lseekResult = lseek(*fileDescriptor, 0, SEEK_SET); // change the file offset to the start position of the first page
		if (lseekResult == -1) // check whether lseek worked
		{
			printf("The file offset remains unchanged"); // file offset is not changed
			return lseekResult;
		}
		
		long readResult = read(*fileDescriptor, memPage, PAGE_SIZE); // read the first page into buffer memPage
		if (readResult == -1) // check whether read worked
		{
			printf("The page is not read into the buffer"); // page is not read into buffer
			return readResult;
		}
			
		fHandle->curPagePos = 0; // assign pageNum to curPagePos
		return RC_OK;
	}
	else
	{
		printError(RC_FILE_HANDLE_NOT_INIT); // printout the message for the error
		return RC_FILE_HANDLE_NOT_INIT; // error: fHandle is null
	}
}

extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{	
	if(fHandle != NULL)
	{
		if( fHandle->curPagePos == 0) // if the current page is the first page
		{
			int *fileDescriptor = (int *) fHandle->mgmtInfo; // extract the fileDescriptor
		
			long lseekResult = lseek(*fileDescriptor, -PAGE_SIZE, SEEK_CUR); // change the file offset to the start position of the first page
			if (lseekResult == -1) // check whether lseek worked
			{
				printf("The file offset remains unchanged"); // file offset is not changed
				return lseekResult;
			}
			
			long readResult = read(*fileDescriptor, memPage, PAGE_SIZE); // read the first page into buffer memPage
			if (readResult == -1) // check whether read worked
			{
				printf("The page is not read into the buffer"); // page is not read into buffer
				return readResult;
			}
				
			fHandle->curPagePos -= 1; //  curPagePos minus one
			return RC_OK;
		}
		else
		{
			printf("The current page is the first page. There is no previous page");
			printError(RC_READ_NON_EXISTING_PAGE); // printout the message for the error
			return RC_READ_NON_EXISTING_PAGE; // error: there is no previous page
		}
	}
	else
	{
		printError(RC_FILE_HANDLE_NOT_INIT); // printout the message for the error
		return RC_FILE_HANDLE_NOT_INIT; // error: fHandle is null
	}
}

/*..............
.....
....
....
....
....
....this is kun mei's code.................**/
//user guides of the functions we used in this section is mainly referenced in the follow two websites
//http://c.biancheng.net/cpp/html/236.html  lseek()
//http://c.biancheng.net/cpp/html/236.html  read()


extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) 
// reading the contents in the current block into memory
{
    if(fHandle==NULL)  //FileHandle is not properly initialized
    {
        printf("File Handle is not properly initialized\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }

    if(fHandle->totalNumPages<1) //total page number is not a valid value
    {
        printf("Total page number should be a positive integer\n");
        return RC_FILE_HANDLE_NOT_INIT; 
    }
    if(fHandle->curPagePos<0)   // current page is not in a valid state
    {
        printf("current page position is not in a valid state (current page number should be a non negative integer)\n");
        return RC_READ_NON_EXISTING_PAGE;
    }

    if(fHandle->totalNumPages-1<fHandle->curPagePos) // current page is not a  valid state
    {
        printf("current page position is not in a valid state (current page number exceeding the maximum number of pages)\n"); 
        return RC_READ_NON_EXISTING_PAGE;
    }
    /*****************main parts*************/
    int *fileDescriptor; //define the local file descriptor
    fileDescriptor=(int *)fHandle->mgmtInfo; 
    //receiving the file descriptor of the loading File
    long readResult;
    int errno;
    errno=0;

    readResult=read(*fileDescriptor,memPage,PAGE_SIZE);  //read function fails
    if(readResult=-1)   //reading blcok fails
    {  
        printf("reading block fails\n");
        printf("error type %d\n",errno);      
                 //output errno value
        printf("%s\n",strerror(errno));            //output errnor message
       // perror("");                                //output errnor message
       // printf("%m\n");                            //output errnor message
       return RC_READ_NON_EXISTING_PAGE;
    }
    if(readResult<PAGE_SIZE)
    {
        printf("this is possible the end of the file, return less bytes(%ld bytes) than the page size (4kbytes)\n",readResult );
    }
    return RC_OK;
    }


extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)  
// reading the contents in the next block into memory
{
    if(fHandle==NULL)  //FileHandle is not properly initialized
    {
        printf("File Handle is not properly initialized\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }

    if(fHandle->totalNumPages<1) //total page number is not a valid value
    {
        printf("Total page number should be a positive integer\n");
        return RC_FILE_HANDLE_NOT_INIT; 
    }
    if(fHandle->curPagePos<0)   // current page is not in a valid state
    {
        printf("current page position is not in a valid state (current page number should be a non negative integer)\n");
        return RC_READ_NON_EXISTING_PAGE;
    }

    if(fHandle->totalNumPages-1==fHandle->curPagePos) // current page is the last page, no next block exist
    {
        printf("current Block is the last block, next block does not exist\n"); 
        return RC_READ_NON_EXISTING_PAGE;
    }
     /*****************main parts*************/
    fHandle->curPagePos+=1;   //set the current page position to the next page
   
    int *fileDescriptor;                        //define the local file descriptor
    fileDescriptor=(int *)fHandle->mgmtInfo;    //receiving the file descriptor of the loading File
     int errno;
	errno=0;
    long lseekResult;                           //define the return value of the lseek()
    lseekResult=lseek(*fileDescriptor, PAGE_SIZE ,SEEK_CUR);
    if(lseekResult==-1)  //lseeking fails
    {
	
        printf("The page is not read into the buffer"); // page is not read into buffer
        printf("error type %d\n",errno);                       //output errno value
        printf("%s\n",strerror(errno));            //output errnor message
       // perror("");                                //output errnor message
       // printf("%m\n");                            //output errnor message
                return RC_READ_NON_EXISTING_PAGE;
    }

   
    long readResult;
    readResult=read(*fileDescriptor,memPage,PAGE_SIZE);  
    if(readResult=-1)  //reading blcok fails
    {  

        printf("reading block fails\n");
        printf("error type %d\n",errno);                       //output errno value
        printf("%s\n",strerror(errno));            //output errnor message
       // perror("");                                //output errnor message
       // printf("%m\n");                            //output errnor message
       return RC_READ_NON_EXISTING_PAGE;
    }
    if(readResult<PAGE_SIZE)
    {
        printf("this is possible the end of the file, return less bytes(%ld bytes) than the page size (4kbytes)\n",readResult );
    }
    return RC_OK;
}



extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) 
// reading the contents in the next block into memory
{
    if(fHandle==NULL)  //FileHandle is not properly initialized
    {
        printf("File Handle is not properly initialized\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }

    if(fHandle->totalNumPages<1) //total page number is not a valid value
    {
        printf("Total page number should be a positive integer\n");
        return RC_FILE_HANDLE_NOT_INIT; 
    }
    if(fHandle->curPagePos<0)   // current page is not in a valid state
    {
        printf("current page position is not in a valid state (current page number should be a non negative integer)\n");
        return RC_READ_NON_EXISTING_PAGE;
    }
    /*****************main parts*************/

    fHandle->curPagePos=fHandle->totalNumPages-1; //set the current page position to the next page
    int *fileDescriptor;                        //define the local file descriptor
    fileDescriptor=(int *)fHandle->mgmtInfo;    //receiving the file descriptor of the loading File
    int totalOffset;                            //total offset, we use this variable to locate the last block
    totalOffset=PAGE_SIZE*(fHandle->totalNumPages-1); //get to the beginning of the last page
     int errno;
	errno=0;

    long lseekResult;                           //define the return value of the lseek()
    lseekResult=lseek(*fileDescriptor, totalOffset ,SEEK_SET);
    if(lseekResult==-1)  //lseeking fails
    {	
	
        printf("The page is not read into the buffer"); // page is not read into buffer
        printf("error type %d\n",errno);                       //output errno value
        printf("%s\n",strerror(errno));            //output errnor message
       // perror("");                                //output errnor message
       // printf("%m\n");                            //output errnor message
                return RC_READ_NON_EXISTING_PAGE;
    }
   
    long readResult;
    readResult=read(*fileDescriptor,memPage,PAGE_SIZE);  
    if(readResult=-1)  //reading block fails
    {  
        printf("reading block fails\n");
        printf("error type %d\n",errno);                       //output errno value
        printf("%s\n",strerror(errno));            //output errnor message
       // perror("");                                //output errnor message
       // printf("%m\n");                            //output errnor message
       return RC_READ_NON_EXISTING_PAGE;
    }
    if(readResult<PAGE_SIZE)
    {
        printf("last block, return less bytes(%ld bytes) than the page size (4kbytes)\n",readResult );
    }
    return RC_OK;

}


/*........................
.......
....
....
...
...........below is fan zhang's code..................*/

/* writing blocks to a page file */

//Write a page to disk using an absolute position
extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)		
{
	int *fileDescriptor;
  fileDescriptor=(int *)fHandle->mgmtInfo;
	//check if the given page number is greater than the total pages, if so, return RC_WRITE_FAILED
	if(pageNum >= fHandle->totalNumPages)              
	{
		printf("Writing failed! Page full!\n");
		return RC_WRITE_FAILED;
	}
  FILE *fp ;
  fp=fopen(fHandle->fileName,"w");
	//seek the space of pageNumth PAGE_SIZE from the beginning for writting
	fseek(fp,pageNum*PAGE_SIZE,SEEK_SET);
	//check if the writing is successful
	int write_return=fwrite(&memPage,sizeof(PAGE_SIZE),1,fp);
	if(write_return==-1)
	{
		printf("writeBlock! Writing error!\n");
		return RC_WRITE_FAILED;
	}
	//set writed page as current position
	fHandle->curPagePos=pageNum;
	return RC_OK;
}

//Write a page to disk using  current position
extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{	
	if(fHandle==NULL)  //FileHandle is not properly initialized
    {
        printf("File Handle is not properly initialized\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }

	int *fileDescriptor;
  fileDescriptor=(int *)fHandle->mgmtInfo;
	//use lseek() to set the write position, if get -1, return RC_WRITE_FAILED
	int lseek_return=lseek(*fileDescriptor, fHandle->curPagePos*PAGE_SIZE, SEEK_SET);
	if(lseek_return==-1)		
	{
		printf("writeCurrentBlock failed! wrong page number!\n");
		return RC_WRITE_FAILED;
	}
		int errno;
	errno=0;
  //use write() to write the data to the page, if get -1, return RC_WRITE_FAILED
  int write_return=write(*fileDescriptor, memPage, PAGE_SIZE);  
  if(write_return==-1)
	{
	
        printf("writing Current block fails\n");
        printf("error type %d\n",errno);                       //output errno value
        printf("%s\n",strerror(errno));            //output errnor message
       // perror("");                                //output errnor message
       // printf("%m\n");                            //output errnor message
		return RC_WRITE_FAILED;
	}
    
	return RC_OK;
}


//Increase the number of pages in the file by one. The new last page should be filled with zero bytes.
extern RC appendEmptyBlock (SM_FileHandle *fHandle)
{	
	if(fHandle==NULL)  //FileHandle is not properly initialized
    {
        printf("File Handle is not properly initialized\n");
        return RC_FILE_HANDLE_NOT_INIT;
    }

	int *fileDescriptor;
  fileDescriptor=(int *)fHandle->mgmtInfo;
	//use fstat() to get the file status to a buffer, if get -1, return RC_WRITE_FAILED
	struct stat buffer;
	int fstat_return=fstat(*fileDescriptor, &buffer);
	if(fstat_return==-1)                 
	{
		printf("appendEmptyBlock! fstat error! \n");
		return RC_WRITE_FAILED;
	}
	//calculate the total bytes we need append
	long totalbyte;
	if(buffer.st_size%PAGE_SIZE==0)
		totalbyte=PAGE_SIZE;
	else
		totalbyte=PAGE_SIZE*2-buffer.st_size%PAGE_SIZE;         
  //allocate the memory for writing
	SM_PageHandle newpage = (SM_PageHandle)malloc(totalbyte);  
	//use lseek() to set the write position to the end, if get -1, return RC_WRITE_FAILED
	int lseek_return=lseek(*fileDescriptor, 0, SEEK_END);
	if(lseek_return==-1)                 
	{
		printf("appendEmptyBlock! lseek error! \n");
		return RC_WRITE_FAILED;
	}
	//use write() to fill the last page of the file with zero bytes, if get -1 return RC_WRITE_FAILED
	int write_return=write(*fileDescriptor, newpage, totalbyte);
	if(write_return==-1)                
	{
		printf("appendEmptyBlock! write error! \n");
		return RC_WRITE_FAILED;
	}
	//increase the totalNumPages by 1
	fHandle->totalNumPages++;                       
	return RC_OK;
}

//If the file has less than numberOfPages pages then increase the size to numberOfPages.
extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
{       
	int *fileDescriptor;
  fileDescriptor=(int *)fHandle->mgmtInfo;

	if(numberOfPages<=fHandle->totalNumPages)         
	{
		return RC_OK;
	}
	//use fstat() to get the file status to buffer, if get -1, return RC_WRITE_FAILED
	struct stat buffer;
	int fstat_return=fstat(*fileDescriptor, &buffer);
	if(fstat_return==-1)                 
	{
		printf("ensureCapacity! fstat error! \n");
		return RC_WRITE_FAILED;
	}
	//calculate the total bytes needed to be writen
	long totalbyte= numberOfPages*PAGE_SIZE-buffer.st_size;     
  //allocate memory for writing
  SM_PageHandle newpage = (SM_PageHandle)malloc(totalbyte);
	//use lseek() to set the write position to the end, if get -1, return RC_WRITE_FAILED
	int lseek_return=lseek(*fileDescriptor, 0, SEEK_END);
  if(lseek_return==-1)
	{
		printf("ensureCapacity failed! lseek error! \n");
		return RC_WRITE_FAILED;
	}
  //use write() to fill the last page of the file with zero bytes, if get -1 return RC_WRITE_FAILED
  int write_return=write(*fileDescriptor, newpage, totalbyte);
  if(write_return==-1)   
	{
		printf("AppendEmptyBlock failed! write error! \n");
		return RC_WRITE_FAILED;
	}
	//update the totalNumPages
	fHandle->totalNumPages=numberOfPages;
	return RC_OK;
}

