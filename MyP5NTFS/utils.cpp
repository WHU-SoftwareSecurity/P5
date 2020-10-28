#include <fstream>
#include <vector>
#include <iostream>
#include "utils.h"


/* win10 采用大端编制
 * 这里将读取的 UCHAR 类型的值转换为 long long int
 */
long long UCHARToLL(const UCHAR* buf, int len) {
	long long result = 0;
	long long tmp;
	for (int i = 0; i < len; i++) {
		tmp = (long long)buf[i];
		result += (tmp << (i * 8));
	}
	return result;
}

unsigned char* FileToUCHAR(const char* file_path) {
	std::fstream file;
	file.open(file_path, std::ios::in);

	file.seekg(0, file.end);
	int file_size = file.tellg();
	unsigned char* content = new unsigned char[file_size + 1];

	file.seekg(0, file.beg);
	file.read((char*)(content), file_size);
	content[file_size] = '\0';
	file.close();

	return content;
}

void ShowClusters(std::vector<UINT64> clusters) {
	if (!clusters.size()) return;

	std::cout << std::hex;
	for (int i = 0; i != clusters.size() - 1; i++) {
		if (!(i % 8)) std::cout << std::endl << std::endl;
		std::cout << clusters[i];
		std::cout << " -> ";
	}
	std::cout << clusters[clusters.size() - 1];

	std::cout << std::oct;
}