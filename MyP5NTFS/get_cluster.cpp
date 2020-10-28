#include <Windows.h>
#include <iostream>
#include "get_cluster.h"
#include "file.h"
#include "crypto.h"

using namespace std;


PLDPartionList GetDiskPartion() {
	/* �������*/
	static unsigned int disk_number = 0;
	/* ��������*/
	static char disk_name[7] = "\\\\.\\?:";
	static DWORD d = GetLogicalDrives();
	/* store disk dbr*/
	static UCHAR disk_dbr[512];

	if (!d) {
		cout << "��ȡӲ��ʧ��: " << GetLastError() << endl;
	}

	/* ��C�̿�ʼ*/
	PLDPartionList disk_info = new LDPartionList;
	/* ������̷�*/
	while (!(d & (1 << (disk_number + 2)))) {
		disk_number++;
	}

	if (disk_number >= MAX_DISK_NUMBER) {
		std::cout << "���̱�����ϣ������¿�ʼ��ȡ��Ϣ" << std::endl;
		disk_number = 0;

		disk_info->logicaldisk = 0;
		disk_info->type = INVALID;
		disk_info->size = 0;

		return disk_info;
	}
	
	if (d & (1 << (disk_number + 2))) {
		disk_name[4] = 'C' + disk_number;
		/* fixme*/
		switch (GetFileSystemTypeByName(disk_name)) {
		case NTFS: {
			NTFSDisk disk = GetNTFSDiskInfo(disk_name);
			disk.Display();
			disk_number++;

			disk_info->logicaldisk = disk_number;
			disk_info->type = disk.GetType();
			disk_info->size = disk.GetSectorNumber();
			
			break;
		}
		case FAT32: {
			disk_number++;

			FAT32Disk disk = GetFAT32DiskInfo(disk_name);
			disk_info->logicaldisk = disk_number;
			disk_info->type = FAT32;
			disk_info->size = disk.GetSectorNumber();

			break;
		}
		default: {
			cout << "Not supported file system type!" << endl;
			disk_number++;

			disk_info->logicaldisk = disk_number;
			disk_info->type = INVALID;
			disk_info->size = 0;
		}
		}

	}

	return disk_info;
}


vector<UINT64> GetFileCluster(const char* file_path) {
	char disk_name[7] = "\\\\.\\?:";
	disk_name[4] = file_path[0];
	vector<UINT64> cluster_streams;

	switch (GetFileSystemTypeByName(disk_name)) {
	case NTFS: {
		NTFSFile file;
		GetNTFSFileInfo(file_path, file);

		for (int i = 0; i != file.getClusterStreamNum(); i++) {
			UINT64 cluster_number = file.getClusterStream(i);
			UINT64 cluster_size = file.getClusterStreamSize(i);
			for (UINT64 j = 0; j != cluster_size; j++) {
				cluster_streams.push_back(cluster_number + j);
			}
		}
		break;
	}
	case FAT32: {
		struct findfilepath* p = NULL, * head = NULL;
		int filenameindex = 1;//��1��ʼ����
		
		/* ·������*/
		char filepath[101] = { 0 };
		strcpy_s(filepath, file_path);
		cout << endl << ">>>>>>>>>>>>>>>>>>>>��ʼ����<<<<<<<<<<<<<<<<<<<<<<" << endl << endl;
		//ָ��sҪ���������ͣ���Ȼ�Ǹ������ڴ�й©
		getpathlist(filepath, head, &filenameindex);
		cluster_streams = move(findfile(head, disk_name));
		cout << "��ʼ��: " << hex << cluster_streams[0] << ", ��������: " << oct << cluster_streams.size() << endl;
		cout << endl << ">>>>>>>>>>>>>>>>>>>>�������<<<<<<<<<<<<<<<<<<<<<<" << endl << endl;
		freepathlist(head);
		break;
	}
	case INVALID: {
		cout << "Invalid file path!" << endl;
	}
	}
	
	return cluster_streams;
}


