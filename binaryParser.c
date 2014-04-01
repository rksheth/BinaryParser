#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "binaryParser.h"



/***********************************************************************************************************************************
 Written by Rushi Sheth


 This program takes a bin file filled with unsigned 12 bit values.
 It outputs a txt file with the last 32 values and the largest 32 values in the file.
 If the number of values in the file (n) is less than 32 in the bin file, it writes the last n values and the largest n values.
 It handles bad input gracefully.



 Some notes on data structures, algorithms, and optimizations:

 1) Maintaining the n (32 in this case) largest values:

    This is an interesting problem. My initial approach was to use a binary heap.
    Traditionally binary heap operations are done using recursion, but there exist iterative solutions, so
    I felt that it was still suited to use on an embedded platform.
    The heap, I thought, would have the best performance for large data sets, as insert & remove would be O(log n).
    Upon further investigation, however, I realized that while a binary heap *scales* very well, its performance
    suffers for smaller values of n. 

    For example, a remove operation (required every time you insert a new value into a full list), requires,
    in its worst case, ~ 2 * (log n) reads & writes (you have to read and swap your way down until the heap property is satisfied).
    The same is true for the actual insertion. In the case where you insert a value that is the new smallest value in the list,
    you have to insert at the bottom and then swap your way to the top to satisfy the heap property. 
    (Once again ~ 2*(log n) reads & writes). This comes to a rough total of 4* (O(log n)) reads & writes,
    or 40 reads and 40 writes for n=32.

    This convinced me to to switch to a statically allocated linked list, which requires O(n) reads in the worst case,
    but only O(1) writes. They both take up the same amount of space in memory.

    TL:DR - A binary heap sounds nice, but a linked list may offer better performance if you have to store less data.

 2) fread() size
 	I chose to read in 24 bit chunks because fread has a minimum read size of 1 byte and 24 is the LCM of 12 & 8.
 	I could have gone with a larger read size and reduced the # of file reads that take place, but it would have
 	made the code messier, both in terms of the data structure that I read data into and how I handled the
 	remaining bits at the end of the file.
 	While there is some concern that so many reads would be a performance hit, in real life fread is buffered and
 	the performance penalty shouldnt be that high. 
 	As a side note, I had to implement a big-endian to little-endian converter for the 24 bit read size,
 	but it turned out to be pretty small.

 	TL:DR - fread size was chosen for simplicity, not necessarily performance, but it shouldnt be a big deal.

 3) Further Optimizations & Performance thoughts:
    a) I used a statically allocated linked list. Its reasonably fast, and doesnt take up much room.
       Static allocation allows us to keep it neat and tidy and avoids dynamic allocation failures,
       which can be a real possibility on an embedded platform.

    b) I use bitmasks and bitshifts wherever possible to reduce expensive operations like / and %.

    c) Strict C89 does not support 'inline'. However the compiler should inline the smaller functions if 
       it feels it's worth it.

 ***********************************************************************************************************************************/



int main(const int argc, const char* argv[]){

	FILE * inputFile;
	FILE * outputFile;
	int closeInputVal, closeOutputVal, i;

	/*Check for correct cmd line args*/
	if(argc != 3){
		printf("Incorrect usage. Please provide 2 arguments - the input file, then the output file.");
	}

	/*open files*/
	/*Note: If you run it twice with the same output filename, that file will be overwritten*/
	inputFile = fopen(argv[1], "r");
	outputFile = fopen(argv[2], "w+");

	if(!inputFile || !outputFile){
		printf("Error opening one of the files! \n");
		return -1;
	}

	/*Initialize global data structures*/
	memset(&maxList[0], 0x00, NumberOfValuesToPrint * sizeof(listNode));
	memset(&lastValuesBuffer[0], 0x00, NumberOfValuesToPrint * sizeof(unsigned short int));
	lastValuesWriteIdx = 0;
	totalValueCount = 0;
	listSize = 0;
	listHead = 0;

	/*We need an invalid value for each of the next pointers, as 0 is a valid value*/
	for(i = 0; i < NumberOfValuesToPrint; i++){
		maxList[i].next = -1;
	}


	/**** Main Processing ****/
	/*Call reading helper function*/
	parseAccData(inputFile);

	/*Call Print Helper function*/
	printMaxValues(outputFile);
	printLastValues(outputFile);

	/**** Clean Up ****/
	/*Close the files and check to make sure they close correctly*/
	closeInputVal = fclose(inputFile);
	closeOutputVal = fclose(outputFile);

	if(closeInputVal != 0 || closeOutputVal != 0){
		printf("File close failed!");
		return -1;
	}

	/*Good work team.*/
	return 0;

}


