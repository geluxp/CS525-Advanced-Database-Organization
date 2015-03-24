Assignment 1 of CS 525 Advanced Database Organization

////Storage Manager/////////

Group Member:
        Kun Mei 		A20291908
	Xiaopei Zhang		A20302816
	Fan Zhang		A20280966
	Jun Qian		A20304025

//////////how to use those source code//////////////////
1. put all the files in the same repository and change the directory to this repository
2. type  make
3. type ./run_test



//////////////Main Design Idea and Architecture////

we wrote a .c file called storage_mgr.c to implement all the interface required in storage_mgr.h.
we also wrote a makefile called Makefile for the tessting purpose of this project.

*******header files we are using in this project*****
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <error.h>
#include <string.h>
#include <limits.h>


*******we are using the following system function in this project****************
file function:
fopen()		fwirte()	fclose()
open()		fstat()	

page function:
lseek() 	read()		write()


********main desgin of the interfacec***************




we are using fileDescriptor to get the mgmtInfo in the all following blocks
***read block*********

1.we first judge if the fHandler exists
2.we use a seires of condition sentences to judege if the contents in the fHanle(totalNumPages, curPagePos,etc) is in the correct range of values
3.call system function lseek() to move the disk head to the desired postion of the file
4.call system function read() to get the page from the disk to the memory
5.update the curPagePos if needed.
6.If we meet an eror in those process, we would get a error info,specifically, we would get detail info( errno function)  if the read() fails. Otherwise, the process would return RC_OK


***write block******  
1.we first judge if the fHandler exists
3.call system function lseek() to move the disk head to the desired postion of the file
4.call system function write() to put the page the meomory to the disk
5.update the curPagePos and totalNumPage if needed.
6.If we meet an eror in those process, we would get a error info,specifically, we would get detail info( errno function)  if the read() fails. Otherwise, the process would return RC_OK


***misc block******
appendEmptyBlock()
1.Judge if the filehandler is initialized
2.use fstat() to get the file status to a buffer, if get -1, return RC_WRITE_FAILED
3.calculate the total bytes we need append
4.allocate the memory for writing
5.use lseek() to set the write position to the end, if get -1, return RC_WRITE_FAILED
6.use write() to fill the last page of the file with zero bytes, if get -1 return RC_WRITE_FAILED
7.increase the totalNumPages by 1
8.Return error information whenever a error occurs, otherwise return RC_OK.



ensureCapacity()
1.Judge if the filehandler is initialized
2.Judge if the capacity is enough, if so return RC_OK,else print error message;
3.use fstat() to get the file status to buffer, if get -1, return RC_WRITE_FAILED
4.Allocate a block of memory with desired capacity and fill it with zero bytes;
5.use lseek() to set the write position to the end, if get -1, return RC_WRITE_FAILED
6.use write() to fill the last page of the file with zero bytes, if get -1 return RC_WRITE_FAILED
7.Update totalNumPage;
8.Return error information whenever a error occurs, otherwise return RC_OK.




**************Extensions****************
Robust: in our implementation, we code handle different types of erorr and print the correspondingng error message to the user,including: non exsiting file,wrong file handle, wrong read/write position






***********Future Improvement**********
1. we may try to implement a new pointer array to storte multiple the fileDescriptor to modify this implementation to open/read/write multiple files simultaneously.
2. we could create the GUI for the manager






