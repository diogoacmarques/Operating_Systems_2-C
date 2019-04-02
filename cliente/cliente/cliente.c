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
HANDLE hMutex,fraseEvento;
TCHAR frase[MAX];
#define MName TEXT("testeMutex")
#define EName TEXT("testeEvent")
								
int _tmain(int argc, LPTSTR argv[]) {
	TCHAR resp;
	DWORD threadId;
	hMutex = CreateMutex(NULL, FALSE, MName);
	fraseEvento = CreateEvent(NULL,TRUE, FALSE, EName);
	//UNICODE: Por defeito, a consola Windows não processe caracteres wide.
	//A maneira mais fácil para ter esta funcionalidade é chamar _setmode:
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif
	_tprintf(TEXT("Lançar consumidor?"));
	_tscanf_s(TEXT("%c"), &resp, 1);
	if (resp == 'S' || resp == 's') {
		TCHAR strLocal[MAX];
		_tprintf(TEXT("[Consumidor(%d)]Listening...\n"), GetCurrentThreadId());
		Sleep(100);
		do {
			WaitForSingleObject(fraseEvento, INFINITE);
			WaitForSingleObject(hMutex, INFINITE);
			//read from file
			_tprintf(TEXT("[Consumidor(%d)]:I got an event\n"), GetCurrentThreadId());
			ReleaseMutex(hMutex);
			//_tprintf(TEXT("[Consumidor(%d)]:%s"), GetCurrentThreadId(), strLocal);
		} while (_tcsncmp(strLocal, TEXT("fim"), 3));
		SetEvent(fraseEvento);
	
	}
	_tprintf(TEXT("[Thread Principal %d]Finalmente vou terminar..."),GetCurrentThreadId());
	return 0;
}
