#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include "const.h"
#include "inode.h"
#include "superblk.h"
#include "dir.h"
#include "path.h"
#include "block.h"

extern Dir 	dir_table[MaxDirNum];//将当前目录文件的内容都载入内存
extern int 	dir_num;//相应编号的目录项数
extern int	 	inode_num;//当前目录的inode编号
extern Inode 	curr_inode;//当前目录的inode结构
extern SuperBlk	super_blk;//文件系统的超级块
extern FILE*	Disk;
extern char	path[40];

int open_dir(int inode)
{
	int		i;
	int 	pos = 0;
	int 	left;
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);

	/*读出相应的i节点*/
	fread(&curr_inode, sizeof(Inode), 1, Disk);
	//	printf("%d\n",curr_inode.file_size);

	for (i = 0; i<curr_inode.blk_num - 1; ++i) {
		fseek(Disk, BlockBeg + BlkSize*curr_inode.blk_identifier[i], SEEK_SET);
		fread(dir_table + pos, sizeof(Dir), DirPerBlk, Disk);
		pos += DirPerBlk;
	}

	/*left为最后一个磁盘块内的目录项数*/
	left = curr_inode.file_size / sizeof(Dir) - DirPerBlk*(curr_inode.blk_num - 1);
	fseek(Disk, BlockBeg + BlkSize*curr_inode.blk_identifier[i], SEEK_SET);
	fread(dir_table + pos, sizeof(Dir), left, Disk);
	pos += left;

	dir_num = pos;

	return 1;
}

int close_dir(int inode)
{
	int i, pos = 0, left;

	/*读出相应的i节点*/
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&curr_inode, sizeof(Inode), 1, Disk);

	/*数据写回磁盘块*/
	for (i = 0; i<curr_inode.blk_num - 1; ++i) {
		fseek(Disk, BlockBeg + BlkSize*curr_inode.blk_identifier[i], SEEK_SET);
		fwrite(dir_table + pos, sizeof(Dir), DirPerBlk, Disk);
		pos += DirPerBlk;
	}

	left = dir_num - pos;
	//	printf("left:%d",left);
	fseek(Disk, BlockBeg + BlkSize*curr_inode.blk_identifier[i], SEEK_SET);
	fwrite(dir_table + pos, sizeof(Dir), left, Disk);

	/*inode写回*/
	curr_inode.file_size = dir_num*sizeof(Dir);
	fseek(Disk, InodeBeg + inode*sizeof(Inode), SEEK_SET);
	fwrite(&curr_inode, sizeof(curr_inode), 1, Disk);

	return 1;

}


