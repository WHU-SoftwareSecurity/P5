#pragma once

/* ntfs 中一些重要字段的偏移 */
const int NTFS_SECTOR_SIZE_BYTE_OFFSET = 0xB;
const int NTFS_SECTOR_NUMBER_OFFSET = 0x28;
const int NTFS_CLUSTER_SIZE_BYTE_OFFSET = 0xD;
const int NTFS_MFT_START_CLUSTER_BYTE_OFFSET = 0x30;
const int NTFS_CLUPERMFT_BYTE_OFFSET = 0x40;
const int NTFS_CLUPERIDX_BYTE_OFFSET = 0x44;
const int NTFS_FISRST_ENTRY_BYTE_OFFSET = 20;

/* ntfs 标志位*/
const unsigned char FILE_TAG[11] = { 0x46,0x49,0x4c,0x45 };
const unsigned char NTFS_TAG[11] = { 0xeb,0x52,0x90,0x4e,0x54,0x46,0x53,0x20,0x20,0x20,0x20 };