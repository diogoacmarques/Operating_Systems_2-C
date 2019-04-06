#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

//includes dos exercícios anteriores
#define MAX 256
HANDLE hMutex, hEvent;
#define MName TEXT("testeMutex")
#define EName TEXT("testeEvent")
#define FILE_PATH TEXT("../../sharedFile.txt")

int _tmain(int argc, LPTSTR argv[]) {
	BOOL bErrorFlag = FALSE;
	TCHAR resp;
	DWORD threadId,res,n;
	HANDLE hThreadProd, hFile;
	hMutex = CreateMutex(NULL, FALSE, MName);
	hEvent = CreateEvent(NULL, TRUE, FALSE, EName);

	hFile = CreateFile(FILE_PATH, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == NULL || hMutex == NULL || hEvent== NULL) {
		_tprintf(TEXT("Erro a criar recursos de comunicação e/ou sincronização."));
		return -1;
	}
	//UNICODE: Por defeito, a consola Windows não processe caracteres wide.
	//A maneira mais fácil para ter esta funcionalidade é chamar _setmode:
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif
	_tprintf(TEXT("Lançar produtor?"));
	_tscanf_s(TEXT("%c"), &resp, 1);
	if (resp == 'S' || resp == 's') {
		TCHAR strLocal[MAX];
		_tprintf(TEXT("[Produtor]Prima\'fim\' para terminar...\n"), GetCurrentThreadId());
		Sleep(100);
		do {
			_fgetts(strLocal, MAX, stdin);
			fflush(stdin);
			WaitForSingleObject(hMutex, INFINITE);
			//writes to file
			res = WriteFile(hFile, strLocal, (_tcslen(strLocal) + 1) * sizeof(TCHAR), &n, NULL);
			ReleaseMutex(hMutex);
			SetEvent(hEvent);
			ResetEvent(hEvent);
		} while (_tcsncmp(strLocal, TEXT("fim"), 3));
		
	}
	CloseHandle(hMutex);
	CloseHandle(hEvent);
	CloseHandle(hFile);
	_tprintf(TEXT("[Thread Principal %d]Finalmente vou terminar..."), GetCurrentThreadId());
	return 0;
}