#pragma once
#include<Windows.h>
//常驻属性和非常驻属性的公用部分
typedef struct _CommonAttributeHeader {
	UINT32 ATTR_Type; //属性类型	4
	UINT32 ATTR_Size; //属性头和属性体的总长度	8
	BYTE ATTR_ResFlag; //是否是常驻属性（0常驻 1非常驻）	
	BYTE ATTR_NamSz; //属性名的长度	10
	UINT16 ATTR_NamOff; //属性名的偏移 相对于属性头	12
	UINT16 ATTR_Flags; //标志（0x0001压缩 0x4000加密 0x8000稀疏）	14
	UINT16 ATTR_Id; //属性唯一ID	16
}CommonAttributeHeader, * pCommonAttributeHeader;

//常驻属性 属性头
typedef struct _ResidentAttributeHeader {
	UINT32 ATTR_DatSz; //属性数据的长度
	UINT16 ATTR_DatOff; //属性数据相对于属性头的偏移
	BYTE ATTR_Indx; //索引
	BYTE ATTR_Resvd; //保留
	BYTE ATTR_AttrNam[0];//属性名，Unicode，结尾无0
}ResidentAttributeHeader, * pResidentAttributeHeader;

//非常驻属性 属性头
typedef struct _NonResidentAttributeHeader {
	UINT64 ATTR_StartVCN; //本属性中数据流起始虚拟簇号 
	UINT64 ATTR_EndVCN; //本属性中数据流终止虚拟簇号
	UINT16 ATTR_DatOff; //簇流列表相对于属性头的偏移
	UINT16 ATTR_CmpSz; //压缩单位 2的N次方
	UINT32 ATTR_Resvd;
	UINT64 ATTR_AllocSz; //属性分配的大小
	UINT64 ATTR_ValidSz; //属性的实际大小
	UINT64 ATTR_InitedSz; //属性的初始大小
	BYTE ATTR_AttrNam[0];
}NonResidentAttributeHeader, * pNonResidentAttributeHeader;

typedef struct Value0x30
{
	UINT64 FN_ParentFR; /*父目录的MFT记录的记录索引。
							注意：该值的低6字节是MFT记录号，高2字节是该MFT记录的序列号*/
	FILETIME FN_CreatTime;
	FILETIME FN_AlterTime;
	FILETIME FN_MFTChg;
	FILETIME FN_ReadTime;
	UINT64 FN_AllocSz;
	UINT64 FN_ValidSz;//文件的真实尺寸
	UINT32 FN_DOSAttr;//DOS文件属性
	UINT32 FN_EA_Reparse;//扩展属性与链接
	BYTE FN_NameSz;//文件名的字符数
	BYTE FN_NamSpace;/*命名空间，该值可为以下值中的任意一个
						0：POSIX　可以使用除NULL和分隔符“/”之外的所有UNICODE字符，最大可以使用255个字符。注意：“：”是合法字符，但Windows不允许使用。
						1：Win32　Win32是POSIX的一个子集，不区分大小写，可以使用除““”、“＊”、“?”、“：”、“/”、“<”、“>”、“/”、“|”之外的任意UNICODE字符，但名字不能以“.”或空格结尾。
						2：DOS　DOS命名空间是Win32的子集，只支持ASCII码大于空格的8BIT大写字符并且不支持以下字符““”、“＊”、“?”、“：”、“/”、“<”、“>”、“/”、“|”、“+”、“,”、“;”、“=”；同时名字必须按以下格式命名：1~8个字符，然后是“.”，然后再是1~3个字符。
						3：Win32&DOS　这个命名空间意味着Win32和DOS文件名都存放在同一个文件名属性中。*/
	BYTE* FN_FileName = nullptr;
	Value0x30(BYTE fn_nameSz, BYTE* fn_fileName) {
		FN_FileName = new BYTE[FN_NameSz * 2];
		memcpy(FN_FileName, fn_fileName, fn_nameSz);
		FN_NameSz = fn_nameSz;
	}
	~Value0x30() {
		delete[] FN_FileName;
		FN_FileName = nullptr;
		FN_NameSz = 0;
	}
}Value0x30;

//索引头
typedef struct _INDEX_HEADER {
	UINT32 IH_EntryOff;//第一个目录项的偏移
	UINT32 IH_TalSzOfEntries;//目录项的总尺寸(包括索引头和下面的索引项)
	UINT32 IH_AllocSize;//目录项分配的尺寸
	BYTE IH_Flags;/*标志位，此值可能是以下和值之一：
				  0x00       小目录(数据存放在根节点的数据区中)
				  0x01       大目录(需要目录项存储区和索引项位图)*/
	BYTE IH_Resvd[3];
}INDEX_HEADER, * pINDEX_HEADER;

//INDEX_ROOT 0X90属性体
typedef struct _INDEX_ROOT {
	//索引根
	UINT32 IR_AttrType;//属性的类型
	UINT32 IR_ColRule;//整理规则
	UINT32 IR_EntrySz;//目录项分配尺寸
	BYTE IR_ClusPerRec;//每个目录项占用的簇数
	BYTE IR_Resvd[3];
	//索引头
	INDEX_HEADER IH;
	//索引项  可能不存在
	BYTE* IR_IndexEntry;
}INDEX_ROOT, * pINDEX_ROOT;


//INDEX_ALLOCATION 0XA0属性体
typedef struct _INDEX_ALLOCATION {
	//UINT64 IA_DataRuns;
	BYTE* IA_DataRuns = nullptr;
}INDEX_ALLOCATION, * pINDEX_ALLOCATION;