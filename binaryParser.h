/* ##################################
   Data Structures & Global Variables
   ################################## */

#define Lower12BitMask 0xFFF
#define NumberOfValuesToPrint 32

/*We read 3 bytes into the 2 values array, and keep one empty byte so we have 32 bits*/
/*Assuming the machine is little endian*/
typedef struct{
	char twoValues[3];
	char unused;
}inputWord;

/*We union it with an int so that we can do bit manipulations to get the values we want*/
typedef union{
	inputWord inputStruct; 
	unsigned int word;
}inputWordUnion;

/*This is the node for the statically allocated linked list*/
/*The next ptr is actually just the index of the next node, since they're all in an array*/
typedef struct{
	unsigned short int value;
	int next;
}listNode;

/*List for storing the 32 largest values*/
/*We will maintain it from smallest to largest. (Head pointer is always the smallest value.)*/
listNode maxList[NumberOfValuesToPrint];
/*Keep track of the size of the list so we know if it is full or not*/
int listSize;
/*Keep track of the index of the head of the list.*/
int listHead;

/*Circular buffer for maintaining last 32 values*/
unsigned short int lastValuesBuffer[NumberOfValuesToPrint];
/*Write ptr for the circular buffer*/
int lastValuesWriteIdx;
/*Total # of values that we have read.*/
int totalValueCount;


/* ###################
   Function Prototypes
   ################### */

/*Parse the data from the binary file and place it into the heap & buffer*/
void parseAccData(FILE * inputFile);

/*Convert from big endian in the binary file to little endian*/
void ntoh24Bit(void * word);

/*Read the circular buffer and print out the last values read (should be <= 32 values)*/
void printLastValues(FILE * outputFile);

/*Add the value to the heap if necessary*/
void listInsert(unsigned short int value);

/*Remove the smallest element from the list*/
/*Returns the index of the removed element, so that the location can be used for the new element*/
int listRemove();
/*prints the contest of the linked list in order from smallest to largest*/
void printMaxValues(FILE * outputFile);
/*returns the smallest number*/
unsigned short int listPeek();

