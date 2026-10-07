#!/usr/bin/env python3
import sys
import os

def merge_binaries(bootloader_path, partition_table_path, app_path, output_path):
    # Flash Offsets for standard ESP32 4MB
    BOOTLOADER_OFFSET = 0x1000       # 4096
    PARTITION_TABLE_OFFSET = 0x8000  # 32768
    APP_OFFSET = 0x10000             # 65536

    print(f"Merging ESP32 binaries into single image: {output_path}")

    # Initialize empty 4MB buffer (0xFF padding like flash memory)
    flash_size = 4 * 1024 * 1024
    merged_data = bytearray([0xFF] * flash_size)

    def write_at_offset(filepath, offset):
        if not os.path.exists(filepath):
            print(f"Error: File not found: {filepath}")
            sys.exit(1)
        with open(filepath, "rb") as f:
            data = f.read()
        merged_data[offset : offset + len(data)] = data
        print(f"  - Wrote {filepath} ({len(data)} bytes) at offset 0x{offset:X}")

    write_at_offset(bootloader_path, BOOTLOADER_OFFSET)
    write_at_offset(partition_table_path, PARTITION_TABLE_OFFSET)
    write_at_offset(app_path, APP_OFFSET)

    # Trim trailing 0xFF padding beyond the last app byte
    with open(app_path, "rb") as f:
        app_len = len(f.read())
    total_len = APP_OFFSET + app_len

    with open(output_path, "wb") as f:
        f.write(merged_data[:total_len])

    print(f"Successfully generated merged binary: {output_path} ({total_len} bytes)")

if __name__ == "__main__":
    if len(sys.argv) < 5:
        print("Usage: merge_bin.py <bootloader.bin> <partition-table.bin> <app.bin> <output_merged.bin>")
        sys.exit(1)

    merge_binaries(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4])
