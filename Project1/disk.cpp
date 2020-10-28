#include <Windows.h>
#include <iostream>
#include "disk.h"
#include "ntfs.h"
#include "fat32.h"
#include "utils.h"


NTFSDisk GetNTFSDiskInfo(const char* disk_name) {
	/* �洢���̵���Ϣ*/
	NTFSDisk disk;
	disk.SetDiskName(disk_name);
	disk.SetType(NTFS);

	/* ��ȡ���̵�dbr(�߼����̵ĵ�һ������)*/
	UCHAR* disk_dbr = new UCHAR[512];
	GetDiskDbr(disk_name, disk_dbr);

	disk.SetType(NTFS);
	disk.SetSectorSize(UCHARToLL(disk_dbr + NTFS_SECTOR_SIZE_BYTE_OFFSET, 2));
	disk.SetSectorsPerCluster(UCHARToLL(disk_dbr + NTFS_CLUSTER_SIZE_BYTE_OFFSET, 1));
	disk.SetClusterSize(disk.GetSectorsPerCluster() * disk.GetSectorSize());
	disk.SetSectorNumber(UCHARToLL(disk_dbr + NTFS_SECTOR_NUMBER_OFFSET, 8));
	disk.SetMFTStartCluster(UCHARToLL(disk_dbr + NTFS_MFT_START_CLUSTER_BYTE_OFFSET, 8));
	disk.SetClustersPerMFT(UCHARToLL(disk_dbr + NTFS_CLUPERMFT_BYTE_OFFSET, 4));
	disk.SetClustersPerIdx(UCHARToLL(disk_dbr + NTFS_CLUPERIDX_BYTE_OFFSET, 4));

	delete[] disk_dbr;

	// disk.Display();

	return disk;
}


long long BigFileSeek(HANDLE hf, __int64 distance, DWORD MoveMethod)
{
	LARGE_INTEGER li;
	li.QuadPart = distance;
	li.LowPart = SetFilePointer(hf,
		li.LowPart,
		&li.HighPart,
		FILE_BEGIN);

	if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
		cout << "����ָ���ƶ�ʧ�ܣ�ʧ��ԭ��: " << GetLastError() << endl;
		exit(-1);
	}
	return li.QuadPart;
}


HANDLE GetDiskHandleAccessAttribute(const char* disk_name, long long int offset, DWORD access, DWORD attribute) {
	HANDLE disk_handle;
	disk_handle = CreateFileA(disk_name,
		access,
		FILE_SHARE_READ | FILE_SHARE_WRITE,//�򿪷�ʽ
		NULL,//��ȫ����
		OPEN_EXISTING,//�����е�
		attribute,
		NULL
	);

	if (disk_handle == INVALID_HANDLE_VALUE) {
		cout << "���̴�ʧ��!,�������: " << GetLastError() << endl;
		exit(-1);
	}
	/* ָ��ƫ��*/
	SetFilePointer(disk_handle, offset, NULL, FILE_BEGIN);

	return disk_handle;
}

HANDLE GetDiskHandleR(const char* disk_name, long long int offset) {
	return GetDiskHandleAccessAttribute(disk_name, offset, GENERIC_READ, FILE_ATTRIBUTE_NORMAL);
}

HANDLE GetDiskHandleRW(const char* disk_name, long long int offset) {
	return GetDiskHandleAccessAttribute(disk_name, offset, GENERIC_READ | GENERIC_WRITE, 0);
}

void GetDiskDbr(HANDLE disk_handle, UCHAR* disk_dbr) {
	DWORD len_read;
	//������������
	if (!ReadFile(disk_handle, disk_dbr, 512, &len_read, NULL)) {
		cout << "��ȡ��������ʧ��,ʧ��ԭ��: " << GetLastError() << endl;
		exit(-1);
	}
}

void GetDiskDbr(const char* disk_name, FAT32Dbr &dbr_struct) {
	HANDLE disk_handle = GetDiskHandleR(disk_name, 0);

	DWORD len_read;
	if (!ReadFile(disk_handle, &dbr_struct, 512, &len_read, NULL)) {
		cout << "��ȡ��������ʧ��,ʧ��ԭ��: " << GetLastError() << endl;
		exit(-1);
	}
	CloseHandle(disk_handle);
}

void GetDiskDbr(const char* disk_name, UCHAR* disk_dbr) {
	HANDLE disk_handle = GetDiskHandleR(disk_name, 0);
	GetDiskDbr(disk_handle, disk_dbr);
	CloseHandle(disk_handle);
}


/* ��ȡ��Ŀ¼�λ�� MFT �еĵ�����*/
void GetDiskRootMFT(HANDLE disk_handle, UCHAR* root_directory) {
	DWORD len_read;
	/* Ŀ¼���Сλ 1024 bytes*/
	if (!ReadFile(disk_handle, root_directory, 1024, &len_read, NULL)) {
		cout << "��ȡ��Ŀ¼MFT��ʧ��,ʧ��ԭ��: " << GetLastError();
		exit(-1);
	}
	// NTFS ��ÿ��Ŀ¼���� 0x46(F), 0x49(I), 0x4c(L), 0x45(E) ��ͷ
	for (int i = 0; i < 4; i++) {
		if (FILE_TAG[i] != root_directory[i]) {
			cout << "��ȡ��Ŀ¼MFT��ʧ��,ʧ��ԭ��: " << GetLastError();
			exit(-1);
		}
	}
}

/* �ж��ļ�ϵͳ����*/
FILE_SYSTEM_TYPE GetFileSystemType(const UCHAR* dbr) {
	bool flag = true;
	for (int i = 0; i < 11; i++) {
		if (dbr[i] != NTFS_TAG[i]) {
			flag = false;
			break;
		}
	}
	if (flag) {
		return NTFS;
	}
	flag = true;
	for (int i = 0; i < 8; i++) {
		if (dbr[i + 0x52] != FAT32_TAG[i]) {
			flag = false;
			break;
		}
	}
	if (flag) {
		return FAT32;
	}

	return INVALID;
}

FILE_SYSTEM_TYPE GetFileSystemTypeByName(const char* disk_name) {
	UCHAR disk_dbr[512];
	GetDiskDbr(disk_name, disk_dbr);

	return GetFileSystemType(disk_dbr);
}

DWORD ReadDisk(HANDLE disk_handle, __int64 offset, DWORD size, BYTE* buffer) {
	DWORD lenRead;
	BigFileSeek(disk_handle, offset, FILE_BEGIN);
	if (!ReadFile(disk_handle, buffer, size, &lenRead, NULL)) {
		cout << "\n��ȡ����ʧ��,ʧ��ԭ��: " << GetLastError();
		exit(-1);
	}
	return lenRead;
}

DWORD WriteDisk(HANDLE disk_handle, __int64 offset, DWORD size, BYTE* buffer) {
	DWORD dwBytesReturned = 0;


	DWORD lenWrite;
	BigFileSeek(disk_handle, offset, FILE_BEGIN);
	//	DeviceIoControl(disk, , buffer, clusterNum * Disk::v().getClusterSize(), NULL, lenWrite, NULL);

	if (!WriteFile(disk_handle, buffer, size, &lenWrite, NULL)) {
		printf("\n\n����д��ʧ�ܣ�������:%d", GetLastError());
		exit(-1);
	}
}

FAT32Disk GetFAT32DiskInfo(const char* disk_name) {
	FAT32Disk disk;
	disk.SetDiskName(disk_name);
	disk.LoadDbr();

	disk.display();
	return disk;
}