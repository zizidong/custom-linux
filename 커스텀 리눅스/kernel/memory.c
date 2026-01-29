#include "memory.h"
#include <string.h>

static memory_manager_t mem_manager;

// 메모리 초기화
void memory_init(uint32_t start_addr, uint32_t size) {
    mem_manager.heap_start = start_addr;
    mem_manager.heap_end = start_addr + size;
    mem_manager.total_memory = size;
    mem_manager.used_memory = 0;
    
    // 초기 메모리 블록 생성
    mem_manager.free_list = (memory_block_t*)start_addr;
    mem_manager.free_list->start_addr = start_addr + sizeof(memory_block_t);
    mem_manager.free_list->size = size - sizeof(memory_block_t);
    mem_manager.free_list->is_allocated = 0;
    mem_manager.free_list->next = NULL;
}

// 메모리 할당 (First Fit 알고리즘)
void* kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    // 4바이트 정렬
    size = (size + 3) & ~3;
    
    memory_block_t* current = mem_manager.free_list;
    memory_block_t* prev = NULL;
    
    while (current != NULL) {
        if (!current->is_allocated && current->size >= size) {
            // 블록을 분할할 수 있는지 확인
            if (current->size > size + sizeof(memory_block_t)) {
                // 새 블록 생성
                memory_block_t* new_block = (memory_block_t*)(current->start_addr + size);
                new_block->start_addr = current->start_addr + size + sizeof(memory_block_t);
                new_block->size = current->size - size - sizeof(memory_block_t);
                new_block->is_allocated = 0;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->is_allocated = 1;
            mem_manager.used_memory += current->size;
            
            return (void*)current->start_addr;
        }
        
        prev = current;
        current = current->next;
    }
    
    return NULL; // 메모리 부족
}

// 메모리 해제
void kfree(void* ptr) {
    if (ptr == NULL) return;
    
    memory_block_t* current = mem_manager.free_list;
    memory_block_t* prev = NULL;
    
    while (current != NULL) {
        if ((void*)current->start_addr == ptr) {
            current->is_allocated = 0;
            mem_manager.used_memory -= current->size;
            
            // 인접한 빈 블록들과 병합
            if (prev && !prev->is_allocated) {
                prev->size += current->size + sizeof(memory_block_t);
                prev->next = current->next;
                // current 블록은 무효화됨
            }
            
            if (current->next && !current->next->is_allocated) {
                current->size += current->next->size + sizeof(memory_block_t);
                current->next = current->next->next;
            }
            
            return;
        }
        
        prev = current;
        current = current->next;
    }
}

// 정렬된 메모리 할당
void* kmalloc_aligned(size_t size, size_t alignment) {
    if (alignment == 0) return kmalloc(size);
    
    // 정렬된 크기 계산
    size_t aligned_size = size + alignment - 1;
    void* ptr = kmalloc(aligned_size);
    
    if (ptr == NULL) return NULL;
    
    // 정렬된 주소 계산
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    
    return (void*)aligned_addr;
}

// 메모리 통계 출력
void memory_dump_stats(void) {
    // 간단한 통계 출력 (실제 구현에서는 콘솔 출력 함수 필요)
    uint32_t total = get_total_memory();
    uint32_t used = get_used_memory();
    uint32_t free = get_free_memory();
    
    // 여기서는 실제 출력 대신 변수에 저장
    (void)total;
    (void)used;
    (void)free;
}

uint32_t get_total_memory(void) {
    return mem_manager.total_memory;
}

uint32_t get_used_memory(void) {
    return mem_manager.used_memory;
}

uint32_t get_free_memory(void) {
    return mem_manager.total_memory - mem_manager.used_memory;
}

// 페이징 시스템 구현
static page_directory_t* current_page_directory = NULL;

void paging_init(void) {
    // 페이지 디렉토리 생성
    current_page_directory = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t), PAGE_SIZE);
    memset(current_page_directory, 0, sizeof(page_directory_t));
    
    // 페이지 디렉토리 활성화
    switch_page_directory(current_page_directory);
}

void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t page_dir_index = virtual_addr >> 22;
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;
    
    // 페이지 테이블이 없으면 생성
    if (!(current_page_directory->entries[page_dir_index].value & PAGE_PRESENT)) {
        page_table_t* page_table = (page_table_t*)kmalloc_aligned(sizeof(page_table_t), PAGE_SIZE);
        memset(page_table, 0, sizeof(page_table_t));
        
        current_page_directory->entries[page_dir_index].value = 
            (uint32_t)page_table | PAGE_PRESENT | PAGE_WRITE;
    }
    
    // 페이지 테이블 엔트리 설정
    page_table_t* page_table = (page_table_t*)(current_page_directory->entries[page_dir_index].value & ~0xFFF);
    page_table->entries[page_table_index].value = physical_addr | flags;
    
    // TLB 무효화
    __asm__ volatile("invlpg (%0)" : : "r" (virtual_addr) : "memory");
}

void unmap_page(uint32_t virtual_addr) {
    uint32_t page_dir_index = virtual_addr >> 22;
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;
    
    if (current_page_directory->entries[page_dir_index].value & PAGE_PRESENT) {
        page_table_t* page_table = (page_table_t*)(current_page_directory->entries[page_dir_index].value & ~0xFFF);
        page_table->entries[page_table_index].value = 0;
        
        // TLB 무효화
        __asm__ volatile("invlpg (%0)" : : "r" (virtual_addr) : "memory");
    }
}

void switch_page_directory(page_directory_t* dir) {
    current_page_directory = dir;
    __asm__ volatile("mov %0, %%cr3" : : "r" (dir) : "memory");
    
    // 페이징 활성화
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r" (cr0));
    cr0 |= 0x80000000; // PG 비트 설정
    __asm__ volatile("mov %0, %%cr0" : : "r" (cr0) : "memory");
}

void page_fault_handler(void) {
    uint32_t fault_addr;
    __asm__ volatile("mov %%cr2, %0" : "=r" (fault_addr));
    
    // 페이지 폴트 처리 (실제 구현에서는 더 복잡)
    (void)fault_addr;
}
