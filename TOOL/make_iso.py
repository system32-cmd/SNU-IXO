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

print("ISO source directory:", iso_dir)
print("UEFI boot file:", efi_path)

if not iso_dir.exists():
    raise FileNotFoundError(f"Missing ISO source directory: {iso_dir}")
if not efi_path.exists():
    raise FileNotFoundError(f"Missing UEFI EFI boot file: {efi_path}")

build_dir.mkdir(parents=True, exist_ok=True)

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
        "-eltorito-alt-boot",
        "-e", "EFI/BOOT/BOOTX64.EFI",
        "-no-emul-boot",
        str(iso_dir),
    ]

print("Using tool:", tool)
print("Running:", " ".join(cmd))
subprocess.run(cmd, check=True)
print(f"Created UEFI bootable ISO: {output_iso}")
