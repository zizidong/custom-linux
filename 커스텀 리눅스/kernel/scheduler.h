#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

// 프로세스 상태
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_SLEEPING,
    PROCESS_ZOMBIE
} process_state_t;

// 프로세스 우선순위
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_REALTIME = 3
} priority_t;

// 프로세스 구조체
typedef struct process {
    uint32_t pid;                    // 프로세스 ID
    char name[32];                   // 프로세스 이름
    process_state_t state;           // 프로세스 상태
    priority_t priority;             // 우선순위
    uint32_t time_slice;            // 남은 시간 슬라이스
    uint32_t total_time;            // 총 실행 시간
    uint32_t stack_top;             // 스택 최상단
    uint32_t stack_bottom;          // 스택 최하단
    uint32_t esp;                   // 현재 스택 포인터
    uint32_t ebp;                   // 베이스 포인터
    uint32_t eip;                   // 명령어 포인터
    uint32_t eflags;                // 플래그 레지스터
    uint32_t cr3;                   // 페이지 디렉토리
    struct process* next;           // 다음 프로세스
    struct process* prev;           // 이전 프로세스
} process_t;

// 스케줄러 구조체
typedef struct scheduler {
    process_t* current_process;     // 현재 실행 중인 프로세스
    process_t* ready_queue;         // 준비 큐
    process_t* blocked_queue;       // 블록된 큐
    process_t* sleeping_queue;      // 슬립 큐
    uint32_t next_pid;             // 다음 PID
    uint32_t total_processes;      // 총 프로세스 수
    uint32_t time_quantum;         // 시간 양자
} scheduler_t;

// 스케줄러 함수들
void scheduler_init(void);
void scheduler_add_process(process_t* process);
void scheduler_remove_process(process_t* process);
void scheduler_schedule(void);
void scheduler_yield(void);
void scheduler_sleep(uint32_t ticks);
void scheduler_wakeup(process_t* process);
void scheduler_set_priority(process_t* process, priority_t priority);

// 프로세스 관리 함수들
process_t* process_create(const char* name, void (*entry_point)(void), priority_t priority);
void process_destroy(process_t* process);
void process_block(process_t* process);
void process_unblock(process_t* process);
process_t* process_get_current(void);
uint32_t process_get_pid(void);

// 스케줄링 알고리즘
void scheduler_round_robin(void);
void scheduler_priority(void);
void scheduler_multilevel_feedback(void);

// 타이머 관련
void timer_init(uint32_t frequency);
void timer_handler(void);
uint32_t timer_get_ticks(void);
void timer_sleep(uint32_t ticks);

// 컨텍스트 스위칭
void context_switch(process_t* from, process_t* to);
void save_context(process_t* process);
void restore_context(process_t* process);

// 스케줄러 통계
void scheduler_dump_stats(void);
uint32_t scheduler_get_load_average(void);

#endif // SCHEDULER_H
