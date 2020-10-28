#pragma once
//MFT表项的结构
// 文件记录头
#include<Windows.h>
typedef struct MFT_ENTRY_HEADER
{
	/*+0x00*/	BYTE Type[4];            // 固定值'FILE'
	/*+0x04*/	UINT16 USNOffset;        // 更新序列号偏移, 与操作系统有关
	/*+0x06*/	UINT16 USNCount;         // 固定列表大小Size in words of Update Sequence Number & Array (S)
	/*+0x08*/  UINT64 Lsn;               // 日志文件序列号(LSN)
	/*+0x10*/  UINT16  SequenceNumber;   // 序列号(用于记录文件被反复使用的次数)
	/*+0x12*/  UINT16  LinkCount;        // 硬连接数
	/*+0x14*/  UINT16  AttributeOffset;  // 第一个属性偏移

	///*+0x16*/  UINT16  Flags;            // flags, 00表示删除文件,01表示正常文件,02表示删除目录,03表示正常目录


	/*+0x16*/  BYTE  Flags;              // flags, 00表示删除文件,01表示正常文件,02表示删除目录,03表示正常目录
	/*+0x17*/  BYTE cryptoFlags;		 // cryptoFlags,表示是否加密

	/*+0x18*/  UINT32  BytesInUse;       // 文件记录实时大小(字节) 当前MFT表项长度,到FFFFFF的长度+4
	/*+0x1C*/  UINT32  BytesAllocated;   // 文件记录分配大小(字节)
	/*+0x20*/  UINT64  BaseFileRecord;   // = 0 基础文件记录 File reference to the base FILE record
	/*+0x28*/  UINT16  NextAttributeNumber; // 下一个自由ID号
	/*+0x2A*/  UINT16  Pading;           // 边界
	/*+0x2C*/  UINT32  MFTRecordNumber;  // windows xp中使用,本MFT记录号
	/*+0x30*/  UINT16  USN;      // 更新序列号
	/*+0x32*/  BYTE  UpdateArray[0];      // 更新数组
} MFT_ENTRY_HEADER, * pMFT_ENTRY_HEADER;