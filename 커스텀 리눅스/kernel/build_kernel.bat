@echo off
echo 커널 빌드 시작...

REM NASM이 설치되어 있는지 확인
where nasm >nul 2>nul
if %errorlevel% neq 0 (
    echo NASM이 설치되어 있지 않습니다. NASM을 설치해주세요.
    pause
    exit /b 1
)

REM GCC가 설치되어 있는지 확인
where gcc >nul 2>nul
if %errorlevel% neq 0 (
    echo GCC가 설치되어 있지 않습니다. GCC를 설치해주세요.
    pause
    exit /b 1
)

echo 커널 컴파일 중...

REM 커널 소스 파일들 컴파일
gcc -m32 -c -fno-pie -fno-stack-protector -nostdlib -nostdinc -fno-builtin -fno-pic -mno-red-zone -ffreestanding -std=c99 -Wall -Wextra -O2 -I. -o memory.o memory.c
gcc -m32 -c -fno-pie -fno-stack-protector -nostdlib -nostdinc -fno-builtin -fno-pic -mno-red-zone -ffreestanding -std=c99 -Wall -Wextra -O2 -I. -o interrupt.o interrupt.c
gcc -m32 -c -fno-pie -fno-stack-protector -nostdlib -nostdinc -fno-builtin -fno-pic -mno-red-zone -ffreestanding -std=c99 -Wall -Wextra -O2 -I. -o scheduler.o scheduler.c
gcc -m32 -c -fno-pie -fno-stack-protector -nostdlib -nostdinc -fno-builtin -fno-pic -mno-red-zone -ffreestanding -std=c99 -Wall -Wextra -O2 -I. -o filesystem.o filesystem.c
gcc -m32 -c -fno-pie -fno-stack-protector -nostdlib -nostdinc -fno-builtin -fno-pic -mno-red-zone -ffreestanding -std=c99 -Wall -Wextra -O2 -I. -o kernel.o kernel.c

echo 커널 링크 중...
ld -m elf_i386 -T kernel.ld -o kernel.bin memory.o interrupt.o scheduler.o filesystem.o kernel.o

echo 커널 빌드 완료!
echo 생성된 파일:
echo - kernel.bin (커널 바이너리)

REM 파일 크기 확인
echo.
echo 파일 크기 확인:
dir *.bin

pause
