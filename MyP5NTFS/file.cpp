#include <iostream>
#include <vector>
#include "file.h"
#include "mft_entry_attr.h"
#include "mft_entry_header.h"
#include "index.h"
#include "crypto.h"


/* �������ִ������������ҵ���Ӧ��������ظ����� MFT �Ĳο���(����λ�ֽ�)*/
UINT64 ParseIndexEntry(UINT32 validLeft, BYTE* buffer, const char* name) {
	BYTE* stream = buffer;
	const char* pname;
	UINT32 read = 0;
	while (true) {
		if (read + 82 > validLeft)
			break;
		STD_INDEX_ENTRY sie = STD_INDEX_ENTRY();

		memcpy(&sie, stream, sizeof(STD_INDEX_ENTRY));
		char* fileName = (CHAR*)(stream + 82);
		int nameLength = sie.SIE_FileNameSize;
		bool flag = true;
		pname = name;
		read = read + sie.SIE_IndexEntrySize;
		for (int i = 0; i < nameLength * 2; i++) {
			if (*fileName != *pname) {
				flag = false;
				break;
			}
			fileName++;
			pname++;
		}

		if (flag) {
			/* ��6λ�ֽڣ��� MFT �ο���*/
			return (sie.SIE_MFTReferNumber & 0x0000ffffffffffff);
		}
		else {
			stream += sie.SIE_IndexEntrySize;
			continue;
		}
	}
	return 0;
}


