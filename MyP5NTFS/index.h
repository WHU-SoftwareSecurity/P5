#pragma once
#include<Windows.h>
//标准索引头的结构
typedef struct _STD_INDEX_HEADER {
	BYTE SIH_Flag[4];  //固定值 "INDX"
	UINT16 SIH_USNOffset;//更新序列号偏移
	UINT16 SIH_USNSize;//更新序列号和更新数组大小
	UINT64 SIH_Lsn;               // 日志文件序列号(LSN)
	UINT64 SIH_IndexCacheVCN;//本索引缓冲区在索引分配中的VCN
	UINT32 SIH_IndexEntryOffset;//索引项的偏移 相对于当前位置
	UINT32 SIH_IndexEntrySize;//索引项的大小
	UINT32 SIH_IndexEntryAllocSize;//索引项分配的大小
	UINT8 SIH_HasLeafNode;//置一 表示有子节点
	BYTE SIH_Fill[3];//填充
	UINT16 SIH_USN;//更新序列号
	BYTE SIH_USNArray[0];//更新序列数组
}STD_INDEX_HEADER, * pSTD_INDEX_HEADER;

//标准索引项的结构
typedef struct _STD_INDEX_ENTRY {
	UINT64 SIE_MFTReferNumber;//文件的MFT参考号 8
	UINT16 SIE_IndexEntrySize;//索引项的大小 10
	UINT16 SIE_FileNameAttriBodySize;//文件名属性体的大小 12
	UINT16 SIE_IndexFlag;//索引标志 14
	BYTE SIE_Fill[2];//填充 16
	UINT64 SIE_FatherDirMFTReferNumber;//父目录MFT文件参考号 24
	FILETIME SIE_CreatTime;//文件创建时间 32 
	FILETIME SIE_AlterTime;//文件最后修改时间 40
	FILETIME SIE_MFTChgTime;//文件记录最后修改时间 48
	FILETIME SIE_ReadTime;//文件最后访问时间 66
	UINT64 SIE_FileAllocSize;//文件分配大小 74
	UINT64 SIE_FileRealSize;//文件实际大小 82
	UINT64 SIE_FileFlag;//文件标志 90
	UINT8 SIE_FileNameSize;//文件名长度 91
	UINT8 SIE_FileNamespace;//文件命名空间 82
	BYTE SIE_FileNameAndFill[0];//文件名和填充
}STD_INDEX_ENTRY, * pSTD_INDEX_ENTRY;