#include "fs.h"
#include <stdio.h>
#include <string.h>
#include "const.h"
#include "inode.h"
#include "superblk.h"
#include "dir.h"

extern Dir 	dir_table[MaxDirNum];//将当前目录文件的内容都载入内存
extern int 	dir_num;//相应编号的目录项数
extern int	 	inode_num;//当前目录的inode编号
extern Inode 	curr_inode;//当前目录的inode结构
extern SuperBlk	super_blk;//文件系统的超级块
extern FILE*	Disk;
extern char	path[40];

int init_fs(void)
{
	fseek(Disk, SuperBeg, SEEK_SET);
	fread(&super_blk, sizeof(SuperBlk), 1, Disk);//读取超级块

	inode_num = 0;//当前根目录的inode为0

	if (!open_dir(inode_num)) {
		printf("CANT'T OPEN ROOT DIRECTORY\n");
		return 0;
	}

	return 1;
}

int close_fs(void)
{
	fseek(Disk, SuperBeg, SEEK_SET);
	fwrite(&super_blk, sizeof(SuperBlk), 1, Disk);

	close_dir(inode_num);
	return 1;
}

int format_fs(void)
{
	/*格式化inode_map,保留根目录*/
	memset(super_blk.inode_map, 0, sizeof(super_blk.inode_map));
	super_blk.inode_map[0] = 1;
	super_blk.inode_used = 1;
	/*格式化blk_map,保留第一个磁盘块给根目录*/
	memset(super_blk.blk_map, 0, sizeof(super_blk.blk_map));
	super_blk.blk_map[0] = 1;
	super_blk.blk_used = 1;

	inode_num = 0;//将当前目录改为根目录

				  /*读取根目录的i节点*/
	fseek(Disk, InodeBeg, SEEK_SET);
	fread(&curr_inode, sizeof(Inode), 1, Disk);
	//	printf("%d\n",curr_inode.file_size/sizeof(Dir));

	curr_inode.file_size = 2 * sizeof(Dir);
	curr_inode.blk_num = 1;
	curr_inode.blk_identifier[0] = 0;//第零块磁盘一定是根目录的
	curr_inode.type = Directory;
	curr_inode.access[0] = 1;//可读
	curr_inode.access[1] = 1;//可写
	curr_inode.access[2] = 1;//可执行
	curr_inode.i_atime = time(NULL);
	curr_inode.i_ctime = time(NULL);
	curr_inode.i_mtime = time(NULL);

	fseek(Disk, InodeBeg, SEEK_SET);
	fwrite(&curr_inode, sizeof(Inode), 1, Disk);

	dir_num = 2;//仅.和..目录项有效

	strcpy(dir_table[0].name, ".");
	dir_table[0].inode_num = 0;
	strcpy(dir_table[1].name, "..");
	dir_table[1].inode_num = 0;

	strcpy(path, "monster@root:");

	return 1;
}