#pragma once
#ifndef CONST_H
#define CONST_H

#define InodeNum		1024//i节点数目
#define BlkNum			(80*1024)//磁盘块的数目
#define BlkSize			1024//磁盘块大小为1K
#define BlkPerNode		1024//每个文件包含的最大的磁盘块数目
#define DISK 			"disk.txt"
#define BUFF			"buff.txt"//读写文件时的缓冲文件
#define SuperBeg		0//超级块的起始地址
#define InodeBeg		sizeof(SuperBlk)//i节点区起始地址
#define BlockBeg		(InodeBeg+InodeNum*sizeof(Inode))//数据区起始地址
#define MaxDirNum		(BlkPerNode*(BlkSize/sizeof(Dir)))//每个目录最大的文件数
#define DirPerBlk		(BlkSize/sizeof(Dir))//每个磁盘块包含的最大目录项
#define Directory		0
#define File			1
#define CommanNum		(sizeof(command)/sizeof(char*))//指令数目
#define NameLength		30 //文件名长度
#define ValueSize		256 //权值统计中，一个字节有0~255种可能取值
#define HaffCodeLen		128//单个字符的哈夫曼编码后的最大码长
#define ZipMagicNumber	0xA8EF1314//魔数
#define MagicSize		sizeof(ZipMagicNumber)//魔数长度

#endif