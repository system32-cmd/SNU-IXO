from pathlib import Path
import sys

try:
    from pycdlib import PyCdlib
except ImportError as exc:
    raise RuntimeError(
        "pycdlib is required to create the ISO image.\n"
        "Install it with: python -m pip install pycdlib"
    ) from exc

root = Path(__file__).resolve().parent
repo = root.parent
iso_dir = repo / "iso"
build_dir = repo / "build"
output_iso = build_dir / "snu.iso"
efi_path = iso_dir / "EFI" / "BOOT" / "BOOTX64.EFI"
bios_img = build_dir / "snu.img"
kernel_src = build_dir / "kernel.bin"

print("ISO source directory:", iso_dir)
print("UEFI boot file:", efi_path)
print("BIOS boot image:", bios_img)
print("Kernel binary:", kernel_src)

if not iso_dir.exists():
    raise FileNotFoundError(f"Missing ISO source directory: {iso_dir}")
if not efi_path.exists():
    raise FileNotFoundError(f"Missing UEFI EFI boot file: {efi_path}")
if not bios_img.exists():
    raise FileNotFoundError(f"Missing BIOS boot image: {bios_img}")
if not kernel_src.exists():
    raise FileNotFoundError(f"Missing kernel binary: {kernel_src}")

build_dir.mkdir(parents=True, exist_ok=True)

iso = PyCdlib()
iso.new(interchange_level=3, joliet=True, rock_ridge=None, vol_ident='SNU')

open_files = []

fp = open(bios_img, 'rb')
open_files.append(fp)
iso.add_fp(fp, bios_img.stat().st_size,
           iso_path='/SNU.IMG;1',
           joliet_path='/SNU.IMG')

fp = open(kernel_src, 'rb')
open_files.append(fp)
iso.add_fp(fp, kernel_src.stat().st_size,
           iso_path='/KERNEL.BIN;1',
           joliet_path='/KERNEL.BIN')

iso.add_directory('/EFI', joliet_path='/EFI')
iso.add_directory('/EFI/BOOT', joliet_path='/EFI/BOOT')

fp = open(efi_path, 'rb')
open_files.append(fp)
iso.add_fp(fp, efi_path.stat().st_size,
           iso_path='/EFI/BOOT/BOOTX64.EFI;1',
           joliet_path='/EFI/BOOT/BOOTX64.EFI')

iso.add_eltorito('/EFI/BOOT/BOOTX64.EFI;1', efi=True, boot_info_table=False, media_name='noemul')
iso.add_eltorito('/SNU.IMG;1', boot_info_table=True, media_name='floppy')

iso.write(str(output_iso))
iso.close()
for fp in open_files:
    fp.close()
print(f"Created hybrid BIOS/UEFI ISO: {output_iso}")
