#include "scheduler.h"
#include "memory.h"
#include "interrupt.h"
#include <string.h>

static scheduler_t scheduler;
static uint32_t timer_ticks = 0;
static uint32_t timer_frequency = 100; // 100Hz

// 스케줄러 초기화
void scheduler_init(void) {
    memset(&scheduler, 0, sizeof(scheduler_t));
    scheduler.current_process = NULL;
    scheduler.ready_queue = NULL;
    scheduler.blocked_queue = NULL;
    scheduler.sleeping_queue = NULL;
    scheduler.next_pid = 1;
    scheduler.total_processes = 0;
    scheduler.time_quantum = 10; // 10ms
    
    // 타이머 초기화
    timer_init(timer_frequency);
}

// 프로세스 생성
process_t* process_create(const char* name, void (*entry_point)(void), priority_t priority) {
    process_t* process = (process_t*)kmalloc(sizeof(process_t));
    if (!process) return NULL;
    
    // 프로세스 초기화
    process->pid = scheduler.next_pid++;
    strncpy(process->name, name, 31);
    process->name[31] = '\0';
    process->state = PROCESS_READY;
    process->priority = priority;
    process->time_slice = scheduler.time_quantum;
    process->total_time = 0;
    process->next = NULL;
    process->prev = NULL;
    
    // 스택 할당 (4KB)
    process->stack_bottom = (uint32_t)kmalloc(4096);
    process->stack_top = process->stack_bottom + 4096;
    process->esp = process->stack_top - 16; // 스택 정렬
    process->ebp = process->esp;
    
    // 스택에 초기 컨텍스트 설정
    uint32_t* stack = (uint32_t*)process->stack_top;
    stack[-1] = (uint32_t)entry_point; // EIP
    stack[-2] = 0x202;                 // EFLAGS (IF=1, IOPL=0)
    stack[-3] = 0x10;                  // CS (커널 코드 세그먼트)
    stack[-4] = 0;                     // EAX
    stack[-5] = 0;                     // ECX
    stack[-6] = 0;                     // EDX
    stack[-7] = 0;                     // EBX
    stack[-8] = 0;                     // ESP (원래 스택)
    stack[-9] = 0;                     // EBP
    stack[-10] = 0;                    // ESI
    stack[-11] = 0;                    // EDI
    
    process->esp = (uint32_t)&stack[-11];
    
    // 페이지 디렉토리 생성 (간단한 구현)
    process->cr3 = (uint32_t)kmalloc_aligned(4096, 4096);
    
    scheduler_add_process(process);
    scheduler.total_processes++;
    
    return process;
}

// 프로세스 제거
void process_destroy(process_t* process) {
    if (!process) return;
    
    scheduler_remove_process(process);
    
    // 메모리 해제
    if (process->stack_bottom) {
        kfree((void*)process->stack_bottom);
    }
    if (process->cr3) {
        kfree((void*)process->cr3);
    }
    
    kfree(process);
    scheduler.total_processes--;
}

// 스케줄러에 프로세스 추가
void scheduler_add_process(process_t* process) {
    if (!process) return;
    
    // 준비 큐에 추가 (우선순위 기반)
    if (!scheduler.ready_queue) {
        scheduler.ready_queue = process;
        process->next = process;
        process->prev = process;
    } else {
        // 우선순위에 따라 삽입
        process_t* current = scheduler.ready_queue;
        do {
            if (process->priority > current->priority) {
                // 현재 위치에 삽입
                process->next = current;
                process->prev = current->prev;
                current->prev->next = process;
                current->prev = process;
                
                if (current == scheduler.ready_queue) {
                    scheduler.ready_queue = process;
                }
                return;
            }
            current = current->next;
        } while (current != scheduler.ready_queue);
        
        // 마지막에 추가
        process->next = scheduler.ready_queue;
        process->prev = scheduler.ready_queue->prev;
        scheduler.ready_queue->prev->next = process;
        scheduler.ready_queue->prev = process;
    }
}

