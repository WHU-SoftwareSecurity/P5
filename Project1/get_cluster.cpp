#include <Windows.h>
#include <iostream>
#include "get_cluster.h"
#include "file.h"
#include "crypto.h"

using namespace std;


PLDPartionList GetDiskPartion() {
	/* 磁盘序号*/
	static unsigned int disk_number = 0;
	/* 磁盘名称*/
	static char disk_name[7] = "\\\\.\\?:";
	static DWORD d = GetLogicalDrives();
	/* store disk dbr*/
	static UCHAR disk_dbr[512];

	if (!d) {
		cout << "读取硬盘失败: " << GetLastError() << endl;
	}

	/* 从C盘开始*/
	PLDPartionList disk_info = new LDPartionList;
	/* 遍过空盘符*/
	while (!(d & (1 << (disk_number + 2)))) {
		disk_number++;
	}

	if (disk_number >= MAX_DISK_NUMBER) {
		std::cout << "磁盘遍历完毕，将重新开始读取信息" << std::endl;
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
		int filenameindex = 1;//从1开始解析
		
		/* 路径副本*/
		char filepath[101] = { 0 };
		strcpy_s(filepath, file_path);
		cout << endl << ">>>>>>>>>>>>>>>>>>>>开始解析<<<<<<<<<<<<<<<<<<<<<<" << endl << endl;
		//指针s要用引用类型，不然是副本，内存泄漏
		getpathlist(filepath, head, &filenameindex);
		cluster_streams = move(findfile(head, disk_name));
		cout << "起始簇: " << hex << cluster_streams[0] << ", 簇链长度: " << oct << cluster_streams.size() << endl;
		cout << endl << ">>>>>>>>>>>>>>>>>>>>解析完毕<<<<<<<<<<<<<<<<<<<<<<" << endl << endl;
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
		printf("文件已经加密过了!\n结束任务\n");
		return 0;
	}
	/* 设置密钥信息*/
	unsigned char** key_copy = file.getKeyPointer();
	file.setKeySize(strlen((const char*)key));
	*key_copy = new unsigned char[file.getKeySize()];
	memcpy((*key_copy), key, file.getKeySize());

	HANDLE disk_handle = GetDiskHandleRW(disk_name, 0);
	DWORD dwBytesReturned;
	/* 独占磁盘*/
	if (!DeviceIoControl(disk_handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL)) {
		cout << "\n独占磁盘失败，失败原因: " << GetLastError() << endl;
		CloseHandle(disk_handle);
		/* -1 表示失败*/
		return -1;
	}


	/* 簇大小*/
	BYTE* buffer = new BYTE [disk.GetClusterSize()];
	if (file.getClusterStreamNum() == 0) {
		cout << "\n\n该文件过小，无法按簇加密，采用流加密方式！" << endl;
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>开始加密<<<<<<<<<<<<<<<<<<<<<<\n" << endl;
		ReadDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
		ModifyMFT(buffer, true, file);
		WriteDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>加密完毕<<<<<<<<<<<<<<<<<<<<<<\n\n";
	}
	else {
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>开始加密<<<<<<<<<<<<<<<<<<<<<<\n\n";
		//对第i个簇流加密
		for (int i = 0; i < file.getClusterStreamNum(); i++) {
			//有几个簇
			UINT64 clusterNum = file.getClusterStreamSize(i);
			//偏移
			UINT64 clusterOffset = file.getClusterStream(i);
			cout << "\n正在加密第" << (i + 1) << "个簇流!" << endl;
			for (int j = 0; j < clusterNum; j++) {

				ReadDisk(disk_handle, (clusterOffset + __int64(j)) * disk.GetClusterSize(), disk.GetClusterSize(), buffer);
				unsigned char* newBuffer =
					enc_cluster((unsigned char*)buffer, disk.GetClusterSize(),file.getKey());
				//写回磁盘
				WriteDisk(disk_handle, (clusterOffset + __int64(j)) * disk.GetClusterSize(), disk.GetClusterSize(), newBuffer);
				free(newBuffer);
			}
		}
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>加密完毕<<<<<<<<<<<<<<<<<<<<<<\n\n";
	}

	//对于文件MFT进行写入
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
		printf("文件没有加密过!\n结束任务\n");
		return 0;
	}
	/* 设置密钥信息*/
	unsigned char** key_copy = file.getKeyPointer();
	file.setKeySize(strlen((const char*)key));
	*key_copy = new unsigned char[file.getKeySize()];
	memcpy((*key_copy), key, file.getKeySize());

	HANDLE disk_handle = GetDiskHandleRW(disk_name, 0);
	DWORD dwBytesReturned;
	/* 独占磁盘*/
	if (!DeviceIoControl(disk_handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dwBytesReturned, NULL)) {
		cout << "\n独占磁盘失败，失败原因: " << GetLastError() << endl;
		CloseHandle(disk_handle);
		return -1;
	}

	/* 簇大小*/
	BYTE* buffer = new BYTE[disk.GetClusterSize()];
	if (file.getClusterStreamNum() == 0) {
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>开始解密<<<<<<<<<<<<<<<<<<<<<<\n\n";
		ReadDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
		ModifyMFT(buffer, false, file);
		WriteDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>解密完毕<<<<<<<<<<<<<<<<<<<<<<\n\n";
	}
	else {
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>开始解密<<<<<<<<<<<<<<<<<<<<<<\n\n" << endl;
		//对第i个簇流加密
		for (int i = 0; i < file.getClusterStreamNum(); i++) {
			//有几个簇
			UINT64 clusterNum = file.getClusterStreamSize(i);
			//偏移
			UINT64 clusterOffset = file.getClusterStream(i);
			cout << "\n正在解密第" << (i + 1) << "个簇流!";
			for (int j = 0; j < clusterNum; j++) {

				ReadDisk(disk_handle, (clusterOffset + __int64(j)) * disk.GetClusterSize(), disk.GetClusterSize(), buffer);
				unsigned char* newBuffer =
					dec_cluster((unsigned char*)buffer, disk.GetClusterSize(), file.getKey());
				//写回磁盘
				WriteDisk(disk_handle, (clusterOffset + __int64(j)) * disk.GetClusterSize(), disk.GetClusterSize(), newBuffer);
				free(newBuffer);
			}
		}
		cout << "\n\n>>>>>>>>>>>>>>>>>>>>解密完毕<<<<<<<<<<<<<<<<<<<<<<\n\n";
	}


	ReadDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
	buffer[23] = (byte)0;
	WriteDisk(disk_handle, file.getFileMftOffset() * disk.GetSectorSize(), disk.GetClusterSize(), buffer);
	CloseHandle(disk_handle);

	return 0;
}