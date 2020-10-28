#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include "get_cluster.h"
#include "utils.h"
#include "CLI11.hpp"

typedef PLDPartionList(*Sub)();
typedef vector<UINT64>(*Sub1)(const char* file_path);
typedef int(*Sub2)(const char* file_path, unsigned char* key);
#pragma comment(lib,"ddl1.lib")


int main(int argc, char** argv)
{
	HINSTANCE hInst;
	hInst = LoadLibrary(L"ddl1.dll");

	Sub TestProc1 = (Sub)GetProcAddress(hInst, "_GetDiskPartion");//从dll中加载函数出来
	Sub1 TestProc2 = (Sub1)GetProcAddress(hInst, "_GetFileCluster");
	Sub2 TestProc3 = (Sub2)GetProcAddress(hInst, "_EncodedFileByCluster");
	Sub2 TestProc4 = (Sub2)GetProcAddress(hInst, "_DecodedFileByCluster");

	CLI::App app{ "A disk tool made by group 8" };

	auto sub1 = app.add_subcommand("disk", "get disk information");
	auto sub2 = app.add_subcommand("cluster", "get cluster of a specified file");
	auto sub3 = app.add_subcommand("encrypt", "encrypt a specified file with given key");
	auto sub4 = app.add_subcommand("decrypt", "decrypt a specified file with given key");

	int disk_number = 1;
	string file_path;
	string key_path;
	bool show_clusters = false;


	sub1->add_option("-n", disk_number, "disk number")->check(CLI::PositiveNumber);
	
	sub2->add_option("-f", file_path, "file path(e.g. E:/123.txt)")->check(CLI::ExistingFile)->required();
	sub2->add_flag("-s", show_clusters, "show clusters");
	
	sub3->add_option("-f", file_path, "file path(e.g. E:/123.txt)")->check(CLI::ExistingFile)->required();
	sub3->add_option("-k", key_path, "key path(e.g. E:/key.txt)")->check(CLI::ExistingFile)->required();
	sub4->add_option("-f", file_path, "file path(e.g. E:/123.txt)")->check(CLI::ExistingFile)->required(); 	
	sub4->add_option("-k", key_path, "key path(e.g. E:/key.txt)")->check(CLI::ExistingFile)->required();
	CLI11_PARSE(app, argc, argv);

	if (sub1->parsed()) {
		for (int i = 0; i != disk_number; i++) {
			TestProc1();
		}
	}
	if (sub2->parsed()) {
		vector<UINT64> cluster_streams = TestProc2(file_path.c_str());
		if (show_clusters) {
			ShowClusters(cluster_streams);
		}
	}
	if (sub3->parsed()) {
		TestProc3(file_path.c_str(), FileToUCHAR(key_path.c_str()));
	}
	if (sub4->parsed()) {
		TestProc4(file_path.c_str(), FileToUCHAR(key_path.c_str()));
	}

	FreeLibrary(hInst);       //LoadLibrary后要记得FreeLibrary
	return 0;
}