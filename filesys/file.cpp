#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include "file.h"
#include "inode.h"
#include "path.h"
#include "dir.h"
#include "const.h"
#include "superblk.h"
#include "block.h"

extern Dir 	dir_table[MaxDirNum];//将当前目录文件的内容都载入内存
extern int	 	inode_num;//当前目录的inode编号
extern FILE*	Disk;
extern int 	dir_num;//相应编号的目录项数
extern char	path[40];
extern FILETIME BuffModifyTimeBeforeEdit;
extern FILETIME BuffModifyTimeAfterEdit;

FILETIME getBuffModifyTime();//获取临时文件的文件修改时间
/*读文件函数*/
int file_read(char* name)
{
	int 	inode, i, blk_num;
	Inode	temp;
	FILE*	fp = fopen(BUFF, "wb+");
	char 	buff[BlkSize];
	//printf("read\n");

	inode = check_name(inode_num, name);

	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	int file_size = temp.file_size;
	if (temp.access[0] == 0) { //文件不可读，则直接退出
		return -1;
	}
	if (temp.blk_num == 0) {//如果源文件没有内容,则直接退出
		fclose(fp);
		return 0;
	}
	printf("read\n");
	for (i = 0; i<temp.blk_num - 1; ++i) {
		blk_num = temp.blk_identifier[i];
		/*读出文件包含的磁盘块*/
		fseek(Disk, BlockBeg + BlkSize*blk_num, SEEK_SET);
		fread(buff, sizeof(char), BlkSize, Disk);
		/*写入BUFF*/
		fwrite(buff, sizeof(char), BlkSize, fp);
		file_size -= BlkSize;
	}

	/*最后一块磁盘块可能未满*/
	blk_num = temp.blk_identifier[i];
	fseek(Disk, BlockBeg + BlkSize*blk_num, SEEK_SET);
	fread(buff, sizeof(char), file_size, Disk);
	fwrite(buff, sizeof(char), file_size, fp);

	/*修改inode信息*/
	temp.i_atime = time(NULL);

	/*将修改后的Inode写回*/
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fwrite(&temp, sizeof(Inode), 1, Disk);

	fclose(fp);
	return 0;
}


/*写文件函数*/
int file_write(char* name)
{
	int 	inode;
	int		num, blk_num;
	FILE* 	fp = fopen(BUFF, "rb");
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
	//	printf("file_size:%d\n blocks: %d\n", temp.file_size, temp.blk_num);
		/*将数据写回磁盘块*/
		fseek(Disk, BlockBeg + BlkSize*blk_num, SEEK_SET);
		fwrite(buff, sizeof(char), num, Disk);
	}
	temp.i_mtime = time(NULL);
	temp.i_ctime = time(NULL);


	/*将修改后的Inode写回*/
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fwrite(&temp, sizeof(Inode), 1, Disk);

	/*
	Inode temp2;
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&temp2, sizeof(Inode), 1, Disk);*/

//	printf("file_size:%d\n blocks: %d\n", temp2.file_size, temp2.blk_num);

	fclose(fp);
	return 1;
}

