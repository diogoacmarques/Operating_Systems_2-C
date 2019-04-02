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
HANDLE hMutex, fraseEvento, hFile;
#define MName TEXT("testeMutex")
#define EName TEXT("testeEvent")
#define FILE_PATH TEXT("C:\Users\Diogo Marques\Desktop\isec\SO2\TP\SO2\testes")

int _tmain(int argc, LPTSTR argv[]) {
	BOOL bErrorFlag = FALSE;
	TCHAR resp;
	DWORD threadId;
	HANDLE hThreadProd;
	hMutex = CreateMutex(NULL, FALSE, MName);
	fraseEvento = CreateEvent(NULL, TRUE, FALSE, EName);

	hFile = CreateFile(TEXT("hey"),	// name of the write
		GENERIC_WRITE,				// open for writing
		TRUE,							// do not share
		NULL,						// default security
		CREATE_NEW,					// create new file only
		FILE_ATTRIBUTE_NORMAL,		// normal file
		NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("Erro ao criar ficheiro"));
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
		DWORD dwBytesToWrite = (DWORD)strlen(strLocal), dwBytesWritten;
		_tprintf(TEXT("[Produtor]Prima\'fim\' para terminar...\n"), GetCurrentThreadId());
		Sleep(100);
		do {
			_fgetts(strLocal, MAX, stdin);
			fflush(stdin);
			WaitForSingleObject(hMutex, INFINITE);
			//writes to file
			bErrorFlag = WriteFile(
				hFile,           // open file handle
				strLocal,      // start of data to write
				dwBytesToWrite,  // number of bytes to write
				&dwBytesWritten, // number of bytes that were written
				NULL);
			ReleaseMutex(hMutex);
			if (bErrorFlag == FALSE) {
				_tprintf(TEXT("Erro ao escrever"));
				return -1;
			}

			SetEvent(fraseEvento);
			ResetEvent(fraseEvento);
		} while (_tcsncmp(strLocal, TEXT("fim"), 3));
		
	}
	_tprintf(TEXT("[Thread Principal %d]Finalmente vou terminar..."), GetCurrentThreadId());
	return 0;
}