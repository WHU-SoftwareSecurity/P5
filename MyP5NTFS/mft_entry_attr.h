#pragma once
#include<Windows.h>
//��פ���Ժͷǳ�פ���ԵĹ��ò���
typedef struct _CommonAttributeHeader {
	UINT32 ATTR_Type; //��������	4
	UINT32 ATTR_Size; //����ͷ����������ܳ���	8
	BYTE ATTR_ResFlag; //�Ƿ��ǳ�פ���ԣ�0��פ 1�ǳ�פ��	
	BYTE ATTR_NamSz; //�������ĳ���	10
	UINT16 ATTR_NamOff; //��������ƫ�� ���������ͷ	12
	UINT16 ATTR_Flags; //��־��0x0001ѹ�� 0x4000���� 0x8000ϡ�裩	14
	UINT16 ATTR_Id; //����ΨһID	16
}CommonAttributeHeader, * pCommonAttributeHeader;

//��פ���� ����ͷ
typedef struct _ResidentAttributeHeader {
	UINT32 ATTR_DatSz; //�������ݵĳ���
	UINT16 ATTR_DatOff; //�����������������ͷ��ƫ��
	BYTE ATTR_Indx; //����
	BYTE ATTR_Resvd; //����
	BYTE ATTR_AttrNam[0];//��������Unicode����β��0
}ResidentAttributeHeader, * pResidentAttributeHeader;

//�ǳ�פ���� ����ͷ
typedef struct _NonResidentAttributeHeader {
	UINT64 ATTR_StartVCN; //����������������ʼ����غ� 
	UINT64 ATTR_EndVCN; //����������������ֹ����غ�
	UINT16 ATTR_DatOff; //�����б����������ͷ��ƫ��
	UINT16 ATTR_CmpSz; //ѹ����λ 2��N�η�
	UINT32 ATTR_Resvd;
	UINT64 ATTR_AllocSz; //���Է���Ĵ�С
	UINT64 ATTR_ValidSz; //���Ե�ʵ�ʴ�С
	UINT64 ATTR_InitedSz; //���Եĳ�ʼ��С
	BYTE ATTR_AttrNam[0];
}NonResidentAttributeHeader, * pNonResidentAttributeHeader;

typedef struct Value0x30
{
	UINT64 FN_ParentFR; /*��Ŀ¼��MFT��¼�ļ�¼������
							ע�⣺��ֵ�ĵ�6�ֽ���MFT��¼�ţ���2�ֽ��Ǹ�MFT��¼�����к�*/
	FILETIME FN_CreatTime;
	FILETIME FN_AlterTime;
	FILETIME FN_MFTChg;
	FILETIME FN_ReadTime;
	UINT64 FN_AllocSz;
	UINT64 FN_ValidSz;//�ļ�����ʵ�ߴ�
	UINT32 FN_DOSAttr;//DOS�ļ�����
	UINT32 FN_EA_Reparse;//��չ����������
	BYTE FN_NameSz;//�ļ������ַ���
	BYTE FN_NamSpace;/*�����ռ䣬��ֵ��Ϊ����ֵ�е�����һ��
						0��POSIX������ʹ�ó�NULL�ͷָ�����/��֮�������UNICODE�ַ���������ʹ��255���ַ���ע�⣺�������ǺϷ��ַ�����Windows������ʹ�á�
						1��Win32��Win32��POSIX��һ���Ӽ��������ִ�Сд������ʹ�ó�������������������?��������������/������<������>������/������|��֮�������UNICODE�ַ��������ֲ����ԡ�.����ո��β��
						2��DOS��DOS�����ռ���Win32���Ӽ���ֻ֧��ASCII����ڿո��8BIT��д�ַ����Ҳ�֧�������ַ�������������������?��������������/������<������>������/������|������+������,������;������=����ͬʱ���ֱ��밴���¸�ʽ������1~8���ַ���Ȼ���ǡ�.����Ȼ������1~3���ַ���
						3��Win32&DOS����������ռ���ζ��Win32��DOS�ļ����������ͬһ���ļ��������С�*/
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

//����ͷ
typedef struct _INDEX_HEADER {
	UINT32 IH_EntryOff;//��һ��Ŀ¼���ƫ��
	UINT32 IH_TalSzOfEntries;//Ŀ¼����ܳߴ�(��������ͷ�������������)
	UINT32 IH_AllocSize;//Ŀ¼�����ĳߴ�
	BYTE IH_Flags;/*��־λ����ֵ���������º�ֵ֮һ��
				  0x00       СĿ¼(���ݴ���ڸ��ڵ����������)
				  0x01       ��Ŀ¼(��ҪĿ¼��洢����������λͼ)*/
	BYTE IH_Resvd[3];
}INDEX_HEADER, * pINDEX_HEADER;

//INDEX_ROOT 0X90������
typedef struct _INDEX_ROOT {
	//������
	UINT32 IR_AttrType;//���Ե�����
	UINT32 IR_ColRule;//�������
	UINT32 IR_EntrySz;//Ŀ¼�����ߴ�
	BYTE IR_ClusPerRec;//ÿ��Ŀ¼��ռ�õĴ���
	BYTE IR_Resvd[3];
	//����ͷ
	INDEX_HEADER IH;
	//������  ���ܲ�����
	BYTE* IR_IndexEntry;
}INDEX_ROOT, * pINDEX_ROOT;


//INDEX_ALLOCATION 0XA0������
typedef struct _INDEX_ALLOCATION {
	//UINT64 IA_DataRuns;
	BYTE* IA_DataRuns = nullptr;
}INDEX_ALLOCATION, * pINDEX_ALLOCATION;