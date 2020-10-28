#pragma once
#ifdef GETCLUSTER_EXPORTS
#define GETCLUSTER_API __declspec(dllexport)
#else
#define GETCLUSTER_API __declspec(dllimport)
#endif

typedef struct {
	unsigned int logicaldisk;
	/* ������ char ����ɶ��˼��*/
	/* unsigned char type; */
	FILE_SYSTEM_TYPE type;
	/* ������Ͳ�̫�԰�*/
	/* unsigned int size;*/
	long long int size;
} LDPartionList, * PLDPartionList;

/* �ٶ�ֻ�� C��D��E��F �ĸ���*/
#define MAX_DISK_NUMBER 26

extern __declspec(dllexport) PLDPartionList _GetDiskPartion();

extern __declspec(dllexport) vector<UINT64> _GetFileCluster(const char* file_path);

extern __declspec(dllexport) int _EncodedFileByCluster(const char* file_path, unsigned char* key);

extern __declspec(dllexport) int _DecodedFileByCluster(const char* file_path, unsigned char* key);
