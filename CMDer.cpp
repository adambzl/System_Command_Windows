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

//�������Ķ���ֱ�ӷ��أ������������ݼ���һ��Ԥ���ַ�����ĩβ���������0���ݶ���ĩβ�ַ�ƥ�䣬����Ϊ����ִ�����
//ÿ��д��ָ��ʱ���Ԥ���ַ���
//�û�Ӧ��֤���Ѿ�ȷ������������ɺ��ٻ�ȡ���

string CMDer::readFromPipe() {
	//���������²�����������Զ���᷵�ص�ָ��
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
		//������Ϊһ�ζ�ȡ�ַ���С��4095�����һ����ȡ�ַ�Ϊ>��ܵ��е����ݾ��Ѿ�ȫ����ȡ
		//����Ŀ����Կ�����Ϊ�ǳ�֮��
		//���ַ�ʽ��������windows�£�Linux�¸��ʺϲ�ȡ�������ķ�ʽ
		memset(buf, 0, 4096);
	}
	//ȥ���ַ��������һ�У�Ŀ����ȥ������E:>���������
	if (result == "" || result.substr(result.size() - 2) == "? ")
		//�ڶ��������жϵ���һЩ��Ҫȷ�ϵ����
		return result + "\n";
	else {
		size_t index = result.find_last_of('\n');
		dir = result.substr(index + 1, result.size() - index - 1);
		//���µ�ǰĿ¼
		dir = "��ǰĿ¼:" + dir + "\n";
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
	//�ȴ�����ִ�����
	//ע�������������Ϊexit����cmd���̽���
	return true;

}

bool CMDer::init() {
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = true;
	saAttr.lpSecurityDescriptor = NULL;
	//����in�ܵ�
	if (mark == false) {
		if (!CreatePipe(&hstdinRead, &hstdinWrite, &saAttr, 0)) {
			cout << "�޷���������ܵ�" << endl;
			return false;
		}
		//����out�ܵ�
		if (!CreatePipe(&hstdoutRead, &hstdoutWrite, &saAttr, 0)) {
			cout << "�޷���������ܵ�" << endl;
			return false;
		}
		//������������out���ùܵ�
		if (!DuplicateHandle(GetCurrentProcess(), hstdoutWrite, GetCurrentProcess(), &hstderrWrite, 0, true, DUPLICATE_SAME_ACCESS)) {
			cout << "�޷��������������󶨵��ܵ�" << endl;
			return false;
		}
	}
	else {
		inFileRead = CreateNamedPipeA("\\\\.\\pipe\\cmder_in", PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 4096, 4096, 0, &saAttr);
		SetHandleInformation(inFileRead, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
		if (inFileRead == INVALID_HANDLE_VALUE) {
			cout << "�������������ܵ�ʧ��" << GetLastError() << endl;
			return false;
		}
		//�ӽ��̴ӹܵ��ж�
		outFileWrite = CreateNamedPipeA("\\\\.\\pipe\\cmder_out", PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED, PIPE_TYPE_BYTE, 1, 4096, 4096, 0, &saAttr);
		SetHandleInformation(outFileWrite, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);	
		if (outFileWrite == INVALID_HANDLE_VALUE) {
			cout << "������������ܵ�ʧ��" << GetLastError() << endl;
			return false;
		}
		//�ӽ�����ܵ���д
		//����������������ӽ��̣��൱�ڽ��ӽ�����Ϊ�ܵ��ķ������ˣ���������Ϊ�ܵ��Ŀͻ���

		inFileWrite = CreateFileA("\\\\.\\pipe\\cmder_in", GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
		if (inFileWrite == INVALID_HANDLE_VALUE) {
			cout << "�������޷���ȡ�ܵ�������" << GetLastError() << endl;
			return false;
		}
		//��������ܵ���д

		outFileRead = CreateFileA("\\\\.\\pipe\\cmder_out", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
		if (outFileRead == INVALID_HANDLE_VALUE) {
			cout << "�������޷���ȡ�ܵ�������" << GetLastError() << endl;
			return false;
		}
		//�����̴ӹܵ��ж�
		//��������������ڸ�����
		
	}
	STARTUPINFO startupInfo;//���ݸ��½��̵���Ϣ
	PROCESS_INFORMATION procInfo;//�����½���ʱ���ص���Ϣ
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	ZeroMemory(&procInfo, sizeof(procInfo));
	startupInfo.cb = sizeof(startupInfo);
	startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
	startupInfo.dwFlags |= STARTF_USESTDHANDLES;
	if (mark == false) {
		startupInfo.hStdInput = hstdinRead;//�ض����ӽ��̵ı�׼����
		startupInfo.hStdOutput = hstdoutWrite;//�ض����ӽ��̵ı�׼���
		startupInfo.hStdError = hstderrWrite;
	}
	else {
		startupInfo.hStdInput = inFileRead;//�ض����ӽ��̵ı�׼����
		startupInfo.hStdOutput = outFileWrite;//�ض����ӽ��̵ı�׼���
		startupInfo.hStdError = outFileWrite;
	}
	
	//char cmdLine[] = { "cmd" };//�ӽ��̵�ִ������
	
	char cmdLine[] = { "cmd" };
	if (!CreateProcess(NULL,
		cmdLine,//�ӽ�����ִ�еĳ���
		NULL,//��������¿��Բ����Ǻ���������ֱ���������ü���
		NULL,
		TRUE,//�̳и����̵�����
		0,//CREATE_NEW_PROCESS_GROUP,//���������Ϊһ���µĽ����������̣��������ŵ�ͬ�ڽ��̱��
		NULL,
		NULL,
		&startupInfo,
		&procInfo
	)) {
		cout << "�޷������ӽ���" << endl;
		return false;
	}

	cProcess = procInfo.hProcess;
	Sleep(200);
	//�ȴ��ӽ��̵ı�Ҫ�����
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
	//end���ڴ�������ж������Ѿ�ִ�����
	//д������
	
	if (!writeToPipe(cmd))
		return "�޷�ִ������!\n";
	else
		if (cmd == "exit\n")
			return "";
		else
			return readFromPipe();
}

bool CMDer::docommandNBK(string cmd) {
	result = "";
	cmd += "\n";
	//д������
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
	//cout << "��ʱ��ȡ" << string(buffer) << endl;
	result += string(buffer);
	memset(buffer, 0, 4096);
	if (result == "")
		return false;
	if ((dwRead < 4095) && (result[result.size() - 1] == '>' || result.substr(result.size() - 2) == "? ")) {
		if (result[result.size() - 1] == '>') {
			string dir;
			size_t index = result.find_last_of('\n');
			dir = result.substr(index + 1, result.size() - index - 1);
			//���µ�ǰĿ¼
			dir = "��ǰĿ¼:" + dir + "\n";
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
	//���CMD���ڵ��ӽ��̷���ctrl+cָ��
	DWORD cId = GetProcessId(cProcess);
	//���´���ο���https://blog.csdn.net/binzhongbi757/article/details/50698486
	AttachConsole(cId); // attach to process console
	SetConsoleCtrlHandler(NULL, TRUE); // disable Control+C handling for our app
	if (!GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0)) // generate Control+C event
		cout << "�޷���ֹ��ǰ����" << GetLastError() << endl;
	//��֪��Ϊʲôֱ��������鷢���ź��Ǵ����
	Sleep(1000);
	SetConsoleCtrlHandler(NULL, FALSE);
	return;
}