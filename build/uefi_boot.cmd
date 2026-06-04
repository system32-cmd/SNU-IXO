qemu-system-x86_64 ^
  -drive if=pflash,format=raw,readonly=on,file="C:\Program Files\qemu\share\edk2-x86_64-code.fd" ^
  -drive if=pflash,format=raw,file=ovmf_vars.fd ^
  -cdrom snu.img ^
  -m 512M ^
  -serial stdio ^
  -display none