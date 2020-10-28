#pragma once

#include<Windows.h>
#include<stdio.h>
#include "utils.h"
#include "disk.h"


class NTFSFile
{
private:
	char targetPath[_MAX_PATH] = { 0 };
	char keyPath[_MAX_PATH] = { 0 };

	unsigned char* key = nullptr;
	long keysize = 0;
	UINT64 fileMftOffset = 0;

	int clusterStreamNum = 0;
	UINT64* clusterStream = nullptr;
	UINT64* clusterStreamSize = nullptr;
	BYTE cryptoFlag = 1;

	/*
		0	�г�����
		1	���ܴ�
		2	���ܴ�
	*/
	int function = -1;

public:
	~NTFSFile()
	{
		delete[] key;
		delete[] clusterStream;
		delete[] clusterStreamSize;
	}
	UINT64 getFileMftOffset() {
		return fileMftOffset;
	}
	void setCryptoFlag(BYTE flag) {
		cryptoFlag = flag;
	}
	BYTE getCryptoFlag() {
		return cryptoFlag;
	}
	void setFileMftOffset(UINT64 s) {
		fileMftOffset = s;
	}

	void setClusterStream(BYTE* buffer) {
		BYTE* p = buffer;
		BYTE h, l;
		while (*p != 0) {
			h = *p >> 4;
			l = *p & 0x0f;
			p = p + h + l + 1;
			clusterStreamNum++;
		}
		clusterStream = new UINT64[clusterStreamNum];
		clusterStreamSize = new UINT64[clusterStreamNum];
		printf_s("\n���ļ���%d������.", clusterStreamNum);
		p = buffer;
		for (int i = 0; i < clusterStreamNum; i++) {
			h = *p >> 4;
			l = *p & 0x0f;
			clusterStreamSize[i] = UCHARToLL(p + 1, l);
			if (i != 0)
			{
				BYTE* tmp = new BYTE[h + l + 1];
				memcpy(tmp, p, h + l + 1);
				//����
				if (*(tmp + l + h) > (BYTE) 127) {
					//����ǰ��1�ĸ���
					for (int i = 0; i < h; i++) {
						//������ǡ��û��ǰ��0��
						if (*(tmp + l + h - i) < (BYTE)127) {
							break;
						}
						else {
							BYTE a = 0x80;
							BYTE b = *(tmp + l + h - i);
							while (a & b) {
								b &= ~a;
								a >>= 1;
							}
							*(tmp + l + h - i) = b;
							if (b != 0)
								break;
						}
					}
					long long t = UCHARToLL(tmp + 1 + l, h);
					clusterStream[i] = clusterStream[i - 1] - t;
				}
				else {
					long long t = UCHARToLL(tmp + 1 + l, h);
					clusterStream[i] = clusterStream[i - 1] + t;
				}
			}
			else
				clusterStream[i] = UCHARToLL(p + 1 + l, h);
			printf_s("\n��%d��������ռ��%lld���أ���ƫ��Ϊ:%lld", i + 1, clusterStreamSize[i], clusterStream[i]);
			p = p + 1 + l + h;
		}
	}
	//��ȡ������С
	UINT64 getClusterStreamSize(int index) {
		return clusterStreamSize[index];
	}
	//��ȡ����ƫ��
	UINT64 getClusterStream(int index) {
		return clusterStream[index];
	}

	UINT64* getClusterStreams() {
		return clusterStream;
	}

	int getClusterStreamNum() {
		return clusterStreamNum;
	}
	const char* getTargetPath() {
		return targetPath;
	}

	void setTargetPath(const char* filepath) {
		if (strlen(filepath) > _MAX_PATH) {

			printf_s("������ļ������ȳ�������󳤶�!\n");
			exit(-1);
		}
		strcpy_s(targetPath, _MAX_PATH, filepath);
	}


	void  setKeyPath(const char* filepath) {
		if (strlen(filepath) > _MAX_PATH) {
			printf_s("������ļ������ȳ�������󳤶�!\n");
			exit(-1);
		}
		strcpy_s(keyPath, _MAX_PATH, filepath);
	}

	const char* getKeyPath() {
		return keyPath;
	}

	int getFunc() {
		return function;
	}
	void setFunc(int tag) {
		if (tag < 0 || tag > 3) {
			printf_s("���ܲ������ô���!\n");
		}
		function = tag;
	}
	void printFile() {
		printf_s("targetPath = %s", targetPath);
		printf_s("\nfunction = %d", function);
		printf_s("\nkeyPath = %s", keyPath);
		if (keyPath != "\0" && keysize > 0) {
			printf_s("\n��Կ�ļ���С:%ld\n\n", keysize);
			printf_s("��Կ���ֶ���������:\n");
			for (long i = 0; i < 32; i++) {
				if (i % 0xf == 0 && i) {
					printf_s("\n");
				}
				printf_s("%4x  ", key[i]);
			}
		}
		printf_s("\n");
	}
	unsigned char** getKeyPointer() {
		return &key;
	}
	unsigned char* getKey() {
		return key;
	}
	long getKeySize() {
		return keysize;
	}
	void setKeySize(long size) {
		if (size <= 0) {
			printf_s("��Կ�ļ����󣬴�СΪ0");
			exit(-1);
		}
		keysize = size;
	}
};


UINT64 ParseIndexEntry(UINT32 validLeft, BYTE* buffer, const char* name);
BYTE* ParseMFTEntry(BYTE* buffer, const char* path, NTFSDisk disk, NTFSFile& file, bool tag);
void GetNTFSFileInfo(const char* file_path, NTFSFile& file);
void ModifyMFT(BYTE* buffer, bool flag, NTFSFile &file);

/* FAT32*/
void freepathlist(struct findfilepath*& head);
vector<uint64_t> findfile(struct findfilepath* head, char* partition);
void getpathlist(char* filepath, struct findfilepath*& head, int* filenameindex);