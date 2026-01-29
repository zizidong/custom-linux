#include "interrupt.h"
#include <string.h>

// IDT 엔트리 배열
static idt_entry_t idt[256];
static idtr_t idtr;

// 인터럽트 핸들러 배열
static interrupt_handler_t interrupt_handlers[256];
static interrupt_handler_t irq_handlers[16];

// 시스템 콜 핸들러 배열
static syscall_handler_t syscall_handlers[SYSCALL_MAX];

// 인터럽트 초기화
void interrupt_init(void) {
    // IDT 초기화
    memset(&idt, 0, sizeof(idt));
    memset(interrupt_handlers, 0, sizeof(interrupt_handlers));
    memset(irq_handlers, 0, sizeof(irq_handlers));
    memset(syscall_handlers, 0, sizeof(syscall_handlers));
    
    // IDTR 설정
    idtr.limit = sizeof(idt) - 1;
    idtr.base = (uint32_t)&idt;
    
    // PIC 초기화
    pic_init();
    
    // 시스템 콜 초기화
    syscall_init();
    
    // IDT 로드
    __asm__ volatile("lidt %0" : : "m" (idtr));
}

// 인터럽트 핸들러 설정
void set_interrupt_handler(uint8_t num, interrupt_handler_t handler) {
    interrupt_handlers[num] = handler;
}

// 인터럽트 활성화/비활성화
void enable_interrupts(void) {
    __asm__ volatile("sti");
}

void disable_interrupts(void) {
    __asm__ volatile("cli");
}

// IRQ 핸들러 설치/제거
void irq_install_handler(int irq, interrupt_handler_t handler) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = handler;
    }
}

void irq_uninstall_handler(int irq) {
    if (irq >= 0 && irq < 16) {
        irq_handlers[irq] = NULL;
    }
}

// PIC 초기화
void pic_init(void) {
    // ICW1: 초기화 명령
    __asm__ volatile("outb %0, %1" : : "a" (0x11), "Nd" (0x20));
    __asm__ volatile("outb %0, %1" : : "a" (0x11), "Nd" (0xA0));
    
    // ICW2: 벡터 오프셋
    __asm__ volatile("outb %0, %1" : : "a" (0x20), "Nd" (0x21));
    __asm__ volatile("outb %0, %1" : : "a" (0x28), "Nd" (0xA1));
    
    // ICW3: 마스터/슬레이브 연결
    __asm__ volatile("outb %0, %1" : : "a" (0x04), "Nd" (0x21));
    __asm__ volatile("outb %0, %1" : : "a" (0x02), "Nd" (0xA1));
    
    // ICW4: 8086 모드
    __asm__ volatile("outb %0, %1" : : "a" (0x01), "Nd" (0x21));
    __asm__ volatile("outb %0, %1" : : "a" (0x01), "Nd" (0xA1));
    
    // 모든 IRQ 마스킹
    __asm__ volatile("outb %0, %1" : : "a" (0xFF), "Nd" (0x21));
    __asm__ volatile("outb %0, %1" : : "a" (0xFF), "Nd" (0xA1));
}

// EOI (End of Interrupt) 전송
void pic_send_eoi(unsigned char irq) {
    if (irq >= 8) {
        __asm__ volatile("outb %0, %1" : : "a" (0x20), "Nd" (0xA0));
    }
    __asm__ volatile("outb %0, %1" : : "a" (0x20), "Nd" (0x20));
}

// IRQ 마스킹/언마스킹
void pic_mask_irq(unsigned char irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = 0x21;
    } else {
        port = 0xA1;
        irq -= 8;
    }
    
    value = __asm__ volatile("inb %1, %0" : "=a" (value) : "Nd" (port));
    value |= (1 << irq);
    __asm__ volatile("outb %0, %1" : : "a" (value), "Nd" (port));
}

void pic_unmask_irq(unsigned char irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = 0x21;
    } else {
        port = 0xA1;
        irq -= 8;
    }
    
    value = __asm__ volatile("inb %1, %0" : "=a" (value) : "Nd" (port));
    value &= ~(1 << irq);
    __asm__ volatile("outb %0, %1" : : "a" (value), "Nd" (port));
}

// 시스템 콜 초기화
void syscall_init(void) {
    // 시스템 콜 인터럽트 핸들러 설정
    set_interrupt_handler(0x80, (interrupt_handler_t)syscall_handler);
}

// 시스템 콜 등록
void register_syscall(int num, syscall_handler_t handler) {
    if (num >= 0 && num < SYSCALL_MAX) {
        syscall_handlers[num] = handler;
    }
}