BYTE* ParseMFTEntry(BYTE* buffer, const char* path, NTFSDisk disk, NTFSFile& file, bool tag) {
	//����'\'
	path++;
	const char* endp = path;
	char* tmpName = nullptr;

	/* tmpName ��ʾһ��Ŀ¼������ļ���*/
	while (*endp != '\\' && *endp != '\0' && *endp != '/')
		endp++;
	int PathNumber = endp - path;
	tmpName = new char[PathNumber + 1];
	memcpy(tmpName, path, PathNumber);
	tmpName[PathNumber] = '\0';
	BYTE* pTmpName = nullptr;

	if (tag)
		cout << "��������Ѱ����Ŀ¼/�ļ���" << tmpName;

	BYTE* p = buffer;
	MFT_ENTRY_HEADER meh = MFT_ENTRY_HEADER();
	//����MFTͷ
	memcpy(&meh, p, sizeof(MFT_ENTRY_HEADER));

	CommonAttributeHeader cah = CommonAttributeHeader();
	ResidentAttributeHeader rah = ResidentAttributeHeader();
	NonResidentAttributeHeader nah = NonResidentAttributeHeader();
	//��������
	UINT32 attrType;

	BYTE* attrStart = (BYTE*)p;
	//ָ���һ�����Ե�����ͷ����ʼ��ַ
	attrStart += meh.AttributeOffset;

	while (true) {
		//���������Ե�����
		//��0��ֹδ֪��bug
		memset(&cah, 0, sizeof(CommonAttributeHeader));
		/* ��פ����*/
		memset(&rah, 0, sizeof(ResidentAttributeHeader));
		/* �ǳ�פ����*/
		memset(&nah, 0, sizeof(NonResidentAttributeHeader));
		/* �����ԵĿ�ʼ�õ���������*/
		memcpy(&attrType, attrStart, 4);
		/* MFT ������־*/
		if (attrType == 0xffffffff) {
			cout << "�������!" << endl;
			break;
		}
		memcpy(&cah, attrStart, sizeof(CommonAttributeHeader));

		//��פ����: ֱ����MFT���м�¼������
		if (!cah.ATTR_ResFlag)
			memcpy(&rah, attrStart + sizeof(CommonAttributeHeader), sizeof(ResidentAttributeHeader));
		else
			memcpy(&nah, attrStart + sizeof(CommonAttributeHeader), sizeof(NonResidentAttributeHeader));
		//������������
		switch (attrType) {
			/* MFT �����Լ��京��ɲμ� https://blog.csdn.net/shimadear/article/details/89287916
			 *                     �Լ� https://blog.csdn.net/Hilavergil/article/details/82622470
			 * ��פ����: ���Ե������ܹ���1KB���ļ���¼�б���
			 * �ǳ�פ����: �ǳ�פ�����޷����ļ���¼�д�ţ����ŵ�MFT������λ��
			 * �����õ��ļ�������
			 * 80H ����: �ļ�����������
			 *		- ����ļ��Ƚϴ���MFT���޷�������������ݣ���ô����ļ���80����Ϊ�ǳ�פ���ԣ�
			 *		  ��������һ��data runs��ָ���Ÿ��ļ����ݵĴء�����ļ��ܹ�����ڳ�פ��80�����У�
			 *		  ��80���Ե������岢����data runs������ֱ�Ӵ�����ļ�����
			 * 90H ����: ���������ԣ�����Ÿ�Ŀ¼�µ���Ŀ¼�����ļ���������
			 *		- ��3���ֹ��ɣ�������������ͷ�������
			 *		  ������Щ�����90�������ǲ�����������ģ����ʱ���Ŀ¼����������A0�����е�data runsָ����
			 *		  A0���Ե������弴Ϊdata runs��ָ�����ɸ���������Ĵ���
			 * A0H ����: ָ���������ڵ㣬ÿ�������ڵ����ж�������һ���������Ӧһ���ļ�
			 *		- ��ĳ��Ŀ¼�����ݽ϶࣬�Ӷ�����90�����޷���ȫ���ʱ��A0���Ի�ָ��һ����������
			 *	      ���������������˸�Ŀ¼������ʣ�����ݵ��������������ΪA0�����Ƕ�90���ԵĲ��䣩
			 */
		case 0x80: {
			file.setCryptoFlag(meh.cryptoFlags);
			if (!cah.ATTR_ResFlag) {
				cout << "\n\n�ļ����ݹ�С��ֱ�Ӵ洢��MFT����!" << endl;
				file.setFileMftOffset(file.getFileMftOffset() / disk.GetSectorSize());
				cout << "\nMFT������ƫ��: " << file.getFileMftOffset();
			}
			else {
				file.setFileMftOffset(file.getFileMftOffset() / disk.GetSectorSize());
				cout << "\n\n�ļ�MFT������ƫ��: " << file.getFileMftOffset() << endl;
				BYTE* clusterStream = attrStart + nah.ATTR_DatOff;
				/* �����ļ�����*/
				file.setClusterStream(clusterStream);
			}
			return nullptr;
		}
		case (BYTE)0x90: {
			//90һ����פ
			//����INDEX_ROOT�ռ�
			INDEX_ROOT ir = INDEX_ROOT();
			memcpy(&ir, attrStart + rah.ATTR_DatOff, sizeof(ir));
			//����û���������Ҫ����A0
			if (ir.IR_IndexEntry == nullptr) {
				break;
			}//��������������
			else {
				int validLeft = cah.ATTR_Size - rah.ATTR_DatOff - 0x20;
				int read = 0;
				BYTE* pClusterStream = rah.ATTR_DatOff + attrStart + 0x20;
				STD_INDEX_ENTRY sie = STD_INDEX_ENTRY();
				memcpy(&sie, pClusterStream, sizeof(sie));
				//�����������ӵ�еñ���
				int bufsize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, tmpName, strlen(tmpName), nullptr, 0) * 2 + 1;
				pTmpName = new BYTE[bufsize];
				pTmpName[bufsize - 1] = 0;
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, tmpName, strlen(tmpName), (LPWSTR)pTmpName, bufsize);
				UINT64 refNum = ParseIndexEntry(validLeft, pClusterStream, (const char*)pTmpName);
				//˵����90��ƥ�䲻��
				if (refNum == 0) {
					break;
				}
				cout << "\nָ���MFT��Ϊ��" << refNum << "��, MFT���������ƫ��: "
					<< (((disk.GetMFTStartCluster() * disk.GetClusterSize() + refNum * 2 * disk.GetSectorSize())) / disk.GetSectorSize()) << endl;

				HANDLE disk_handle = GetDiskHandleR(disk.GetDiskName().c_str(), 0);
				BigFileSeek(disk_handle, (disk.GetMFTStartCluster() * disk.GetClusterSize() + refNum * 2 * disk.GetSectorSize()), FILE_BEGIN);
				//printf("%llx", (Disk::v().getMFTStartCluster() * Disk::v().getClusterSize() + refNum * 2 * Disk::v().getSectorSize()));
				BYTE* data1 = new BYTE[disk.GetSectorSize() * 2];
				DWORD readNum;
				if (!ReadFile(disk_handle, data1, disk.GetSectorSize() * 2, &readNum, NULL)) {
					cout << "��ȡMFT��ʧ��,ʧ��ԭ��: " << GetLastError() << endl;
					exit(-1);
				}
				if (*endp == '\0') {
					file.setFileMftOffset((disk.GetMFTStartCluster() * disk.GetClusterSize() + refNum * 2 * disk.GetSectorSize()));
					return data1;
				}
				else {
					//printf("90�����е�ƫ��:\nllx", (Disk::v().getMFTStartCluster() * Disk::v().getClusterSize() + refNum * 2 * Disk::v().getSectorSize()));
					return ParseMFTEntry(data1, endp, disk, file, true);
				}
			}
			break;
		}
					   //����A0���ԣ�˵��90ûƥ���ϣ�����ƥ��A0����
		case (BYTE)0xA0: {
			BYTE* pClusterStream = nah.ATTR_DatOff + attrStart;
			BYTE* data = nullptr;
			UINT64 clusterNum = 0;
			int offsetByteNum = 0;
			int clusterNumByteNum = 0;
			UINT64 clusterOffset = 0;
			DWORD readNum;
			//��ÿ���������н���
			while (*pClusterStream != 0) {
				//��ȡ��ռ�õĴصĸ���
				offsetByteNum = (*pClusterStream) >> 4;
				//��ȡ����Сռ�õĴصĸ���
				clusterNumByteNum = (*pClusterStream) & 0x0f;
				//��������Ĵ�С
				clusterNum = UCHARToLL(pClusterStream + 1, clusterNumByteNum);
				//���������ƫ��
				clusterOffset = UCHARToLL(pClusterStream + 1 + clusterNumByteNum, offsetByteNum);

				int bufsize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, tmpName, strlen(tmpName), nullptr, 0) * 2 + 1;
				pTmpName = new BYTE[bufsize];
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, tmpName, strlen(tmpName), (LPWSTR)pTmpName, bufsize);
				pTmpName[bufsize - 1] = 0;

				HANDLE disk_handle = GetDiskHandleR(disk.GetDiskName().c_str(), 0);
				BigFileSeek(disk_handle, clusterOffset * disk.GetClusterSize(), FILE_BEGIN);
				/* data ���������洢�ļ�������*/
				if (data == nullptr) {
					data = new BYTE[clusterNum * disk.GetClusterSize()];
				}
				if (!ReadFile(disk_handle, data, clusterNum * disk.GetClusterSize(), &readNum, NULL)) {
					cout << "��ȡMFT��ʧ��,ʧ��ԭ��: " << GetLastError() << endl;
					exit(-1);
				}
				int tag = clusterNum;
				BYTE* origin_data = data;
				for (int i = 0; i < tag; i++) {
					data = origin_data + disk.GetClusterSize() * i;
					STD_INDEX_HEADER ih = STD_INDEX_HEADER();
					memcpy(&ih, data, sizeof(STD_INDEX_HEADER));
					//�����������ӵ�еñ���

					UINT64 refNum = ParseIndexEntry(ih.SIH_IndexEntrySize - (ih.SIH_IndexEntryOffset + 24), data + ih.SIH_IndexEntryOffset + 24, (const char*)pTmpName);
					//���������������Ҫ�ı���
					if (refNum == 0) {
						continue;
					}
					cout << "\nָ���MFT��Ϊ��" << refNum << "��, MFT���������ƫ��: "
						<< ((disk.GetMFTStartCluster() * disk.GetClusterSize() + refNum * 2 * disk.GetSectorSize()) / disk.GetSectorSize()) << endl;
					BigFileSeek(disk_handle, (disk.GetMFTStartCluster() * disk.GetClusterSize() + refNum * 2 * disk.GetSectorSize()), FILE_BEGIN);
					//	printf("\nA0�����е�ƫ��:%llx", (Disk::v().getMFTStartCluster() * Disk::v().getClusterSize() + refNum * 2 * Disk::v().getSectorSize()));
					BYTE* data1 = new BYTE[disk.GetSectorSize() * 2];
					if (!ReadFile(disk_handle, data1, disk.GetSectorSize() * 2, &readNum, NULL)) {
						printf_s("��ȡMFT��ʧ��,ʧ��ԭ��: %d", GetLastError());
						exit(-1);
					}
					if (*endp == '\0') {
						file.setFileMftOffset((disk.GetMFTStartCluster() * disk.GetClusterSize() + refNum * 2 * disk.GetSectorSize()));
						return data1;
					}
					else {
						return ParseMFTEntry(data1, endp, disk, file, true);
					}
				}
				//��ָ����ǰ��1����ʶ�ֽڣ�x��ƫ���ֽں�y�ʹ�С�ֽ�
				pClusterStream = pClusterStream + 1 + offsetByteNum + clusterNumByteNum;
			}
			break;
		}
					   break;
		}
		attrStart += cah.ATTR_Size;
	}
	return nullptr;
}


