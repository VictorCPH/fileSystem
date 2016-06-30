#include <stdio.h>
#include <string.h>
#include "inode.h"
#include "const.h"
#include "superblk.h"
#include "block.h"
#include "dir.h"

extern SuperBlk	super_blk;//文件系统的超级块
extern FILE*	Disk;

/*申请inode*/
int apply_inode()
{
	int i;

	if (super_blk.inode_used >= InodeNum) {
		return -1;//inode节点用完
	}

	super_blk.inode_used++;

	for (i = 0; i<InodeNum; ++i) {
		if (!super_blk.inode_map[i]) {//找到一个空的i节点
			super_blk.inode_map[i] = 1;
			return i;
		}
	}
	return 0;
}

int free_inode(int inode)
{
	Inode 	temp;
	int 	i;
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	for (i = 0; i<temp.blk_num; ++i) {
		free_blk(temp.blk_identifier[i]);
	}

	super_blk.inode_map[inode] = 0;
	super_blk.inode_used--;

	return 1;
}

/*初始化新建文件的indoe*/
int init_file_inode(int inode)
{
	Inode temp;
	/*读取相应的Inode*/
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	temp.blk_num = 0;
	temp.type = File;
	temp.file_size = 0;
	temp.access[0] = 1;//可读
	temp.access[1] = 1;//可写
	temp.access[2] = 0;//不可执行
	temp.i_atime = time(NULL);
	temp.i_ctime = time(NULL);
	temp.i_mtime = time(NULL);

	/*将已经初始化的Inode写回*/
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fwrite(&temp, sizeof(Inode), 1, Disk);

	return 1;
}


/*初始化新建目录的inode*/
int init_dir_inode(int child, int father)
{
	Inode	temp;
	Dir 	dot[2];
	int		blk_pos;

	fseek(Disk, InodeBeg + sizeof(Inode)*child, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	blk_pos = get_blk();//获取新磁盘块的编号

	temp.blk_num = 1;
	temp.blk_identifier[0] = blk_pos;
	temp.type = Directory;
	//printf("temp.type = %d\n", temp.type);
	temp.file_size = 2 * sizeof(Dir);
	temp.access[0] = 1;//可读
	temp.access[1] = 1;//可写
	temp.access[2] = 1;//可执行
	temp.i_atime = time(NULL);
	temp.i_ctime = time(NULL);
	temp.i_mtime = time(NULL);

	/*将初始化完毕的Inode结构写回*/
	fseek(Disk, InodeBeg + sizeof(Inode)*child, SEEK_SET);
	fwrite(&temp, sizeof(Inode), 1, Disk);

	strcpy(dot[0].name, ".");//指向目录本身
	dot[0].inode_num = child;

	strcpy(dot[1].name, "..");
	dot[1].inode_num = father;

	/*将新目录的数据写进数据块*/
	fseek(Disk, BlockBeg + BlkSize*blk_pos, SEEK_SET);
	fwrite(dot, sizeof(Dir), 2, Disk);

	return 1;
}