
#include <windows.h>
#include <stdio.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <stdlib.h>
#include <string>
#include <iostream>

/*
ͨ�����������PID
*/
std::wstring s2ws(const std::string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}


	/*
	ͨ��PID��ý��̵ľ��
	*/
	HANDLE GetProcessHandle(int nID)
	{
		return OpenProcess(PROCESS_ALL_ACCESS, FALSE, nID);
	}

	//ͨ��������������׺.exe����ȡ���̾��
	HANDLE GetProcessHandle(LPCWSTR lpName)
	{
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (INVALID_HANDLE_VALUE == hSnapshot)
		{
			return NULL;
		}
		PROCESSENTRY32 pe = { sizeof(pe) };
		BOOL fOk;
		for (fOk = Process32First(hSnapshot, &pe); fOk; fOk = Process32Next(hSnapshot, &pe))
		{
			if (!_tcsicmp(pe.szExeFile, lpName)) // �����ִ�Сд ��������unicode����
			{
				CloseHandle(hSnapshot);
				return GetProcessHandle(pe.th32ProcessID);
			}
		}
		return NULL;
	}

/*
������
*/
int main()
{
	//�Ա���Server�˽���hook����
	std::string myString = "ControlServer.exe";
	std::cout<<"���ٽ�����:" << myString<< "���ٽ���pid:";
	std::wstring stemp = s2ws(myString);
	LPCWSTR ProcName = stemp.c_str();

	DWORD Process_ID = GetProcessId(GetProcessHandle(ProcName));
	std::cout << Process_ID << std::endl;

	
	//�򿪽���
	HANDLE hPro = OpenProcess(PROCESS_CREATE_THREAD | //����Զ�̴����߳�
		PROCESS_VM_OPERATION | //����Զ��VM����
		PROCESS_VM_WRITE,//����Զ��VMд
		FALSE, Process_ID);
	if (hPro == NULL) {
		MessageBoxA(NULL, "���ض˽��̲����ڣ�", "����", 0);
	}

	// ��Զ�̽�����Ϊ·�����Ʒ���ռ�
	char str[256] = { 0 };
	strcpy_s(str, "C:\\Users\\NOIKH\\OneDrive\\����\\Server_Hook\\DLL.dll");	// ע�����õ�dll�ļ�·��
	int dwSize = (lstrlenA(str) + 1);	//·����С+1����Ϊ\0

	//���ٿռ�
	LPVOID pszLibFileRemote = (PWSTR)VirtualAllocEx(hPro, NULL, dwSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (pszLibFileRemote == NULL) {
		MessageBoxA(NULL, "׼��hook.dllʧ�ܣ�", "����", 0);
		return -1;
	}

	//д������
	SIZE_T  dwWritten;	//д���ֽ���
	DWORD n = WriteProcessMemory(hPro, (LPTHREAD_START_ROUTINE)pszLibFileRemote, str, dwSize, &dwWritten);//д�뵽�ڴ�
	if (n == NULL) {
		MessageBoxA(NULL, "д��hook.dllʧ�ܣ�", "����", 0);
		return -1;
	}

	//Kernel32ģ���Ѿ������������������Զ����ؽ����ˣ��������ǿ���ֱ�ӻ�ȡ
	PTHREAD_START_ROUTINE pfnThreadRtn = 
		(PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibraryA");
	if (pfnThreadRtn == NULL) {
		MessageBoxA(NULL, "��ȡdll�⺯��ʧ�ܣ�", "����", 0);
		return -1;
	}

	//����Զ�߳�
	DWORD dwID = NULL;
	HANDLE hThread = CreateRemoteThread(hPro, 0, 0, pfnThreadRtn, pszLibFileRemote, 0, &dwID);
	if (hThread == NULL) {
		MessageBoxA(NULL, "����Զ�߳�ʧ�ܣ�", "����", 0);
		printf("Զ���̴߳��� %d", GetLastError());

	}
	printf("remote success\n");

	//�ȴ��߳̽���
	WaitForSingleObject(hThread, INFINITE);
	return 0;

}