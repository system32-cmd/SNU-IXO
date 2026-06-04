@echo off
setlocal enabledelayedexpansion

if not exist build mkdir build

cd kernel

echo Compiling SNU kernel...
g++ -m32 -ffreestanding -fno-stack-protector -O2 -Wall -Wextra -c kernel.cpp -o kernel.o
gcc -m32 -ffreestanding -fno-stack-protector -O2 -Wall -Wextra -c afs_set.c -o afs_set.o
gcc -m32 -ffreestanding -fno-stack-protector -O2 -Wall -Wextra -c disk.cpp -o disk.o
g++ -m32 -ffreestanding -fno-stack-protector -O2 -Wall -Wextra -c screen.cpp -o screen.o
g++ -m32 -ffreestanding -fno-stack-protector -O2 -Wall -Wextra -c fs.cpp -o fs.o
g++ -m32 -ffreestanding -fno-stack-protector -O2 -Wall -Wextra -c shell.cpp -o shell.o
g++ -m32 -ffreestanding -fno-stack-protector -O2 -Wall -Wextra -c keyboard.cpp -o keyboard.o

echo Linking kernel image...
"C:\Users\feltoza\gcc\bin\..\lib\gcc\x86_64-w64-mingw32\15.2.0\..\..\..\..\x86_64-w64-mingw32\bin\ld.exe" -b binary -Ttext=0x10000 --image-base=0x10000 --file-alignment 4 --section-alignment 4 -o kernel.bin kernel.o afs_set.o disk.o screen.o fs.o shell.o keyboard.o

move /Y kernel.o ..\build
move /Y afs_set.o ..\build
move /Y disk.o ..\build
move /Y screen.o ..\build
move /Y fs.o ..\build
move /Y shell.o ..\build
move /Y keyboard.o ..\build
move /Y kernel.bin ..\build
cd ..

echo Assembling boot components...
fasm boot\boot.asm build\boot.bin
fasm loader\loader.asm build\loader.bin
fasm boot\bootload.asm build\BOOTX64.EFI
if exist iso\EFI\BOOT copy /Y build\BOOTX64.EFI iso\EFI\BOOT\BOOTX64.EFI >nul

echo Creating bootable image...
python -u build\make_image.py

echo Creating UEFI ISO image...
python -u TOOL\make_iso.py || echo ISO creation skipped. Install xorriso or mkisofs to generate build\snu.iso.

echo Build complete. Output files in build\
dir build\
