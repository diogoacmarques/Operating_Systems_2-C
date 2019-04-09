#include <windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

#include "DLL.h"

int _tmain(int argc, TCHAR *argv[]) {
	DWORD num = 0;
	TCHAR str[TAM];
	_tprintf(TEXT("Utilizador:"));
	_tscanf_s(TEXT("%s"), str,TAM);
	//sendMessage();
	Login(str);
	return 0;
}