/*创建新的目录项*/
int make_file(int inode, char* name, int type)
{
	char original_name_path[30];
	int original_inode = inode_num;//记录当前的inode

	strcpy(original_name_path, name);
	if (eat_path(name) == -1) {
		if (type == File)
			printf("touch: cannot touch‘%s’: No such file or directory\n", original_name_path);
		if (type == Directory)
			printf("mkdir: cannot create directory ‘%s’: No such file or directory\n", original_name_path);
		return -1;
	}

	int new_node;
	int blk_need = 1;//本目录需要增加磁盘块则blk_need=2
	int t;
	Inode temp;

	/*读取当前目录的Inode*/
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	if (temp.access[1] == 0) { //当前目录不允许写
		if (type == Directory)
			printf("mkdir: cannot create directory ‘%s’: Permission denied\n", original_name_path);
		if (type == File)
			printf("touch: cannot touch ‘%s’: Permission denied\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}
	if (dir_num>MaxDirNum) {//超过了目录文件能包含的最大目录项
		if (type == Directory)
			printf("mkdir: cannot create directory '%s' : Directory full\n", original_name_path);
		if (type == File)
			printf("touch: cannot create file '%s' : Directory full\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	if (check_name(inode, name) != -1) {//防止重命名
		if (type == Directory)
			printf("mkdir: cannnot create directory '%s' : Directory exist\n", original_name_path);
		if (type == File)
			printf("touch: cannot create file '%s' : File exist\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	if (dir_num / DirPerBlk != (dir_num + 1) / DirPerBlk) {//本目录也要增加磁盘块
		blk_need = 2;
	}

	//	printf("blk_used:%d\n",super_blk.blk_used);
	if (super_blk.blk_used + blk_need>BlkNum) {
		if (type == Directory)
			printf("mkdir: cannot create directory '%s' :Block used up\n", original_name_path);
		if (type == File)
			printf("touch: cannot create file '%s' : Block used up\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	if (blk_need == 2) {//本目录需要增加磁盘块
		t = curr_inode.blk_num++;
		curr_inode.blk_identifier[t] = get_blk();
	}

	/*申请inode*/
	new_node = apply_inode();

	if (new_node == -1) {
		if (type == Directory)
			printf("mkdir: cannot create directory '%s' :Inode used up\n", original_name_path);
		if (type == File)
			printf("touch: cannot create file '%s' : Inode used up\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	if (type == Directory) {
		/*初始化新建目录的inode*/
		init_dir_inode(new_node, inode);
	}
	else if (type == File) {
		/*初始化新建文件的inode*/
		init_file_inode(new_node);
	}

	strcpy(dir_table[dir_num].name, name);
	dir_table[dir_num++].inode_num = new_node;

	close_dir(inode_num);
	inode_num = original_inode;
	open_dir(inode_num);
	return 0;
}


/*显示目录内容*/
int show_dir(int inode)
{
	int i;
	Inode temp;
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	if (temp.access[0] == 0) { //目录无读写权限
		printf("ls: cannot open directory .: Permission denied\n");
		return -1;
	}
	for (i = 0; i<dir_num; ++i) {
		char name_suffix[NameLength];
		strcpy(name_suffix, dir_table[i].name + strlen(dir_table[i].name) - 4);

		int inode_tmp = check_name(inode_num, dir_table[i].name);
		Inode tempInode;
		fseek(Disk, InodeBeg + sizeof(Inode)*inode_tmp, SEEK_SET);
		fread(&tempInode, sizeof(Inode), 1, Disk);

		if (type_check(dir_table[i].name) == Directory) {
			color(11);
			printf("%s\t", dir_table[i].name);
		}
		else if (strcmp(name_suffix, ".zip") == 0) {
			color(12);
			printf("%s\t", dir_table[i].name);
		}
		else if (tempInode.access[2] == 1){
			color(10);
			printf("%s\t", dir_table[i].name);
		}
		else{
			color(7);
			printf("%s\t", dir_table[i].name);
		}
		//printf("%d  ", type_check(dir_table[i].name));
		if (!((i+1) % 5)) printf("\n");//5个一行
	}
	if (dir_num % 5 != 0) printf("\n");
	color(7);
	return 1;
}



/*进入子目录*/
int enter_child_dir(int inode, char* name)
{
	if (type_check(name) != Directory) {
		return -1;
	}

	int child;
	child = check_name(inode, name);

	/*关闭当前目录,进入下一级目录*/
	close_dir(inode);
	inode_num = child;
	open_dir(child);

	return 1;
}

int adjust_dir(char* name)
{
	int pos;
	for (pos = 0; pos<dir_num; ++pos) {
		/*先找到被删除的目录的位置*/
		if (strcmp(dir_table[pos].name, name) == 0)
			break;
	}
	for (pos++; pos<dir_num; ++pos) {
		/*pos之后的元素都往前移动一位*/
		dir_table[pos - 1] = dir_table[pos];
	}

	dir_num--;
	return 1;
}

/*递归删除文件夹*/
int del_file(int inode, char* name, int deepth)
{
	int 	child, i, t;
	Inode	temp;

	if (!strcmp(name, ".") || !strcmp(name, "..")) {
		/*不允许删除.和..*/
		printf("rmdir: failed to remove '%s': Invalid argument\n", name);
		return -1;
	}

	child = check_name(inode, name);

	/*读取当前子目录的Inode结构*/
	fseek(Disk, InodeBeg + sizeof(Inode)*child, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	if (temp.type == File) {
		/*如果是文件则释放相应Inode即可*/
		free_inode(child);
		/*若是最上层文件，需调整目录*/
		if (deepth == 0) {
			adjust_dir(name);
		}
		return 1;
	}
	else {
		/*否则进入子目录*/
		enter_child_dir(inode, name);
	}

	for (i = 2; i<dir_num; ++i) {
		del_file(child, dir_table[i].name, deepth + 1);
	}

	enter_child_dir(child, "..");//返回上层目录
	free_inode(child);

	if (deepth == 0) {
		/*删除自身在目录中的内容*/
		if (dir_num / DirPerBlk != (dir_num - 1) / DirPerBlk) {
			/*有磁盘块可以释放*/
			curr_inode.blk_num--;
			t = curr_inode.blk_identifier[curr_inode.blk_num];
			free_blk(t);//释放相应的磁盘块
		}
		adjust_dir(name);//因为可能在非末尾处删除，因此要移动dir_table的内容
	}/*非初始目录直接释放Inode*/

	return 1;
}

int enter_dir(char* name)
{
	int tmp = inode_num;//记录原始inode节点
	char tmpPath[40], nameCopy[30];
	char dst[30][NameLength];

	strcpy(tmpPath, path);
	strcpy(nameCopy, name);
	int cnt = split(dst, nameCopy, "/");
	if (name[0] == '/') {//从根目录开始
		 //printf("1111111111\n");
		close_dir(inode_num);
		inode_num = 0;
		open_dir(inode_num);
		strcpy(path, "monitor@root:");
	}
	for (int i = 0; i < cnt; i++) {
		//printf("%d\n", i);
		int res = enter_child_dir(inode_num, dst[i]);
		change_path(dst[i]);
		if (res == -1) {
			inode_num = tmp;
			open_dir(inode_num);
			strcpy(path, tmpPath);
			return -1;
		}
	}
	return 0;
}

int remove_file(int inode, char* name, int deepth, int type)
{
	char original_name_path[30];
	int original_inode = inode_num;//记录当前的inode

	strcpy(original_name_path, name);
	if (eat_path(name) == -1) {
		if (type == Directory)
			printf("rmdir: failed to remove‘%s’: No such file or directory\n", original_name_path);
		if (type == File)
			printf("rm: cannot remove‘%s’: No such file or directory\n", original_name_path);
		return -1;
	}

	int check_type_result = type_check(name);
	if (check_type_result == -1) {//要删除的文件不存在
		if (type == Directory)
			printf("rmdir: failed to remove '%s': No such file or directory\n", original_name_path);
		if (type == File)
			printf("rm: cannot remove '%s': No such file or directory\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	Inode father_inode;
	/*读取要删除的目录或文件的父目录的Inode*/
	fseek(Disk, InodeBeg + sizeof(Inode)*inode_num, SEEK_SET);
	fread(&father_inode, sizeof(father_inode), 1, Disk);

	if (father_inode.access[1] == 0) { //要删除的目录或文件的父目录不允许写
		if (type == Directory)
			printf("rmdir: failed to remove ‘%s’: Permission denied\n", original_name_path);
		if (type == File)
			printf("rm: cannot remove‘%s’: Permission denied\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	if (check_type_result == Directory && type == File) {//删除的文件类型与对应指令不符，如：尝试用rm删除目录
		printf("rm: cannot remove '%s': Not a file\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}
	if (check_type_result == File && type == Directory) {//删除的文件类型与对应指令不符，如:用rmdir删除文件
		printf("rmdir: failed to remove '%s': Not a directory\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	del_file(inode_num, name, 0);

	close_dir(inode_num);
	inode_num = original_inode;
	open_dir(inode_num);
	return 0;
}

void color(const unsigned short color1)
{        /*仅限改变0-15的颜色;如果在0-15那么实现他的颜色   因为如果超过15后面的改变的是文本背景色。*/
	if (color1 >= 0 && color1 <= 15)
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color1);
	/*如果不在0-15的范围颜色，那么改为默认的颜色白色；*/
	else
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}