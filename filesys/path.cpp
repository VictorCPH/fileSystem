#include <stdio.h>
#include <string.h>
#include "const.h"
#include "inode.h"
#include "superblk.h"
#include "dir.h"
#include "path.h"

extern char	path[40];
extern int	inode_num;//当前目录的inode编号
extern int 	dir_num;//相应编号的目录项数
extern Dir 	dir_table[MaxDirNum];//将当前目录文件的内容都载入内存
extern FILE*	Disk;

void change_path(char *name)
{
	int pos;
	if (strcmp(name, ".") == 0) {//进入本目录则路径不变
		return;
	}
	else if (strcmp(name, "..") == 0) {//进入上层目录，将最后一个'/'后的内容去掉
		pos = strlen(path) - 1;
		for (; pos >= 0; --pos) {
			if (path[pos] == '/') {
				path[pos] = '\0';
				break;
			}
		}
	}
	else {//否则在路径末尾添加子目录
		strcat(path, "/");
		strcat(path, name);
	}

	return;
}

int eat_path(char* name)
{
	int tmp = inode_num;//记录原始inode节点
	char dst[30][NameLength];
	int cnt = split(dst, name, "/");
	if (name[0] == '/') {//从根目录开始
						 //printf("1111111111\n");
		close_dir(inode_num);
		inode_num = 0;
		open_dir(inode_num);
	}
	for (int i = 0; i < cnt - 1; i++) {
		//printf("%d\n", i);
		int res = enter_child_dir(inode_num, dst[i]);
		if (res == -1) {
			inode_num = tmp;
			open_dir(inode_num);
			return -1;
		}
	}
	if (cnt == 0)
		strcpy(name, ".");
	else
		strcpy(name, dst[cnt - 1]);
	return 0;
}

int split(char dst[][NameLength], char* str, const char* spl)
{
	int n = 0;
	char *result = NULL;
	result = strtok(str, spl);
	while (result != NULL)
	{
		strcpy(dst[n++], result);
		result = strtok(NULL, spl);
	}
	return n;
}


/*检查重命名*/
int check_name(int inode, char* name)
{
	int i;
	for (i = 0; i<dir_num; ++i) {
		/*存在重命名*/
		if (strcmp(name, dir_table[i].name) == 0) {
			return dir_table[i].inode_num;
		}
	}

	return -1;
}

int type_check(char* name)
{
	int 	i, inode;
	Inode 	temp;
	for (i = 0; i<dir_num; ++i) {
		if (strcmp(name, dir_table[i].name) == 0) {
			inode = dir_table[i].inode_num;
			fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);

			fread(&temp, sizeof(Inode), 1, Disk);
			return temp.type;
		}
	}
	return -1;//该文件或目录不存在
}
