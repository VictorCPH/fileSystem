#include "const.h"
#include "superblk.h"
#include <stdio.h>

extern SuperBlk	super_blk;//文件系统的超级块
/*申请未被使用的磁盘块*/
int get_blk()
{
	int i;
	super_blk.blk_used++;
	for (i = 0; i<BlkNum; ++i) {//找到未被使用的块
		if (!super_blk.blk_map[i]) {
			super_blk.blk_map[i] = 1;
			return i;
		}
	}

	return -1;//没有多余的磁盘块
}


/*释放磁盘块*/
int free_blk(int blk_pos)
{
	super_blk.blk_used--;
	super_blk.blk_map[blk_pos] = 0;
	return 0;
}

void show_disk_usage()//显示磁盘使用情况
{
	printf("Filesystem\t1K-blocks\tUsed\tAvailable\tUse%%\tMounted on\n");
	printf("%s\t%d\t\t%d\t%d\t\t%d\t%s\n", "myFileSystem", BlkNum, super_blk.blk_used,
		BlkNum - super_blk.blk_used, super_blk.blk_used * 100 / BlkNum, "/");
}

void show_inode_usage()//显示inode节点使用情况
{
	printf("Filesystem\tInodes\tIUsed\tIFree\tIUse%%\tMounted on\n");
	printf("%s\t%d\t%d\t%d\t%d\t%s\n", "myFileSystem", InodeNum, super_blk.inode_used,
		InodeNum - super_blk.inode_used, super_blk.inode_used * 100 / InodeNum, "/");
}