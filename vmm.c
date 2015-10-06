/*
	 ============================================================================
	 Name        : vmm.c
	 Author      : David Whipple
	 Version     : 1.0
	 Copyright   : Written for CS543 @ Drexel Homework #4
	 Description : This is a Virtual Memory Manager, Ansi-style
	 ============================================================================
	 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vmm.h"
/*
 * This is the main function that generally executes the following algorithm
 *
 *
 * 		While Virtual Addresses to be read, Read a Virtual Address
 *         Translate Virtual Address to Physical Address (extract page number, and offset)
 *         Look in the TLB (translation look-aside buffer) for a page number match
 *             if TLB-HIT (page number match), look up element at physical memory location
 *             and print out value
 *         else
 *             if TLB-MISS, consult the page-table looking for a match
 *             If PAGE-TABLE-HIT
 *                Update TLB (with correct page number-frame number correlation)
 *                look up element at physical memory location and print out value
 *             IF PAGE-TABLE-MISS
 *                Implement demand-paging by reading from BACKING_STORE.bin (read in 256 bytes
 *                and store in Physical Memory)
 *                Update PAGE-TABLE (with correct page number-frame number correlation)
 *      END
 *
 *      LRU PAGE REPLACEMENT ALGORITHM
 *      -------------------------------
 *      I implemented a LRU (Least Recently Used) algorithm for page table replacement,
 *      so when I adjust the physical memory to less than the virtual memory, I track the
 *      last time the page was accessed by keeping the most recent virtual address to touch
 *      the frame with each frame.
 *
 *      LEAST USED TLB REPLACEMENT ALGORITHM
 *      ------------------------------------
 *      I also implemented a Least Used algorithm for updating the TLB, by tracking the number
 *      of accesses each TLB entry has.  When the TLB is full, I find the TLB entry with the least
 *      number of accesses and replace it next.
 *
 */
