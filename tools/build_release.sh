#!/usr/bin/env bash
set -e

echo "=== ESP32 Hardware Showcase Release Builder ==="

BUILD_DIR="build"
OUTPUT_DIR="release"

mkdir -p "${OUTPUT_DIR}"

if [ ! -f "${BUILD_DIR}/bootloader/bootloader.bin" ] || [ ! -f "${BUILD_DIR}/partition_table/partition-table.bin" ] || [ ! -f "${BUILD_DIR}/esp32_hardware_showcase.bin" ]; then
    echo "Build artifacts missing. Running idf.py build..."
    idf.py build
fi

cp "${BUILD_DIR}/bootloader/bootloader.bin" "${OUTPUT_DIR}/bootloader.bin"
cp "${BUILD_DIR}/partition_table/partition-table.bin" "${OUTPUT_DIR}/partition-table.bin"
cp "${BUILD_DIR}/esp32_hardware_showcase.bin" "${OUTPUT_DIR}/esp32_hardware_showcase.bin"

echo "Merging binaries into single ready-to-flash image..."
python3 tools/merge_bin.py \
    "${OUTPUT_DIR}/bootloader.bin" \
    "${OUTPUT_DIR}/partition-table.bin" \
    "${OUTPUT_DIR}/esp32_hardware_showcase.bin" \
    "${OUTPUT_DIR}/esp32_hardware_showcase_merged_0x0.bin"

echo "=== Release assets packaged successfully in ${OUTPUT_DIR}/ ==="
ls -la "${OUTPUT_DIR}"
