#pragma once

#include <string>
#include <iostream>
#include <Windows.h>
#include "utils.h"

/* todo*/
using namespace std;

enum FILE_SYSTEM_TYPE {
	INVALID,
	NTFS,
	FAT32,
};


class NTFSDisk {
private:
	FILE_SYSTEM_TYPE type = NTFS;
	string disk_name;
	int sector_size;
	long long int sector_number;
	/* 每簇扇区数*/
	int sectors_per_cluster;
	int cluster_size;

	/* MFT 起始簇 */
	long long int MFT_start_cluster;

	/* 每条记录所占的簇数 */
	int clusters_per_MFT;
	/* 每个索引所占的簇数*/
	int cluters_per_idx;

public:
	string GetDiskName() {
		return disk_name;
	}

	int GetSectorSize() {
		return sector_size;
	}

	int GetClusterSize() {
		return cluster_size;
	}

	int GetSectorsPerCluster() {
		return sectors_per_cluster;
	}

	long long int GetSectorNumber() {
		return sector_number;
	}

	FILE_SYSTEM_TYPE GetType() {
		return type;
	}

	long long int GetMFTStartCluster() {
		return MFT_start_cluster;
	}

	int GetClustersPerMFT() {
		return clusters_per_MFT;
	}

	int GetClustersPerIdx() {
		return cluters_per_idx;
	}

	void SetMFTStartCluster(long long int mft_sc) {
		MFT_start_cluster = mft_sc;
	}

	void SetClustersPerMFT(int c_per_mft) {
		clusters_per_MFT = c_per_mft;
	}

	void SetClustersPerIdx(int c_per_idx) {
		cluters_per_idx = c_per_idx;
	}

	void SetType(FILE_SYSTEM_TYPE f_type) {
		type = f_type;
	}

	void SetSectorsPerCluster(int s_per_c) {
		sectors_per_cluster = s_per_c;
	}

	void SetDiskName(const char* d_name) {
		disk_name = d_name;
	}

	void SetDiskName(const string& d_name) {
		disk_name = d_name;
	}

	void SetSectorSize(int s_size) {
		sector_size = s_size;
	}

	void SetClusterSize(int c_size) {
		cluster_size = c_size;
	}

	void SetSectorNumber(long long int s_number) {
		sector_number = s_number;
	}

	void Display() {
		cout << "磁盘名称: " << disk_name << endl;
		cout << "磁盘类型: " << "NTFS" << endl;
		cout << "扇区大小: " << sector_size << 'b' << endl;
		cout << "扇区总数: " << sector_number << endl;
		cout << "每簇扇区数: " << sectors_per_cluster << endl;
		cout << "簇大小: " << cluster_size << 'b' << endl;
		cout << "MFT起始簇: "  << MFT_start_cluster << endl;
		cout << "每记录簇数: " << clusters_per_MFT << endl;
		cout << "每索引簇数: " << cluters_per_idx << endl;
		cout << "==============================================================" << endl;
	}
};


long long BigFileSeek(HANDLE hf, __int64 distance, DWORD MoveMethod);
NTFSDisk GetNTFSDiskInfo(const char* disk_name);
HANDLE GetDiskHandleR(const char* disk_name, long long int offset);
HANDLE GetDiskHandleRW(const char* disk_name, long long int offset);
HANDLE GetDiskHandleAccessAttribute(const char* disk_name, long long int offset, DWORD access, DWORD attribute);
void GetDiskDbr(HANDLE disk_handle, UCHAR* disk_dbr);
void GetDiskDbr(const char* disk_name, UCHAR* disk_dbr);
void GetDiskRootMFT(HANDLE disk_handle, UCHAR* root_directory);
FILE_SYSTEM_TYPE GetFileSystemType(const UCHAR* dbr);
FILE_SYSTEM_TYPE GetFileSystemTypeByName(const char* disk_name);
DWORD ReadDisk(HANDLE disk_handle, __int64 offset, DWORD size, BYTE* buffer);
DWORD WriteDisk(HANDLE disk_handle, __int64 offset, DWORD size, BYTE* buffer);


typedef struct _FAT32Dbr {
	uint8_t jumpcode[3];//EB 58 90
	uint8_t OEM[8];//OEM代号
	uint8_t bytes_per_sector[2];//扇区字节数
	uint8_t sectors_per_cluster[1];//每簇扇区数
	uint8_t reserve_sectors[2];//包括DBR自己在内的FAT之前的扇区个数
	uint8_t FATnum[1];//FAT个数，一般为2
	uint8_t unimportant1[11];
	uint8_t DBR_LBA[4];//该分区的DBR所在的相对扇区号，如果是扩展分区，是相对于扩展分区首的
	uint8_t totalsectors[4];//本分区的总扇区数
	uint8_t sectors_per_FAT[4];//每个FAT的扇区数
	uint8_t unimportant2[4];
	uint8_t root_cluster_number[4];//根目录簇号
	uint8_t file_info[2];
	uint8_t backup_DBR[2];//备份引导扇区的相对于DBR的扇区号，一般为6，内容和DBR一模一样
	uint8_t zero1[12];
	uint8_t extBPB[26];//扩展BPB
	uint8_t osboot[422];//引导代码和55AA
}FAT32Dbr;

void GetDiskDbr(const char* disk_name, FAT32Dbr& dbr_struct);


class FAT32Disk {
private:
	string disk_name;
	FAT32Dbr dbr;

public:
	void display() {
		cout << "磁盘名称: " << disk_name << endl;
		cout << "磁盘类型: " << "FAT32" << endl;
		cout << "扇区大小: " << UCHARToLL(dbr.bytes_per_sector, 2) << "b" << endl;
		cout << "每簇扇区数: " << UCHARToLL(dbr.sectors_per_cluster, 1) << endl;
		cout << "FAT个数: " << UCHARToLL(dbr.FATnum, 2) << endl;
		cout << "扇区总数: " << UCHARToLL(dbr.totalsectors, 4) << endl;
		cout << "根目录簇号: " << UCHARToLL(dbr.root_cluster_number, 4) << endl;
		cout << "==============================================================" << endl;
	}

	void SetDiskName(string name) {
		disk_name = name;
	}

	long long int GetSectorNumber() {
		return UCHARToLL(dbr.totalsectors, 4);
	}

	void LoadDbr() {
		GetDiskDbr(disk_name.c_str(), dbr);
	}
};


FAT32Disk GetFAT32DiskInfo(const char* disk_name);