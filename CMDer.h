#pragma once
//ִ��ϵͳ����ķ�װ��
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
	//in�ܵ����˾��
	HANDLE hstdoutRead;
	HANDLE hstdoutWrite;

	HANDLE hstderrWrite;
	//out�ܵ����˾��
	bool writeToPipe(std::string cmd);
	std::string readFromPipe();
	//����init��docommand���ڲ�����
	bool mark = false;
	//ָʾִ�������ǲ���������ʽ���Ƿ�������ʽ
	std::string result;
	//���������ģʽ������ִ�еĽ��
	HANDLE cProcess;
	//�ӽ��̺�
	HANDLE inFileWrite;
	HANDLE outFileRead;
	HANDLE inFileRead;
	HANDLE outFileWrite;
	//���ڷ�����ģʽ�µĹܵ�ͨ��
	OVERLAPPED ov = { 0 };
	//���ڷ�������д�Ĳ���

public:
	CMDer();
	CMDer(bool mark);
	~CMDer();
	bool init();
	//�����ܵ���ִ��cmd������ӽ���
	//ע��init���Ȱѹܵ��ʼ�������ȡһ�²����������ӳ����������
	std::string docommand(std::string cmd);
	//ִ��һ��ָ�����������
	std::string getResult();
	bool check();
	void interupt();
	bool docommandNBK(std::string cmd);
};