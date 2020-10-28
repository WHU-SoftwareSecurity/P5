#pragma once

#include "disk.h"
#include <vector>

typedef struct {
	unsigned int logicaldisk;
	/* ������ char ����ɶ��˼��*/
	/* unsigned char type; */
	FILE_SYSTEM_TYPE type;
	/* ������Ͳ�̫�԰�*/
	/* unsigned int size;*/
	long long int size;
} LDPartionList, *PLDPartionList;

/* ���Ĵ�������*/
#define MAX_DISK_NUMBER 26

PLDPartionList GetDiskPartion();

vector<UINT64> GetFileCluster(const char* file_path);

int EncodedFileByCluster(const char* file_path, unsigned char* key);

int DecodedFileByCluster(const char* file_path, unsigned char* key);

