#pragma once
#include <Windows.h>
#include <vector>

long long UCHARToLL(const UCHAR* buf, int len);
unsigned char* FileToUCHAR(const char* file_path);
void ShowClusters(std::vector<UINT64> clusters);