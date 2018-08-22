#include "CMDer.h"

using std::string;
using std::cout;
using std::endl;
using std::to_string;
//using std::vector;

CMDer::CMDer() {}
CMDer::CMDer(bool mark) {
	this->mark = mark;
}
CMDer::~CMDer()
{
	if (mark == false) {
		CloseHandle(hstdinRead);
		CloseHandle(hstdinWrite);
		CloseHandle(hstdoutRead);
		CloseHandle(hstdoutWrite);
	}
	else {
		CloseHandle(inFileRead);
		CloseHandle(inFileWrite);
		CloseHandle(outFileRead);
		CloseHandle(outFileWrite);
	}
	CloseHandle(cProcess);
	
}

//非阻塞的读会直接返回，将读到的内容加入一个预设字符串的末尾，如果读到0数据而且末尾字符匹配，则认为程序执行完成
//每次写新指令时清空预设字符串
//用户应保证在已经确定命令运行完成后再获取结果

string CMDer::readFromPipe() {
	//阻塞调用下不可以输入永远不会返回的指令
	DWORD dwRead;
	char buf[4096] = { 0 };
	string result = "";
	string dir;
	//vector<string> vec;
	while (1) {
		if (!ReadFile(hstdoutRead, buf, 4095, &dwRead, NULL))
			break;
		string temp = string(buf);
		result += temp;	
		if (dwRead < 4095 && (result[result.size() - 1] == '>' || result.substr(result.size() - 2) == "? "))
			break;
		//这里认为一次读取字符数小于4095且最后一个读取字符为>则管道中的数据就已经全部读取
		//出错的可能性可以认为非常之低
		//这种方式更适用于windows下，Linux下更适合采取结束符的方式
		memset(buf, 0, 4096);
	}
	//去掉字符串的最后一行，目的是去掉诸如E:>这样的输出
	if (result == "" || result.substr(result.size() - 2) == "? ")
		//第二个条件判断的是一些需要确认的情况
		return result + "\n";
	else {
		size_t index = result.find_last_of('\n');
		dir = result.substr(index + 1, result.size() - index - 1);
		//更新当前目录
		dir = "当前目录:" + dir + "\n";
		result = result.substr(0, index + 1);
		result = result + dir;
		return result;
	}
}

bool CMDer::writeToPipe(string cmd) {
	DWORD dwWritten;
	size_t count = 0;
	size_t l = cmd.size();
	
	while (1) {
		if (!WriteFile(hstdinWrite, cmd.data(), cmd.size(), &dwWritten, NULL))
			return false;
		count += dwWritten;
		cmd = cmd.substr(dwWritten);
		if (count == l)
			break;
	}
	
	//Sleep(100);
	//等待命令执行完成
	//注意如果输入命令为exit，则cmd进程结束
	return true;

}