void GetNTFSFileInfo(const char* file_path, NTFSFile &file) {
	char disk_name[7] = "\\\\.\\?:";
	disk_name[4] = file_path[0];

	file.setTargetPath(file_path);
	NTFSDisk disk = GetNTFSDiskInfo(disk_name);

	/* ������Ŀ¼���Ŀ¼��λ�� MFT �ĵ�����*/
	long long int root_directory_offset = disk.GetMFTStartCluster() * disk.GetClusterSize()
		+ disk.GetSectorSize() * 10;

	HANDLE disk_handle = GetDiskHandleR(disk_name, root_directory_offset);
	BigFileSeek(disk_handle, root_directory_offset, FILE_BEGIN);
	/* ��ȡ��Ŀ¼�� mft*/
	UCHAR* root_directory = new UCHAR[1024];
	GetDiskRootMFT(disk_handle, root_directory);

	cout << "\n\n>>>>>>>>>>>>>>>>>>>>��ʼ����<<<<<<<<<<<<<<<<<<<<<<\n" << endl;
	//�ݹ����
	BYTE* fileData = ParseMFTEntry(root_directory, file.getTargetPath() + 2, disk, file, true);
	if (!fileData) {
		cout << "\n\n·������ʧ��!" << endl;
		exit(-1);
	}
	ParseMFTEntry(fileData, file.getTargetPath() + 2, disk, file, false);
	cout << "\n\n>>>>>>>>>>>>>>>>>>>>�������<<<<<<<<<<<<<<<<<<<<<<\n" << endl;

	CloseHandle(disk_handle);
}