int main(int argc, char *argv[] ) {

	FILE *file;
	
	/*
	*   The following variables are used:
	* 
	*   address - The Virtual address read from the file         
	*   currentFrame = The current physical frame
	*   pageNumber - The decoded page number (0 to 255)
	*   offset     - The decoded offset (0 to 255)
	*   myInt      - A temporary integer variable
	*   aFrame     - A temporary physical frame
	*   numPageFaults - The total number of page faults
	*   numAddressLookups - The total number of address lookups
	*   numPageHits - The total number of page hits
	*   numTblHits - The total number of Table hits
	*   numTblMisses - The total number of table misses
	*/
    unsigned int address, currentFrame=START_FRAME, pageNumber, offset, myInt;
    int aFrame, numPageFaults=0, numAddressLookups=0, numPageHits=0, numTlbHits=0, numTlbMisses=0;
    /* This is the page table */
    pageTableType pageTable;
    /* This is what I used to represent Physical Memory */
    physicalMemoryType physicalMemory;
    /* This is my TLB */
    tlbType tlb;
    int mode=0, arguments;
    char accessType;
    BOOLEAN done=FALSE, addressWrite=FALSE;

    if ( argc != 3 ) /* argc should be 2 for correct execution */
    {
        /* We print argv[0] assuming it is the program name */
        printf( "usage: %s [read or write] filename", argv[0] );
        exit(1);
    }
    if (strcmp(argv[1], "read") == 0 ) mode = READ;
    if (strcmp(argv[1], "write") == 0 ) mode = WRITE;

    printf("Running with file %s, in mode", argv[2]);
    if (mode == READ) printf(" read only capable.\n");
    if (mode == WRITE) printf(" write capable.\n");

    initialize(&pageTable, &physicalMemory, &tlb);
    dump_page_table(&pageTable);
    dump_tlb(&tlb);

	file=fopen(argv[2], "r");

	while (!feof(file))
	{
		if (mode == READ) {
		   arguments = fscanf(file, "%d", &address);
		   if (arguments != 1) {
			   done=TRUE;
		   }
		}
		if (mode == WRITE) {
		   arguments = fscanf(file, "%d %c", &address, &accessType);
		   if (arguments != 2) {
		   	   done=TRUE;
		   }
		   if (DEBUG_LEVEL_3) printf("address=%d, accessType=%c.\n", address, accessType);
		   if (accessType == 'W') {
			   addressWrite = TRUE;
		   }
		   else {
			   addressWrite = FALSE;
		   }
		}

       if (!done) {
		   if (DEBUG_LEVEL_2) printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
		   if (DEBUG_LEVEL_2) printf("%d \n", address);
		   /* Translate Logical to Physical Address */
		   numAddressLookups++; /* Sum the total instructions, used for LRU algorithm */
		   pageNumber=extract_pagenumber(address);
		   offset=extract_offset(address);

		   /* Do a TLB Lookup */
		   if (DEBUG_LEVEL_2) printf("Doing lookup in TLB for pageNumber %d.\n", pageNumber);
		   aFrame=lookup_tlb(&tlb, pageNumber);

		   if (DEBUG_LEVEL_2) printf("\nVirtual Address   (decimal=%5u), Physical Address = %d\n", address, ((currentFrame*FRAME_SIZE)+offset));
		   /* showbits(address);*/
		   if (DEBUG_LEVEL_2) printf("Page Number       (decimal=%5u) = ", pageNumber);
		   if (DEBUG_LEVEL_2) showbits(pageNumber);
		   if (DEBUG_LEVEL_2) printf("Offset            (decimal=%5u) = ", offset);
		   if (DEBUG_LEVEL_2) showbits(offset);

		   /* If the TLB misses */
		   if (aFrame == -1) {
			  numTlbMisses++; /* Sum the number of TLB misses */
			  /* Do a Page Table Lookup */
			  aFrame=lookup_frame(&pageTable, pageNumber);

			  if (DEBUG_LEVEL_2) printf("After TLB Miss, aFrame=%d.\n", aFrame);
			  if (aFrame == -1) {
				  insert_tlb(&tlb, pageNumber, currentFrame); /* Insert the correct information into TLB */
			  }
			  else {
				  insert_tlb(&tlb, pageNumber, aFrame); /* Insert the correct information into TLB */
			  }

			  if (DEBUG_LEVEL_2) printf("TLB Lookup failed, pageNumber=%d, currentFrame=%d.\n", pageNumber, currentFrame);

			  /* Do a Page Table Lookup */
			  aFrame=lookup_frame(&pageTable, pageNumber);
			  if (DEBUG_LEVEL_2) printf("aFrame=%d.\n",aFrame);

         	  /* if (pageTable.validInvalidBit[pageNumber] == FALSE) { */
         	  /* This is a Page Fault */
		      if (aFrame == -1) {
		    	 page_fault(&pageTable, pageNumber, &physicalMemory, &currentFrame);
		      	 aFrame=lookup_frame(&pageTable, pageNumber);
		      	 numPageFaults++;
		      	 /* Let's keep track of Address Counter, used for LRU algorithm */
		      	 if (DEBUG_LEVEL_1) printf("\nPAGE-MISS for address %d, page=%d, frame=%d.",address, pageNumber, aFrame);
		      	 if (DEBUG_LEVEL_2) printf("(Page-MISS)-Storing address counter (%d) in lruCounter for frame (%d)\n", numAddressLookups, aFrame);
		      	 physicalMemory.lruCounter[aFrame] = numAddressLookups;
		      	 if (addressWrite == WRITE) physicalMemory.dirty[aFrame]=TRUE;
		      	 if (DEBUG_LEVEL_1) {
		            if (addressWrite == WRITE) printf("\nMarking frame %d dirty, address access at %d is Write.\n", aFrame, address);
		      	 }
		      }
		      else {
		    	 /* This is a page HIT */
		         numPageHits++;
	      	     aFrame=lookup_frame(&pageTable, pageNumber);
	      		 if (DEBUG_LEVEL_2) printf("(Page-HIT)-Storing address counter (%d) in lruCounter for frame (%d)", numAddressLookups, aFrame);
	      		 physicalMemory.lruCounter[aFrame] = numAddressLookups;
	      		 if (DEBUG_LEVEL_1) printf("\nPAGE-HIT for address %d, page=%d, frame=%d.\n",address, pageNumber, aFrame);
	      		 if (addressWrite == WRITE) physicalMemory.dirty[aFrame]=TRUE;
	      		 if (DEBUG_LEVEL_1) {
	      	        if (addressWrite == WRITE) printf("\nMarking frame %d dirty, address access at %d is Write.\n", aFrame, address);
	      		 }
	      	  }
		   }
		   else {
			  /* This is a TBL Hit */
			  if (DEBUG_LEVEL_1) printf("\nTLB-HIT for address %d, page=%d, frame=%d.",address, pageNumber, aFrame);
			  numTlbHits++;
			  if (addressWrite == WRITE) physicalMemory.dirty[aFrame]=TRUE;
			  if (DEBUG_LEVEL_1) {
   		         if (addressWrite == WRITE) printf("\nMarking frame %d dirty, address access at %d is Write.\n", aFrame, address);
			  }
		   }
		   if (DEBUG_LEVEL_2) dump_tlb(&tlb);
		   if (DEBUG_LEVEL_2) dump_page_table(&pageTable);
		   if (DEBUG_LEVEL_2) dump_physical_memory(&physicalMemory);

		   /* We now know the TBL and the Page Table are upto date */
		   myInt = physicalMemory.physicalMemory[(aFrame*FRAME_SIZE)+offset];
		   if (DEBUG_LEVEL_2) {
			   printf("\n****Virtual Address: %5u, Physical Address = %d, ", address, ((aFrame*FRAME_SIZE)+offset));
			   printf("Character is %d.\n",myInt);
		   }
		   else {
			   printf("\nVirtual address: %u Physical address: %d ", address, ((aFrame*FRAME_SIZE)+offset));
			   printf("Value: %d", myInt);
		   }
		   if (DEBUG_LEVEL_2) printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n\n");
       }
	}

	fclose(file);
	printf("\n\nNumber of address lookups=%d.\n", numAddressLookups);
	printf("Number of TLB misses=%d.\n", numTlbMisses);
	printf("Number of TLB hits=%d.\n", numTlbHits);
	printf("Number of page faults=%d.\n", numPageFaults);
	printf("Number of page hits=%d.\n", numPageHits);

	return EXIT_SUCCESS;
}

