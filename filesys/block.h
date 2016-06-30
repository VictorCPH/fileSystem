#pragma once
#ifndef BLOCK_H
#define BLOCK_H

int free_blk(int);//释放相应的磁盘块
int get_blk(void);//获取磁盘块
void show_disk_usage();//显示磁盘使用情况
void show_inode_usage();//显示inode节点使用情况

#endif