int file_copy(char* name, char* cpname)
{
	int originalInode = inode_num;//记录当前的inode
	int source_inode_num, dest_inode_num;
	char originalNamePath[30];
	char originalCpNamePath[30];

	strcpy(originalNamePath, name);
	strcpy(originalCpNamePath, cpname);

	if (eat_path(name) == -1) {
		printf("cp: cannot stat ‘%s’: No such file or directory\n", originalNamePath);
		return -1;
	}
	if (type_check(name) != File) {
		printf("cp: cannot copy '%s': Not a file\n", originalNamePath);
		close_dir(inode_num);
		inode_num = originalInode;
		open_dir(inode_num);//返回操作前的目录位置
		return -1;
	}

	if (file_read(name) == -1) {//若原文件不可读
		printf("cp: cannot open '%s' for reading: Permission denied\n", originalNamePath);
		close_dir(inode_num);
		inode_num = originalInode;
		open_dir(inode_num);//返回操作前的目录位置
		return -1;
	}
	source_inode_num = inode_num;//记录原文件的父目录节点

	close_dir(inode_num);
	inode_num = originalInode;
	open_dir(inode_num);//返回操作前的目录位置

	if (eat_path(cpname) == -1) {
		printf("cp: cannot stat ‘%s’: No such file or directory\n", originalCpNamePath);
		return -1;
	}
	dest_inode_num = inode_num;//记录目标目录的父目录节点

	if (source_inode_num == dest_inode_num && strcmp(name, cpname) == 0) {
		printf("cp: '%s' and '%s' are the same file\n", originalNamePath, originalCpNamePath);
		close_dir(inode_num);
		inode_num = originalInode;
		open_dir(inode_num);//返回操作前的目录位置
		return -1;
	}

	Inode temp;

	/*将原文件拷贝到另一文件中*/
	if (type_check(cpname) != Directory) {
		if (check_name(inode_num, cpname) != -1) {//目标文件已经存在，则将数据拷贝进去
			if (file_write(cpname) == -1) { //若目标文件不可写
				printf("cp: cannot create regular file‘%s’: Permission denied\n", originalCpNamePath);
				close_dir(inode_num);
				inode_num = originalInode;
				open_dir(inode_num);//返回操作前的目录位置
				return -1;
			}
		}
		else {									 //目标文件不存在，创建文件，并将数据拷贝进去
			/*读取目标目录的inode节点，判断该目录是否可写*/
			fseek(Disk, InodeBeg + sizeof(Inode)*dest_inode_num, SEEK_SET);
			fread(&temp, sizeof(Inode), 1, Disk);

			if (temp.access[1] == 0) { //目标目录不可写
				printf("cp: cannot create regular file ‘%s’: Permission denied\n", originalCpNamePath);
				close_dir(inode_num);
				inode_num = originalInode;
				open_dir(inode_num);//返回操作前的目录位置
				return -1;
			}
			make_file(inode_num, cpname, File);
			file_write(cpname);//将数据从BUFF写入文件
		}		
	}
	/*将原文件拷贝到某个子目录下，包括当前目录(.), 原目录的父目录（..）*/
	else {
		if (source_inode_num == dest_inode_num && strcmp(cpname, ".") == 0) {//将当前目录下的某个文件复制到当前目录，提示已经存在
			printf("cp: '%s' and '%s/%s' are the same file\n", originalNamePath, originalCpNamePath, name);
			return -1;
		}

		enter_child_dir(inode_num, cpname);//进入子目录或父目录

		/*记录原目录名*/
		int pos = strlen(path) - 1; 
		for (; pos >= 0; --pos) {
			if (path[pos] == '/') {
				break;
			}
		}
		char curDirName[30];
		int i = 0;
		for (pos = pos + 1; pos <= strlen(path); ++pos)
			curDirName[i++] = path[pos];
		//printf("%s\n", curDirName);

		if (check_name(inode_num, name) != -1) {//是否已有同名文件	
			file_write(name);//将数据从BUFF写入文件
		}
		else {
			/*读取目标目录的inode节点，判断该目录是否可写*/
			fseek(Disk, InodeBeg + sizeof(Inode)*inode_num, SEEK_SET);
			fread(&temp, sizeof(Inode), 1, Disk);

			if (temp.access[1] == 0) { //目标目录不可写
				printf("cp: cannot create regular file ‘./%s’: Permission denied\n", name);
				close_dir(inode_num);
				inode_num = originalInode;
				open_dir(inode_num);//返回操作前的目录位置
				return -1;
			}

			make_file(inode_num, name, File);
			file_write(name);//将数据从BUFF写入文件
		}
		/*返回原目录*/
	
		if (strcmp(cpname, "..") == 0)
			enter_child_dir(inode_num, curDirName);//如果复制到父目录，则复制完返回原目录
		else
			enter_child_dir(inode_num, "..");//如果复制到子目录，则复制完返回到子目录的".."目录，即原目录
	}

	close_dir(inode_num);
	inode_num = originalInode;
	open_dir(inode_num);//返回操作前的目录位置
	return 0;
}

