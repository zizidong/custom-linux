; Stage 2 부트로더
; 32비트 보호 모드로 전환

[org 0x8000]        ; Stage 2가 로드되는 주소
[bits 16]          ; 16비트 모드로 시작

; Stage 2 시작
stage2_start:
    ; 세그먼트 레지스터 재설정
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x8000  ; 스택 포인터 설정

    ; Stage 2 메시지 출력
    mov si, stage2_msg
    call print_string_16

    ; A20 게이트 활성화
    call enable_a20

    ; GDT 로드
    lgdt [gdt_descriptor]

    ; 보호 모드로 전환
    cli             ; 인터럽트 비활성화
    mov eax, cr0
    or eax, 1       ; PE 비트 설정
    mov cr0, eax

    ; 보호 모드로 점프
    jmp 0x08:protected_mode

[bits 32]
protected_mode:
    ; 세그먼트 레지스터 설정
    mov ax, 0x10    ; 데이터 세그먼트
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; 보호 모드 메시지 출력
    mov esi, protected_msg
    call print_string_32

    ; 커널 로딩 준비
    call load_kernel

    ; 커널로 점프 (32비트 주소)
    jmp 0x100000    ; 커널이 로드된 주소

; 16비트 문자열 출력 함수
print_string_16:
    push ax
    push si
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    jmp .loop
.done:
    pop si
    pop ax
    ret

; 32비트 문자열 출력 함수
print_string_32:
    push eax
    push edi
    mov edi, 0xb8000  ; 비디오 메모리 주소
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0f      ; 흰색 텍스트
    mov [edi], ax
    add edi, 2
    jmp .loop
.done:
    pop edi
    pop eax
    ret

; A20 게이트 활성화
enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

; 커널 로딩 함수
load_kernel:
    ; 여기에 커널 로딩 코드가 들어갈 예정
    ; 예: 디스크에서 커널을 읽어서 0x100000에 로드
    ret

; GDT (32비트 보호 모드용)
gdt:
    dq 0x0000000000000000  ; Null descriptor
    dq 0x00CF9A000000FFFF  ; Code descriptor
    dq 0x00CF92000000FFFF  ; Data descriptor
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt - 1
    dd gdt

; 데이터 섹션
stage2_msg db 'Stage 2 부트로더 시작...', 0x0d, 0x0a, 0
protected_msg db '32비트 보호 모드로 전환 완료!', 0