int EncodedFileByCluster(const char* file_path, unsigned char* key) {
	char disk_name[7] = "\\\\.\\?:";
	disk_name[4] = file_path[0];

	NTFSDisk disk = GetNTFSDiskInfo(disk_name);
	NTFSFile file;


	GetNTFSFileInfo(file_path, file);
	if (file.getCryptoFlag()) {
		printf("�ļ��Ѿ����ܹ���!\n��������\n");
		return 0;
	}
	/* ������Կ��Ϣ*/
	unsigned char** key_copy = file.getKeyPointer();
	file.setKeySize(strlen((const char*)key));
	*key_copy = new unsigned char[file.getKeySize()];
	memcpy((*key_copy), key, file.getKeySize());

	HANDLE disk_handle = GetDiskHandleRW(disk_name, 0);
	DWORD dwBytesReturned;
	/* ��ռ����*/
	if (!DeviceIoControl(disk_handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL)) {
		cout << "\n��ռ����ʧ�ܣ�ʧ��ԭ��: " << GetLastError() << endl;
		CloseHandle(disk_handle);
		/* -1 ��ʾʧ��*/
		return -1;
	}


	/* �ش�С*/
	BYTE* buffer = new BYTE [disk.GetClusterSize()];
	if (file.getClusterStreamNum() == 0) {
		cout << "\n\n���ļ���С���޷����ؼ��ܣ����������ܷ�ʽ��" << endl;
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>��ʼ����<<<<<<<<<<<<<<<<<<<<<<\n" << endl;
		ReadDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
		ModifyMFT(buffer, true, file);
		WriteDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>�������<<<<<<<<<<<<<<<<<<<<<<\n\n";
	}
	else {
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>��ʼ����<<<<<<<<<<<<<<<<<<<<<<\n\n";
		//�Ե�i����������
		for (int i = 0; i < file.getClusterStreamNum(); i++) {
			//�м�����
			UINT64 clusterNum = file.getClusterStreamSize(i);
			//ƫ��
			UINT64 clusterOffset = file.getClusterStream(i);
			cout << "\n���ڼ��ܵ�" << (i + 1) << "������!" << endl;
			for (int j = 0; j < clusterNum; j++) {

				ReadDisk(disk_handle, (clusterOffset + __int64(j)) * disk.GetClusterSize(), disk.GetClusterSize(), buffer);
				unsigned char* newBuffer =
					enc_cluster((unsigned char*)buffer, disk.GetClusterSize(),file.getKey());
				//д�ش���
				WriteDisk(disk_handle, (clusterOffset + __int64(j)) * disk.GetClusterSize(), disk.GetClusterSize(), newBuffer);
				free(newBuffer);
			}
		}
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>�������<<<<<<<<<<<<<<<<<<<<<<\n\n";
	}

	//�����ļ�MFT����д��
	ReadDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
	buffer[23] = (byte)1;
	WriteDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);

	CloseHandle(disk_handle);

	return 0;
}

int DecodedFileByCluster(const char* file_path, unsigned char* key) {
	char disk_name[7] = "\\\\.\\?:";
	disk_name[4] = file_path[0];

	NTFSDisk disk = GetNTFSDiskInfo(disk_name);
	NTFSFile file;
	GetNTFSFileInfo(file_path, file);
	if (!file.getCryptoFlag()) {
		printf("�ļ�û�м��ܹ�!\n��������\n");
		return 0;
	}
	/* ������Կ��Ϣ*/
	unsigned char** key_copy = file.getKeyPointer();
	file.setKeySize(strlen((const char*)key));
	*key_copy = new unsigned char[file.getKeySize()];
	memcpy((*key_copy), key, file.getKeySize());

	HANDLE disk_handle = GetDiskHandleRW(disk_name, 0);
	DWORD dwBytesReturned;
	/* ��ռ����*/
	if (!DeviceIoControl(disk_handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL)) {
		cout << "\n��ռ����ʧ�ܣ�ʧ��ԭ��: " << GetLastError() << endl;
		CloseHandle(disk_handle);
		return -1;
	}

	/* �ش�С*/
	BYTE* buffer = new BYTE[disk.GetClusterSize()];
	if (file.getClusterStreamNum() == 0) {
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>��ʼ����<<<<<<<<<<<<<<<<<<<<<<\n\n";
		ReadDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
		ModifyMFT(buffer, false, file);
		WriteDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>�������<<<<<<<<<<<<<<<<<<<<<<\n\n";
	}
	else {
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>��ʼ����<<<<<<<<<<<<<<<<<<<<<<\n\n" << endl;
		//�Ե�i����������
		for (int i = 0; i < file.getClusterStreamNum(); i++) {
			//�м�����
			UINT64 clusterNum = file.getClusterStreamSize(i);
			//ƫ��
			UINT64 clusterOffset = file.getClusterStream(i);
			cout << "\n���ڽ��ܵ�" << (i + 1) << "������!";
			for (int j = 0; j < clusterNum; j++) {

				ReadDisk(disk_handle, (clusterOffset + __int64(j)) * disk.GetClusterSize(), disk.GetClusterSize(), buffer);
				unsigned char* newBuffer =
					dec_cluster((unsigned char*)buffer, disk.GetClusterSize(), file.getKey());
				//д�ش���
				WriteDisk(disk_handle, (clusterOffset + __int64(j)) * disk.GetClusterSize(), disk.GetClusterSize(), newBuffer);
				free(newBuffer);
			}
		}
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>�������<<<<<<<<<<<<<<<<<<<<<<\n\n";
	}


	ReadDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
	buffer[23] = (byte)0;
	WriteDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
	CloseHandle(disk_handle);

	return 0;
}