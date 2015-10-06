/*
	 ============================================================================
	 Name        : vmm.h
	 Author      : David Whipple
	 Version     : 1.0
	 Copyright   : Written for CS543 @ Drexel Homework #4
	 Description : This is a Virtual Memory Manager, Ansi-style
	 ============================================================================
*/
#ifndef VMM_H_
#define VMM_H_

/*
 * These are my constants
 *
 * To change the physical memory, modify the FRAME_ENTRIES (to 128)
 */

#define PAGE_SIZE 256
#define PAGE_ENTRIES 256
#define FRAME_SIZE 256
#define FRAME_ENTRIES 256
#define MEMORY_SIZE (FRAME_SIZE*FRAME_ENTRIES)
#define LRU_LIST_LENGTH 10000
#define TLB_ENTRIES 16
#define START_FRAME 0
#define BOOLEAN int
#define TRUE 1
#define FALSE 0
#define READ 0
#define WRITE 1

/* DEBUG LEVEL is defined as follows:
 * The higher the level that is TRUE, the more detailed DEBUGGING
 */
#define DEBUG_LEVEL_1 FALSE
#define DEBUG_LEVEL_2 FALSE
#define DEBUG_LEVEL_3 FALSE

/*
 * This is my TLB
 */
typedef struct tlbEntrys {
	    BOOLEAN inUse[TLB_ENTRIES];
    	unsigned int page[TLB_ENTRIES];
    	unsigned int frame[TLB_ENTRIES];
    	unsigned int numTimesUsed[TLB_ENTRIES];
} tlbType;

/*
 * This represents my physical memory
 */
typedef struct memoryLocations {
	BOOLEAN frameInUse[FRAME_ENTRIES];
	int numTimesAccessed[FRAME_ENTRIES];
	BOOLEAN dirty[FRAME_ENTRIES];
	int lruCounter[FRAME_ENTRIES];
	char physicalMemory[MEMORY_SIZE];

} physicalMemoryType;

/*
 * This is my page table
 */
typedef struct pageTableEntries {
	unsigned int pageTable[PAGE_ENTRIES];
	unsigned int frameTable[PAGE_ENTRIES];
	BOOLEAN validInvalidBit[PAGE_ENTRIES];
} pageTableType;

/*
 * These are my function prototypes, please see primary code for comments
 */
void initialize(pageTableType *pageTable, physicalMemoryType *physicalMemory, tlbType *tlb);
void insert_tlb(tlbType *tlb, unsigned int pageNumber, unsigned int currentFrame);
int lookup_tlb(tlbType *tlb, unsigned int pageNumber);
int lookup_frame(pageTableType *pageTable, unsigned int pageNumber);
int least_used_tlb_entry(tlbType *tlb);
unsigned int find_lru_frame(physicalMemoryType *physicalMemory);
void dump_tlb(tlbType *tlb);
void dump_page_table(pageTableType *pageTable);
void dump_physical_memory(physicalMemoryType *physicalMemory);
void page_fault(pageTableType *pageTable, unsigned int pageNumber, physicalMemoryType *physicalMemory, unsigned int *currentFrame);
void showbits(unsigned int x);
void showbitschar(char x);
unsigned int extract_pagenumber(unsigned int address);
unsigned int extract_offset(unsigned int address);
void load_page_from_backing_store(unsigned int pageNumber, physicalMemoryType *physicalMemory, unsigned int *currentFrame);
void print_page(char *page);

#endif /* VMM_H_ */
