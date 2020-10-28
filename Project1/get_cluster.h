#pragma once

#include "disk.h"
#include <vector>

typedef struct {
	unsigned int logicaldisk;
	/* 这里用 char 代表啥意思？*/
	/* unsigned char type; */
	FILE_SYSTEM_TYPE type;
	/* 这个类型不太对吧*/
	/* unsigned int size;*/
	long long int size;
} LDPartionList, *PLDPartionList;

/* 最大的磁盘数量*/
#define MAX_DISK_NUMBER 26

PLDPartionList GetDiskPartion();

vector<UINT64> GetFileCluster(const char* file_path);

int EncodedFileByCluster(const char* file_path, unsigned char* key);

int DecodedFileByCluster(const char* file_path, unsigned char* key);

