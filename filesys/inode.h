#pragma once
#ifndef INODE_H
#define INODE_H
#include <time.h>
#include "const.h"

typedef struct {
	int blk_identifier[BlkPerNode];//占用的磁盘块编号
	int blk_num;//占用的磁盘块数目
	int file_size;//文件的大小
	int type;//文件的类型
	int access[3];//权限
	time_t i_atime;//最后访问时间
	time_t i_mtime;//最后修改(modify)时间
	time_t i_ctime;//最后改变(change)时间
}Inode;


int free_inode(int);//释放相应的inode
int apply_inode();//申请inode,返还相应的inode号，返还-1则INODE用完
int init_dir_inode(int, int);//初始化新建目录的inode
int init_file_inode(int);//初始化新建文件的inode

#endif
