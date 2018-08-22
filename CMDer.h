#pragma once
//执行系统命令的封装类
#include <windows.h>
#include <string>
#include <cstring>
#include <iostream>
#include <regex>

#include <cstdio>

//#include <vector>

class CMDer {
	HANDLE hstdinRead;
	HANDLE hstdinWrite;
	//in管道两端句柄
	HANDLE hstdoutRead;
	HANDLE hstdoutWrite;

	HANDLE hstderrWrite;
	//out管道两端句柄
	bool writeToPipe(std::string cmd);
	std::string readFromPipe();
	//用于init和docommand的内部调用
	bool mark = false;
	//指示执行命令是采用阻塞方式还是非阻塞方式
	std::string result;
	//保存非阻塞模式下命令执行的结果
	HANDLE cProcess;
	//子进程号
	HANDLE inFileWrite;
	HANDLE outFileRead;
	HANDLE inFileRead;
	HANDLE outFileWrite;
	//用于非阻塞模式下的管道通信
	OVERLAPPED ov = { 0 };
	//用于非阻塞读写的参数

public:
	CMDer();
	CMDer(bool mark);
	~CMDer();
	bool init();
	//创建管道与执行cmd命令的子进程
	//注：init里先把管道最开始的输出读取一下并丢弃，增加程序的美观性
	std::string docommand(std::string cmd);
	//执行一条指令并返回输出结果
	std::string getResult();
	bool check();
	void interupt();
	bool docommandNBK(std::string cmd);
};