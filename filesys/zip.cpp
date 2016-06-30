#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "zip.h"
#include "path.h"
#include "dir.h"
#include "file.h"
#include "block.h"
#include "const.h"
#include "superblk.h"

maptr map;//哈夫曼编码表
int len;//哈夫曼编码表的表长
int count = 0;
int weight[ValueSize] = { 0 };//权值统计
int supple;//01串长补位长
char FILENAME[NameLength];//被压缩的文件的文件名
Inode unzip_file_inode;//被压缩的文件的inode信息

extern int inode_num;//当前目录的inode编号
extern FILE* Disk;
extern Dir 	dir_table[MaxDirNum];//将当前目录文件的内容都载入内存
extern int 	dir_num;//相应编号的目录项数
extern SuperBlk	super_blk;//文件系统的超级块
extern Inode curr_inode;//当前目录的inode结构

int zip(char* zip_name, char* name)
{
	char original_name_path[NameLength];
	char original_zip_name_path[NameLength];
	int original_inode = inode_num;//记录当前的inode
	int inode_source_file, inode_zip_file;
	
	strcpy(original_name_path, name); //记录name
	strcpy(original_zip_name_path, zip_name);//记录zip_name

	if (eat_path(name) == -1) {//路径解析出错
		printf("zip warning: name not matched: %s\n", original_name_path);
		printf("zip error: Nothing to do!(%s)\n", original_zip_name_path);
		return -1;
	}

	if (type_check(name) == Directory) {//压缩的文件名是目录，不允许
		printf("zip warning: cannot zip '%s': Not a file\n", name);
		close_dir(inode_num);
	    inode_num = original_inode;
	    open_dir(inode_num);
		return -1;
	}

	if (type_check(name) == -1) {//要压缩的文件不存在
		printf("zip warning: name not matched: %s\n", original_name_path);
		printf("zip error: Nothing to do!(%s)\n", original_zip_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	if (file_read(name) == -1) {//文件读取失败，不可读
		printf("adding: %s\n", original_name_path);
		printf("zip warning: Permission denied\n");
		printf("zip warning: could not open for reading: %s\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	inode_source_file = check_name(inode_num, name);//记录原文件的inode

	close_dir(inode_num);
	inode_num = original_inode;
	open_dir(inode_num);//返回操作前的目录位置

	if (eat_path(zip_name) == -1) {//路径解析出错
		printf("zip I/O error: No such file or directory\n");
		printf("zip error: Could not create output file (%s.zip)\n", original_zip_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	link root = NULL;
	char s[HaffCodeLen] = { 0 };

	map = (maptr)malloc(sizeof(mapnode)*ValueSize);//创建空的哈希编码表

	strcpy(FILENAME, BUFF);//要压缩的文件名填入

	StaFrequency();//统计频率

	root = CreatHuff();//生成哈夫曼树

	HuffCode(root, s);//生成哈夫曼编码表
	len = count;//获得表长

	Compression(name);//压缩文件

	strcat(zip_name, ".zip");
	if (check_name(inode_num, zip_name) == -1) { //目标文件不存在，则创建
		make_file(inode_num, zip_name, File);//创建空的压缩文件
		zip_write(zip_name);//将压缩后的数据写入压缩文件
		inode_zip_file = check_name(inode_num, zip_name);//记录压缩文件的inode

		if (root == NULL) {//原文件为空
			printf("adding: %s (stored 0%%)\n", original_name_path);
		}
		else {//原文件不为空
			printf("adding: %s (deflated %d%%)\n", original_name_path,
			getCompressionRatio(inode_source_file, inode_zip_file));
		}
	}
	else {//目标文件存在，则更新
		zip_write(zip_name);//将压缩后的数据写入压缩文件
		inode_zip_file = check_name(inode_num, zip_name);//记录压缩文件的inode

		if (root == NULL) {//原文件为空
			printf("updating: %s (stored 0%%)\n", original_name_path);
		}
		else {//原文件不为空
			printf("updating:  %s (deflated %d%%)\n", original_name_path,
			getCompressionRatio(inode_source_file, inode_zip_file));
		}
	}

	close_dir(inode_num);
	inode_num = original_inode;
	open_dir(inode_num);
	return 0;
}

int unzip(char* name)
{
	char original_name_path[30];
	int original_inode = inode_num;//记录当前的inode
	strcpy(original_name_path, name);
	if (eat_path(name) == -1) {
		printf("unzip: cannot find or open‘%s'.\n", original_name_path);
		return -1;
	}

	if (type_check(name) == -1) {
		printf("unzip: cannot find or open‘%s'.\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	if (file_read(name) == -1) {//文件读取失败
		printf("error: cannot open zipfile [ %s ], permission denied\n", original_name_path);
		printf("unzip: cannot find or open‘%s'.\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	char s[HaffCodeLen] = { 0 };

	map = (maptr)malloc(sizeof(mapnode)*ValueSize);

	int res = DeCompression();//解压文件

	if (res == -1) {
		printf("unzip:  cannot find zipfile directory in %s, it is not a zipfile\n", original_name_path);
		return -1;
	}

	if (check_name(inode_num, FILENAME) == -1) {
		make_file(inode_num, FILENAME, File);//创建空的解压文件
		unzip_write(FILENAME);//将解压后的数据写入解压文件
	}
	else {
		printf("Archive: %s\n", original_name_path);

		char choice;
		char new_name[NameLength];
		int flag = 1;

		char ch;
		while ((ch = getchar()) != '\n' && ch != EOF);//清空输入缓冲区

		while (flag) {
			printf("replace %s? [y]es, [n]o, [r]ename: ", FILENAME);
			scanf("%c", &choice);

			switch (choice) {
				case 'y':
					unzip_write(FILENAME);//将解压后的数据写入解压文件
					flag = 0;
					break;

				case 'n':
					flag = 0;
					break;

				case 'r':
					printf("new name: ");
					scanf("%s", new_name);
					make_file(inode_num, new_name, File);//创建空的解压文件
					unzip_write(new_name);//将解压后的数据写入解压文件
					flag = 0;
					break;

				default:
					printf("error: invalid response\n");
					while ((ch = getchar()) != '\n' && ch != EOF);//指令错误，清空输入缓冲区
					break;
			}
		}
	}

	close_dir(inode_num);
	inode_num = original_inode;
	open_dir(inode_num);
	return 0;
}

void StaFrequency()
{
	FILE *fp;
	unsigned char c;

	memset(weight, 0, sizeof(int) * ValueSize);
	if ((fp = fopen(FILENAME, "rb")) == NULL) {
		printf("文件打开失败!\n");
		exit(0);
	}

	while (!feof(fp)) {
		if (fread(&c, 1, 1, fp) == 1)
			weight[int(c)]++;
	}
	fclose(fp);
}

link CreatFreQueue()
{
	int i, length = 0;
	link q, front, rear;

	front = rear = (link)malloc(sizeof(node));
	front->next = NULL;

	for (i = 0; i <= 255; i++)
		if (weight[i] != 0) {
			q = (link)malloc(sizeof(node));

			if (q) {
				q->weight = weight[i];
				q->lch = NULL;
				q->rch = NULL;
				q->date = i;
				q->next = NULL;
				length++;

				rear->next = q;
				rear = rear->next;
			}
		}
	front->weight = length;//头结点的weight记录队列长度
	return front;
}

link CreatHuff()
{
	link front, minnode1, minnode2, q, r;//minnode1最小节点，minnode2次小节点
	link parent, temp;
	int length, i;

	front = CreatFreQueue();
	length = front->weight;

	for (i = 1; i <= length - 1; i++) {
		minnode1 = front->next;
		minnode2 = minnode1->next;

		if (minnode1->weight > minnode2->weight) {
			temp = minnode1;
			minnode1 = minnode2;
			minnode2 = temp;
		}

		for (q = front->next->next->next; q != NULL; q = q->next) {
			if (minnode1->weight > q->weight) {
				minnode2 = minnode1;
				minnode1 = q;
			}
			else
				if (minnode2->weight > q->weight)
					minnode2 = q;
		}

		parent = (link)malloc(sizeof(node));
		parent->date = 0;
		parent->lch = minnode1;
		parent->rch = minnode2;
		parent->weight = minnode1->weight + minnode2->weight;

		for (r = front, q = front->next; q != NULL; q = q->next, r = r->next) {//两最小节点删除
			if (q == minnode1) {
				r->next = q->next;
				break;
			}
		}
		for (r = front, q = front->next; q != NULL; q = q->next, r = r->next) {//两最小节点删除
			if (q == minnode2) {
				r->next = q->next;
				break;
			}
		}

		parent->next = front->next;//合成的父亲节点插入队列
		front->next = parent;
	}
	return front->next;
}

void HuffCode(link root, char s[])
{
	char s1[HaffCodeLen] = { 0 },
		s2[HaffCodeLen] = { 0 };

	if (root == NULL)
		return;

	if (root->lch) {
		strcpy(s1, s);
		strcat(s1, "0");
		HuffCode(root->lch, s1);
	}

	if (root->rch) {
		strcpy(s2, s);
		strcat(s2, "1");
		HuffCode(root->rch, s2);
	}

	if (root->lch == NULL && root->rch == NULL) {
		map[count].ch = root->date;
		strcpy(map[count].s, s);
		//	printf("%d  %s\n", root->date,map[count].s);
		count++;
	}
}

void Compression(char* name)
{
	int i, j, k;
	__int64 length = 0;
	unsigned char ch;
	int key;
	char s1[9] = { 0 };
	link root;
	FILE *fp, *fout, *fout1, *fp1;

	if ((fp = fopen(FILENAME, "rb")) == NULL) {
		printf("原始文件打开失败!\n");
		exit(0);
	}

	if ((fout = fopen("Compress.txt", "wb")) == NULL) {
		printf("压缩文件打开失败!\n");
		exit(0);
	}

	if ((fout1 = fopen("code.txt", "wb")) == NULL) {
		printf("编码文件打开失败!\n");
		exit(0);
	}

	root = CreatHuff();
	fprintf(fout, "%x ", ZipMagicNumber);//文件头为zip格式文件的魔数
	fprintf(fout, "%s ", name);//被压缩的文件的文件名
	PrintfTree(root, fout);

	while (!feof(fp)) {
		fread(&ch, 1, 1, fp);
		for (i = 0; i <= len; i++)
			if (int(ch) == map[i].ch) {
				fprintf(fout1, "%s", map[i].s);
				length = length + strlen(map[i].s);
			}
	}

	fclose(fp);

	if (length % 8 != 0) {//补成8的倍数
		for (i = 1; i <= (8 - length % 8); i++)
			fprintf(fout1, "0");
		supple = 8 - length % 8;
	}
	fclose(fout1);
	fp1 = fopen("code.txt", "rb");

	if (length % 8 != 0)
		length = length + (8 - length % 8);

	for (i = 1; i <= length / 8; i++) {
		key = 0;

		for (k = 0; k<8; k++) {
			ch = fgetc(fp1);
			s1[k] = ch;
		}

		for (j = 0; j<8; j++)
			key = key + (s1[j] - 48)*(int)pow(2, 8 - j - 1);
		fprintf(fout, "%c", key);
	}
	fclose(fout);
}

int DeCompression()
{
	char ch, s1[9] = { 0 };
	unsigned char key;
	__int64 i, length = 0;
	link root, q;
	FILE *fp, *fout, *fout1, *fp1;

	fp = fopen(BUFF, "rb");
	fout = fopen("DeCompress.txt", "wb");
	fout1 = fopen("decode.txt", "wb");

	int test_magic_number;
	fscanf(fp, "%x", &test_magic_number);
	//printf("%x\n", test_magic_number);

	if (test_magic_number != ZipMagicNumber) {
		return -1;
	}

	fscanf(fp, "%s", &FILENAME);//原文件的文件名
	//printf("%s\n", FILENAME);
	root = InputTree(fp);

	fgetc(fp);
	while (!feof(fp)) {
		fread(&key, 1, 1, fp);

		if (feof(fp)) break;

		for (i = 7; i >= 0; i--) {
			s1[i] = ((int)key) % 2 + 48;
			key /= 2;
		}

		s1[8] = '\0';
		fprintf(fout1, "%s", s1);
		length = length + 8;
	}
	fclose(fp);
	fclose(fout1);

	fp1 = fopen("decode.txt", "rb");

	q = root;
	i = 0;
	while (i<(length - supple)) {
		ch = fgetc(fp1);

		if (!q->lch && !q->rch) {
			if (q->date > 127)
				fprintf(fout, "%c", q->date - 256);
			else
				fprintf(fout, "%c", q->date);
			q = root;
		}

		if (ch == '0' && q->lch != NULL)
			q = q->lch;
		else if (ch && q->rch != NULL)
			q = q->rch;

		i++;
	}
	fclose(fp1);
	fclose(fout);

	return 0;
}

void PrintfTree(link root, FILE *fp)
{
	if (root) {
		fprintf(fp, "%d ", root->date);

		PrintfTree(root->lch, fp);
		PrintfTree(root->rch, fp);
	}
	else
		fprintf(fp, "-1 ");

}

link InputTree(FILE *fp)
{
	int date;
	link p = NULL;

	fscanf(fp, "%d", &date);

	if (date != -1) {
		p = (link)malloc(sizeof(node));
		p->date = date;

		p->lch = InputTree(fp);
		p->rch = InputTree(fp);
	}

	return p;
}

/*压缩后的数据写入对应的压缩文件中*/
int zip_write(char* name)
{
	int 	inode;
	int		num, blk_num;
	FILE* 	fp = fopen("Compress.txt", "rb");
	Inode	temp;
	char	buff[BlkSize];

	inode = check_name(inode_num, name);

	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	if (temp.access[1] == 0) { //文件不可写
		printf("%d\n", temp.access[1]);
		return -1;
	}

	//将原文件内容对应地磁盘块释放，文件大小设为0，
	//然后重新申请磁盘块存放修改后的文件内容，更新文件的大小和修改时间，属性改变时间
	temp.blk_num = 0;
	temp.file_size = 0;

	while (num = fread(buff, sizeof(char), BlkSize, fp)) {
		printf("num:%d\n", num);
		if ((blk_num = get_blk()) == -1) {
			printf("error:	block has been used up\n");
			break;
		}
		/*改变Inode结构的相应状态*/
		temp.blk_identifier[temp.blk_num++] = blk_num;
		temp.file_size += num;

		/*将数据写回磁盘块*/
		fseek(Disk, BlockBeg + BlkSize*blk_num, SEEK_SET);
		fwrite(buff, sizeof(char), num, Disk);
	}
	temp.i_mtime = time(NULL);
	temp.i_ctime = time(NULL);

	/*将修改后的Inode写回*/
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fwrite(&temp, sizeof(Inode), 1, Disk);

	fclose(fp);
	return 1;
}

int getCompressionRatio(int inodeBefore, int inodeAfter)
{
	Inode temp1, temp2;

	fseek(Disk, InodeBeg + sizeof(Inode)*inodeBefore, SEEK_SET);
	fread(&temp1, sizeof(Inode), 1, Disk);

	fseek(Disk, InodeBeg + sizeof(Inode)*inodeAfter, SEEK_SET);
	fread(&temp2, sizeof(Inode), 1, Disk);

	return temp2.file_size * 100 / temp1.file_size;
}

int unzip_write(char* name)//将解压后的数据写入解压文件
{
	int 	inode;
	int		num, blk_num;
	FILE* 	fp = fopen("Decompress.txt", "rb");
	Inode	temp;
	char	buff[BlkSize];

	inode = check_name(inode_num, name);

	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	if (temp.access[1] == 0) { //文件不可写
		printf("%d\n", temp.access[1]);
		return -1;
	}

	//将原文件内容对应地磁盘块释放，文件大小设为0，
	//然后重新申请磁盘块存放修改后的文件内容，更新文件的大小和修改时间，属性改变时间
	temp.blk_num = 0;
	temp.file_size = 0;

	while (num = fread(buff, sizeof(char), BlkSize, fp)) {
		printf("num:%d\n", num);
		if ((blk_num = get_blk()) == -1) {
			printf("error:	block has been used up\n");
			break;
		}
		//改变Inode结构的相应状态
		temp.blk_identifier[temp.blk_num++] = blk_num;
		temp.file_size += num;

		//将数据写回磁盘块
		fseek(Disk, BlockBeg + BlkSize*blk_num, SEEK_SET);
		fwrite(buff, sizeof(char), num, Disk);
	}

	temp.i_mtime = time(NULL);
	temp.i_ctime = time(NULL);
	//将修改后的Inode写回
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fwrite(&temp, sizeof(Inode), 1, Disk);

	fclose(fp);
	return 1;
}
