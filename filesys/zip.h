#pragma once
#ifndef ZIP_H
#define ZIP_H
#include <stdio.h>
#include "const.h"
#include "inode.h"

typedef struct Hnode//哈夫曼节点
{
	int weight;
	int date;
	struct Hnode *lch, *rch;
	struct Hnode * next;
}node, *link;

typedef struct Mnode//哈夫曼编码表节点
{
	int ch;
	char s[HaffCodeLen];
}mapnode, *maptr;

int zip(char* zip_name, char* name);//zip指令实现，文件压缩
int unzip(char* name);//文件解压
void StaFrequency();//统计频率
link CreatFreQueue();//频率队列
link CreatHuff();//建立哈夫曼树
void HuffCode(link root, char s[]);//进行哈夫曼编码
void Compression(char* name);//压缩文件，将文件中的字符转换成01字符串，再将字符串转换成int
int DeCompression();//解压文件
void PrintfTree(link root, FILE *fp);//打印哈夫曼树
link InputTree(FILE *fp);//读取恢复哈夫曼树
int zip_write(char* name);//压缩后的数据写入对应的压缩文件中
int getCompressionRatio(int inodeBefore, int inodeAfter);//获得压缩率
int unzip_write(char* name);//将解压后的数据写入解压文件

#endif