/*
 * Function Name - lookup_tlb
 * Purpose       - To lookup the TLB entry that matches page number.
 * Parameters    - tlbType *tlb - This is the TLB (translation look-aside buffer)
 *                 pageNumber   - This is the page number to search for in the TLB.
 * Returns       - Returns the TLB entry that matches, or -1 if no match.
 */
int lookup_tlb(tlbType *tlb, unsigned int pageNumber)
{
	int i=0, frameNumber;
	if (DEBUG_LEVEL_2) printf("Searching TLB for pageNumber %d.\n", pageNumber);
	for (i=0;i<TLB_ENTRIES;i++)
	{
		/* printf("i=%d, tlb->inUse[i]=%d, tlb->page[i]=%d, tlb->frame[i]=%d.\n",i, tlb->inUse[i],tlb->page[i], tlb->frame[i]);*/
		if (tlb->inUse[i] == TRUE) {
			if (tlb->page[i] == pageNumber) {
				frameNumber = tlb->frame[i];
				tlb->numTimesUsed[i]++; /* Increment times used */
				if (DEBUG_LEVEL_2) printf("TLB Hit: pageNumber=%d, frameNumber=%d, times used=%d.\n", pageNumber, frameNumber, tlb->numTimesUsed[i]);
				return frameNumber;
			}
		}
	}
	if (DEBUG_LEVEL_2) printf("TLB Miss: pageNumber=%d.\n", pageNumber);
	return -1;
}


/*
 * Function Name - insert_tlb
 * Purpose       - To insert an element into the TLB.
 * Parameters    - tlbType *tlb - This is the TLB (translation look-aside buffer)
 *                 pageNumber   - This is the page number to insert in the TLB.
 *                 currentFrame - This is the frame to insert into the TLB
 * Returns       - Nothing
 */
void insert_tlb(tlbType *tlb, unsigned int pageNumber, unsigned int currentFrame)
{

	int i=0, leastUsed;
	BOOLEAN inserted=FALSE;

    while ((i < TLB_ENTRIES) && (inserted == FALSE))
    {
    	if (tlb->inUse[i] == TRUE) {
    		i++;
    	}
    	else {
    		inserted=TRUE;
    		tlb->inUse[i]=TRUE;
    		tlb->page[i]=pageNumber;
    		tlb->frame[i]=currentFrame;
    		tlb->numTimesUsed[i]=1;
    		if (DEBUG_LEVEL_2) printf("Inserting page number %d into TLB entry %d, frame=%d.\n",pageNumber, i, currentFrame);
    	}
    }
    if (!(inserted))
    {
    	if (DEBUG_LEVEL_2) printf("TLB is full.\n");
    	leastUsed=least_used_tlb_entry(tlb);
    	if (DEBUG_LEVEL_2) printf("Inserting page number %d into TLB entry %d, frame=%d.\n",pageNumber, leastUsed, currentFrame);
    	tlb->inUse[leastUsed]=TRUE;
    	tlb->page[leastUsed]=pageNumber;
    	tlb->frame[leastUsed]=currentFrame;
    	tlb->numTimesUsed[leastUsed]=1;
    }
}


