#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "DLL.h"

void hidecursor();
void gotoxy(int x, int y);
DWORD WINAPI readGameThread(LPVOID param);

HANDLE hTreadGame, canRead, canWrite;

int _tmain(int argc, LPTSTR argv[]) {
	//Um cliente muito simples em consola que invoca cada funcionalidade da DLL através de uma sequência pré - definida
	//exemplo: cliente consola pede o username ao utilizador;
	//envia ao servidor;
	//recebe confirmação / rejeição; 
	//entra em ciclo a receber novas posições da bola até uma tecla ser premida pelo utilizador).
	TCHAR str[MAX_NAME_LENGTH];
	TCHAR KeyPress;
	BOOLEAN res;
	msg newMsg;
	DWORD threadId;

	canRead = CreateSemaphore(NULL, 0,1, SEMAPHORE_MEMORY_READ);
	canWrite = CreateSemaphore(NULL, 1, 1, SEMAPHORE_MEMORY_WRITE);


	createSharedMemory();

	do {
		_tprintf(TEXT("--Welcome to Client[%d]--\n"),GetCurrentThreadId());
		_tprintf(TEXT("1:Login\n"));
		_tprintf(TEXT("2:Send Message\n"));
		_tprintf(TEXT("3:See game\n"));
		_tprintf(TEXT("4:Check var gData\n"));
		_tprintf(TEXT("5:Exit\n"));

		fflush(stdin);
		KeyPress = _gettch();
		_puttchar(KeyPress);
		system("cls");

		switch (KeyPress) {
		case '1':
			_tprintf(TEXT("Nome do User:"), GetCurrentThreadId());
			_tscanf_s(TEXT("%s"), str, MAX_NAME_LENGTH);
			res = Login(str);
			if (res) {_tprintf(TEXT("Sucesso no login\n"));}
			else { _tprintf(TEXT("Erro no login\n")); }
			break;
		case '2':
			_tprintf(TEXT("Mensagem:"));
			_tscanf_s(TEXT("%s"), newMsg.messageInfo, TAM);
			newMsg.codigoMsg = 0;
			sendMessage(newMsg);
			break;
		case '3':
			hTreadGame = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)readGameThread, NULL, 0, &threadId);
			if (hTreadGame == NULL) {
				_tprintf(TEXT("Erro ao criar thread readGameThread!"));
				return -1;
			}
			break;
		case '4':
			checkVar();
			break;
		}
	} while (KeyPress != '5');



	closeSharedMemory();
	return 0;

	//createSharedMemory();
	//msg content;
	//HANDLE canRead,canWrite;
	//canRead = CreateSemaphore(NULL, 0,10, SEMAPHORE_MEMORY_READ);
	//canWrite = CreateSemaphore(NULL, 10, 10, SEMAPHORE_MEMORY_WRITE);

	//TCHAR str[TAM];
	//_tprintf(TEXT("Cliente(%d)-Utilizador:"), GetCurrentThreadId());
	//_tscanf_s(TEXT("%s"), str, TAM);
	//boolean res = Login(str);
	//if (res) {
	//	_tprintf(TEXT("Sucesso no login!\n"));
	//}
	//int oposx = 0, oposy = 0, posx = 0 , posy = 0;
	//system("cls");
	//do{
	//	oposx = posx;
	//	oposy = posy;		
	//	gotoxy(oposx, oposy);
	//	WaitForSingleObject(canRead, INFINITE);
	//	_tprintf(TEXT(" "));
	//	content = readFromSharedMemory();
	//	posx = content.posx;
	//	posy = content.posy;
	//	gotoxy(posx,posy);
	//	_tprintf(TEXT("B"));
	//	//_tprintf(TEXT("pos(%d,%d)\n"), content.posx, content.posy);
	//	ReleaseSemaphore(canWrite, 1, NULL);
	//	gotoxy(0, 0);
	//	_tprintf(TEXT("Codigo:%d"),content.codigoMsg);
	//} while (content.codigoMsg);
	//
	//_tprintf(TEXT("[Thread Principal %d] Vou terminar..."),GetCurrentThreadId());
	//return 0;
}