/*This function parses the binary file and populates the global data structures*/
void parseAccData(FILE * inputFile){

	inputWordUnion inputValues;
	int size;
	unsigned short int twelveBitValue; 

	/*Some housekeeping*/
	memset(&inputValues, 0x00, sizeof(inputWordUnion));

	/*Read the values from the file*/
	/*Read 3 1-byte chunks into the character array - 24 bits*/
	size = fread(&inputValues.inputStruct.twoValues[0], 1, 3, inputFile);

	while(size == 3){

		/*Convert from big to little endian*/
		ntoh24Bit(&inputValues.word);

		/*get the first value in the 24 bit chunk*/
		twelveBitValue = (inputValues.word >> 12) & Lower12BitMask;
		/*Store the value in the circular buffer*/
		lastValuesBuffer[lastValuesWriteIdx] = twelveBitValue;
		/*Incriment the write index and wrap around if necessary*/
		lastValuesWriteIdx = (lastValuesWriteIdx + 1) & 0x1F;
		/*populate list with the value*/
		listInsert(twelveBitValue);

		/*get the second value*/
		twelveBitValue = (inputValues.word) & Lower12BitMask;
		/*Store the value in the circular buffer*/
		lastValuesBuffer[lastValuesWriteIdx] = twelveBitValue;
		/*Incriment the write index and wrap around if necessary*/
		lastValuesWriteIdx = (lastValuesWriteIdx + 1) & 0x1F;
		/*populate list with the value*/
		listInsert(twelveBitValue);
		
		/*This ensures that if we copy less than 3 bytes in we dont have a stale byte hanging around*/
		memset(&inputValues, 0x00, sizeof(inputWordUnion));
		size = fread(&inputValues.inputStruct.twoValues[0], 1, 3, inputFile);

		totalValueCount +=2;

	}

	/*We need to take care of the case where we have an odd # of values in the file*/
	if(size == 2){

		/*Big to little endian for the 16 bit chunk - we can use the library function for this one*/
		inputValues.word = ntohs((unsigned short int)inputValues.word);
		/*Ignore the nibble*/
		twelveBitValue = (inputValues.word >> 4) & Lower12BitMask;
		/*Store the value in the circular buffer*/
		lastValuesBuffer[lastValuesWriteIdx] = twelveBitValue;
		/*Incriment the write index and wrap around if necessary*/
		lastValuesWriteIdx = (lastValuesWriteIdx + 1) & 0x1F;
		/*populate list with the value*/
		listInsert(twelveBitValue);
		
		totalValueCount++;
	}

	/* 
	 * Size = 1 indicates that for whatever reason, there are 8 bits at the end of the file.
	 * These clearly can't contain another 12 bit value, so we ignore them.
	 */

}


