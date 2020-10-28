#pragma once
#include<Windows.h>
//��׼����ͷ�Ľṹ
typedef struct _STD_INDEX_HEADER {
	BYTE SIH_Flag[4];  //�̶�ֵ "INDX"
	UINT16 SIH_USNOffset;//�������к�ƫ��
	UINT16 SIH_USNSize;//�������кź͸��������С
	UINT64 SIH_Lsn;               // ��־�ļ����к�(LSN)
	UINT64 SIH_IndexCacheVCN;//�����������������������е�VCN
	UINT32 SIH_IndexEntryOffset;//�������ƫ�� ����ڵ�ǰλ��
	UINT32 SIH_IndexEntrySize;//������Ĵ�С
	UINT32 SIH_IndexEntryAllocSize;//���������Ĵ�С
	UINT8 SIH_HasLeafNode;//��һ ��ʾ���ӽڵ�
	BYTE SIH_Fill[3];//���
	UINT16 SIH_USN;//�������к�
	BYTE SIH_USNArray[0];//������������
}STD_INDEX_HEADER, * pSTD_INDEX_HEADER;

//��׼������Ľṹ
typedef struct _STD_INDEX_ENTRY {
	UINT64 SIE_MFTReferNumber;//�ļ���MFT�ο��� 8
	UINT16 SIE_IndexEntrySize;//������Ĵ�С 10
	UINT16 SIE_FileNameAttriBodySize;//�ļ���������Ĵ�С 12
	UINT16 SIE_IndexFlag;//������־ 14
	BYTE SIE_Fill[2];//��� 16
	UINT64 SIE_FatherDirMFTReferNumber;//��Ŀ¼MFT�ļ��ο��� 24
	FILETIME SIE_CreatTime;//�ļ�����ʱ�� 32 
	FILETIME SIE_AlterTime;//�ļ�����޸�ʱ�� 40
	FILETIME SIE_MFTChgTime;//�ļ���¼����޸�ʱ�� 48
	FILETIME SIE_ReadTime;//�ļ�������ʱ�� 66
	UINT64 SIE_FileAllocSize;//�ļ������С 74
	UINT64 SIE_FileRealSize;//�ļ�ʵ�ʴ�С 82
	UINT64 SIE_FileFlag;//�ļ���־ 90
	UINT8 SIE_FileNameSize;//�ļ������� 91
	UINT8 SIE_FileNamespace;//�ļ������ռ� 82
	BYTE SIE_FileNameAndFill[0];//�ļ��������
}STD_INDEX_ENTRY, * pSTD_INDEX_ENTRY;