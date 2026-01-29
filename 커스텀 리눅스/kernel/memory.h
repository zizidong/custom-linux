#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

// 메모리 블록 구조체
typedef struct memory_block {
    uint32_t start_addr;
    uint32_t size;
    int is_allocated;
    struct memory_block* next;
} memory_block_t;

// 메모리 관리자 구조체
typedef struct memory_manager {
    memory_block_t* free_list;
    uint32_t total_memory;
    uint32_t used_memory;
    uint32_t heap_start;
    uint32_t heap_end;
} memory_manager_t;

// 메모리 관리 함수들
void memory_init(uint32_t start_addr, uint32_t size);
void* kmalloc(size_t size);
void kfree(void* ptr);
void* kmalloc_aligned(size_t size, size_t alignment);
void memory_dump_stats(void);
uint32_t get_total_memory(void);
uint32_t get_used_memory(void);
uint32_t get_free_memory(void);

// 페이지 관리 (페이징 시스템)
#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x1
#define PAGE_WRITE 0x2
#define PAGE_USER 0x4

typedef struct page_table_entry {
    uint32_t value;
} page_table_entry_t;

typedef struct page_table {
    page_table_entry_t entries[1024];
} page_table_t;

typedef struct page_directory {
    page_table_entry_t entries[1024];
} page_directory_t;

// 페이징 함수들
void paging_init(void);
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void unmap_page(uint32_t virtual_addr);
void switch_page_directory(page_directory_t* dir);
void page_fault_handler(void);

#endif // MEMORY_H
