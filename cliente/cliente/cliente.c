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
DWORD WINAPI BolaThread(LPVOID param);
DWORD WINAPI UserThread(LPVOID param);

HANDLE hTreadGame,hTBola[MAX_BALLS],hTUsers[MAX_USERS], canRead, canWrite,hStdoutMutex;

pgame gameInfo;

int _tmain(int argc, LPTSTR argv[]) {
	//Um cliente muito simples em consola que invoca cada funcionalidade da DLL através de uma sequência pré - definida
	//exemplo: cliente consola pede o username ao utilizador;
	//envia ao servidor;
	//recebe confirmação / rejeição; 
	//entra em ciclo a receber novas posições da bola até uma tecla ser premida pelo utilizador).
	TCHAR str[MAX_NAME_LENGTH];
	TCHAR userLogged[MAX_NAME_LENGTH];
	TCHAR KeyPress;
	BOOLEAN res;
	msg newMsg;
	DWORD threadId;

	canRead = CreateSemaphore(NULL, 0, 1, SEMAPHORE_MEMORY_READ);
	canWrite = CreateSemaphore(NULL, 1, 1, SEMAPHORE_MEMORY_WRITE);
	gameEvent = CreateEvent(NULL, TRUE, FALSE, GAME_EVENT_NNAME);
	hStdoutMutex = CreateMutex(NULL, TRUE, NULL);

	gameInfo = (game *)malloc(sizeof(game));
	if (!gameInfo) { _tprintf(TEXT("Erro a reservar memoria para gameInfo\n!")); }

	createSharedMemoryMsg();
	gameInfo = createSharedMemoryGame();
	/*_tprintf(TEXT("Nome do User:"));
	_tscanf_s(TEXT("%s"), userLogged, MAX_NAME_LENGTH);
	res = Login(userLogged);
	if (res) { _tprintf(TEXT("Sucesso no login\n")); }
	else { _tprintf(TEXT("Erro no login\n")); return 0; }
	Sleep(2000);*/
	system("cls");

	do {
		_tprintf(TEXT("--Welcome to Client[%d]--\n"),GetCurrentThreadId());
		_tprintf(TEXT("1:Login?\n"));
		_tprintf(TEXT("2:Send Message\n"));
		_tprintf(TEXT("3:Play game\n"));
		_tprintf(TEXT("4:Check var gData\n"));
		_tprintf(TEXT("5:Exit\n"));

		fflush(stdin);
		KeyPress = _gettch();
		_puttchar(KeyPress);
		system("cls");

		switch (KeyPress) {
		case '1':
			_tprintf(TEXT("Nome do User:"));
			_tscanf_s(TEXT("%s"), str, MAX_NAME_LENGTH);
			res = Login(str);
			if (res) {_tprintf(TEXT("Sucesso no login\n"));}
			else { _tprintf(TEXT("Erro no login\n")); }
			hTreadGame = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)readGameThread, NULL, 0, &threadId);
			if (hTreadGame == NULL) {
				_tprintf(TEXT("Erro ao criar thread readGameThread!"));
				return -1;
			}
			WaitForSingleObject(hTreadGame, INFINITE);
			break;
		case '2':
			_tprintf(TEXT("Mensagem:"));
			_tscanf_s(TEXT("%s"), newMsg.messageInfo, TAM);
			newMsg.codigoMsg = 0;
			sendMessage(newMsg);
			break;
		case '3':
			//newMsg.codigoMsg = 10;//new game
			//_tcscpy_s(newMsg.messageInfo, MAX_NAME_LENGTH, userLogged);
			//sendMessage(newMsg);
			hTreadGame = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)readGameThread, NULL, 0, &threadId);
			if (hTreadGame == NULL) {
				_tprintf(TEXT("Erro ao criar thread readGameThread!"));
				return -1;
			}
			WaitForSingleObject(hTreadGame, INFINITE);
			break;
		case '4':
			ReleaseSemaphore(canWrite, 1, NULL);
			break;
		}
	} while (KeyPress != '5');

	free(gameInfo);
	closeSharedMemoryMsg();
	closeSharedMemoryGame();
	return 0;

}

DWORD WINAPI readGameThread(LPVOID param) {
	DWORD threadId;
	game gameData;
	_tprintf(TEXT("Waiting for game to start..."));
	WaitForSingleObject(gameEvent, INFINITE);
	_tprintf(TEXT("Starting!"));
	hidecursor();
	gameData = *gameInfo;
	_tprintf(TEXT("1-Creating %d balls\n"), gameData.numBalls);
	_tprintf(TEXT("2-Creating %d balls\n"), gameInfo->numBalls);
	for (int i = 0; i < gameData.numBalls; i++) {
		hTBola[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BolaThread, (LPVOID)i, 0, &threadId);
		if (hTBola[i] == NULL) {
			_tprintf(TEXT("Erro ao criar thread para bola numero:%d!"),i);
			return -1;
		}
	}

	for (int i = 0; i < 0; i++) { //not yet
		hTUsers[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UserThread, (LPVOID)i, 0, &threadId);
		if (hTUsers[i] == NULL) {
			_tprintf(TEXT("Erro ao criar thread para bola numero:%d!"), i);
			return -1;
		}
	}

	WaitForMultipleObjects(gameData.numBalls, hTBola, TRUE, INFINITE);
	WaitForMultipleObjects(gameData.numUsers, hTUsers, TRUE, INFINITE);
	//system("cls");
	_tprintf(TEXT("end of game!!\n"));
	return 0;
}


DWORD WINAPI UserThread(LPVOID param){
	DWORD id = ((DWORD)param);
	user userInfo;
	do {
		userInfo = gameInfo->nUsers[id];
		//moves barreira
	} while (userInfo.lifes);

}


DWORD WINAPI BolaThread(LPVOID param) {
	int oposx = 0, oposy = 0, posx = 0, posy = 0;
	DWORD id = ((DWORD)param);
	ball ballInfo;
	_tprintf(TEXT("Bola[%d]-Created\n"),id);
	do {
		WaitForSingleObject(updateBalls, INFINITE);
		ballInfo = gameInfo->nBalls[id];
	
		oposx = posx;
		oposy = posy;
		WaitForSingleObject(hStdoutMutex, INFINITE);
		gotoxy(oposx, oposy);
		_tprintf(TEXT(" "));
		ReleaseMutex(hStdoutMutex);

		posx = ballInfo.posx;
		posy = ballInfo.posy;
		WaitForSingleObject(hStdoutMutex, INFINITE);
		gotoxy(posx, posy);
		_tprintf(TEXT("%d"),id);
		ReleaseMutex(hStdoutMutex);
		//_tprintf(TEXT("pos(%d,%d)\n"), game.posx, game.posy);
		//ReleaseSemaphore(canWrite, 1, NULL);
		//gotoxy(0, 0);
		//_tprintf(TEXT("Codigo:%d"),game.status);
	} while (ballInfo.status);

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