/*
 * Function Name - least_used_tlb_entry
 * Purpose       - To return the least used TLB entry (by number of accesses)
 * Parameters    - tlbType *tlb - This is the TLB (translation look-aside buffer)
 * Returns       - Returns the least used TLB entry (by number of accesses)
 */

int least_used_tlb_entry(tlbType *tlb)
{
	int i;
	unsigned int leastUsed, numUsed;
	leastUsed = 0;
	numUsed = tlb->numTimesUsed[leastUsed];
	for (i=1;i<TLB_ENTRIES;i++) {
		if (tlb->numTimesUsed[i] < numUsed) {
			leastUsed = i;
			numUsed = tlb->numTimesUsed[leastUsed];
		}
	}
	if (DEBUG_LEVEL_2) printf("least used TLB entry is %d, it was used %d times.\n", leastUsed, numUsed);
	return leastUsed;
}


/*
 * Function Name - lookup_frame
 * Purpose       - To lookup for a match in the page table for a specific page
 * Parameters    - pageTable - This is the page table to look in
 *                 pageNumber   - This is the page number to search for in the page table.
 * Returns       - Returns the page table entry that matches, or -1 if no match.
 */
int lookup_frame(pageTableType *pageTable, unsigned int pageNumber)
{
	int i;
	if (DEBUG_LEVEL_2) printf("In lookup_frame, searching for pageNumber=%d.\n", pageNumber);
	for (i=0;i<PAGE_ENTRIES;i++)
	{
		if (pageTable->pageTable[i] == pageNumber) {
			if (pageTable->validInvalidBit[i] == TRUE) {
				if (DEBUG_LEVEL_2) printf("Found page table entry for pageNumber %d, frame is %d.\n", pageNumber, pageTable->frameTable[i]);
				return pageTable->frameTable[i];
			}
			else {
				if (pageNumber > 0) {
				   printf("ERROR: Invalid page entry.\n");
				   exit(1);
				}
			}
		}
	}
	return -1;
}


/*
 * Function Name - page_fault
 * Purpose       - To execute a page fault, which will load from the store into physical memory
 * Parameters    - pageTable - This is the page table
 *                 pageNumber   - This is the page number that caused the page fault
 *                 physicalMemory - This is the physical memory that I load into
 *                 currentFrame - This is the frame to load into page table
 * Returns       - Nothing
 */
void page_fault(pageTableType *pageTable, unsigned int pageNumber,
		physicalMemoryType *physicalMemory, unsigned int *currentFrame)
{
	if (DEBUG_LEVEL_2) printf("Page Fault on Page Table Entry %d, currentFrame=%d.\n", pageNumber, *currentFrame);
	load_page_from_backing_store(pageNumber, physicalMemory, currentFrame);
	pageTable->pageTable[pageNumber]=pageNumber;
	pageTable->validInvalidBit[pageNumber]=TRUE;
	pageTable->frameTable[pageNumber]=*currentFrame;
	(*currentFrame)++;
}

/*
 * Function Name - find_lru_frame
 * Purpose       - To find the Least Recently Used frame in the physical memory
 * Parameters    - physicalMemory - This is the physical memory that I search for Least Recently Used
 * Returns       - Returns the Least Recently Used frame
 */

unsigned int find_lru_frame(physicalMemoryType *physicalMemory)
{
	int i, lruFrame=0;
	for (i=0;i<FRAME_ENTRIES;i++)
	{
		if (physicalMemory->lruCounter[i] < physicalMemory->lruCounter[lruFrame])
		{
			lruFrame = i;
		}
	}
	if (DEBUG_LEVEL_2) printf("Found LRU Page=%d, dirty bit is %d.\n", lruFrame, physicalMemory->dirty[lruFrame]);
	if (physicalMemory->dirty[lruFrame] == TRUE) {
		if (DEBUG_LEVEL_1) printf("Frame is dirty, it has been written too, writing to swap.\n");
		physicalMemory->dirty[lruFrame] == FALSE;
	}
	return lruFrame;
}

