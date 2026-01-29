#include "memory.h"
#include "interrupt.h"
#include "scheduler.h"
#include "filesystem.h"
#include <stdint.h>

// 커널 진입점
void kernel_main(void) {
    // 1. 메모리 관리 초기화
    memory_init(0x100000, 64 * 1024 * 1024); // 64MB 힙
    paging_init();
    
    // 2. 인터럽트 시스템 초기화
    interrupt_init();
    
    // 3. 파일 시스템 초기화
    fs_init();
    
    // 4. 스케줄러 초기화
    scheduler_init();
    
    // 5. 인터럽트 활성화
    enable_interrupts();
    
    // 6. 초기 프로세스 생성
    // process_create("init", init_process, PRIORITY_NORMAL);
    
    // 7. 메인 루프
    while (1) {
        // 커널 메인 루프
        // 여기서 시스템 콜 처리, 프로세스 스케줄링 등 수행
        
        // 간단한 대기
        for (volatile int i = 0; i < 1000000; i++) {
            // CPU 사용량 조절
        }
    }
}

// 초기화 프로세스 (간단한 예시)
void init_process(void) {
    // 시스템 초기화 작업
    while (1) {
        // 초기화 프로세스 루프
        scheduler_yield();
    }
}

// 시스템 콜 핸들러들
int sys_read(int fd, void* buffer, int size) {
    return fs_read(fd, buffer, size);
}

int sys_write(int fd, const void* buffer, int size) {
    return fs_write(fd, buffer, size);
}

int sys_open(const char* path, int mode) {
    return fs_open(path, mode);
}

int sys_close(int fd) {
    return fs_close(fd);
}

int sys_fork(void) {
    // 프로세스 포크 구현 (간단한 예시)
    return -1; // 아직 구현되지 않음
}

int sys_exec(const char* path, char* const argv[]) {
    // 프로세스 실행 구현 (간단한 예시)
    (void)path;
    (void)argv;
    return -1; // 아직 구현되지 않음
}

int sys_exit(int status) {
    // 프로세스 종료 구현 (간단한 예시)
    (void)status;
    return 0;
}

// 시스템 콜 등록
void register_system_calls(void) {
    register_syscall(0, sys_read);    // read
    register_syscall(1, sys_write);   // write
    register_syscall(2, sys_open);    // open
    register_syscall(3, sys_close);   // close
    register_syscall(4, sys_fork);    // fork
    register_syscall(5, sys_exec);    // exec
    register_syscall(6, sys_exit);    // exit
}

// 커널 초기화 함수
void kernel_init(void) {
    // 시스템 콜 등록
    register_system_calls();
    
    // 메인 커널 함수 호출
    kernel_main();
}
