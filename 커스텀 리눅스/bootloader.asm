; Stage 1 MBR 부트로더 (512바이트 제한)
; Stage 2를 로드하고 제어권을 넘김

[org 0x7c00]        ; 부트로더가 로드되는 주소
[bits 16]          ; 16비트 모드

; 초기화
_start:
    ; 세그먼트 레지스터 초기화
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00  ; 스택 포인터 설정

    ; 화면 클리어
    mov ah, 0x00
    mov al, 0x03
    int 0x10

    ; 부트 메시지 출력
    mov si, boot_msg
    call print_string

    ; Stage 2 로드
    call load_stage2

    ; Stage 2로 점프
    jmp 0x0000:0x8000

; 문자열 출력 함수
print_string:
    pusha
.loop:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0e
    int 0x10
    jmp .loop
.done:
    popa
    ret

; Stage 2 로드 함수
load_stage2:
    ; 디스크 읽기 파라미터 설정
    mov ah, 0x02    ; BIOS 읽기 함수
    mov al, 16      ; 읽을 섹터 수 (16 * 512 = 8KB)
    mov ch, 0       ; 실린더 0
    mov cl, 2       ; 섹터 2 (Stage 2는 섹터 2부터 시작)
    mov dh, 0       ; 헤드 0
    mov dl, 0x80    ; 첫 번째 하드 디스크
    mov bx, 0x8000  ; 로드할 메모리 주소

    ; 디스크 읽기 실행
    int 0x13
    jc disk_error   ; 에러 발생시 점프

    ; 성공 메시지 출력
    mov si, load_success_msg
    call print_string
    ret

disk_error:
    ; 에러 메시지 출력
    mov si, disk_error_msg
    call print_string
    jmp $           ; 무한 루프

; 데이터 섹션
boot_msg db 'Stage 1 부트로더 시작...', 0x0d, 0x0a, 0
load_success_msg db 'Stage 2 로드 완료!', 0x0d, 0x0a, 0
disk_error_msg db '디스크 읽기 오류!', 0x0d, 0x0a, 0

; 부트 시그니처 (512바이트의 마지막 2바이트)
times 510-($-$$) db 0  ; 0으로 패딩
dw 0xaa55              ; 부트 시그니처
