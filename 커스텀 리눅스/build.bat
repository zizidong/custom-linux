@echo off
echo 부트로더 빌드 시작...

REM NASM이 설치되어 있는지 확인
where nasm >nul 2>nul
if %errorlevel% neq 0 (
    echo NASM이 설치되어 있지 않습니다. NASM을 설치해주세요.
    pause
    exit /b 1
)

echo Stage 1 부트로더 컴파일 중...
nasm -f bin -o stage1.bin bootloader.asm

echo Stage 2 부트로더 컴파일 중...
nasm -f bin -o stage2.bin stage2.asm

echo 부트로더 결합 중...
REM Stage 1 (512바이트) + Stage 2 (8KB) 결합
copy /b stage1.bin + stage2.bin bootloader.bin

echo 빌드 완료!
echo 생성된 파일:
echo - stage1.bin (512바이트)
echo - stage2.bin (8KB)
echo - bootloader.bin (전체 부트로더)

REM 파일 크기 확인
echo.
echo 파일 크기 확인:
dir *.bin

pause
