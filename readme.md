### P5 实验代码

可执行程序位于 `build` 文件夹，使用时需要管理员权限



#### 使用说明

- `load_dll.exe [{subcommand}] -h` 获取帮助信息
- `load_dll.exe disk [-n {num}]`
  - 获取从 C 盘开始 `num` 个磁盘的信息
  - 支持 FAT32 和 NTFS
- `load_dll.exe cluster -f {file_path} [-s]`
  - 获取 `file_path` 文件簇链信息
  - `-s` 打印簇链
  - 支持 FAT32 和 NTFS
- `load_dlll.exe encrypt  -f {file_path} -k {key_path}`
  - 对文件进行加密
  - 仅支持 NTFS
- `load_dll.exe decrypt -f {file_path} -k {key_path}`
  - 对文件进行解密
  - 仅支持 NTFS



