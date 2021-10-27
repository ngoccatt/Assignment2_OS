
#include "mem.h"
#include "stdlib.h"
#include "string.h"
#include <pthread.h>
#include <stdio.h>

static BYTE _ram[RAM_SIZE];

#define NO_PROC_HERE	0

static struct {
	uint32_t proc;	// ID of process currently uses this page
	int index;	// Index of the page in the list of pages allocated
			// to the process.
	int next;	// The next page in the list. -1 if it is the last
			// page.
} _mem_stat [NUM_PAGES];

static pthread_mutex_t mem_lock;

void init_mem(void) {
	memset(_mem_stat, 0, sizeof(*_mem_stat) * NUM_PAGES);
	memset(_ram, 0, sizeof(BYTE) * RAM_SIZE);
	pthread_mutex_init(&mem_lock, NULL);
}

/* get offset of the virtual address */
static addr_t get_offset(addr_t addr) {
	return addr & ~((~0U) << OFFSET_LEN);
}

/* get the first layer index */
static addr_t get_first_lv(addr_t addr) {
	return addr >> (OFFSET_LEN + PAGE_LEN);
}

/* get the second layer index */
static addr_t get_second_lv(addr_t addr) {
	return (addr >> OFFSET_LEN) - (get_first_lv(addr) << PAGE_LEN);
}

/* Search for page table table from the a segment table */
static struct page_table_t * get_page_table(
		addr_t index, 	// Segment level index
		struct seg_table_t * seg_table) { // first level table
	
	/*
	 * TODO: Given the Segment index [index], you must go through each
	 * row of the segment table [seg_table] and check if the v_index
	 * field of the row is equal to the index
	 *
	 * */

	int i;
	for (i = 0; i < seg_table->size; i++) {
		// Enter your code here
		if (seg_table->table[i].v_index == index) {
			return seg_table->table[i].pages;
		}
	}
	return NULL;

}

/* Translate virtual address to physical address. If [virtual_addr] is valid,
 * return 1 and write its physical counterpart to [physical_addr].
 * Otherwise, return 0 */
static int translate(
		addr_t virtual_addr, 	// Given virtual address
		addr_t * physical_addr, // Physical address to be returned
		struct pcb_t * proc) {  // Process uses given virtual address

	/* Offset of the virtual address */
	addr_t offset = get_offset(virtual_addr);
	/* The first layer index */
	addr_t first_lv = get_first_lv(virtual_addr);
	/* The second layer index */
	addr_t second_lv = get_second_lv(virtual_addr);
	
	/* Search in the first level */
	struct page_table_t * page_table = NULL;
	page_table = get_page_table(first_lv, proc->seg_table);
	if (page_table == NULL) {
		return 0;
	}

	int i;
	for (i = 0; i < page_table->size; i++) {
		if (page_table->table[i].v_index == second_lv) {
			/* TODO: Concatenate the offset of the virtual addess
			 * to [p_index] field of page_table->table[i] to 
			 * produce the correct physical address and save it to
			 * [*physical_addr]  */
			//p_index chi moi la index. * no voi PAGE_SIZE de co the cong them offset.
			*physical_addr = page_table->table[i].p_index*PAGE_SIZE + offset;
			return 1;
		}
	}
	return 0;	
}

