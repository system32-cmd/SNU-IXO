from pathlib import Path
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parent
repo = root.parent
iso_dir = repo / "iso"
build_dir = repo / "build"
output_iso = build_dir / "snu.iso"
efi_path = iso_dir / "EFI" / "BOOT" / "BOOTX64.EFI"
bios_img = build_dir / "snu.img"
kernel_src = build_dir / "kernel.bin"
iso_bios_img = iso_dir / "snu.img"
iso_kernel = iso_dir / "kernel.bin"

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

shutil.copy2(bios_img, iso_bios_img)
print(f"Copied BIOS boot image into ISO source: {iso_bios_img}")
shutil.copy2(kernel_src, iso_kernel)
print(f"Copied kernel binary into ISO source: {iso_kernel}")

def find_tool(names):
    for name in names:
        path = shutil.which(name)
        if path:
            return path
    return None

tool = find_tool(["xorriso", "genisoimage", "mkisofs"])
if tool is None:
    raise RuntimeError(
        "No ISO creation tool found. Install xorriso, genisoimage, or mkisofs, then rerun this script.\n"
        "Example: genisoimage -o build/snu.iso -V SNU -r -J -joliet-long "
        "-c boot.catalog -b snu.img -no-emul-boot -boot-load-size 4 -boot-info-table "
        "-eltorito-alt-boot -e EFI/BOOT/BOOTX64.EFI -no-emul-boot iso"
    )

if tool.endswith("xorriso"):
    cmd = [
        tool,
        "-as", "mkisofs",
        "-r",
        "-J",
        "-joliet-long",
        "-o", str(output_iso),
        "-V", "SNU",
        "-c", "boot.catalog",
        "-b", "snu.img",
        "-no-emul-boot",
        "-boot-load-size", "4",
        "-boot-info-table",
        "-eltorito-alt-boot",
        "-e", "EFI/BOOT/BOOTX64.EFI",
        "-no-emul-boot",
        "-isohybrid-gpt-basdat",
        str(iso_dir),
    ]
else:
    cmd = [
        tool,
        "-o", str(output_iso),
        "-V", "SNU",
        "-r",
        "-J",
        "-joliet-long",
        "-c", "boot.catalog",
        "-b", "snu.img",
        "-no-emul-boot",
        "-boot-load-size", "4",
        "-boot-info-table",
        "-eltorito-alt-boot",
        "-e", "EFI/BOOT/BOOTX64.EFI",
        "-no-emul-boot",
        str(iso_dir),
    ]

print("Using tool:", tool)
print("Running:", " ".join(cmd))
subprocess.run(cmd, check=True)
print(f"Created hybrid BIOS/UEFI ISO: {output_iso}")
