@echo off
setlocal enabledelayedexpansion

if not exist build mkdir build

cd kernel

echo Compiling SNU kernel...
g++ -m32 -ffreestanding -fno-stack-protector -O2 -Wall -Wextra -c kernel.cpp -o kernel.o
gcc -m32 -ffreestanding -fno-stack-protector -O2 -Wall -Wextra -c afs_set.c -o afs_set.o
gcc -m32 -ffreestanding -fno-stack-protector -O2 -Wall -Wextra -c disk.cpp -o disk.o
g++ -m32 -ffreestanding -fno-stack-protector -O2 -Wall -Wextra -c screen.cpp -o screen.o

echo Linking kernel image...
g++ -m32 -ffreestanding -nostdlib -Wl,-Ttext=0x100000 -Wl,--image-base=0x100000 -o kernel.exe kernel.o afs_set.o disk.o screen.o
C:\Users\feltoza\gcc\bin\..\lib\gcc\x86_64-w64-mingw32\15.2.0\..\..\..\..\x86_64-w64-mingw32\bin\objcopy.exe -O binary kernel.exe kernel.bin

move /Y kernel.o afs_set.o disk.o kernel.elf kernel.bin ..\build\
cd ..

echo Assembling boot components...
fasm boot\boot.asm build\boot.bin
fasm loader\loader.asm build\loader.bin

echo Creating bootable image...
python -u build\make_image.py

echo Build complete. Output files in build\
dir build\
