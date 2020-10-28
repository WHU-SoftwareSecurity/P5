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
	/* ÿ��������*/
	int sectors_per_cluster;
	int cluster_size;

	/* MFT ��ʼ�� */
	long long int MFT_start_cluster;

	/* ÿ����¼��ռ�Ĵ��� */
	int clusters_per_MFT;
	/* ÿ��������ռ�Ĵ���*/
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
		cout << "��������: " << disk_name << endl;
		cout << "��������: " << "NTFS" << endl;
		cout << "������С: " << sector_size << 'b' << endl;
		cout << "��������: " << sector_number << endl;
		cout << "ÿ��������: " << sectors_per_cluster << endl;
		cout << "�ش�С: " << cluster_size << 'b' << endl;
		cout << "MFT��ʼ��: "  << MFT_start_cluster << endl;
		cout << "ÿ��¼����: " << clusters_per_MFT << endl;
		cout << "ÿ��������: " << cluters_per_idx << endl;
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
	uint8_t OEM[8];//OEM����
	uint8_t bytes_per_sector[2];//�����ֽ���
	uint8_t sectors_per_cluster[1];//ÿ��������
	uint8_t reserve_sectors[2];//����DBR�Լ����ڵ�FAT֮ǰ����������
	uint8_t FATnum[1];//FAT������һ��Ϊ2
	uint8_t unimportant1[11];
	uint8_t DBR_LBA[4];//�÷�����DBR���ڵ���������ţ��������չ���������������չ�����׵�
	uint8_t totalsectors[4];//����������������
	uint8_t sectors_per_FAT[4];//ÿ��FAT��������
	uint8_t unimportant2[4];
	uint8_t root_cluster_number[4];//��Ŀ¼�غ�
	uint8_t file_info[2];
	uint8_t backup_DBR[2];//�������������������DBR�������ţ�һ��Ϊ6�����ݺ�DBRһģһ��
	uint8_t zero1[12];
	uint8_t extBPB[26];//��չBPB
	uint8_t osboot[422];//���������55AA
}FAT32Dbr;

void GetDiskDbr(const char* disk_name, FAT32Dbr& dbr_struct);


class FAT32Disk {
private:
	string disk_name;
	FAT32Dbr dbr;

public:
	void display() {
		cout << "��������: " << disk_name << endl;
		cout << "��������: " << "FAT32" << endl;
		cout << "������С: " << UCHARToLL(dbr.bytes_per_sector, 2) << "b" << endl;
		cout << "ÿ��������: " << UCHARToLL(dbr.sectors_per_cluster, 1) << endl;
		cout << "FAT����: " << UCHARToLL(dbr.FATnum, 2) << endl;
		cout << "��������: " << UCHARToLL(dbr.totalsectors, 4) << endl;
		cout << "��Ŀ¼�غ�: " << UCHARToLL(dbr.root_cluster_number, 4) << endl;
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