// 스케줄러에서 프로세스 제거
void scheduler_remove_process(process_t* process) {
    if (!process) return;
    
    // 큐에서 제거
    if (process->next == process) {
        // 마지막 프로세스
        if (scheduler.ready_queue == process) {
            scheduler.ready_queue = NULL;
        } else if (scheduler.blocked_queue == process) {
            scheduler.blocked_queue = NULL;
        } else if (scheduler.sleeping_queue == process) {
            scheduler.sleeping_queue = NULL;
        }
    } else {
        process->prev->next = process->next;
        process->next->prev = process->prev;
        
        if (scheduler.ready_queue == process) {
            scheduler.ready_queue = process->next;
        } else if (scheduler.blocked_queue == process) {
            scheduler.blocked_queue = process->next;
        } else if (scheduler.sleeping_queue == process) {
            scheduler.sleeping_queue = process->next;
        }
    }
    
    process->next = NULL;
    process->prev = NULL;
}

// 라운드 로빈 스케줄링
void scheduler_round_robin(void) {
    if (!scheduler.ready_queue) return;
    
    process_t* next = scheduler.ready_queue->next;
    scheduler.ready_queue = next;
    
    if (scheduler.current_process) {
        scheduler.current_process->state = PROCESS_READY;
    }
    
    next->state = PROCESS_RUNNING;
    scheduler.current_process = next;
    
    // 컨텍스트 스위칭
    if (scheduler.current_process != next) {
        context_switch(scheduler.current_process, next);
    }
}

// 우선순위 스케줄링
void scheduler_priority(void) {
    if (!scheduler.ready_queue) return;
    
    // 가장 높은 우선순위 프로세스 선택
    process_t* highest = scheduler.ready_queue;
    process_t* current = scheduler.ready_queue->next;
    
    while (current != scheduler.ready_queue) {
        if (current->priority > highest->priority) {
            highest = current;
        }
        current = current->next;
    }
    
    if (scheduler.current_process != highest) {
        if (scheduler.current_process) {
            scheduler.current_process->state = PROCESS_READY;
        }
        
        highest->state = PROCESS_RUNNING;
        scheduler.current_process = highest;
        
        context_switch(scheduler.current_process, highest);
    }
}

// 멀티레벨 피드백 큐 스케줄링
void scheduler_multilevel_feedback(void) {
    // 간단한 구현: 우선순위 기반 + 시간 슬라이스 조정
    if (!scheduler.ready_queue) return;
    
    process_t* current = scheduler.ready_queue;
    
    // 시간 슬라이스가 남아있으면 계속 실행
    if (current->time_slice > 0) {
        current->time_slice--;
        return;
    }
    
    // 시간 슬라이스 소진: 우선순위 낮추고 다음 프로세스로
    if (current->priority > PRIORITY_LOW) {
        current->priority--;
    }
    current->time_slice = scheduler.time_quantum;
    
    scheduler_round_robin();
}

// 스케줄러 실행
void scheduler_schedule(void) {
    if (!scheduler.ready_queue) return;
    
    // 현재 스케줄링 알고리즘 선택
    scheduler_multilevel_feedback();
}

// 프로세스 양보
void scheduler_yield(void) {
    if (scheduler.current_process) {
        scheduler.current_process->state = PROCESS_READY;
        scheduler_schedule();
    }
}

// 프로세스 블록
void process_block(process_t* process) {
    if (!process) return;
    
    scheduler_remove_process(process);
    process->state = PROCESS_BLOCKED;
    
    // 블록된 큐에 추가
    if (!scheduler.blocked_queue) {
        scheduler.blocked_queue = process;
        process->next = process;
        process->prev = process;
    } else {
        process->next = scheduler.blocked_queue;
        process->prev = scheduler.blocked_queue->prev;
        scheduler.blocked_queue->prev->next = process;
        scheduler.blocked_queue->prev = process;
    }
}

// 프로세스 언블록
void process_unblock(process_t* process) {
    if (!process || process->state != PROCESS_BLOCKED) return;
    
    scheduler_remove_process(process);
    process->state = PROCESS_READY;
    scheduler_add_process(process);
}

// 현재 프로세스 가져오기
process_t* process_get_current(void) {
    return scheduler.current_process;
}

// 현재 PID 가져오기
uint32_t process_get_pid(void) {
    return scheduler.current_process ? scheduler.current_process->pid : 0;
}

