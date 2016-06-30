#pragma once
#ifndef DIR_H
#define DIR_H

typedef struct {
	char name[NameLength];//目录名
	short  inode_num;//目录对应的inode
}Dir;

int open_dir(int);//打开相应inode对应的目录
int close_dir(int);//保存相应inode的目录
int show_dir(int);//显示目录
int make_file(int, char*, int);//创建新的目录或文件
int del_file(int, char*, int);//删除子目录或子文件, remove_file的子程序，不含错误处理
int remove_file(int inode, char* name, int deepth, int type);//删除子目录或子文件
int enter_child_dir(int, char*);//进入子目录
int enter_dir(char* name);//进入任意目录
int adjust_dir(char*);//删除子目录后，调整原目录，使中间无空隙
void color(const unsigned short color1);//设置输出字符颜色

#endif