int file_move(char* name, char* mvname)
{
	int originalInode = inode_num;//记录当前的inode
	int source_inode_num, dest_inode_num;
	int inode;//原文件的inode
	char originalNamePath[30];
	char originalMvNamePath[30];

	strcpy(originalNamePath, name);
	strcpy(originalMvNamePath, mvname);

	if (eat_path(name) == -1) {
		printf("mv: cannot stat ‘%s’: No such file or directory\n", originalNamePath);
		return -1;
	}
	if (type_check(name) != File) {
		printf("mv: cannot move '%s': Not a file or no exist\n", originalNamePath);
		close_dir(inode_num);
		inode_num = originalInode;
		open_dir(inode_num);//返回操作前的目录位置
		return -1;
	}

	source_inode_num = inode_num;//记录原文件的父目录节点

	close_dir(inode_num);
	inode_num = originalInode;
	open_dir(inode_num);//返回操作前的目录位置

	if (eat_path(mvname) == -1) {
		printf("mv: cannot stat ‘%s’: No such file or directory\n", originalMvNamePath);
		return -1;
	}
	dest_inode_num = inode_num;//记录目标目录的父目录节点

	if (source_inode_num == dest_inode_num && strcmp(name, mvname) == 0) {
		printf("mv: '%s' and '%s' are the same file\n", originalNamePath, originalMvNamePath);
		close_dir(inode_num);
		inode_num = originalInode;
		open_dir(inode_num);//返回操作前的目录位置
		return -1;
	}

	Inode temp;
	/*读取原文件目录的inode节点，判断该目录是否可写*/
	fseek(Disk, InodeBeg + sizeof(Inode)*source_inode_num, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	if (temp.access[1] == 0) { //原文件目录不可写
		if (type_check(mvname) == Directory)
			printf("mv: cannot move ‘%s’ to ‘%s/%s’: Permission denied\n", originalNamePath, originalMvNamePath, name);
		else 
			printf("mv: cannot move ‘%s’ to ‘%s’: Permission denied\n", originalNamePath, originalMvNamePath);
		close_dir(inode_num);
		inode_num = originalInode;
		open_dir(inode_num);//返回操作前的目录位置
		return -1;
	}

	/*形如：mv a.txt b.txt*/
	if (type_check(mvname) == -1) {  //如果b.txt不存在

		/*读取b.txt的父目录的inode，判断是否可写*/
		fseek(Disk, InodeBeg + sizeof(Inode)*dest_inode_num, SEEK_SET);
		fread(&temp, sizeof(temp), 1, Disk);

		if (temp.access[1] == 0) { //b.txt的父目录不可写
			printf("mv: cannot move ‘%s’ to ‘%s’: Permission denied\n", originalNamePath, originalMvNamePath);
			close_dir(inode_num);
			inode_num = originalInode;
			open_dir(inode_num);//返回操作前的目录位置
			return -1;
		}

		if (source_inode_num == dest_inode_num) {//如果b.txt不存在，且a.txt 与 b.txt 在同一父目录下，则相当于a.txt重命名
			for (int pos = 0; pos < dir_num; ++pos) {
				if (strcmp(dir_table[pos].name, name) == 0)
					strcpy(dir_table[pos].name, mvname);
			}
		}
		else {//如果b.txt不存在，且a.txt 与 b.txt 不在同一父目录下，则将a.txt的目录项删除，创建b.txt的目录项指向a.txt的inode
			inode_num = source_inode_num;
			open_dir(source_inode_num);//返回a.txt父目录位置
			inode = check_name(inode_num, name);
			adjust_dir(name);//删除a.txt的目录项

			close_dir(inode_num);
			inode_num = dest_inode_num;
			open_dir(dest_inode_num);//返回b.txt父目录位置
			strcpy(dir_table[dir_num].name, mvname);
			dir_table[dir_num++].inode_num = inode;
		}
	}
	else if (type_check(mvname) == File) {//如果b.txt为文件
		if (source_inode_num == dest_inode_num) {//若b.txt 与 a.txt在同一父目录下，则删除b.txt的目录项，同时将a.txt的目录项重命名
			adjust_dir(mvname);						//b.txt 与 a.txt在同一父目录下，则b.txt的父目录权限在前面已经判断，是可写的
			for (int pos = 0; pos < dir_num; ++pos) {
				if (strcmp(dir_table[pos].name, name) == 0)
					strcpy(dir_table[pos].name, mvname);
			}
		}
		else {//b.txt 已经存在 且 a.txt与b.txt不在同一目录，则将a.txt的目录项删除，修改b.txt的目录项指向a.txt的inode
			inode_num = source_inode_num;
			open_dir(source_inode_num);//返回a.txt父目录位置
			inode = check_name(inode_num, name);
			adjust_dir(name);//删除a.txt的目录项

			close_dir(inode_num);
			inode_num = dest_inode_num;
			open_dir(dest_inode_num);//返回b.txt父目录位置
			for (int pos = 0; pos < dir_num; ++pos) {
				if (strcmp(dir_table[pos].name, mvname) == 0)
					dir_table[pos].inode_num = inode;
			}
		}
	}
	else {  //如果b.txt为目录
		if (source_inode_num == dest_inode_num && strcmp(mvname, ".") == 0) {
			printf("mv: '%s' and '%s/%s' are the same file\n", originalNamePath, originalMvNamePath, name);
			return -1;//移动到本目录下，即不需要移动	
		}
		//如果b.txt是目录项，进入b.txt目录中，创建一个目录项指向a.txt, 再将a.txt的目录项删除，

		inode_num = source_inode_num;
		open_dir(source_inode_num);//返回a.txt父目录位置
		inode = check_name(inode_num, name);//记录a.txt的inode

		close_dir(inode_num);
		inode_num = dest_inode_num;
		open_dir(dest_inode_num);//返回b.txt父目录位置

		enter_child_dir(inode_num, mvname);
		if (check_name(inode_num, mvname) == -1) { //b.txt目录下不存在与a.txt重名的项目，则创建一个目录项指向a.txt

            /*读取b.txt的inode，判断是否可写*/
			fseek(Disk, InodeBeg + sizeof(Inode)*inode_num, SEEK_SET);
			fread(&temp, sizeof(Inode), 1, Disk);

			if (temp.access[1] == 0) { //b.txt不可写
				printf("mv: cannot move ‘%s’ to ‘%s/%s’: Permission denied\n", originalNamePath, originalMvNamePath, name);
				close_dir(inode_num);
				inode_num = originalInode;
				open_dir(inode_num);//返回操作前的目录位置
				return -1;
			}

			strcpy(dir_table[dir_num].name, name);
			dir_table[dir_num++].inode_num = inode;
		}
		else { //b.txt目录下存在与a.txt重名的项目，修改该项目的目录项指向a.txt的inode
			dir_table[check_name(inode, mvname)].inode_num = inode;
		}
		close_dir(inode_num);
		inode_num = source_inode_num;
		open_dir(source_inode_num);//返回a.txt父目录位置
		inode = check_name(inode_num, name);
		adjust_dir(name);//删除a.txt的目录项
	}

	close_dir(inode_num);
	inode_num = originalInode;
	open_dir(inode_num);//返回操作前的目录位置
	return 0;
}

int show_file_info(char* name) 
{
	int inode;
	Inode temp;
	char original_name_path[30];
	int original_inode = inode_num;//记录当前的inode

	strcpy(original_name_path, name);
	if (eat_path(name) == -1) {
		printf("stat: cannot stat‘%s’: No such file or directory\n", original_name_path);
		return -1;
	}

	inode = check_name(inode_num, name);
	if (inode == -1) {
		printf("stat: cannot stat '%s': No such file or directory\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}
	
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	printf("File: '%s'\n", original_name_path);

	printf("Size: %d\tBlocks: %d\t", temp.file_size, temp.blk_num);
	temp.type == Directory ? printf("type: directory\n") : printf("type: regular file\n");

	printf("Inode: %d\t", inode);
	printf("Access: ");
	temp.access[0] ? printf("r") : printf("-");
	temp.access[1] ? printf("w") : printf("-");
	temp.access[2] ? printf("x") : printf("-");
	printf("\n");
	printf("Access: %s", ctime(&temp.i_atime));
	printf("Modify: %s", ctime(&temp.i_mtime));
	printf("Change: %s", ctime(&temp.i_ctime));

	close_dir(inode_num);
	inode_num = original_inode;
	open_dir(inode_num);
	return 0;
}

int change_mode(char* parameter, char* name)
{
	int inode;
	Inode temp;
	char original_name_path[30];
	int original_inode = inode_num;//记录当前的inode

	strcpy(original_name_path, name);
	if (eat_path(name) == -1) {
		printf("chmod: cannot access‘%s’: No such file or directory\n", original_name_path);
		return -1;
	}
	inode = check_name(inode_num, name);
	if (inode == -1) {
		printf("stat: cannot stat '%s': No such file or directory\n", original_name_path);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	if (strcmp(parameter, "+r") == 0) {
		temp.access[0] = 1;
		temp.i_ctime = time(NULL);
	}
	else if (strcmp(parameter, "-r") == 0) {
		temp.access[0] = 0;
		temp.i_ctime = time(NULL);
	}
	else if (strcmp(parameter, "+w") == 0) {
		temp.access[1] = 1;
		temp.i_ctime = time(NULL);
	}
	else if (strcmp(parameter, "-w") == 0) {
		temp.access[1] = 0;
		temp.i_ctime = time(NULL);
	}
	else if (strcmp(parameter, "+x") == 0) {
		temp.access[2] = 1;
		temp.i_ctime = time(NULL);
	}
	else if (strcmp(parameter, "-x") == 0) {
		temp.access[2] = 0;
		temp.i_ctime = time(NULL);
	}
	else {
		printf("chmod: invalid option -- '%s'\n", parameter);
	}

	/*将修改后的Inode写回*/
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fwrite(&temp, sizeof(Inode), 1, Disk);

	close_dir(inode_num);
	inode_num = original_inode;
	open_dir(inode_num);
	return 0;
}

int file_edit(char* name)
{
	char original_name_path[30];
	int original_inode = inode_num;//记录当前的inode
	strcpy(original_name_path, name);
	if (eat_path(name) == -1) {
		printf("vi: cannot stat‘%s’: No such file or directory\n", original_name_path);
		return -1;
	}
	if (type_check(name) == Directory) {
		printf("vi: cannot edit '%s': Not a file\n", name);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}
	if (type_check(name) == -1) {
		printf("vi: cannot edit '%s': No such file\n", name);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	if (file_read(name) == -1) {//文件读取失败
		printf("vi:cannot open‘%s’for reading: Permission denied\n", name);
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	BuffModifyTimeBeforeEdit = getBuffModifyTime();//文件信息载入到buff.txt后，获取buff.txt的修改时间，用来判断载入的内容在记事本中是否被修改
	/*
	SYSTEMTIME *STime = new SYSTEMTIME;
	FileTimeToSystemTime(&BuffModifyTimeBeforeEdit, STime);
	printf("%d-%d-%d-%d-%d-%d\n", STime->wYear, STime->wMonth, STime->wDay, STime->wHour, STime->wMinute, STime->wSecond);
	*/
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	LPTSTR szCmdline = _tcsdup(TEXT("notepad.exe buff.txt"));

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Start the child process.
	if (!CreateProcess(NULL,   // No module name (use command line)
		szCmdline,      // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		printf("CreateProcess failed (%d).\n", GetLastError());
		return 0;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	Sleep(10);
	BuffModifyTimeAfterEdit = getBuffModifyTime();//buff.txt关闭后，获取buff.txt的修改时间，用来判断载入的内容在记事本中是否被修改
	/*
	SYSTEMTIME *STime = new SYSTEMTIME;
	FileTimeToSystemTime(&BuffModifyTimeAfterEdit, STime);
	printf("%d-%d-%d-%d-%d-%d\n", STime->wYear, STime->wMonth, STime->wDay, STime->wHour, STime->wMinute, STime->wSecond);
	*/
	if (BuffModifyTimeBeforeEdit.dwLowDateTime == BuffModifyTimeAfterEdit.dwLowDateTime
		&& BuffModifyTimeBeforeEdit.dwHighDateTime == BuffModifyTimeAfterEdit.dwHighDateTime) {
								//若buff.txt的修改时间没有发生变化。则载入记事本的内容没有发生改变，则不必重新写入		
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}
	
	if (file_write(name) == -1) { //将数据从BUFF写入文件
		printf("vi: 'readonly' option is set, can not write this file\n");
		close_dir(inode_num);
		inode_num = original_inode;
		open_dir(inode_num);
		return -1;
	}

	close_dir(inode_num);
	inode_num = original_inode;
	open_dir(inode_num);
	return 0;
}

FILETIME getBuffModifyTime()
{
	HFILE hFile = _lopen("buff.txt", OF_READ);

	FILETIME fCreateTime, fAccessTime, fWriteTime;
	GetFileTime((HANDLE*)hFile, &fCreateTime, &fAccessTime, &fWriteTime);//获取文件时间
	return fWriteTime;
}


int temp_file_read(char* name)//读取文件信息，生成临时文件，供进程执行使用
{
	int 	inode, i, blk_num;
	Inode	temp;
	char 	buff[BlkSize];
	
	//printf("read\n");

	inode = check_name(inode_num, name);

	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	int file_size = temp.file_size;

	if (temp.access[0] == 0) { //文件不可读，则直接退出，返回-1
		return -1;
	}

	if (temp.access[2] == 0) { //文件不可执行，则返回-2
		return -2;
	}

	if (temp.blk_num == 0) {//如果源文件没有内容,则直接退出
		return 0;
	}

	FILE*	fp = fopen(name, "wb+");
	//printf("read\n");

	for (i = 0; i<temp.blk_num - 1; ++i) {
		blk_num = temp.blk_identifier[i];
		/*读出文件包含的磁盘块*/
		fseek(Disk, BlockBeg + BlkSize*blk_num, SEEK_SET);
		fread(buff, sizeof(char), BlkSize, Disk);
		/*写入到临时文件*/
		fwrite(buff, sizeof(char), BlkSize, fp);
		file_size -= BlkSize;
	}

	/*最后一块磁盘块可能未满*/
	blk_num = temp.blk_identifier[i];
	fseek(Disk, BlockBeg + BlkSize*blk_num, SEEK_SET);
	fread(buff, sizeof(char), file_size, Disk);
	fwrite(buff, sizeof(char), file_size, fp);
	
	/*修改inode信息*/
	temp.i_atime = time(NULL);

	/*将修改后的Inode写回*/
	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fwrite(&temp, sizeof(Inode), 1, Disk);

	fclose(fp);
	return 0;
}

void show_manual()//打印帮助手册
{
	FILE *fp = NULL; 
	fp = fopen("man.txt", "rb");

	if (NULL == fp) return;

	char ch;
	while (fscanf(fp, "%c", &ch) != EOF) 
		printf("%c", ch); //从文本中读入并在控制台打印出来
	printf("\n");
	fclose(fp);
}

int get_file_size(char *name)
{
	Inode temp;
	int inode = check_name(inode_num, name);

	fseek(Disk, InodeBeg + sizeof(Inode)*inode, SEEK_SET);
	fread(&temp, sizeof(Inode), 1, Disk);

	return temp.file_size;
}