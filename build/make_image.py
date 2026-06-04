from pathlib import Path

root = Path(__file__).parent
boot = root / 'boot.bin'
loader = root / 'loader.bin'
kernel = root / 'kernel.bin'
image = root / 'snu.img'

for path in (boot, loader, kernel):
    if not path.exists():
        raise FileNotFoundError(f"Missing {path}")

boot_data = boot.read_bytes()
loader_data = loader.read_bytes()
kernel_data = kernel.read_bytes()

if len(boot_data) != 512:
    raise ValueError(f"Boot sector must be exactly 512 bytes, got {len(boot_data)}")
if len(loader_data) > 2048:
    raise ValueError(f"Loader must be at most 2048 bytes, got {len(loader_data)}")
if len(kernel_data) > 64 * 512:
    raise ValueError(f"Kernel binary too large for loader ({len(kernel_data)} bytes)")

image_data = bytearray(1440 * 1024)
image_data[0:512] = boot_data
image_data[512:512 + len(loader_data)] = loader_data
image_data[5 * 512:5 * 512 + len(kernel_data)] = kernel_data

image.write_bytes(image_data)
print(f"Created bootable image: {image.resolve()}")