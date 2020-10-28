#pragma once
//MFT����Ľṹ
// �ļ���¼ͷ
#include<Windows.h>
typedef struct MFT_ENTRY_HEADER
{
	/*+0x00*/	BYTE Type[4];            // �̶�ֵ'FILE'
	/*+0x04*/	UINT16 USNOffset;        // �������к�ƫ��, �����ϵͳ�й�
	/*+0x06*/	UINT16 USNCount;         // �̶��б��СSize in words of Update Sequence Number & Array (S)
	/*+0x08*/  UINT64 Lsn;               // ��־�ļ����к�(LSN)
	/*+0x10*/  UINT16  SequenceNumber;   // ���к�(���ڼ�¼�ļ�������ʹ�õĴ���)
	/*+0x12*/  UINT16  LinkCount;        // Ӳ������
	/*+0x14*/  UINT16  AttributeOffset;  // ��һ������ƫ��

	///*+0x16*/  UINT16  Flags;            // flags, 00��ʾɾ���ļ�,01��ʾ�����ļ�,02��ʾɾ��Ŀ¼,03��ʾ����Ŀ¼


	/*+0x16*/  BYTE  Flags;              // flags, 00��ʾɾ���ļ�,01��ʾ�����ļ�,02��ʾɾ��Ŀ¼,03��ʾ����Ŀ¼
	/*+0x17*/  BYTE cryptoFlags;		 // cryptoFlags,��ʾ�Ƿ����

	/*+0x18*/  UINT32  BytesInUse;       // �ļ���¼ʵʱ��С(�ֽ�) ��ǰMFT�����,��FFFFFF�ĳ���+4
	/*+0x1C*/  UINT32  BytesAllocated;   // �ļ���¼�����С(�ֽ�)
	/*+0x20*/  UINT64  BaseFileRecord;   // = 0 �����ļ���¼ File reference to the base FILE record
	/*+0x28*/  UINT16  NextAttributeNumber; // ��һ������ID��
	/*+0x2A*/  UINT16  Pading;           // �߽�
	/*+0x2C*/  UINT32  MFTRecordNumber;  // windows xp��ʹ��,��MFT��¼��
	/*+0x30*/  UINT16  USN;      // �������к�
	/*+0x32*/  BYTE  UpdateArray[0];      // ��������
} MFT_ENTRY_HEADER, * pMFT_ENTRY_HEADER;