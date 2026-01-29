#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>

// 인터럽트 디스크립터 구조체
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

// IDTR 구조체
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr_t;

// 인터럽트 핸들러 타입
typedef void (*interrupt_handler_t)(void);

// 인터럽트 컨텍스트 구조체
typedef struct {
    uint32_t edi, esi, ebp, esp;
    uint32_t ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
    uint32_t user_esp, user_ss;
} interrupt_context_t;

// 인터럽트 관련 함수들
void interrupt_init(void);
void set_interrupt_handler(uint8_t num, interrupt_handler_t handler);
void enable_interrupts(void);
void disable_interrupts(void);
void irq_install_handler(int irq, interrupt_handler_t handler);
void irq_uninstall_handler(int irq);

// PIC 관련 함수들
void pic_init(void);
void pic_send_eoi(unsigned char irq);
void pic_mask_irq(unsigned char irq);
void pic_unmask_irq(unsigned char irq);

// 시스템 콜 관련
#define SYSCALL_MAX 256
typedef int (*syscall_handler_t)(int, int, int);

void syscall_init(void);
void register_syscall(int num, syscall_handler_t handler);
int syscall_handler(interrupt_context_t* context);

// 인터럽트 번호 정의
#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

// 예외 번호 정의
#define EXCEPTION_DIVIDE_ERROR 0
#define EXCEPTION_DEBUG 1
#define EXCEPTION_NMI 2
#define EXCEPTION_BREAKPOINT 3
#define EXCEPTION_OVERFLOW 4
#define EXCEPTION_BOUND_RANGE 5
#define EXCEPTION_INVALID_OPCODE 6
#define EXCEPTION_DEVICE_NOT_AVAILABLE 7
#define EXCEPTION_DOUBLE_FAULT 8
#define EXCEPTION_COPROCESSOR_SEGMENT 9
#define EXCEPTION_INVALID_TSS 10
#define EXCEPTION_SEGMENT_NOT_PRESENT 11
#define EXCEPTION_STACK_SEGMENT_FAULT 12
#define EXCEPTION_GENERAL_PROTECTION 13
#define EXCEPTION_PAGE_FAULT 14
#define EXCEPTION_FPU_ERROR 16
#define EXCEPTION_ALIGNMENT_CHECK 17
#define EXCEPTION_MACHINE_CHECK 18
#define EXCEPTION_SIMD_FPU_ERROR 19

#endif // INTERRUPT_H
