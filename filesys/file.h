#pragma once
#ifndef FILE_H
#define FILE_H

int file_write(char*);//写文件
int file_read(char*);//读文件
int file_edit(char*);//编辑文件
int file_copy(char* name, char* cpname);//拷贝文件
int file_move(char* name, char* mvname);//移动文件
int show_file_info(char* name);//显示文件信息
int change_mode(char* parameter, char* name);//改变文件权限
int temp_file_read(char* name);//读取文件信息，生成临时文件，供进程执行使用
int exec(char* name);//执行 可执行的文件
void show_manual();//打印帮助手册
int get_file_size(char *name);//获取文件大小

#endif