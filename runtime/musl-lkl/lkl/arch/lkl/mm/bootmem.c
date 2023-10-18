#include <linux/bootmem.h>
#include <linux/mm.h>
#include <linux/swap.h>

unsigned long memory_start, memory_end;
static unsigned long _memory_start, mem_size;

void *empty_zero_page;

//#define K_SIZE (200*1024*1024LL)
//static char k_heap[K_SIZE]  __attribute__ ((aligned (4096)));


void __init bootmem_init(unsigned long mem_sz)
{
	unsigned long bootmap_size;

#if 0
//	mem_size = mem_sz;
	mem_size = K_SIZE;

//	_memory_start = (unsigned long)lkl_ops->mem_alloc(mem_size);
//	_memory_start = (unsigned long)lkl_ops->mem_executable_alloc(mem_size);
	_memory_start = &k_heap[0];
	memory_start = _memory_start;
#else
	extern unsigned long cvm_heap_begin;
	extern unsigned long cvm_heap_size;

	mem_size = cvm_heap_size;
	_memory_start = cvm_heap_begin;
	memory_start = _memory_start;
#endif
	BUG_ON(!memory_start);
	memory_end = memory_start + mem_size;

	if (PAGE_ALIGN(memory_start) != memory_start) {
		mem_size -= PAGE_ALIGN(memory_start) - memory_start;
		memory_start = PAGE_ALIGN(memory_start);
		mem_size = (mem_size / PAGE_SIZE) * PAGE_SIZE;
	}
	pr_info("bootmem address range: 0x%lx - 0x%lx\n", memory_start,
		memory_start+mem_size);
	/*
	 * Give all the memory to the bootmap allocator, tell it to put the
	 * boot mem_map at the start of memory.
	 */
	max_low_pfn = virt_to_pfn(memory_end);
	min_low_pfn = virt_to_pfn(memory_start);
	bootmap_size = init_bootmem_node(NODE_DATA(0), min_low_pfn, min_low_pfn,
					 max_low_pfn);

	/*
	 * Free the usable memory, we have to make sure we do not free
	 * the bootmem bitmap so we then reserve it after freeing it :-)
	 */
	free_bootmem(memory_start, mem_size);
	reserve_bootmem(memory_start, bootmap_size, BOOTMEM_DEFAULT);

	empty_zero_page = alloc_bootmem_pages(PAGE_SIZE);
	memset((void *)empty_zero_page, 0, PAGE_SIZE);

	{
		unsigned long zones_size[MAX_NR_ZONES] = {0, };

		zones_size[ZONE_NORMAL] = (mem_size) >> PAGE_SHIFT;
		free_area_init(zones_size);
	}
}

void __init mem_init(void)
{
	max_mapnr = (((unsigned long)high_memory) - PAGE_OFFSET) >> PAGE_SHIFT;
	/* this will put all memory onto the freelists */
	totalram_pages = free_all_bootmem();
	pr_info("Memory available: %luk/%luk RAM\n",
		(nr_free_pages() << PAGE_SHIFT) >> 10, mem_size >> 10);
}

/*
 * In our case __init memory is not part of the page allocator so there is
 * nothing to free.
 */
void free_initmem(void)
{
}

void free_mem(void)
{
	lkl_ops->mem_free((void *)_memory_start);
//	lkl_ops->mem_executable_free((void *) memory_start,  memory_end - memory_start);
}