// 타이머 초기화
void timer_init(uint32_t frequency) {
    timer_frequency = frequency;
    
    // PIT (Programmable Interval Timer) 설정
    uint32_t divisor = 1193180 / frequency;
    
    // PIT 명령 바이트
    __asm__ volatile("outb %0, %1" : : "a" (0x36), "Nd" (0x43));
    __asm__ volatile("outb %0, %1" : : "a" (divisor & 0xFF), "Nd" (0x40));
    __asm__ volatile("outb %0, %1" : : "a" ((divisor >> 8) & 0xFF), "Nd" (0x40));
    
    // 타이머 인터럽트 핸들러 등록
    irq_install_handler(0, timer_handler);
    pic_unmask_irq(0);
}

// 타이머 핸들러
void timer_handler(void) {
    timer_ticks++;
    
    // 스케줄러 호출 (매 10틱마다)
    if (timer_ticks % 10 == 0) {
        scheduler_schedule();
    }
    
    // 슬립 큐 처리
    process_t* current = scheduler.sleeping_queue;
    while (current) {
        process_t* next = current->next;
        
        if (current->time_slice <= timer_ticks) {
            scheduler_remove_process(current);
            current->state = PROCESS_READY;
            scheduler_add_process(current);
        }
        
        current = next;
        if (current == scheduler.sleeping_queue) break;
    }
}

// 현재 틱 수 가져오기
uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

// 타이머 슬립
void timer_sleep(uint32_t ticks) {
    uint32_t wake_time = timer_ticks + ticks;
    
    if (scheduler.current_process) {
        scheduler.current_process->time_slice = wake_time;
        scheduler.current_process->state = PROCESS_SLEEPING;
        
        scheduler_remove_process(scheduler.current_process);
        
        // 슬립 큐에 추가
        if (!scheduler.sleeping_queue) {
            scheduler.sleeping_queue = scheduler.current_process;
            scheduler.current_process->next = scheduler.current_process;
            scheduler.current_process->prev = scheduler.current_process;
        } else {
            scheduler.current_process->next = scheduler.sleeping_queue;
            scheduler.current_process->prev = scheduler.sleeping_queue->prev;
            scheduler.sleeping_queue->prev->next = scheduler.current_process;
            scheduler.sleeping_queue->prev = scheduler.current_process;
        }
        
        scheduler_schedule();
    }
}

// 컨텍스트 스위칭 (어셈블리에서 구현)
void context_switch(process_t* from, process_t* to) {
    if (from) {
        save_context(from);
    }
    
    if (to) {
        restore_context(to);
    }
}

// 컨텍스트 저장 (어셈블리에서 구현)
void save_context(process_t* process) {
    // 실제 구현은 어셈블리에서
    (void)process;
}

// 컨텍스트 복원 (어셈블리에서 구현)
void restore_context(process_t* process) {
    // 실제 구현은 어셈블리에서
    (void)process;
}

// 스케줄러 통계 출력
void scheduler_dump_stats(void) {
    // 간단한 통계 출력
    uint32_t total = scheduler.total_processes;
    uint32_t ready = 0;
    uint32_t blocked = 0;
    uint32_t sleeping = 0;
    
    // 준비 큐 카운트
    if (scheduler.ready_queue) {
        process_t* current = scheduler.ready_queue;
        do {
            ready++;
            current = current->next;
        } while (current != scheduler.ready_queue);
    }
    
    // 블록된 큐 카운트
    if (scheduler.blocked_queue) {
        process_t* current = scheduler.blocked_queue;
        do {
            blocked++;
            current = current->next;
        } while (current != scheduler.blocked_queue);
    }
    
    // 슬립 큐 카운트
    if (scheduler.sleeping_queue) {
        process_t* current = scheduler.sleeping_queue;
        do {
            sleeping++;
            current = current->next;
        } while (current != scheduler.sleeping_queue);
    }
    
    // 통계 출력 (실제 구현에서는 콘솔 출력)
    (void)total;
    (void)ready;
    (void)blocked;
    (void)sleeping;
}

// 로드 평균 계산
uint32_t scheduler_get_load_average(void) {
    // 간단한 로드 평균 계산
    return scheduler.total_processes;
}