addr_t alloc_mem(uint32_t size, struct pcb_t * proc) {
	pthread_mutex_lock(&mem_lock);
	addr_t ret_mem = 0;
	/* TODO: Allocate [size] byte in the memory for the
	 * process [proc] and save the address of the first
	 * byte in the allocated memory region to [ret_mem].
	 * */

	uint32_t num_pages = (size % PAGE_SIZE) ? size / PAGE_SIZE + 1 :
		size / PAGE_SIZE; // Number of pages we will use (haha nice joke)
	int mem_avail = 0; // We could allocate new memory region or not?

	/* First we must check if the amount of free memory in
	 * virtual address space and physical address space is
	 * large enough to represent the amount of required 
	 * memory. If so, set 1 to [mem_avail].
	 * Hint: check [proc] bit in each page of _mem_stat
	 * to know whether this page has been used by a process.
	 * For virtual memory space, check bp (break pointer).
	 * */


	//check if physical memory have enough space for us
	int free_physical_memory_page = 0;
	for (int i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc == NO_PROC_HERE) free_physical_memory_page++;
		if (free_physical_memory_page == num_pages) {
			//we have enough physical memory. then how about virtual memory?
			if (proc->bp + num_pages * PAGE_SIZE <= RAM_SIZE) {
				mem_avail = 1;
				break;
			}
		}
	}
	
	if (mem_avail) {
		/* We could allocate new memory region to the process */
		ret_mem = proc->bp;
		proc->bp += num_pages * PAGE_SIZE;
		/* Update status of physical pages which will be allocated
		 * to [proc] in _mem_stat. Tasks to do:
		 * 	- Update [proc], [index], and [next] field
		 * 	- Add entries to segment table page tables of [proc]
		 * 	  to ensure accesses to allocated memory slot is
		 * 	  valid. */
		//FIRST: FIND A PHYSICAL FRAME (PAGE) THAT WE CAN USE: LOOP THROUGH _mem_stat
		int num_of_frame = 0;
		int previous_page = 0;
		for(int i = 0; i < NUM_PAGES; i++) {
			//FIND AN EMPTY PAGE: _mem_stat->proc = 0;
			if (_mem_stat[i].proc == 0) {
				//found an emtpy page
				_mem_stat[i].proc = proc->pid;
				//[index]:
				_mem_stat[i].index = num_of_frame;
				//[next]: if we have found enough frame, then next attribute is -1;
				//[next] is not an address! it's an index of a page within 2^10 page in memory.
				//We only interested in the whole page, not a single address.
				//We'll start assign [next] attribute of the previous allocated page if num_of_frame >= 1
				//Well, in this case, [i] also act as page index.
				
				if (num_of_frame >= 1) {
					_mem_stat[previous_page].next = i;
				}
				previous_page = i;
				// We have index of the physical page, now we need to create a virtual address.
				// Virtual address always start from 0. We'll make several "virtual page" by calculating 
				// the first address of that page. Since virtual page size is equal to physcial page (1KB = 2^10)
				// We only interested from the 11th bit of the virtual address onward - the last 10 bits are use for offset, 
				// which, we don't care, we only need the first address of the page
				// We'll start making virtual address from ret_mem. Since we've increase [proc->bp] value by num_page * PAGE_SIZE
				// we can allocate "virtual page" that fill up to the new [proc->bp] starting from ret_mem.
				// Yes, we're making new virtual address by "stacking" 0, 1, 2... page on ret_mem.
				int first_virtual_address_of_page = ret_mem + (num_of_frame * PAGE_SIZE);
				// After making virtual address, we'll create seg_table and page_table to keep and "decode" that virtual address to 
				// physical address. If we've created seg_table and page_table, and there still some space, we just need to add
				// more entry into it.

				//-------------------------------------------------------------------------------------------------------------
				// FIND IF SEG_TABLE AND PAGE_TABLE IS CREATED AND STILL HAVE SOME SPACE:
				//-------------------------------------------------------------------------------------------------------------

				int found = 0;
				int seg_v_index = get_first_lv(first_virtual_address_of_page);
				int page_v_index = get_second_lv(first_virtual_address_of_page);
				// We will use [seg_table] to change content "inside" the pointer, so it also affect the real pointer.
				struct seg_table_t* seg_table = proc->seg_table;
				// In case the [seg_table] haven't created, the [size] is unknown. We can check if the [seg_table] is created or not
				// by sample seg_table->table[0]->pages, if this pages is NULL, then [seg_table] is empty. I mean, if the [seg_table]
				// is in use, then at table[0], there must be a pages there. Checking this help us initialize [size]
				if (seg_table->table[0].pages == NULL) {
					seg_table->size = 0;
				}
				for(int j = 0; j < seg_table->size; j++) {
					if (seg_table->table[j].v_index == seg_v_index) {
						struct page_table_t * current_page = seg_table->table[j].pages;
						// We have seg_table, at table[j], and there's a page there. So we'll continue add entry inside that pages.
						// Where did v_index come from? well, since the seg_table contain multiple pages,
						// so virtual address of multiple pages can have the same v_index. 
						// virtual address is CONTINUOUS, so it will fill up page after page, table after table.
						// reminder: each table of seg_table have a pages. And, in a page, have a table which each row contain 
						// [v_index] and [p_index]
						int page_size = current_page->size;
						current_page->table[page_size].v_index = page_v_index;
						current_page->table[page_size].p_index = i;
						current_page->size++;
						found = 1;
						break;
					}
				}
				//-------------------------------------------------------------------------------------------------------------
				// SEG_TABLE IS THERE, BUT THE TABLE INSIDE IT HAVE EMPTY PAGES
				//-------------------------------------------------------------------------------------------------------------
				// If this event occur, this mean that data has fill up all the previous table. now we have add a new table (row),
				// allocate a new page for our next table and initialize v_index of the new table
				if (!found) {
					int seg_size = seg_table->size;
					seg_table->table[seg_size].v_index = seg_v_index;
					seg_table->table[seg_size].pages = (struct page_table_t *)malloc(sizeof(struct page_table_t));

					seg_table->table[seg_size].pages->table[0].v_index = page_v_index;
					seg_table->table[seg_size].pages->table[0].p_index = i;
					seg_table->table[seg_size].pages->size = 1;

					seg_table->size++;
				}
				// increase number of frame allocated. If we've allocate enough space, exit the loop using break and
				// set [next] of this final _mem_stat = -1.
				num_of_frame++;
				if (num_of_frame == num_pages) {
					_mem_stat[i].next = -1;
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&mem_lock);
	return ret_mem;
}

int free_mem(addr_t address, struct pcb_t * proc) {
	/*TODO: Release memory region allocated by [proc]. The first byte of
	 * this region is indicated by [address]. Task to do:
	 * 	- Set flag [proc] of physical page use by the memory block
	 * 	  back to zero to indicate that it is free.
	 * 	- Remove unused entries in segment table and page tables of
	 * 	  the process [proc].
	 * 	- Remember to use lock to protect the memory from other
	 * 	  processes.  */
	// address used here is virtual address
	// first, let check if the address is valid or not:
	pthread_mutex_lock(&mem_lock);

	
	addr_t physical_address = 0;
	if (!translate(address, &physical_address, proc)) return -1;		//address is invalid

	int seg_v_index;
	int page_v_index;

	addr_t virtual_address = address;

	int seg_table_index = 0;
	//find seg_table table[i] that the address is currently in.
	struct seg_table_t * seg_table = proc->seg_table;
	uint8_t delete_complete = 0;
	int num_of_free_pages = 0;
	while(!delete_complete) {
		//Find seg_table
		seg_v_index = get_first_lv(virtual_address);
		page_v_index = get_second_lv(virtual_address);

		int end_index = 0;

		struct page_table_t * current_page = get_page_table(seg_v_index, seg_table);
		int num_of_free_space_current_page = 0;
		for(int i = 0; i < current_page->size; i++) {
			if (current_page->table[i].v_index == (page_v_index)) {
				//found the table that contain v_index and p_index.
				//increase virtual_address.
				virtual_address += PAGE_SIZE;
				page_v_index = get_second_lv(virtual_address);
				//clear physic memory:
				int p_index = current_page->table[i].p_index;
				_mem_stat[p_index].proc = 0;

				num_of_free_space_current_page++;
				if (_mem_stat[p_index].next == -1) {
					//condition to end while loop
					end_index = i;
					delete_complete = 1;
					break;
				}
			}
		}
		//in case virtual address span across multiple seg_table (pages)
		if (end_index == 0) end_index = current_page->size - 1;

		//Rearrange page
		for(int j = end_index + 1; j < current_page->size; j++) {
			current_page->table[j - num_of_free_space_current_page] = current_page->table[j];
		}
		current_page->size -= num_of_free_space_current_page;
		if (current_page->size == 0) {
			free(current_page);
			//rearrange the seg_table
			//find index of table to rearrange
			for(int i = 0; i < seg_table->size; i++) {
				if (seg_table->table[i].v_index == seg_v_index) {
					seg_table_index = i;
					break;
				}
			}
			//rearrange seg_table
			for(int j = seg_table_index; j < (seg_table->size - 1); j++) {
				seg_table->table[j] = seg_table->table[j+1];
			}
			seg_table->size--;
		}
		

		//after scanning the whole page at table[seg_table_index], if somehow delete_not_complete is still 1
		//then our data has span to the next table.

		num_of_free_pages += num_of_free_space_current_page;
	}


	pthread_mutex_unlock(&mem_lock);
	return 0;
}

int read_mem(addr_t address, struct pcb_t * proc, BYTE * data) {
	//pthread_mutex_lock(&mem_lock);
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		*data = _ram[physical_addr];
		return 0;
	}else{
		return 1;
	}
	//pthread_mutex_unlock(&mem_lock);
}


//don't need to use mutex_lock in write_mem, even though it access the same ram, but
//each process have its own seperate space, distinct from each other.
int write_mem(addr_t address, struct pcb_t * proc, BYTE data) {
	addr_t physical_addr;
	if (translate(address, &physical_addr, proc)) {
		_ram[physical_addr] = data;
		return 0;
	}else{
		return 1;
	}
}

void dump(void) {
	int i;
	for (i = 0; i < NUM_PAGES; i++) {
		if (_mem_stat[i].proc != 0) {
			printf("%03d: ", i);
			printf("%05x-%05x - PID: %02d (idx %03d, nxt: %03d)\n",
				i << OFFSET_LEN,
				((i + 1) << OFFSET_LEN) - 1,
				_mem_stat[i].proc,
				_mem_stat[i].index,
				_mem_stat[i].next
			);
			int j;
			for (	j = i << OFFSET_LEN;
				j < ((i+1) << OFFSET_LEN) - 1;
				j++) {
				
				if (_ram[j] != 0) {
					printf("\t%05x: %02x\n", j, _ram[j]);
				}
					
			}
		}
	}
}


