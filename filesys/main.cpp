// filesys.cpp : 定义控制台应用程序的入口点。
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Windows.h>
#include <tchar.h>
#include "const.h"
#include "inode.h"
#include "superblk.h"
#include "dir.h"
#include "zip.h"
#include "fs.h"
#include "file.h"
#include "block.h"

Dir 	dir_table[MaxDirNum];//将当前目录文件的内容都载入内存
int 	dir_num;//相应编号的目录项数
int	 	inode_num;//当前目录的inode编号
FILE*	Disk;
Inode 	curr_inode;//当前目录的inode结构
SuperBlk	super_blk;//文件系统的超级块
FILETIME BuffModifyTimeBeforeEdit;
FILETIME BuffModifyTimeAfterEdit;

/*指令集合*/
char*	command[] = { "mkfs","q","mkdir","rmdir","cd","ls","touch","rm","vi",
					  "cp","mv", "stat", "chmod", "zip", "unzip", "man", "df", "ps"};
char	path[40] = "monitor@root:";

int main()
{
	char comm[NameLength], name[NameLength],
		 cp_name[NameLength], mv_name[NameLength],
		 zip_name[NameLength];
	char parameter[10];
	int i, quit = 0, choice;

	Disk = fopen(DISK, "rb+");
	if (!Disk) {
		printf("open fail\n");
		system("pause");
		exit(-1);
	}
	init_fs();

	while (1) {
		printf("%s# ", path);
		scanf("%s", comm);
		choice = -1;

		for (i = 0; i < CommanNum; ++i) {
			if (strcmp(comm, command[i]) == 0) {
				choice = i;
				break;
			}
		}

		switch (choice) {
			/*格式化文件系统*/
		case 0: 
			format_fs();
			break;

			/*退出文件系统*/
		case 1: 
			quit = 1;
			break;

			/*创建子目录*/
		case 2: 
			scanf("%s", name);
			make_file(inode_num, name, Directory);
			break;

			/*删除子目录*/
		case 3: 
			scanf("%s", name);
			remove_file(inode_num, name, 0, Directory);
			break;

			/*进入目录*/
		case 4:	
			scanf("%s", name);
			if (enter_dir(name) == -1) {
				printf("cd: '%s': No such file or directory\n", name);
			}
			break;

			/*显示目录内容*/
		case 5: show_dir(inode_num);
			break;

			/*创建文件*/
		case 6: 
			scanf("%s", name);	
			make_file(inode_num, name, File);	
			break;

			/*删除文件*/
		case 7: 
			scanf("%s", name);		
			remove_file(inode_num, name, 0, File);
			break;

			/*对文件进行编辑*/
		case 8: 
			scanf("%s", name);
			file_edit(name);
			break;

			/*拷贝文件*/
		case 9:
			scanf("%s %s", name, cp_name);
			file_copy(name, cp_name);
			break;

			/*移动文件*/
		case 10:
			scanf("%s %s", name, mv_name);
			file_move(name, mv_name);
			break;

			/*查看文件信息*/
		case 11:
			scanf("%s", name);
			show_file_info(name);
			break;

			/*改变文件权限*/
		case 12:
			scanf("%s %s", parameter, name);
			change_mode(parameter, name);
			break;

			/*文件压缩*/
		case 13:
			scanf("%s %s", zip_name, name);
			zip(zip_name, name);
			break;
		
			/*文件解压*/
		case 14:
			scanf("%s", name);
			unzip(name);
			break;

			/*指令说明手册*/
		case 15:
			show_manual();
			break;

			/*显示磁盘空间*/
		case 16:
			show_disk_usage();//显示磁盘使用情况
			printf("\n");
			show_inode_usage();//显示inode节点使用情况
			break;

			/*进程管理*/
		case 17:
	
			break;

		default:
			printf("'%s' command not found\n", comm);
			char ch;
			while ((ch = getchar()) != '\n' && ch != EOF);//指令错误，清空输入缓冲区
		}
		close_dir(inode_num);
		close_fs();//每执行完一条指令就保存一次数据
		if (quit) break;
	}

	fclose(Disk);
	return 0;
}