bool CMDer::init() {
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = true;
	saAttr.lpSecurityDescriptor = NULL;
	//创建in管道
	if (mark == false) {
		if (!CreatePipe(&hstdinRead, &hstdinWrite, &saAttr, 0)) {
			cout << "无法创建输入管道" << endl;
			return false;
		}
		//创建out管道
		if (!CreatePipe(&hstdoutRead, &hstdoutWrite, &saAttr, 0)) {
			cout << "无法创建输出管道" << endl;
			return false;
		}
		//错误输出句柄和out共用管道
		if (!DuplicateHandle(GetCurrentProcess(), hstdoutWrite, GetCurrentProcess(), &hstderrWrite, 0, true, DUPLICATE_SAME_ACCESS)) {
			cout << "无法将错误输出句柄绑定到管道" << endl;
			return false;
		}
	}
	else {
		inFileRead = CreateNamedPipeA("\\\\.\\pipe\\cmder_in", PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 4096, 4096, 0, &saAttr);
		SetHandleInformation(inFileRead, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
		if (inFileRead == INVALID_HANDLE_VALUE) {
			cout << "创建输入命名管道失败" << GetLastError() << endl;
			return false;
		}
		//子进程从管道中读
		outFileWrite = CreateNamedPipeA("\\\\.\\pipe\\cmder_out", PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 4096, 4096, 0, &saAttr);
		SetHandleInformation(outFileWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);	
		if (outFileWrite == INVALID_HANDLE_VALUE) {
			cout << "创建输出命名管道失败" << GetLastError() << endl;
			return false;
		}
		//子进程向管道中写
		//以上两个句柄用于子进程，相当于将子进程作为管道的服务器端，父进程作为管道的客户端

		inFileWrite = CreateFileA("\\\\.\\pipe\\cmder_in", GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
		if (inFileWrite == INVALID_HANDLE_VALUE) {
			cout << "服务器无法获取管道输入句柄" << GetLastError() << endl;
			return false;
		}
		//父进程向管道中写

		outFileRead = CreateFileA("\\\\.\\pipe\\cmder_out", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
		if (outFileRead == INVALID_HANDLE_VALUE) {
			cout << "服务器无法获取管道输出句柄" << GetLastError() << endl;
			return false;
		}
		//父进程从管道中读
		//以上两个句柄用于父进程
		
	}
	STARTUPINFO startupInfo;//传递给新进程的信息
	PROCESS_INFORMATION procInfo;//创建新进程时返回的信息
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	ZeroMemory(&procInfo, sizeof(procInfo));
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
	startupInfo.dwFlags |= STARTF_USESTDHANDLES;
	if (mark == false) {
		startupInfo.hStdInput = hstdinRead;//重定向子进程的标准输入
		startupInfo.hStdOutput = hstdoutWrite;//重定向子进程的标准输出
		startupInfo.hStdError = hstderrWrite;
	}
	else {
		startupInfo.hStdInput = inFileRead;//重定向子进程的标准输入
		startupInfo.hStdOutput = outFileWrite;//重定向子进程的标准输出
		startupInfo.hStdError = outFileWrite;
	}
	
	//char cmdLine[] = { "cmd" };//子进程的执行命令
	
	char cmdLine[] = { "cmd" };
	if (!CreateProcess(NULL,
		cmdLine,//子进程中执行的程序
		NULL,//正常情况下可以不考虑后续参数，直接如下设置即可
		NULL,
		TRUE,//继承父进程的属性
		0,//CREATE_NEW_PROCESS_GROUP,//这个进程作为一个新的进程组的祖进程，进程组编号等同于进程编号
		NULL,
		NULL,
		&startupInfo,
		&procInfo
	)) {
		cout << "无法开启子进程" << endl;
		return false;
	}

	cProcess = procInfo.hProcess;
	Sleep(200);
	//等待子进程的必要的输出
	if (mark == false)
		readFromPipe();
	else 
		check();
	//	check();
	//readFromPipe();
	return true;
}

string CMDer::docommand(string cmd) {
	cmd += "\n";
	//end用于从输出来判断命令已经执行完毕
	//写入命令
	
	if (!writeToPipe(cmd))
		return "无法执行命令!\n";
	else
		if (cmd == "exit\n")
			return "";
		else
			return readFromPipe();
}

bool CMDer::docommandNBK(string cmd) {
	result = "";
	cmd += "\n";
	//写入命令
	DWORD dwWritten;
	size_t count = 0;
	size_t l = cmd.size();
	while (1) {
		if (!WriteFile(inFileWrite, cmd.data(), cmd.size(), &dwWritten, &ov)) {
			if (GetLastError() != ERROR_IO_PENDING)
				return false;
		}
		Sleep(100);
		if (ov.Internal != STATUS_PENDING)
			break;
		count += dwWritten;
		cmd = cmd.substr(dwWritten);
		if (count == l)
			break;
	}
	return true;
}

bool CMDer::check() {
	
	char buffer[4096] = { 0 };
	DWORD dwRead;
	if (!ReadFile(outFileRead, buffer, 4095, &dwRead, &ov)) 
		return false;
	//cout << "临时读取" << string(buffer) << endl;
	result += string(buffer);
	memset(buffer, 0, 4096);
	if (result == "")
		return false;
	if ((dwRead < 4095) && (result[result.size() - 1] == '>' || result.substr(result.size() - 2) == "? ")) {
		if (result[result.size() - 1] == '>') {
			string dir;
			size_t index = result.find_last_of('\n');
			dir = result.substr(index + 1, result.size() - index - 1);
			//更新当前目录
			dir = "当前目录:" + dir + "\n";
			result = result.substr(0, index + 1);
			result = result + dir;
		}
		else {
			result += "\n";
		}
		return true;
	}
	else
		return false;
}

string CMDer::getResult() {
	return result;
}

void CMDer::interupt() {
	//CloseHandle(cProcess);
	//向该CMD窗口的子进程发送ctrl+c指令
	DWORD cId = GetProcessId(cProcess);
	//以下代码参考自https://blog.csdn.net/binzhongbi757/article/details/50698486
	AttachConsole(cId); // attach to process console
	SetConsoleCtrlHandler(NULL, TRUE); // disable Control+C handling for our app
	if (!GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0)) // generate Control+C event
		cout << "无法终止当前命令" << GetLastError() << endl;
	//不知道为什么直接向进程组发送信号是错误的
	Sleep(1000);
	SetConsoleCtrlHandler(NULL, FALSE);
	return;
}