void ModifyMFT(BYTE* buffer, bool flag, NTFSFile& file) {
	BYTE* p = buffer;
	MFT_ENTRY_HEADER meh = MFT_ENTRY_HEADER();
	//����MFTͷ
	memcpy(&meh, p, sizeof(MFT_ENTRY_HEADER));
	CommonAttributeHeader cah = CommonAttributeHeader();
	ResidentAttributeHeader rah = ResidentAttributeHeader();
	NonResidentAttributeHeader nah = NonResidentAttributeHeader();
	//��������
	UINT32 attrType;
	BYTE* attrStart = (BYTE*)p;
	//ָ���һ�����Ե�����ͷ����ʼ��ַ
	attrStart += meh.AttributeOffset;

	while (true) {
		//���������Ե�����
		//��0��ֹδ֪��bug
		memset(&cah, 0, sizeof(CommonAttributeHeader));
		memset(&rah, 0, sizeof(ResidentAttributeHeader));
		memset(&nah, 0, sizeof(NonResidentAttributeHeader));
		memcpy(&attrType, attrStart, 4);
		if (attrType == 0xffffffff) {
			return;
			printf_s("\n�������!\n");
			break;
		}
		memcpy(&cah, attrStart, sizeof(CommonAttributeHeader));

		//��פ����
		if (!cah.ATTR_ResFlag)
			memcpy(&rah, attrStart + sizeof(CommonAttributeHeader), sizeof(ResidentAttributeHeader));
		else
			memcpy(&nah, attrStart + sizeof(CommonAttributeHeader), sizeof(NonResidentAttributeHeader));
		//������������
		switch (attrType) {
		case 0x80: {
			if (!cah.ATTR_ResFlag) {
				if (flag) {
					int size = cah.ATTR_Size - rah.ATTR_DatOff;
					BYTE* result = RC4_for_MTR_data(attrStart + rah.ATTR_DatOff, size, file.getKey());
					printf("\n�ļ�ʮ���������ݣ�\n");
					for (int i = 0; i < size; i++) {
						if (i % 0x10 == 0)
							printf("\n");
						printf("%4x", *(attrStart + rah.ATTR_DatOff + i));
					}
					memcpy(attrStart + rah.ATTR_DatOff, result, size);
					free(result);
				}
				else {
					int size = cah.ATTR_Size - rah.ATTR_DatOff;
					BYTE* result = RC4_for_MTR_data(attrStart + rah.ATTR_DatOff, size, file.getKey());
					printf("\n�ļ�ʮ����������:");
					for (int i = 0; i < size; i++) {
						if (i % 0x10 == 0)
							printf("\n");
						printf("%4x", *(attrStart + rah.ATTR_DatOff + i));
					}
					memcpy(attrStart + rah.ATTR_DatOff, result, size);
					free(result);
				}
				return;
			}
		}
				 break;
		}
		attrStart += cah.ATTR_Size;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
/* FAT32*/

typedef struct _FAT32FDT {
    char content[32];
}FAT32FDT;

//���ļ�Ŀ¼��32�ֽ�
typedef struct _FAT32shortFDT {
    uint8_t filename[8];//��һ�����ļ���
    uint8_t extname[3];//�ļ���չ��
    uint8_t attr;//���� 0F��˵���ǳ��ļ���Ҫ��������0F��Ȼ���Ŷ�����
    uint8_t reserve;
    uint8_t time1;
    uint8_t creattime[2];
    uint8_t createdate[2];
    uint8_t visittime[2];
    uint8_t high_cluster[2];//�ļ���ʼ�غŸ�16λ
    uint8_t changetim2[2];
    uint8_t changedate[2];
    uint8_t low_cluster[2];//�ļ���ʼ�غŵ�16λ
    uint8_t filelen[4];//�ļ�����
}FAT32shortFDT;

typedef struct _FAT32longFDT {
    char flag;//�����0x4*,��6λ��λ�ˣ�˵�������һ�����ļ�Ŀ¼����λ�����滹�м���
    char name1[10];
    char attr;//����ǳ��ļ���������������һ��������0F
    char reserve;
    char checksum;
    char name2[12];
    char rel_cluster[2];//�����ʼ�غ�
    char name3[4];
}FAT32longFDT;

//�ļ������ܺܳ�����������ļ���
struct FAT32LongFDTList {
    FAT32longFDT lfdt;
    struct FAT32LongFDTList* up;//����ģ��͵�ַ��
    struct FAT32LongFDTList* down;
};

struct findfilepath {
    char prename[100];
    char rearname[10];
    int flag;//1�Ƕ��ļ���2�ǳ��ļ���3�Ƕ�Ŀ¼��4�ǳ�Ŀ¼;��3/4��next�ǿ�
    struct findfilepath* next;
};


uint16_t uint8to16(uint8_t twouint8[2]) {
    return *(uint16_t*)twouint8;
}

uint32_t uint8to32(uint8_t fouruint8[4]) {
    return *(uint32_t*)fouruint8;
}


//�򵥵�Ӣ�ĵ�����תһ��,�����������������00�����־ͱ�ʾ�Ѿ�����β��
//�ļ������ִ�Сд�ģ�ȫ��toupper���ǵ��Ǹ�'.'���������Ǵ��������!!!!�Ѷ������ĵ�ɾ��
void double2single(char* filename, char* upperfilename, int num) {
    int j = 0;
    for (int i = 0; i < num; i = i + 2)
    {    //�������ĸ�����ط��㣬���򷵻��㣡����
        if (isalpha(filename[i]) == 0)
        {
            upperfilename[j] = filename[i];
        }
        else
            upperfilename[j] = toupper(filename[i]);
        j++;
    }
}
//�ļ������ִ�Сд�ģ�ȫ��toupper
void short2upper(char* filename) {
    for (int i = 0; i < strlen(filename); i++)
        filename[i] = toupper(filename[i]);
}

//��һ������������Ѿ������ļ�β����ν����������ļ�������ô����0[[����û��б����'/']
//����Ҫ�ٴε��ý�������Ŀ¼����1[���滹��һ��б��]
//�����������-1
//�������1������0����һ�ε�Ŀ¼/�ļ�����������nextname����[�ɵ�����ά��������鳤��]
//indexptr���������浱ǰ�������������һ��ֱ�Ӽ���[������ά��]
int nextpath(char* filepath, char* nextname, int* indexptr) {
    int flag = -1;
    int firstindex = 0, lastindex = 0;
    int filepathlen = strlen(filepath);
    short2upper(filepath);
    if (*indexptr >= filepathlen)//6���ռ䣬�������Ϊ5
        return -1;
    //�����ļ���
    while (1) {
        //�ж��Ƿ�������߳���
        if (*indexptr >= filepathlen && flag == -1)//�������
        {
            flag = -1;//������
            break;
        }
        else if (*indexptr == filepathlen && flag == 1) {
            flag = 0;//������,���赽�ļ���
            break;
        }

        if (filepath[*indexptr] == ':')//�����һ��ʼ��ð��,���������
        {
            ;
        }
        if (filepath[*indexptr] == '/' && flag == -1)//����ǵ�һ����б��,��־��ʼ
        {
            if ((*indexptr) + 1 < filepathlen) {
                firstindex = (*indexptr) + 1;
                //printf("firstindex = %d\n",firstindex);
                flag = 1;//��ʱ����1
            }
            else {
                flag = -1; break;//������
            }
        }
        else if (filepath[*indexptr] == '/' && flag != -1)//������һ����
        {
            lastindex = (*indexptr) - 1;//��β��
            //printf("lastindex = %d\n",lastindex);
            break;
        }
        (*indexptr)++;
    }
    if (flag != -1)
    {
        memset(nextname, 0, sizeof(nextname));
        if (flag == 1)//Ŀ¼
            strncpy(nextname, &filepath[firstindex], lastindex - firstindex + 1);//�����ַ���
        else
            strncpy(nextname, &filepath[firstindex], filepathlen - firstindex);//�����ַ���
        //printf("next name=%s\n",nextname);
    }
    return flag;
}

//���ļ������Ϊ�ļ����ͺ�׺������׺������3�ģ�Ҳ�����ó��ļ�����ʽ;���пո��Ҳ���ǳ��ļ���
int dividefilename(char* filename, char* prename, char* rearname) {
    //printf("dividename is %s %d\n",filename,strlen(filename));
    int dotindex = 0;
    for (int i = 0; i < strlen(filename); i++) {
        if (filename[i] == '.')//�ҵ��ֽ���
        {
            dotindex = i;
            //printf("dotindex = %d\n",dotindex);
            break;//���������ǶԵ�
        }
    }
    if (dotindex == 0)
        return -1;
    std::strncpy(prename, filename, dotindex);//����ǰ���ļ���
    std::strncpy(rearname, &filename[dotindex + 1], strlen(filename) - dotindex - 1);
    //printf("copy filename: %s   %s  strlen=%d  %d\n",prename,rearname,strlen(prename),strlen(rearname));

    return 0;
}

void getpathlist(char* filepath, struct findfilepath*& head, int* filenameindex) {
    struct findfilepath* p = NULL, * rear = NULL;
    int t;//��ľ�н���
    char nextname[100];
    char prename[100];
    char rearname[10];
    int padding = 0;
    char paddingnum = 0x20;
    while (1) {
        memset(nextname, 0, sizeof(nextname));
        memset(prename, 0, sizeof(prename));
        memset(rearname, 0, sizeof(rearname));
        t = nextpath(filepath, nextname, filenameindex);
        if (t != -1)//��������ǵõ�Ŀ¼��
        {
            if (t == 1)//Ŀ¼��
            {
                printf("Ŀ¼Ϊ%s  ����Ϊ%d\n", nextname, strlen(nextname));
                p = (struct findfilepath*)malloc(sizeof(struct findfilepath));
                p->next = NULL;
                memset(p->prename, 0, sizeof(p->prename));
                memset(p->rearname, 0, sizeof(p->rearname));

                if (strlen(nextname) > 8)//��Ҫ���ļ���
                {
                    p->flag = 4;//��Ŀ¼
                    strncpy(p->prename, nextname, strlen(nextname));//��Ŀ¼ֱ�Ӵ�
                    //printf("��Ŀ¼ %s %d\n",p->prename,strlen(p->prename));
                }
                else
                {
                    p->flag = 3;//��Ŀ¼
                    strncpy(p->prename, nextname, strlen(nextname));//��Ŀ¼��0x20
                    paddingnum = 8 - strlen(p->prename);
                    if (paddingnum > 0)
                        memset(&p->prename[strlen(p->prename)], 0x20, paddingnum);
                    //printf("��Ŀ¼ %s %d\n",p->prename,strlen(p->prename));
                }
                if (rear == NULL)
                {
                    head = p; rear = p;
                }
                else
                {
                    rear->next = p; rear = p;
                }
            }
            else//t==0�����ļ���
            {
                printf("�ļ�Ϊ%s  ����Ϊ%d\n", nextname, strlen(nextname));
                p = (struct findfilepath*)malloc(sizeof(struct findfilepath));
                p->next = NULL;
                memset(p->prename, 0, sizeof(p->prename));//���ĵ�ԭ���߰������sizeofд����strlen�������һ���5��Сʱ��bug
                memset(p->rearname, 0, sizeof(p->rearname));
                dividefilename(nextname, prename, rearname);
                printf("\n");
                if (strlen(prename) > 8 || strlen(rearname) > 3)//��Ҫ���ļ���
                {
                    p->flag = 2;//���ļ�
                    strncpy(p->prename, prename, strlen(prename));
                    strncpy(p->rearname, rearname, strlen(rearname));
                    //printf("���ļ� %s.%s\n",p->prename,p->rearname);
                }
                else
                {
                    p->flag = 1;//���ļ�
                    strncpy(p->prename, prename, strlen(prename));
                    paddingnum = 8 - strlen(p->prename);
                    //                    printf("padnum=%d\n",paddingnum);
                    if (paddingnum > 0)
                    {
                        memset(&p->prename[strlen(p->prename)], 0x20, paddingnum);
                        //                        for(int i =0;i<8;i++)
                        //                            printf("%x",p->prename[i]);
                    }

                    strncpy(p->rearname, rearname, strlen(rearname));
                    paddingnum = 3 - strlen(p->rearname);
                    if (paddingnum > 0)
                    {
                        memset(&p->rearname[strlen(p->rearname)], 0x20, paddingnum);
                        //                        printf("%x",p->rearname[strlen(p->rearname)]);
                    }
                    //                    printf("���ļ� %s.%s\n",p->prename,p->rearname);
                }
                if (rear == NULL)
                {
                    head = p; rear = p;
                }
                else
                {
                    rear->next = p; rear = p;
                }
            }
        }
        else
            break;
    }
}


//�������ļ���
vector<uint64_t> readshortfileFDT(FAT32shortFDT the_short_FDT, ULONGLONG FAT1_reladdr, HANDLE hDevice) {
    vector<uint64_t> clusters;
    
    uint8_t cluster[4] = { the_short_FDT.low_cluster[0],the_short_FDT.low_cluster[1],
                           the_short_FDT.high_cluster[0],the_short_FDT.high_cluster[1] };
    uint32_t firstcluster = uint8to32(cluster);//��ʼ�غ�
    clusters.push_back(firstcluster);
    //printf("�ļ���ʼ�غ���%08X\n", firstcluster);
    //printf("����:\n%08X\n", firstcluster);
    int next_sector = firstcluster / 128;//�ڵڼ���������512*beg_sector
    int next_byte = (firstcluster % 128) * 4;
    uint32_t FATentry = 0;
    uint8_t Buffer[512] = { 0 };
    int needreadmore = 1;
    int seqindex = 0;
    BOOL bRet;
    DWORD dwCB;

    LARGE_INTEGER offset;
    offset.QuadPart = FAT1_reladdr + (ULONGLONG)(512 * next_sector);
    SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);
    if (!ReadFile(hDevice, Buffer, 512, &dwCB, NULL)) {
        cout << "��ȡ������Ϣʧ��: " << GetLastError() << endl;
    }

    while (needreadmore == 1) {
        FATentry = *(uint32_t*)&Buffer[next_byte];
        //printf("%08X\n", FATentry);
        clusters.push_back(FATentry);
        if (FATentry != 0x0FFFFFFF)//��δ����
        {
            if (next_sector != FATentry / 128)
            {
                next_sector = FATentry / 128;
                offset.QuadPart = FAT1_reladdr + (ULONGLONG)(512 * next_sector);
                SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);
                memset(Buffer, 0, 512);
                bRet = ReadFile(hDevice, Buffer, 512, &dwCB, NULL);
            }
            next_byte = (FATentry % 128) * 4;
        }
        else
            needreadmore = 0;
    }

    return clusters;
}