// 시스템 콜 핸들러
int syscall_handler(interrupt_context_t* context) {
    int syscall_num = context->eax;
    int arg1 = context->ebx;
    int arg2 = context->ecx;
    int arg3 = context->edx;
    
    if (syscall_num >= 0 && syscall_num < SYSCALL_MAX && syscall_handlers[syscall_num]) {
        return syscall_handlers[syscall_num](arg1, arg2, arg3);
    }
    
    return -1; // 잘못된 시스템 콜
}

// 공통 인터럽트 핸들러
void common_interrupt_handler(interrupt_context_t* context) {
    uint32_t int_no = context->int_no;
    
    // IRQ 처리
    if (int_no >= IRQ0 && int_no < IRQ0 + 16) {
        int irq = int_no - IRQ0;
        if (irq_handlers[irq]) {
            irq_handlers[irq]();
        }
        pic_send_eoi(irq);
    }
    // 예외 처리
    else if (int_no < 32) {
        // 예외 처리 (실제 구현에서는 더 복잡)
        (void)int_no;
    }
    // 일반 인터럽트 처리
    else if (interrupt_handlers[int_no]) {
        interrupt_handlers[int_no]();
    }
}

// 인터럽트 게이트 설정 (어셈블리에서 호출)
void set_idt_gate(uint8_t num, uint32_t handler) {
    idt[num].offset_low = handler & 0xFFFF;
    idt[num].selector = 0x08; // 커널 코드 세그먼트
    idt[num].zero = 0;
    idt[num].flags = 0x8E; // 인터럽트 게이트
    idt[num].offset_high = (handler >> 16) & 0xFFFF;
}

// 기본 인터럽트 핸들러들 (어셈블리에서 호출)
void isr0() { common_interrupt_handler(NULL); }
void isr1() { common_interrupt_handler(NULL); }
void isr2() { common_interrupt_handler(NULL); }
void isr3() { common_interrupt_handler(NULL); }
void isr4() { common_interrupt_handler(NULL); }
void isr5() { common_interrupt_handler(NULL); }
void isr6() { common_interrupt_handler(NULL); }
void isr7() { common_interrupt_handler(NULL); }
void isr8() { common_interrupt_handler(NULL); }
void isr9() { common_interrupt_handler(NULL); }
void isr10() { common_interrupt_handler(NULL); }
void isr11() { common_interrupt_handler(NULL); }
void isr12() { common_interrupt_handler(NULL); }
void isr13() { common_interrupt_handler(NULL); }
void isr14() { common_interrupt_handler(NULL); }
void isr15() { common_interrupt_handler(NULL); }
void isr16() { common_interrupt_handler(NULL); }
void isr17() { common_interrupt_handler(NULL); }
void isr18() { common_interrupt_handler(NULL); }
void isr19() { common_interrupt_handler(NULL); }
void isr20() { common_interrupt_handler(NULL); }
void isr21() { common_interrupt_handler(NULL); }
void isr22() { common_interrupt_handler(NULL); }
void isr23() { common_interrupt_handler(NULL); }
void isr24() { common_interrupt_handler(NULL); }
void isr25() { common_interrupt_handler(NULL); }
void isr26() { common_interrupt_handler(NULL); }
void isr27() { common_interrupt_handler(NULL); }
void isr28() { common_interrupt_handler(NULL); }
void isr29() { common_interrupt_handler(NULL); }
void isr30() { common_interrupt_handler(NULL); }
void isr31() { common_interrupt_handler(NULL); }

// IRQ 핸들러들
void irq0() { common_interrupt_handler(NULL); }
void irq1() { common_interrupt_handler(NULL); }
void irq2() { common_interrupt_handler(NULL); }
void irq3() { common_interrupt_handler(NULL); }
void irq4() { common_interrupt_handler(NULL); }
void irq5() { common_interrupt_handler(NULL); }
void irq6() { common_interrupt_handler(NULL); }
void irq7() { common_interrupt_handler(NULL); }
void irq8() { common_interrupt_handler(NULL); }
void irq9() { common_interrupt_handler(NULL); }
void irq10() { common_interrupt_handler(NULL); }
void irq11() { common_interrupt_handler(NULL); }
void irq12() { common_interrupt_handler(NULL); }
void irq13() { common_interrupt_handler(NULL); }
void irq14() { common_interrupt_handler(NULL); }
void irq15() { common_interrupt_handler(NULL); }
