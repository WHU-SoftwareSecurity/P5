#include <iostream>
#include <vector>
#include "file.h"
#include "mft_entry_attr.h"
#include "mft_entry_header.h"
#include "index.h"
#include "crypto.h"


/* 根据名字从索引区域中找到对应的项，并返回该其中 MFT 的参考号(高六位字节)*/
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
			/* 高6位字节，即 MFT 参考号*/
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
	//跳过'\'
	path++;
	const char* endp = path;
	char* tmpName = nullptr;

	/* tmpName 表示一个目录项或者文件名*/
	while (*endp != '\\' && *endp != '\0' && *endp != '/')
		endp++;
	int PathNumber = endp - path;
	tmpName = new char[PathNumber + 1];
	memcpy(tmpName, path, PathNumber);
	tmpName[PathNumber] = '\0';
	BYTE* pTmpName = nullptr;

	if (tag)
		cout << "现在正在寻找子目录/文件：" << tmpName;

	BYTE* p = buffer;
	MFT_ENTRY_HEADER meh = MFT_ENTRY_HEADER();
	//解析MFT头
	memcpy(&meh, p, sizeof(MFT_ENTRY_HEADER));

	CommonAttributeHeader cah = CommonAttributeHeader();
	ResidentAttributeHeader rah = ResidentAttributeHeader();
	NonResidentAttributeHeader nah = NonResidentAttributeHeader();
	//属性类型
	UINT32 attrType;

	BYTE* attrStart = (BYTE*)p;
	//指向第一个属性的属性头的起始地址
	attrStart += meh.AttributeOffset;

	while (true) {
		//解析出属性的类型
		//清0防止未知的bug
		memset(&cah, 0, sizeof(CommonAttributeHeader));
		/* 常驻属性*/
		memset(&rah, 0, sizeof(ResidentAttributeHeader));
		/* 非常驻属性*/
		memset(&nah, 0, sizeof(NonResidentAttributeHeader));
		/* 从属性的开始得到属性类型*/
		memcpy(&attrType, attrStart, 4);
		/* MFT 结束标志*/
		if (attrType == 0xffffffff) {
			cout << "解析完毕!" << endl;
			break;
		}
		memcpy(&cah, attrStart, sizeof(CommonAttributeHeader));

		//常驻属性: 直接在MFT项中记录属性体
		if (!cah.ATTR_ResFlag)
			memcpy(&rah, attrStart + sizeof(CommonAttributeHeader), sizeof(ResidentAttributeHeader));
		else
			memcpy(&nah, attrStart + sizeof(CommonAttributeHeader), sizeof(NonResidentAttributeHeader));
		//解析属性类型
		switch (attrType) {
			/* MFT 的属性及其含义可参见 https://blog.csdn.net/shimadear/article/details/89287916
			 *                     以及 https://blog.csdn.net/Hilavergil/article/details/82622470
			 * 常驻属性: 属性的数据能够在1KB的文件记录中保存
			 * 非常驻属性: 非常驻属性无法在文件记录中存放，需存放到MFT外其他位置
			 * 这里用到的几个属性
			 * 80H 属性: 文件的数据属性
			 *		- 如果文件比较大，在MFT中无法存放其所有数据，那么这个文件的80属性为非常驻属性，
			 *		  属性体是一个data runs，指向存放该文件数据的簇。如果文件能够存放在常驻的80属性中，
			 *		  则80属性的属性体并不是data runs，而是直接存放了文件内容
			 * 90H 属性: 索引根属性，存放着该目录下的子目录和子文件的索引项
			 *		- 由3部分构成：索引根、索引头和索引项。
			 *		  但是有些情况下90属性中是不存在索引项的，这个时候该目录的索引项由A0属性中的data runs指出。
			 *		  A0属性的属性体即为data runs，指向若干个索引区域的簇流
			 * A0H 属性: 指向多个索引节点，每个索引节点中有多个索引项，一个索引项对应一个文件
			 *		- 当某个目录下内容较多，从而导致90属性无法完全存放时，A0属性会指向一个索引区域，
			 *	      这个索引区域包含了该目录下所有剩余内容的索引项。（可以认为A0属性是对90属性的补充）
			 */
		case 0x80: {
			file.setCryptoFlag(meh.cryptoFlags);
			if (!cah.ATTR_ResFlag) {
				cout << "\n\n文件内容过小，直接存储在MFT项中!" << endl;
				file.setFileMftOffset(file.getFileMftOffset() / disk.GetSectorSize());
				cout << "\nMFT项扇区偏移: " << file.getFileMftOffset();
			}
			else {
				file.setFileMftOffset(file.getFileMftOffset() / disk.GetSectorSize());
				cout << "\n\n文件MFT项扇区偏移: " << file.getFileMftOffset() << endl;
				BYTE* clusterStream = attrStart + nah.ATTR_DatOff;
				/* 设置文件簇流*/
				file.setClusterStream(clusterStream);
			}
			return nullptr;
		}
		case (BYTE)0x90: {
			//90一定常驻
			//申请INDEX_ROOT空间
			INDEX_ROOT ir = INDEX_ROOT();
			memcpy(&ir, attrStart + rah.ATTR_DatOff, sizeof(ir));
			//表明没有索引项，需要读到A0
			if (ir.IR_IndexEntry == nullptr) {
				break;
			}//正常读入索引项
			else {
				int validLeft = cah.ATTR_Size - rah.ATTR_DatOff - 0x20;
				int read = 0;
				BYTE* pClusterStream = rah.ATTR_DatOff + attrStart + 0x20;
				STD_INDEX_ENTRY sie = STD_INDEX_ENTRY();
				memcpy(&sie, pClusterStream, sizeof(sie));
				//解析这个簇流拥有得表项
				int bufsize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, tmpName, strlen(tmpName), nullptr, 0) * 2 + 1;
				pTmpName = new BYTE[bufsize];
				pTmpName[bufsize - 1] = 0;
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, tmpName, strlen(tmpName), (LPWSTR)pTmpName, bufsize);
				UINT64 refNum = ParseIndexEntry(validLeft, pClusterStream, (const char*)pTmpName);
				//说明在90中匹配不上
				if (refNum == 0) {
					break;
				}
				cout << "\n指向的MFT项为第" << refNum << "项, MFT项磁盘扇区偏移: "
					<< (((disk.GetMFTStartCluster() * disk.GetClusterSize() + refNum * 2 * disk.GetSectorSize())) / disk.GetSectorSize()) << endl;

				HANDLE disk_handle = GetDiskHandleR(disk.GetDiskName().c_str(), 0);
				BigFileSeek(disk_handle, (disk.GetMFTStartCluster() * disk.GetClusterSize() + refNum * 2 * disk.GetSectorSize()), FILE_BEGIN);
				//printf("%llx", (Disk::v().getMFTStartCluster() * Disk::v().getClusterSize() + refNum * 2 * Disk::v().getSectorSize()));
				BYTE* data1 = new BYTE[disk.GetSectorSize() * 2];
				DWORD readNum;
				if (!ReadFile(disk_handle, data1, disk.GetSectorSize() * 2, &readNum, NULL)) {
					cout << "读取MFT项失败,失败原因: " << GetLastError() << endl;
					exit(-1);
				}
				if (*endp == '\0') {
					file.setFileMftOffset((disk.GetMFTStartCluster() * disk.GetClusterSize() + refNum * 2 * disk.GetSectorSize()));
					return data1;
				}
				else {
					//printf("90属性中的偏移:\nllx", (Disk::v().getMFTStartCluster() * Disk::v().getClusterSize() + refNum * 2 * Disk::v().getSectorSize()));
					return ParseMFTEntry(data1, endp, disk, file, true);
				}
			}
			break;
		}
					   //读到A0属性，说明90没匹配上，继续匹配A0属性
		case (BYTE)0xA0: {
			BYTE* pClusterStream = nah.ATTR_DatOff + attrStart;
			BYTE* data = nullptr;
			UINT64 clusterNum = 0;
			int offsetByteNum = 0;
			int clusterNumByteNum = 0;
			UINT64 clusterOffset = 0;
			DWORD readNum;
			//对每个簇流进行解析
			while (*pClusterStream != 0) {
				//提取出占用的簇的个数
				offsetByteNum = (*pClusterStream) >> 4;
				//提取出大小占用的簇的个数
				clusterNumByteNum = (*pClusterStream) & 0x0f;
				//求出簇流的大小
				clusterNum = UCHARToLL(pClusterStream + 1, clusterNumByteNum);
				//求出簇流的偏移
				clusterOffset = UCHARToLL(pClusterStream + 1 + clusterNumByteNum, offsetByteNum);

				int bufsize = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, tmpName, strlen(tmpName), nullptr, 0) * 2 + 1;
				pTmpName = new BYTE[bufsize];
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, tmpName, strlen(tmpName), (LPWSTR)pTmpName, bufsize);
				pTmpName[bufsize - 1] = 0;

				HANDLE disk_handle = GetDiskHandleR(disk.GetDiskName().c_str(), 0);
				BigFileSeek(disk_handle, clusterOffset * disk.GetClusterSize(), FILE_BEGIN);
				/* data 或许用来存储文件的数据*/
				if (data == nullptr) {
					data = new BYTE[clusterNum * disk.GetClusterSize()];
				}
				if (!ReadFile(disk_handle, data, clusterNum * disk.GetClusterSize(), &readNum, NULL)) {
					cout << "读取MFT项失败,失败原因: " << GetLastError() << endl;
					exit(-1);
				}
				int tag = clusterNum;
				BYTE* origin_data = data;
				for (int i = 0; i < tag; i++) {
					data = origin_data + disk.GetClusterSize() * i;
					STD_INDEX_HEADER ih = STD_INDEX_HEADER();
					memcpy(&ih, data, sizeof(STD_INDEX_HEADER));
					//解析这个簇流拥有得表项

					UINT64 refNum = ParseIndexEntry(ih.SIH_IndexEntrySize - (ih.SIH_IndexEntryOffset + 24), data + ih.SIH_IndexEntryOffset + 24, (const char*)pTmpName);
					//如果这个表项不包含想要的表项
					if (refNum == 0) {
						continue;
					}
					cout << "\n指向的MFT项为第" << refNum << "项, MFT项磁盘扇区偏移: "
						<< ((disk.GetMFTStartCluster() * disk.GetClusterSize() + refNum * 2 * disk.GetSectorSize()) / disk.GetSectorSize()) << endl;
					BigFileSeek(disk_handle, (disk.GetMFTStartCluster() * disk.GetClusterSize() + refNum * 2 * disk.GetSectorSize()), FILE_BEGIN);
					//	printf("\nA0属性中的偏移:%llx", (Disk::v().getMFTStartCluster() * Disk::v().getClusterSize() + refNum * 2 * Disk::v().getSectorSize()));
					BYTE* data1 = new BYTE[disk.GetSectorSize() * 2];
					if (!ReadFile(disk_handle, data1, disk.GetSectorSize() * 2, &readNum, NULL)) {
						printf_s("读取MFT项失败,失败原因: %d", GetLastError());
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
				//将指针向前移1个标识字节，x和偏移字节和y和大小字节
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

	/* 解析根目录项，根目录项位于 MFT 的第五项*/
	long long int root_directory_offset = disk.GetMFTStartCluster() * disk.GetClusterSize()
		+ disk.GetSectorSize() * 10;

	HANDLE disk_handle = GetDiskHandleR(disk_name, root_directory_offset);
	BigFileSeek(disk_handle, root_directory_offset, FILE_BEGIN);
	/* 获取根目录的 mft*/
	UCHAR* root_directory = new UCHAR[1024];
	GetDiskRootMFT(disk_handle, root_directory);

	cout << "\n\n>>>>>>>>>>>>>>>>>>>>开始解析<<<<<<<<<<<<<<<<<<<<<<\n" << endl;
	//递归解析
	BYTE* fileData = ParseMFTEntry(root_directory, file.getTargetPath() + 2, disk, file, true);
	if (!fileData) {
		cout << "\n\n路径解析失败!" << endl;
		exit(-1);
	}
	ParseMFTEntry(fileData, file.getTargetPath() + 2, disk, file, false);
	cout << "\n\n>>>>>>>>>>>>>>>>>>>>解析完毕<<<<<<<<<<<<<<<<<<<<<<\n" << endl;

	CloseHandle(disk_handle);
}



void ModifyMFT(BYTE* buffer, bool flag, NTFSFile& file) {
	BYTE* p = buffer;
	MFT_ENTRY_HEADER meh = MFT_ENTRY_HEADER();
	//解析MFT头
	memcpy(&meh, p, sizeof(MFT_ENTRY_HEADER));
	CommonAttributeHeader cah = CommonAttributeHeader();
	ResidentAttributeHeader rah = ResidentAttributeHeader();
	NonResidentAttributeHeader nah = NonResidentAttributeHeader();
	//属性类型
	UINT32 attrType;
	BYTE* attrStart = (BYTE*)p;
	//指向第一个属性的属性头的起始地址
	attrStart += meh.AttributeOffset;

	while (true) {
		//解析出属性的类型
		//清0防止未知的bug
		memset(&cah, 0, sizeof(CommonAttributeHeader));
		memset(&rah, 0, sizeof(ResidentAttributeHeader));
		memset(&nah, 0, sizeof(NonResidentAttributeHeader));
		memcpy(&attrType, attrStart, 4);
		if (attrType == 0xffffffff) {
			return;
			printf_s("\n解析完毕!\n");
			break;
		}
		memcpy(&cah, attrStart, sizeof(CommonAttributeHeader));

		//常驻属性
		if (!cah.ATTR_ResFlag)
			memcpy(&rah, attrStart + sizeof(CommonAttributeHeader), sizeof(ResidentAttributeHeader));
		else
			memcpy(&nah, attrStart + sizeof(CommonAttributeHeader), sizeof(NonResidentAttributeHeader));
		//解析属性类型
		switch (attrType) {
		case 0x80: {
			if (!cah.ATTR_ResFlag) {
				if (flag) {
					int size = cah.ATTR_Size - rah.ATTR_DatOff;
					BYTE* result = RC4_for_MTR_data(attrStart + rah.ATTR_DatOff, size, file.getKey());
					printf("\n文件十六进制数据：\n");
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
					printf("\n文件十六进制数据:");
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

//短文件目录项32字节
typedef struct _FAT32shortFDT {
    uint8_t filename[8];//第一部分文件名
    uint8_t extname[3];//文件扩展名
    uint8_t attr;//属性 0F则说明是长文件需要索引到非0F，然后倒着读回来
    uint8_t reserve;
    uint8_t time1;
    uint8_t creattime[2];
    uint8_t createdate[2];
    uint8_t visittime[2];
    uint8_t high_cluster[2];//文件起始簇号高16位
    uint8_t changetim2[2];
    uint8_t changedate[2];
    uint8_t low_cluster[2];//文件起始簇号低16位
    uint8_t filelen[4];//文件长度
}FAT32shortFDT;

typedef struct _FAT32longFDT {
    char flag;//如果是0x4*,第6位置位了，说明是最后一个长文件目录，各位是下面还有几个
    char name1[10];
    char attr;//如果是长文件名，除了最下面一个，都是0F
    char reserve;
    char checksum;
    char name2[12];
    char rel_cluster[2];//相对起始簇号
    char name3[4];
}FAT32longFDT;

//文件名可能很长，方便检索文件名
struct FAT32LongFDTList {
    FAT32longFDT lfdt;
    struct FAT32LongFDTList* up;//上面的（低地址）
    struct FAT32LongFDTList* down;
};

struct findfilepath {
    char prename[100];
    char rearname[10];
    int flag;//1是短文件，2是长文件，3是短目录，4是长目录;是3/4则next非空
    struct findfilepath* next;
};


uint16_t uint8to16(uint8_t twouint8[2]) {
    return *(uint16_t*)twouint8;
}

uint32_t uint8to32(uint8_t fouruint8[4]) {
    return *(uint32_t*)fouruint8;
}


//简单的英文的两个转一个,不会出现两个连续的00，出现就表示已经到结尾了
//文件不区分大小写的，全部toupper，记得那个'.'！！！！是存在里面的!!!!把读出来的点删掉
void double2single(char* filename, char* upperfilename, int num) {
    int j = 0;
    for (int i = 0; i < num; i = i + 2)
    {    //如果是字母，返回非零，否则返回零！！！
        if (isalpha(filename[i]) == 0)
        {
            upperfilename[j] = filename[i];
        }
        else
            upperfilename[j] = toupper(filename[i]);
        j++;
    }
}
//文件不区分大小写的，全部toupper
void short2upper(char* filename) {
    for (int i = 0; i < strlen(filename); i++)
        filename[i] = toupper(filename[i]);
}

//下一步解析，如果已经到了文件尾，这次解析出的是文件名，那么返回0[[后面没有斜杠了'/']
//否则还要再次调用解析，是目录返回1[后面还有一个斜杠]
//否则出错，返回-1
//如果返回1，或者0，下一次的目录/文件名，保存在nextname里面[由调用者维护，不检查长度]
//indexptr是用来保存当前检索到了哪里，下一次直接继续[调用者维护]
int nextpath(char* filepath, char* nextname, int* indexptr) {
    int flag = -1;
    int firstindex = 0, lastindex = 0;
    int filepathlen = strlen(filepath);
    short2upper(filepath);
    if (*indexptr >= filepathlen)//6个空间，索引最大为5
        return -1;
    //解析文件名
    while (1) {
        //判断是否结束或者出错
        if (*indexptr >= filepathlen && flag == -1)//如果出错
        {
            flag = -1;//出错了
            break;
        }
        else if (*indexptr == filepathlen && flag == 1) {
            flag = 0;//读完了,假设到文件了
            break;
        }

        if (filepath[*indexptr] == ':')//如果在一开始是冒号,往后读内容
        {
            ;
        }
        if (filepath[*indexptr] == '/' && flag == -1)//如果是第一个反斜杠,标志开始
        {
            if ((*indexptr) + 1 < filepathlen) {
                firstindex = (*indexptr) + 1;
                //printf("firstindex = %d\n",firstindex);
                flag = 1;//暂时等于1
            }
            else {
                flag = -1; break;//出错了
            }
        }
        else if (filepath[*indexptr] == '/' && flag != -1)//解析完一个了
        {
            lastindex = (*indexptr) - 1;//收尾了
            //printf("lastindex = %d\n",lastindex);
            break;
        }
        (*indexptr)++;
    }
    if (flag != -1)
    {
        memset(nextname, 0, sizeof(nextname));
        if (flag == 1)//目录
            strncpy(nextname, &filepath[firstindex], lastindex - firstindex + 1);//复制字符串
        else
            strncpy(nextname, &filepath[firstindex], filepathlen - firstindex);//复制字符串
        //printf("next name=%s\n",nextname);
    }
    return flag;
}

//把文件名存放为文件名和后缀名，后缀名大于3的，也都是用长文件名格式;含有空格的也都是长文件名
int dividefilename(char* filename, char* prename, char* rearname) {
    //printf("dividename is %s %d\n",filename,strlen(filename));
    int dotindex = 0;
    for (int i = 0; i < strlen(filename); i++) {
        if (filename[i] == '.')//找到分界了
        {
            dotindex = i;
            //printf("dotindex = %d\n",dotindex);
            break;//假设输入是对的
        }
    }
    if (dotindex == 0)
        return -1;
    std::strncpy(prename, filename, dotindex);//复制前面文件名
    std::strncpy(rearname, &filename[dotindex + 1], strlen(filename) - dotindex - 1);
    //printf("copy filename: %s   %s  strlen=%d  %d\n",prename,rearname,strlen(prename),strlen(rearname));

    return 0;
}

void getpathlist(char* filepath, struct findfilepath*& head, int* filenameindex) {
    struct findfilepath* p = NULL, * rear = NULL;
    int t;//有木有结束
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
        if (t != -1)//正常情况是得到目录名
        {
            if (t == 1)//目录名
            {
                printf("目录为%s  长度为%d\n", nextname, strlen(nextname));
                p = (struct findfilepath*)malloc(sizeof(struct findfilepath));
                p->next = NULL;
                memset(p->prename, 0, sizeof(p->prename));
                memset(p->rearname, 0, sizeof(p->rearname));

                if (strlen(nextname) > 8)//需要长文件存
                {
                    p->flag = 4;//长目录
                    strncpy(p->prename, nextname, strlen(nextname));//长目录直接存
                    //printf("长目录 %s %d\n",p->prename,strlen(p->prename));
                }
                else
                {
                    p->flag = 3;//短目录
                    strncpy(p->prename, nextname, strlen(nextname));//短目录补0x20
                    paddingnum = 8 - strlen(p->prename);
                    if (paddingnum > 0)
                        memset(&p->prename[strlen(p->prename)], 0x20, paddingnum);
                    //printf("短目录 %s %d\n",p->prename,strlen(p->prename));
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
            else//t==0，是文件名
            {
                printf("文件为%s  长度为%d\n", nextname, strlen(nextname));
                p = (struct findfilepath*)malloc(sizeof(struct findfilepath));
                p->next = NULL;
                memset(p->prename, 0, sizeof(p->prename));//粗心的原作者把这里的sizeof写成了strlen，害得我花了5个小时找bug
                memset(p->rearname, 0, sizeof(p->rearname));
                dividefilename(nextname, prename, rearname);
                printf("\n");
                if (strlen(prename) > 8 || strlen(rearname) > 3)//需要长文件存
                {
                    p->flag = 2;//长文件
                    strncpy(p->prename, prename, strlen(prename));
                    strncpy(p->rearname, rearname, strlen(rearname));
                    //printf("长文件 %s.%s\n",p->prename,p->rearname);
                }
                else
                {
                    p->flag = 1;//短文件
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
                    //                    printf("短文件 %s.%s\n",p->prename,p->rearname);
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


//解析短文件名
vector<uint64_t> readshortfileFDT(FAT32shortFDT the_short_FDT, ULONGLONG FAT1_reladdr, HANDLE hDevice) {
    vector<uint64_t> clusters;
    
    uint8_t cluster[4] = { the_short_FDT.low_cluster[0],the_short_FDT.low_cluster[1],
                           the_short_FDT.high_cluster[0],the_short_FDT.high_cluster[1] };
    uint32_t firstcluster = uint8to32(cluster);//起始簇号
    clusters.push_back(firstcluster);
    //printf("文件开始簇号是%08X\n", firstcluster);
    //printf("簇链:\n%08X\n", firstcluster);
    int next_sector = firstcluster / 128;//在第几个扇区？512*beg_sector
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
        cout << "读取磁盘信息失败: " << GetLastError() << endl;
    }

    while (needreadmore == 1) {
        FATentry = *(uint32_t*)&Buffer[next_byte];
        //printf("%08X\n", FATentry);
        clusters.push_back(FATentry);
        if (FATentry != 0x0FFFFFFF)//尚未结束
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

//解析短目录名，通过相对于根目录，找到绝对地址
ULONGLONG readshortdirFDT(FAT32shortFDT the_short_FDT, ULONGLONG root_reladdr, HANDLE hDevice, int secotrs_per_cluster) {
    uint8_t cluster[4] = { the_short_FDT.low_cluster[0],the_short_FDT.low_cluster[1],
                           the_short_FDT.high_cluster[0],the_short_FDT.high_cluster[1] };
    uint32_t firstcluster = uint8to32(cluster);//起始簇号
    printf("目录开始簇号是%08X\n", firstcluster);
    ULONGLONG nextaddr = (ULONGLONG)((firstcluster - 2) * secotrs_per_cluster * 512) + root_reladdr;
    return nextaddr;
}

vector<uint64_t> findfile(struct findfilepath* head, char* partition) {
    struct findfilepath* p = NULL;

    DISK_GEOMETRY* pdg = nullptr;            // 保存磁盘参数的结构体
    BOOL bResult;                 // generic results flag
    ULONGLONG DiskSize;           // size of the drive, in bytes
    HANDLE hDevice;               // 设备句柄
    DWORD junk;                   // discard resultscc
    bool bRet;
    //通过CreateFile来获得设备的句柄，打开对应的盘
    hDevice = GetDiskHandleR(partition, 0);

    //通过DeviceIoControl函数与设备进行IO，为后面做准备
    bResult = DeviceIoControl(hDevice, // 设备的句柄
        IOCTL_DISK_GET_DRIVE_GEOMETRY, // 控制码，指明设备的类型
        NULL, 0, // no input buffer
        pdg,
        sizeof(*pdg),     // output buffer 输出，保存磁盘参数信息
        &junk,                 // # bytes returned
        (LPOVERLAPPED)NULL); // synchronous I/O

    LARGE_INTEGER offset;//读取位置
    offset.QuadPart = (ULONGLONG)0;//0
    SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);//从这个位置开始读，DBR是FILE_BEGIN，相对位移！！！

    DWORD dwCB;
    FAT32Dbr the_DBR;
    //从这个位置开始读DBR，一开始的512字节有些信息有用
    if (!ReadFile(hDevice, &the_DBR, 512, &dwCB, NULL)) {
        cout << "读取磁盘信息失败: " << GetLastError() << endl;
    }
    ULONGLONG FAT1_reladdr = (ULONGLONG)uint8to16(the_DBR.reserve_sectors) *
        (ULONGLONG)512;//得到FAT1的具体地址，但是偏移需要用相对偏移

    ULONGLONG root_reladdr = FAT1_reladdr + (ULONGLONG)(the_DBR.FATnum[0]) *
        (ULONGLONG)uint8to32(the_DBR.sectors_per_FAT) * (ULONGLONG)512;//根目录的起始相对位置,根目录是在第[01]2簇

    offset.QuadPart = root_reladdr;
    SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);
    
    UCHAR lpBuffer[512] = { 0 };
    if (!ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL)) {
        cout << "读取磁盘信息失败: " << GetLastError() << endl;
    }

    
    //show_onesector(lpBuffer);

    char prename[101] = { 0 };
    char rearname[10] = { 0 };
    int needreadmore = 0;//在这一层目录需要继续读扇区，还没有找到文件/恰好被长文件分割了

    //1次只能读512字节整数倍,16个FDT,每个32字节
    FAT32FDT tempFDT[16] = { 0 }, temp_tempFDT[16] = { 0 };//后面的给长目录和文件准备
    FAT32shortFDT the_short_FDT = { 0 };
    FAT32longFDT the_long_FDT[10];
    int longfdtindex = 0;//索引项
    int index = 0;
    int tempindex = 0;
    LARGE_INTEGER temp_offset;//临时倒退找长文件项的
    int findflag = 0, mayfindflag = 0;
    p = head;
    int i = 0;
    while (p != NULL)//还没有找到，一层层找
    {
        memset(tempFDT, 0, sizeof(tempFDT));
        memcpy(tempFDT, lpBuffer, 512);

        if (p->next == NULL && (p->flag == 1 || p->flag == 2))//如果在这一层是文件
        {
            if (p->flag == 1)//是短文件,找到以后可以用短文件解析
            {
                findflag = 0; index = 0;
                while (findflag == 0) {
                    if (tempFDT[index].content[0] == 0x00) {
                        printf("\n路径错误!\n");
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
                    if (index < 15 && findflag == 0)//没找到
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
                        printf("\n路径错误!\n");
                        return vector<uint64_t>();
                    }
                    if ((tempFDT[index].content[11] & 0x10) == 0)//文件项,先找找文件名头四个对不对
                    {
                        memset(prename, 0, sizeof(prename));
                        memcpy(prename, &tempFDT[index].content[0], 8);
                        if (!strnicmp(prename, p->prename, 4))
                        {
                            memset(&the_short_FDT, 0, 32);
                            memcpy(&the_short_FDT, &tempFDT[index], 32);//里面存着长目录的地址信息
                            tempindex = index;//不能影响环境！！！！
                            memset(prename, 0, sizeof(prename));
                            memcpy(temp_tempFDT, tempFDT, sizeof(tempFDT));
                            while (1) {
                                if (tempindex > 0)//如果暂时不需要倒退
                                {
                                    tempindex--;
                                    if (temp_tempFDT[tempindex].content[11] == 0x0F)//的确没完
                                    {
                                        memset(&the_long_FDT[longfdtindex], 0, 32);
                                        memcpy(&the_long_FDT[longfdtindex], &temp_tempFDT[tempindex], 32);
                                        longfdtindex++;//下一个索引
                                    }
                                    else//读完了，处理字符串
                                    {
                                        for (int i = 0, k = 0; i < longfdtindex; i++)
                                        {
                                            for (int j = 0; j < 3; j++)//复制三部分
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
                                        //printf("文件名是  %s.%s\n", p->prename, p->rearname);
                                        //printf("拼的文件名是  %s\n", prename);
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
                                else {//倒退回去，继续读
                                    tempindex = 16;//后面一进入就要做一次减法
                                    memset(temp_tempFDT, 0, sizeof(temp_tempFDT));
                                    memset(lpBuffer, 0, sizeof(lpBuffer));
                                    temp_offset.QuadPart = offset.QuadPart - (ULONGLONG)512;
                                    SetFilePointer(hDevice, temp_offset.LowPart, &temp_offset.HighPart, FILE_BEGIN);
                                    bRet = ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL);
                                    memcpy(temp_tempFDT, lpBuffer, 512);
                                }
                            }	//end of while(1) 找长文件名目录项
                        }//end of if 文件前四个相同
                    noyet://这个不是，虽然文件名前四个相同
                        longfdtindex = 0;
                    findlongname://是的，找到了
                        if (findflag == 1)
                            return readshortfileFDT(the_short_FDT, FAT1_reladdr, hDevice);

                    }//end of if 如果是一个短文件目录项
                    if (index < 15 && findflag == 0)//没找到
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
                }//end of while(findflag=0)	已经找到了，调出来
            }//长文件，不用读了
        }
        else if (p->next != NULL && (p->flag == 3 || p->flag == 4))
        {

            if (p->flag == 3)//如果是短目录
            {
                findflag = 0; index = 0;
                i = 0;
                while (findflag == 0) {
                    if (tempFDT[index].content[0] == 0x00) {
                        printf("\n路径错误!\n");
                        return vector<uint64_t>();
                    }
                    //注意运算符顺序
                    if ((tempFDT[index].content[11] & 0x10) != 0)//短目录项
                    {
                        memset(prename, 0, sizeof(prename));
                        memcpy(prename, &tempFDT[index].content[0], 8);
                        if (!strcmp(prename, p->prename))
                        {
                            findflag = 1;
                            memset(&the_short_FDT, 0, 32);
                            memcpy(&the_short_FDT, &tempFDT[index], 32);
                            //更新
                            offset.QuadPart = readshortdirFDT(the_short_FDT, root_reladdr,
                                hDevice, the_DBR.sectors_per_cluster[0]);
                        }
                    }
                    if (index < 15 && findflag == 0)//没找到
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
            else if (p->flag == 4)//长目录，有点难受！！！,不如先找头4个字节！！！，大写的
            {

                while (findflag == 0) {
                    if (tempFDT[index].content[0] == 0x00) {
                        printf("\n路径错误!\n");
                        return vector<uint64_t>();
                    }
                    if ((tempFDT[index].content[11] & 0x10) != 0)//目录项，先找找文件名头四个对不对
                    {
                        memset(prename, 0, sizeof(prename));
                        memcpy(prename, &tempFDT[index].content[0], 8);
                        if (!strnicmp(prename, p->prename, 4))
                        {
                            memset(&the_short_FDT, 0, 32);
                            memcpy(&the_short_FDT, &tempFDT[index], 32);//里面存着长目录的地址信息
                            tempindex = index;//不能影响环境！！！！
                            memset(prename, 0, sizeof(prename));
                            memcpy(temp_tempFDT, tempFDT, sizeof(tempFDT));
                            while (1) {
                                if (tempindex > 0)//如果暂时不需要倒退
                                {
                                    tempindex--;
                                    if (temp_tempFDT[tempindex].content[11] == 0x0F)//的确没完
                                    {
                                        memset(&the_long_FDT[longfdtindex], 0, 32);
                                        memcpy(&the_long_FDT[longfdtindex], &temp_tempFDT[tempindex], 32);
                                        longfdtindex++;//下一个索引
                                    }
                                    else//读完了，处理字符串
                                    {
                                        for (int i = 0, k = 0; i < longfdtindex; i++)
                                        {
                                            for (int j = 0; j < 3; j++)//复制三部分
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
                                        //printf("文件目录是  %s\n", p->prename);
                                        //printf("拼的目录是  %s\n", prename);
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
                                else {//倒退回去，继续读
                                    tempindex = 16;//后面一进入就要做一次减法
                                    memset(temp_tempFDT, 0, sizeof(temp_tempFDT));
                                    memset(lpBuffer, 0, sizeof(lpBuffer));
                                    temp_offset.QuadPart = offset.QuadPart - (ULONGLONG)512;
                                    SetFilePointer(hDevice, temp_offset.LowPart, &temp_offset.HighPart, FILE_BEGIN);
                                    bRet = ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL);
                                    memcpy(temp_tempFDT, lpBuffer, 512);
                                }
                            }
                        }//end of while(1) 找长文件名目录项
                    notyet://这个不是，虽然文件名前四个相同
                        longfdtindex = 0;
                    finddir://是的，找到了
                        if (findflag == 1)
                            offset.QuadPart = readshortdirFDT(the_short_FDT, root_reladdr,
                                hDevice, the_DBR.sectors_per_cluster[0]);

                    }//end of if 如果是一个短文件目录项
                    if (index < 15 && findflag == 0)//没找到
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
                }//end of while(findflag=0)	已经找到了，调出来
            }//end of if 如果是当前解析的是长文件目录

            //!!!是目录，继续读下一步扇区!!!在上面复制回来
            memset(tempFDT, 0, sizeof(tempFDT));
            memset(lpBuffer, 0, sizeof(lpBuffer));
            SetFilePointer(hDevice, offset.LowPart, &offset.HighPart, FILE_BEGIN);
            bRet = ReadFile(hDevice, lpBuffer, 512, &dwCB, NULL);
            memcpy(tempFDT, lpBuffer, 512);
        }
        findflag = 0;
        p = p->next;//下一层路径
    }

    CloseHandle(hDevice);//如果打开成功，需要关闭
}

void freepathlist(struct findfilepath*& head) {
    struct findfilepath* p = NULL;
    p = head;
    while (p != NULL) {
        p = p->next;
        free(p);
    }
}