//������Ŀ¼����ͨ������ڸ�Ŀ¼���ҵ����Ե�ַ
ULONGLONG readshortdirFDT(FAT32shortFDT the_short_FDT, ULONGLONG root_reladdr, HANDLE hDevice, int secotrs_per_cluster) {
    uint8_t cluster[4] = { the_short_FDT.low_cluster[0],the_short_FDT.low_cluster[1],
                           the_short_FDT.high_cluster[0],the_short_FDT.high_cluster[1] };
    uint32_t firstcluster = uint8to32(cluster);//��ʼ�غ�
    printf("Ŀ¼��ʼ�غ���%08X\n", firstcluster);
    ULONGLONG nextaddr = (ULONGLONG)((firstcluster - 2) * secotrs_per_cluster * 512) + root_reladdr;
    return nextaddr;
}

vector<uint64_t> findfile(struct findfilepath* head, char* partition) {
    struct findfilepath* p = NULL;

    DISK_GEOMETRY* pdg = nullptr;            // ������̲����Ľṹ��
    BOOL bResult;                 // generic results flag
    ULONGLONG DiskSize;           // size of the drive, in bytes
    HANDLE hDevice;               // �豸���
    DWORD junk;                   // discard resultscc
    bool bRet;
    //ͨ��CreateFile������豸�ľ�����򿪶�Ӧ����
    hDevice = GetDiskHandleR(partition, 0);

    //ͨ��DeviceIoControl�������豸����IO��Ϊ������׼��
    bResult = DeviceIoControl(hDevice, // �豸�ľ��
        IOCTL_DISK_GET_DRIVE_GEOMETRY, // �����룬ָ���豸������
        NULL, 0, // no input buffer
        pdg,
        sizeof(*pdg),     // output buffer �����������̲�����Ϣ
        &junk,                 // # bytes returned
        (LPOVERLAPPED)NULL); // synchronous I/O

    LARGE_INTEGER offset;//��ȡλ��
    offset.QuadPart = (ULONGLONG)0;//0
    SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);//�����λ�ÿ�ʼ����DBR��FILE_BEGIN�����λ�ƣ�����

    DWORD dwCB;
    FAT32Dbr the_DBR;
    //�����λ�ÿ�ʼ��DBR��һ��ʼ��512�ֽ���Щ��Ϣ����
    if (!ReadFile(hDevice, &the_DBR, 512, &dwCB, NULL)) {
        cout << "��ȡ������Ϣʧ��: " << GetLastError() << endl;
    }
    ULONGLONG FAT1_reladdr = (ULONGLONG)uint8to16(the_DBR.reserve_sectors) *
        (ULONGLONG)512;//�õ�FAT1�ľ����ַ������ƫ����Ҫ�����ƫ��

    ULONGLONG root_reladdr = FAT1_reladdr + (ULONGLONG)(the_DBR.FATnum[0]) *
        (ULONGLONG)uint8to32(the_DBR.sectors_per_FAT) * (ULONGLONG)512;//��Ŀ¼����ʼ���λ��,��Ŀ¼���ڵ�[01]2��

    offset.QuadPart = root_reladdr;
    SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);
    
    UCHAR lpBuffer[512] = { 0 };
    if (!ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL)) {
        cout << "��ȡ������Ϣʧ��: " << GetLastError() << endl;
    }

    
    //show_onesector(lpBuffer);

    char prename[101] = { 0 };
    char rearname[10] = { 0 };
    int needreadmore = 0;//����һ��Ŀ¼��Ҫ��������������û���ҵ��ļ�/ǡ�ñ����ļ��ָ���

    //1��ֻ�ܶ�512�ֽ�������,16��FDT,ÿ��32�ֽ�
    FAT32FDT tempFDT[16] = { 0 }, temp_tempFDT[16] = { 0 };//����ĸ���Ŀ¼���ļ�׼��
    FAT32shortFDT the_short_FDT = { 0 };
    FAT32longFDT the_long_FDT[10];
    int longfdtindex = 0;//������
    int index = 0;
    int tempindex = 0;
    LARGE_INTEGER temp_offset;//��ʱ�����ҳ��ļ����
    int findflag = 0, mayfindflag = 0;
    p = head;
    int i = 0;
    while (p != NULL)//��û���ҵ���һ�����
    {
        memset(tempFDT, 0, sizeof(tempFDT));
        memcpy(tempFDT, lpBuffer, 512);

        if (p->next == NULL && (p->flag == 1 || p->flag == 2))//�������һ�����ļ�
        {
            if (p->flag == 1)//�Ƕ��ļ�,�ҵ��Ժ�����ö��ļ�����
            {
                findflag = 0; index = 0;
                while (findflag == 0) {
                    if (tempFDT[index].content[0] == 0x00) {
                        printf("\n·������!\n");
                        return vector<uint64_t>();
                    }
                    if (tempFDT[index].content[11] != 0x0F)
                    {
                        memset(prename, 0, sizeof(prename));
                        memset(rearname, 0, sizeof(rearname));
                        strncpy(prename, &tempFDT[index].content[0], 8);
                        strncpy(rearname, &tempFDT[index].content[8], 3);
                        if (!strcmp(prename, p->prename) && !strcmp(rearname, p->rearname))
                        {
                            findflag = 1;
                            memcpy(&the_short_FDT, &tempFDT[index], 32);
                            return readshortfileFDT(the_short_FDT, FAT1_reladdr, hDevice);
                        }
                    }
                    if (index < 15 && findflag == 0)//û�ҵ�
                        index++;
                    else if (index == 15 && findflag == 0)
                    {
                        index = 0;
                        memset(tempFDT, 0, sizeof(tempFDT));
                        memset(lpBuffer, 0, sizeof(lpBuffer));
                        offset.QuadPart = offset.QuadPart + (ULONGLONG)512;
                        SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);
                        bRet = ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL);
                        memcpy(tempFDT, lpBuffer, 512);
                    }
                }
            }
            else if (p->flag == 2)
            {
                while (findflag == 0) {
                    if (tempFDT[index].content[0] == 0x00) {
                        printf("\n·������!\n");
                        return vector<uint64_t>();
                    }
                    if ((tempFDT[index].content[11] & 0x10) == 0)//�ļ���,�������ļ���ͷ�ĸ��Բ���
                    {
                        memset(prename, 0, sizeof(prename));
                        memcpy(prename, &tempFDT[index].content[0], 8);
                        if (!strnicmp(prename, p->prename, 4))
                        {
                            memset(&the_short_FDT, 0, 32);
                            memcpy(&the_short_FDT, &tempFDT[index], 32);//������ų�Ŀ¼�ĵ�ַ��Ϣ
                            tempindex = index;//����Ӱ�컷����������
                            memset(prename, 0, sizeof(prename));
                            memcpy(temp_tempFDT, tempFDT, sizeof(tempFDT));
                            while (1) {
                                if (tempindex > 0)//�����ʱ����Ҫ����
                                {
                                    tempindex--;
                                    if (temp_tempFDT[tempindex].content[11] == 0x0F)//��ȷû��
                                    {
                                        memset(&the_long_FDT[longfdtindex], 0, 32);
                                        memcpy(&the_long_FDT[longfdtindex], &temp_tempFDT[tempindex], 32);
                                        longfdtindex++;//��һ������
                                    }
                                    else//�����ˣ������ַ���
                                    {
                                        for (int i = 0, k = 0; i < longfdtindex; i++)
                                        {
                                            for (int j = 0; j < 3; j++)//����������
                                            {
                                                if (j == 0) {
                                                    double2single(the_long_FDT[i].name1,
                                                        &prename[k], 10);
                                                    k = k + 5;
                                                }
                                                else if (j == 1) {
                                                    double2single(the_long_FDT[i].name2,
                                                        &prename[k], 12);
                                                    k = k + 6;
                                                }
                                                else {
                                                    double2single(the_long_FDT[i].name3,
                                                        &prename[k], 4);
                                                    k = k + 2;
                                                }

                                            }
                                        }
                                        //printf("�ļ�����  %s.%s\n", p->prename, p->rearname);
                                        //printf("ƴ���ļ�����  %s\n", prename);
                                        if (!strnicmp(prename, p->prename, strlen(p->prename)) &&
                                            !strnicmp(prename + strlen(p->prename) + 1, p->rearname,
                                                strlen(p->rearname)))
                                        {
                                            findflag = 1;
                                            goto findlongname;
                                        }
                                        else
                                        {
                                            findflag = 0;
                                            goto noyet;
                                        }
                                    }
                                }
                                else {//���˻�ȥ��������
                                    tempindex = 16;//����һ�����Ҫ��һ�μ���
                                    memset(temp_tempFDT, 0, sizeof(temp_tempFDT));
                                    memset(lpBuffer, 0, sizeof(lpBuffer));
                                    temp_offset.QuadPart = offset.QuadPart - (ULONGLONG)512;
                                    SetFilePointer(hDevice, temp_offset.LowPart, &temp_offset.HighPart, FILE_BEGIN);
                                    bRet = ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL);
                                    memcpy(temp_tempFDT, lpBuffer, 512);
                                }
                            }	//end of while(1) �ҳ��ļ���Ŀ¼��
                        }//end of if �ļ�ǰ�ĸ���ͬ
                    noyet://������ǣ���Ȼ�ļ���ǰ�ĸ���ͬ
                        longfdtindex = 0;
                    findlongname://�ǵģ��ҵ���
                        if (findflag == 1)
                            return readshortfileFDT(the_short_FDT, FAT1_reladdr, hDevice);

                    }//end of if �����һ�����ļ�Ŀ¼��
                    if (index < 15 && findflag == 0)//û�ҵ�
                        index++;
                    else if (index == 15 && findflag == 0)
                    {
                        index = 0;
                        memset(tempFDT, 0, sizeof(tempFDT));
                        memset(lpBuffer, 0, sizeof(lpBuffer));
                        offset.QuadPart = offset.QuadPart + (ULONGLONG)512;
                        SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);
                        bRet = ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL);
                        memcpy(tempFDT, lpBuffer, 512);
                    }
                }//end of while(findflag=0)	�Ѿ��ҵ��ˣ�������
            }//���ļ������ö���
        }
        else if (p->next != NULL && (p->flag == 3 || p->flag == 4))
        {

            if (p->flag == 3)//����Ƕ�Ŀ¼
            {
                findflag = 0; index = 0;
                i = 0;
                while (findflag == 0) {
                    if (tempFDT[index].content[0] == 0x00) {
                        printf("\n·������!\n");
                        return vector<uint64_t>();
                    }
                    //ע�������˳��
                    if ((tempFDT[index].content[11] & 0x10) != 0)//��Ŀ¼��
                    {
                        memset(prename, 0, sizeof(prename));
                        memcpy(prename, &tempFDT[index].content[0], 8);
                        if (!strcmp(prename, p->prename))
                        {
                            findflag = 1;
                            memset(&the_short_FDT, 0, 32);
                            memcpy(&the_short_FDT, &tempFDT[index], 32);
                            //����
                            offset.QuadPart = readshortdirFDT(the_short_FDT, root_reladdr,
                                hDevice, the_DBR.sectors_per_cluster[0]);
                        }
                    }
                    if (index < 15 && findflag == 0)//û�ҵ�
                        index++;
                    else if (index == 15 && findflag == 0)
                    {
                        index = 0;
                        memset(tempFDT, 0, sizeof(tempFDT));
                        memset(lpBuffer, 0, sizeof(lpBuffer));
                        offset.QuadPart = offset.QuadPart + (ULONGLONG)512;
                        SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);
                        bRet = ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL);
                    }
                }
            }
            else if (p->flag == 4)//��Ŀ¼���е����ܣ�����,��������ͷ4���ֽڣ���������д��
            {

                while (findflag == 0) {
                    if (tempFDT[index].content[0] == 0x00) {
                        printf("\n·������!\n");
                        return vector<uint64_t>();
                    }
                    if ((tempFDT[index].content[11] & 0x10) != 0)//Ŀ¼��������ļ���ͷ�ĸ��Բ���
                    {
                        memset(prename, 0, sizeof(prename));
                        memcpy(prename, &tempFDT[index].content[0], 8);
                        if (!strnicmp(prename, p->prename, 4))
                        {
                            memset(&the_short_FDT, 0, 32);
                            memcpy(&the_short_FDT, &tempFDT[index], 32);//������ų�Ŀ¼�ĵ�ַ��Ϣ
                            tempindex = index;//����Ӱ�컷����������
                            memset(prename, 0, sizeof(prename));
                            memcpy(temp_tempFDT, tempFDT, sizeof(tempFDT));
                            while (1) {
                                if (tempindex > 0)//�����ʱ����Ҫ����
                                {
                                    tempindex--;
                                    if (temp_tempFDT[tempindex].content[11] == 0x0F)//��ȷû��
                                    {
                                        memset(&the_long_FDT[longfdtindex], 0, 32);
                                        memcpy(&the_long_FDT[longfdtindex], &temp_tempFDT[tempindex], 32);
                                        longfdtindex++;//��һ������
                                    }
                                    else//�����ˣ������ַ���
                                    {
                                        for (int i = 0, k = 0; i < longfdtindex; i++)
                                        {
                                            for (int j = 0; j < 3; j++)//����������
                                            {
                                                if (j == 0) {
                                                    double2single(the_long_FDT[i].name1,
                                                        &prename[k], 10);
                                                    k = k + 5;
                                                }
                                                else if (j == 1) {
                                                    double2single(the_long_FDT[i].name2,
                                                        &prename[k], 12);
                                                    k = k + 6;
                                                }
                                                else {
                                                    double2single(the_long_FDT[i].name3,
                                                        &prename[k], 4);
                                                    k = k + 2;
                                                }
                                            }
                                        }
                                        //printf("�ļ�Ŀ¼��  %s\n", p->prename);
                                        //printf("ƴ��Ŀ¼��  %s\n", prename);
                                        if (!strcmp(prename, p->prename))
                                        {
                                            findflag = 1;
                                            goto finddir;
                                        }
                                        else
                                        {
                                            findflag = 0;
                                            goto notyet;
                                        }
                                    }
                                }
                                else {//���˻�ȥ��������
                                    tempindex = 16;//����һ�����Ҫ��һ�μ���
                                    memset(temp_tempFDT, 0, sizeof(temp_tempFDT));
                                    memset(lpBuffer, 0, sizeof(lpBuffer));
                                    temp_offset.QuadPart = offset.QuadPart - (ULONGLONG)512;
                                    SetFilePointer(hDevice, temp_offset.LowPart, &temp_offset.HighPart, FILE_BEGIN);
                                    bRet = ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL);
                                    memcpy(temp_tempFDT, lpBuffer, 512);
                                }
                            }
                        }//end of while(1) �ҳ��ļ���Ŀ¼��
                    notyet://������ǣ���Ȼ�ļ���ǰ�ĸ���ͬ
                        longfdtindex = 0;
                    finddir://�ǵģ��ҵ���
                        if (findflag == 1)
                            offset.QuadPart = readshortdirFDT(the_short_FDT, root_reladdr,
                                hDevice, the_DBR.sectors_per_cluster[0]);

                    }//end of if �����һ�����ļ�Ŀ¼��
                    if (index < 15 && findflag == 0)//û�ҵ�
                        index++;
                    else if (index == 15 && findflag == 0)
                    {
                        index = 0;
                        memset(tempFDT, 0, sizeof(tempFDT));
                        memset(lpBuffer, 0, sizeof(lpBuffer));
                        offset.QuadPart = offset.QuadPart + (ULONGLONG)512;
                        SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);
                        bRet = ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL);
                        memcpy(tempFDT, lpBuffer, 512);
                    }
                }//end of while(findflag=0)	�Ѿ��ҵ��ˣ�������
            }//end of if ����ǵ�ǰ�������ǳ��ļ�Ŀ¼

            //!!!��Ŀ¼����������һ������!!!�����渴�ƻ���
            memset(tempFDT, 0, sizeof(tempFDT));
            memset(lpBuffer, 0, sizeof(lpBuffer));
            SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);
            bRet = ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL);
            memcpy(tempFDT, lpBuffer, 512);
        }
        findflag = 0;
        p = p->next;//��һ��·��
    }

    CloseHandle(hDevice);//����򿪳ɹ�����Ҫ�ر�
}

void freepathlist(struct findfilepath*& head) {
    struct findfilepath* p = NULL;
    p = head;
    while (p != NULL) {
        p = p->next;
        free(p);
    }
}