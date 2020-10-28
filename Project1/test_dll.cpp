//#include <iostream>
//#include <windows.h>
//#include <vector>
//#include <string>
//#include "get_cluster.h"
//#include "utils.h"
//typedef PLDPartionList(*Sub)();
//typedef vector<UINT64>(*Sub1)(const char* file_path);
//typedef int(*Sub2)(const char* file_path, unsigned char* key);
//#pragma comment(lib,"..\\x64\\Release\\ddl1.lib")
//
//int main(int argc, char** argv)
//{
//	HINSTANCE hInst;
//	const char* file_path = { "H:\\1.pdf" };
//	const char* key_path = { "D:\\test.txt" };
//	hInst = LoadLibrary(L"..\\x64\\Release\\ddl1.dll");
//
//	Sub TestProc1 = (Sub)GetProcAddress(hInst, "_GetDiskPartion");//从dll中加载函数出来
//	Sub1 TestProc2 = (Sub1)GetProcAddress(hInst, "_GetFileCluster");
//	Sub2 TestProc3 = (Sub2)GetProcAddress(hInst, "_EncodedFileByCluster");
//	Sub2 TestProc4 = (Sub2)GetProcAddress(hInst, "_DecodedFileByCluster");
//	TestProc1();//运行函数
//
//	//
//	vector<UINT64> cluster_streams = TestProc2(file_path);
//	TestProc3(file_path, FileToUCHAR(key_path));
//	TestProc4(file_path, FileToUCHAR(key_path));
//
//	FreeLibrary(hInst);       //LoadLibrary后要记得FreeLibrary
//	system("pause");
//	return 0;
//}