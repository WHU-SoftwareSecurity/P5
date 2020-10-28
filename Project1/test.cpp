#include "get_cluster.h"
#include "utils.h"


int main() {
	const char* ntfs_file_path = { "E:\\123.txt" };
	const char* fat32_file_path = { "F:/123.txt" };
	const char* key_path = { "E:\\test.txt" };

	//for (int i = 0; i < 5; i++)
	//	GetDiskPartion();
	
	vector<uint64_t> clusters = GetFileCluster(fat32_file_path);
	clusters = GetFileCluster(ntfs_file_path);
	return 0;
}