/*
 * Function Name - load_page_from_backing_store
 * Purpose       - To load an actual 256 bytes from the backing store
 * Parameters    - pageNumber   - This is the page number that caused the page fault
 *                 physicalMemory - This is the physical memory that I load into
 *                 currentFrame - This is the frame to load into page table
 * Returns       - Nothing
 */

void load_page_from_backing_store(unsigned int pageNumber, physicalMemoryType *physicalMemory,
		                          unsigned int *currentFrame)
{
	FILE *file;
	char buffer[PAGE_SIZE];
	int fileLocater;
	fileLocater=pageNumber*PAGE_SIZE;
	int i, elementsRead, locationOfFrame;
	unsigned int lruFrame;

	if (DEBUG_LEVEL_2) printf("Reading page #%d from BACKING_STORE.bin at location %d, currentFrame=%d.\n", pageNumber, fileLocater, (*currentFrame));

	if (physicalMemory->frameInUse[(FRAME_ENTRIES-1)] == TRUE) {
		lruFrame=find_lru_frame(physicalMemory);
		*currentFrame = lruFrame;
		if (DEBUG_LEVEL_2) printf("Memory Full, pageNumber=%d, currentFrame=%d, lruFrame=%d.\n", pageNumber, (*currentFrame), lruFrame);
	}

	file=fopen("BACKING_STORE.bin", "r");
	fseek(file, fileLocater, SEEK_SET);
	elementsRead=fread(buffer, 1, FRAME_SIZE, file);
	if (DEBUG_LEVEL_2) printf("Elements read = %d.\n",elementsRead);
	/* print_page(buffer); */
	physicalMemory->frameInUse[(*currentFrame)]=TRUE;
	physicalMemory->numTimesAccessed[(*currentFrame)]=1;
	/* Copy the buffer read from BACKING STORE into Physical Memory */
	locationOfFrame=(*currentFrame)*FRAME_SIZE;

	if (DEBUG_LEVEL_2) printf("Start of Frame is %d.\n", locationOfFrame);
	for (i=0;i<PAGE_SIZE;i++) {
		physicalMemory->physicalMemory[(locationOfFrame+i)]=buffer[i];
	}
	fclose(file);

}


/*
 * Function Name - initialize
 * Purpose       - To initialize the data structures at the start of execution
 * Parameters    - pageTable - This is the page table
 *                 physicalMemory - This is the physical memory that I load into
 *                 tlb - This is the TLB
 * Returns       - Nothing
 */

void initialize(pageTableType *pageTable, physicalMemoryType *physicalMemory, tlbType *tlb)
{
	int i;
	if (DEBUG_LEVEL_2) printf("Initializing Page Table, setting all valid-Invalid bit's to invalid.\n");
	for (i=0;i<PAGE_ENTRIES;i++) {
		pageTable->validInvalidBit[i]=FALSE;  /* Set validInvalid bit to Invalid (0) */
		pageTable->pageTable[i]=0;
		pageTable->frameTable[i]=0;
	}
	if (DEBUG_LEVEL_2) printf("Initializing physical memory to NULL.\n");
	for (i=0;i<MEMORY_SIZE;i++) {
		physicalMemory->physicalMemory[i]=0;
	}
	for (i=0;i<FRAME_ENTRIES;i++) {
		physicalMemory->frameInUse[i] = FALSE;
		physicalMemory->dirty[i] = FALSE;
		physicalMemory->numTimesAccessed[i]=0;
	}
	for (i=0;i<TLB_ENTRIES;i++) {
		tlb->inUse[i]=FALSE;
	}
}


/*
 * Function Name - dump_physical_memory
 * Purpose       - For troubleshooting this will print out the contents of the data structure representing
 *                 physical memory
 * Parameters    - physicalMemory - This is the physical memory
 * Returns       - Nothing
 */

void dump_physical_memory(physicalMemoryType *physicalMemory)
{
	int i;
	BOOLEAN empty=TRUE;

	if (DEBUG_LEVEL_2) printf("============PHYSICAL MEMORY============\n");
	for (i=0;i<FRAME_ENTRIES;i++) {
		if (physicalMemory->frameInUse[i] == TRUE) {
			if (DEBUG_LEVEL_2) printf("Physical Memory frame [%d] InUse, lruCounter=%d, dirty=%d\n",i, physicalMemory->lruCounter[i], physicalMemory->dirty[i]);
			/*if (physicalMemory->dirty[i] == TRUE) {
				printf("TRUE.\n");
			}
			else {
				printf("FALSE.\n");
			}*/
			empty=FALSE;
		}
	}
	if (DEBUG_LEVEL_2) if (empty) printf("----THE PHYSICAL MEMORY IS EMPTY----\n");
	if (DEBUG_LEVEL_2) printf("============PHYSICAL MEMORY============\n");
}