/*This function inserts the value passed to it only if it is necessary*/
void listInsert(unsigned short int value){

	/*We keep track of 2 indexes here.
	  1) The index of the struct we will clear and use for our new value
	  2) A temp index used while we traverse the list
	  3) A previous index that points to the node before nodeIdx (most of the time)
	  using a prevIdx pointer makes the code easier to read
	 */
	unsigned short int smallestValue;
	int index, nodeIdx, prevIdx;

	smallestValue = listPeek();

	/*First check to see if we need to add the value to the heap*/
	/*WE will add the value under either of two conditions
	  1) The value is greater than the minimum value
	  2) The list is not yet full
	  otherwise, return
	 */
	if(value <= smallestValue && listSize == NumberOfValuesToPrint){
		return;
	}

	/*If the list is already full we need to remove the min;*/
	if(listSize == NumberOfValuesToPrint){
		index = listRemove();
	}
	else{
		/*When we first populate the list we use the array indicies in order*/
		index = listSize;
	}

	/*We now know where to store the value*/
	maxList[index].value = value;

	/*now we figure out where to put it in the list*/
	nodeIdx = listHead;
	prevIdx = nodeIdx;
	while(nodeIdx != -1){
		/*Check if the value in the next largest node is greater than or equal to ours*/
		/*If so, we place the node there*/
		if(maxList[nodeIdx].value >= value){
			break;
		}
		/*Update our trailing pointer and move along*/
		prevIdx = nodeIdx;
		nodeIdx = maxList[nodeIdx].next;
	}


	/*We found the correct location in the list!*/
	/*Need to handle the insert at head case separately*/
	if(nodeIdx == listHead && listSize != 0){
		maxList[index].next = listHead;
		listHead = index;
	}
	else{
		maxList[index].next = maxList[prevIdx].next;
		/*For the first one, the two indicies are the same. We dont want a node that points to itself*/
		if(listSize!=0){
			maxList[prevIdx].next = index;
		}
	}

	listSize++;

	/*Whew!*/
	return; 

}


/*Read the circular buffer and print out the last values read (should be <= 32 values)*/
void printLastValues(FILE * outputFile){

	int i, j,readIdx;
	

	/* j is how many values we print. If we read more than 32, we print 32. 
	 * Otherwise, we print the number of values that we read.
	 * 
	 * readIdx is the location from where we start reading. If we read less than 32,
	 * then the circular buffer has not yet looped and we read from index 0
	 * Otherwise, we read from where the write index is
	 */
	if(totalValueCount >= NumberOfValuesToPrint)
	{
		readIdx = lastValuesWriteIdx;
		j = NumberOfValuesToPrint;
	}
	else
	{
		readIdx = 0;
		j = totalValueCount;
	}

	fprintf(outputFile, "--Last 32 Values--\n");

	for(i = 0; i<j; i++)
	{
		fprintf(outputFile, "%hu\n", lastValuesBuffer[readIdx]);
		readIdx = (readIdx + 1) & 0x1F;
	}

}


/*Iterate through the linked list and print out the values*/
void printMaxValues(FILE * outputFile){

	int listIndex, i;

	fprintf(outputFile, "--Sorted Max 32 Values--\n");

	listIndex = listHead;
	for(i = 0; i < listSize; i++){
		fprintf(outputFile, "%hu\n", maxList[listIndex].value);
		listIndex = maxList[listIndex].next;
	}

}


/*This function removes the value at the head of the list.*/
int listRemove(){

	int index;
	/*Store it so we can return it*/
	index = listHead;
	/*Update the new list head, which is the second smallest value in the list*/
	listHead = maxList[listHead].next;
	/*Clean up the struct at the old index*/
	maxList[index].next = -1;
	listSize--;

	return index;

}


/*Simple helper function to keep the code clean*/
unsigned short int listPeek(){
	return maxList[listHead].value;
}


/*Didnt find support in the standard library for converting 24 bit network order to byte order*/
/*I just implemented it myself.*/
void ntoh24Bit(void * word){

	char tempChar;
	char * wordPtr;
	
	wordPtr = (char *) word;
	
	tempChar = *(wordPtr+2);
	*(wordPtr+2) = *wordPtr;
	*wordPtr = tempChar;
}