DWORD WINAPI readGameThread(LPVOID param) {
	int oposx = 0, oposy = 0, posx = 0 , posy = 0;
	gameData game;
	do{
		oposx = posx;
		oposy = posy;		
		gotoxy(oposx, oposy);
		WaitForSingleObject(canRead, INFINITE);
		receiveGame();
		_tprintf(TEXT(" "));
		game = *gData;
		posx = game.posx;
		posy = game.posy;
		gotoxy(posx,posy);
		_tprintf(TEXT("B"));
		//_tprintf(TEXT("pos(%d,%d)\n"), game.posx, game.posy);
		ReleaseSemaphore(canWrite, 1, NULL);
		gotoxy(0, 0);
		_tprintf(TEXT("Codigo:%d"),game.status);
	} while (game.status);
	
	return 0;
}


//DWORD WINAPI Barreira(LPVOID param) {
//	pdata gameInfo = ((pdata)param);
//	gameInfo->baTam = 15;
//	gameInfo->baPosx = gameInfo->limx / 2, gameInfo->baPosy = gameInfo->limy - 2;
//
//	hidecursor();
//	WaitForSingleObject(stdoutMutex, INFINITE);
//	gotoxy(gameInfo->baPosx, gameInfo->baPosy);
//	for (int i = 0; i < gameInfo->baTam; i++) {
//		_tprintf(TEXT("O"));
//	}
//	ReleaseMutex(stdoutMutex);
//
//	TCHAR KeyPress;
//	HANDLE hTBola;
//	DWORD threadId;
//	while (1) {
//		KeyPress = _getch();
//
//		//_putch(KeyPress);
//		switch (KeyPress) {
//		case 'a':
//		case 'A':
//		case 'K'://left arrow
//			if (gameInfo->baPosx <= 0)
//				break;
//			WaitForSingleObject(stdoutMutex, INFINITE);
//			gotoxy(gameInfo->baPosx + gameInfo->baTam - 1, gameInfo->baPosy);
//			_tprintf(TEXT(" "));
//			gameInfo->baPosx--;
//			gotoxy(gameInfo->baPosx, gameInfo->baPosy);
//			_tprintf(TEXT("O"));
//			ReleaseMutex(stdoutMutex);
//			break;
//
//		case 'd':
//		case 'D':
//		case 'M':
//			//right arrow
//			if (gameInfo->baPosx + gameInfo->baTam >= gameInfo->limx)
//				break;
//			WaitForSingleObject(stdoutMutex, INFINITE);
//			gotoxy(gameInfo->baPosx, gameInfo->baPosy);
//			_tprintf(TEXT(" "));
//			gameInfo->baPosx++;
//			gotoxy(gameInfo->baPosx + gameInfo->baTam - 1, gameInfo->baPosy);
//			_tprintf(TEXT("O"));
//			ReleaseMutex(stdoutMutex);
//			break;
//		case 32:
//			WaitForSingleObject(stdoutMutex, INFINITE);
//			gotoxy(0, gameInfo->limy / 2);
//			for (int i = 0; i < gameInfo->limx; i++) {
//				_tprintf(TEXT(" "));
//			}
//			ReleaseMutex(stdoutMutex);
//			hTBola = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Bola, (LPVOID)gameInfo, 0, &threadId);
//			if (hTBola == NULL) {
//				_tprintf(TEXT("Erro ao criar thread Bola!"));
//				return -1;
//			}
//			break;
//
//		case 27://ESC
//			gotoxy(0, 1);
//			_tprintf(TEXT("EXITING"));
//			ExitThread(NULL);
//			break;
//		}
//	}
//}


void hidecursor()
{
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO info;
	info.dwSize = 100;
	info.bVisible = FALSE;
	SetConsoleCursorInfo(consoleHandle, &info);
}

void gotoxy(int x, int y) {
	static HANDLE hStdout = NULL;
	COORD coord;
	coord.X = x;
	coord.Y = y;
	if (!hStdout)
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(hStdout, coord);
}