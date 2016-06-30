#pragma once
#ifndef PATH_H
#define PATH_H
#include "const.h"

void change_path(char*);
int eat_path(char* name);//解析文件路径名,进入到最终目录下，并将name修改为最后一项的文件名
int split(char dst[][NameLength], char* str, const char* spl);//路径名分词
int check_name(int, char*);//检查重命名,返回-1表示名字不存在，否则返回相应inode
int type_check(char*);//确定文件的类型

#endif