/*
 * Function Name - dump_page_table
 * Purpose       - For troubleshooting this will print out the contents of the data structure representing
 *                 the page table
 * Parameters    - page table - This is the page table
 * Returns       - Nothing
 */

void dump_page_table(pageTableType *pageTable)
{
	int i;
	BOOLEAN empty=TRUE;

	if (DEBUG_LEVEL_2) printf("============PAGE TABLE============\n");
	for (i=0;i<PAGE_ENTRIES;i++) {
		if (pageTable->validInvalidBit[i] == TRUE) {
			if (DEBUG_LEVEL_2) printf("Page Table Entry [%d]=%d, frame=%d\n",i, pageTable->pageTable[i], pageTable->frameTable[i]);
			empty=FALSE;
		}
	}
	if (DEBUG_LEVEL_2) {
		if (empty) printf("----THE PAGE TABLE IS EMPTY----\n");
		printf("============PAGE TABLE============\n");
	}

}


/*
 * Function Name - dump_tlb
 * Purpose       - For troubleshooting this will print out the contents of the data structure representing
 *                 the TLB
 * Parameters    - TLB - This is the TLB
 * Returns       - Nothing
 */

void dump_tlb(tlbType *tlb)
{
	int i;
	BOOLEAN empty=TRUE;

	if (DEBUG_LEVEL_2) printf("============TLB============\n");
	for (i=0;i<TLB_ENTRIES;i++) {
		if (tlb->inUse[i] == TRUE) {
			if (DEBUG_LEVEL_2) printf("TLB Entry [%d] InUse, page=%d, frame=%d.\n",i, tlb->page[i],tlb->frame[i]);
			empty=FALSE;
		}
	}
	if (DEBUG_LEVEL_2) {
		if (empty) printf("----THE TLB IS EMPTY----\n");
		printf("============TLB============\n");
	}
}


/*
 * Function Name - extract_page_number
 * Purpose       - This will extract the Physical Page Number from the Virtual Address
 * Parameters    - Address - The virtual Address
 * Returns       - The pagenumber
 */

unsigned int extract_pagenumber(unsigned int address)
{
	unsigned int mask=65280;
	unsigned int integerPage;
	unsigned int page;
	integerPage = address&mask;
	page = integerPage>>8;
	/* printf("Extracting page number (%d) from address %d.\n", page, address); */
	return page;
}


/*
 * Function Name - extract_offset
 * Purpose       - This will extract the Physical offset inside the frame
 * Parameters    - Address - The virtual Address
 * Returns       - The offset
 */

unsigned int extract_offset(unsigned int address)
{
	unsigned int mask=255;
	unsigned int offset;
	offset=address&mask;
	/* offset = address&mask;printf("Extracting offset (%d) from address %d.\n", offset, address);*/
	return offset;
}


/*
 * Function Name - print_page
 * Purpose       - This is a debugging function used to dump the contents of a particular page
 * Parameters    - page - the page to dump
 * Returns       - Nothing
 */

void print_page(char *page)
{
   int i;
   if (DEBUG_LEVEL_3) printf("Buffer [");
   for (i=0;i<PAGE_SIZE;i++) {
	   if (DEBUG_LEVEL_3) printf("%d-",page[i]);
   }
   if (DEBUG_LEVEL_3) printf("]\n");
}

/*
 * Function Name - showbits
 * Purpose       - This is a debugging function used to print the actual bit values of an integer
 * Parameters    - x - any integer
 * Returns       - Nothing
 */
void showbits(unsigned int x)
{
    int i;
    for(i=(sizeof(int)*8)-1; i>=0; i--){
       (x&(1<<i))?putchar('1'):putchar('0');
       if ((i%8) == 0) printf("-");
    }

    	printf("\n");
}

/*
 * Function Name - showbitschar
 * Purpose       - This is a debugging function used to print the actual bit values of an character
 * Parameters    - x - any char
 * Returns       - Nothing
 */
void showbitschar(char x)
{
    int i;
    for(i=(sizeof(char)*8)-1; i>=0; i--){
       (x&(1<<i))?putchar('1'):putchar('0');
       if ((i%8) == 0) printf("-");
    }

    	printf("\n");
}
