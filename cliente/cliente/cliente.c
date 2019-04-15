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
	hStdoutMutex = CreateMutex(NULL, FALSE, STDOUT_CLIENT_MUTEX_NAME);
	updateBalls = CreateEvent(NULL, TRUE, FALSE, BALL_EVENT_NAME);
	canMove = CreateEvent(NULL, FALSE, FALSE, USER_EVENT_NAME);

	//Message
	createSharedMemoryMsg();
	//Game
	//gameInfo = (game *)malloc(sizeof(game));
	gameInfo = createSharedMemoryGame();


	//system("cls");

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
	_tprintf(TEXT("Waiting for game to start...\n"));
	WaitForSingleObject(gameEvent, INFINITE);
	system("cls");
	hidecursor();
	for (int i = 0; i < gameInfo->numBalls; i++) {
		hTBola[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BolaThread, (LPVOID)i, 0, &threadId);
		if (hTBola[i] == NULL) {
			_tprintf(TEXT("Erro ao criar thread para bola numero:%d!"),i);
			return -1;
		}
	}

	for (int i = 0; i < gameInfo->numUsers; i++) {
		hTUsers[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UserThread, (LPVOID)i, 0, &threadId);
		if (hTUsers[i] == NULL) {
			_tprintf(TEXT("Erro ao criar thread para bola numero:%d!"), i);
			return -1;
		}
	}

	WaitForMultipleObjects(gameInfo->numBalls, hTBola, TRUE, INFINITE);
	WaitForMultipleObjects(gameInfo->numUsers, hTUsers, TRUE, INFINITE);
	//system("cls");
	_tprintf(TEXT("end of game!!\n"));
	return 0;
}


DWORD WINAPI UserThread(LPVOID param){
	DWORD id = ((DWORD)param);
	user userInfo = gameInfo->nUsers[id];
	WaitForSingleObject(hStdoutMutex, INFINITE);
	gotoxy(userInfo.posx, userInfo.posy);
	for (int i = 0; i < userInfo.size; i++) {
		_tprintf(TEXT("%d"),id);
	}
	ReleaseMutex(hStdoutMutex);

	TCHAR KeyPress;
	msg gameMsg;
	BOOLEAN flag;
	gameMsg.user_id = id;
	gameMsg.codigoMsg = 200;
	while(gameInfo->nUsers[0].lifes >= 0) {
		WaitForSingleObject(hStdoutMutex, INFINITE);
		gotoxy(0, 0);
		_tprintf(TEXT("LIFES:%d"),gameInfo->nUsers[0].lifes);
		ReleaseMutex(hStdoutMutex);
		flag = 0;
		fflush(stdin);
		KeyPress = _gettch();
		//_putch(keypress);
		switch (KeyPress) {
			case 'a':
			case 'A':
				flag = 1;
				_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("left"));
				sendMessage(gameMsg);
				break;
		
				case 'd':
				case 'D':	
					flag = 1;
					_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("right"));
					sendMessage(gameMsg);
					break;

				case 32://sends ball
					/*WaitForSingleObject(hStdoutMutex, INFINITE);
					gotoxy(0, gameinfo->limy / 2);
					for (int i = 0; i < gameinfo->limx; i++) {
						_tprintf(text(" "));
					}
					releasemutex(stdoutmutex);
					htbola = createthread(null, 0, (lpthread_start_routine)bola, (lpvoid)gameinfo, 0, &threadid);
					if (htbola == null) {
						_tprintf(text("erro ao criar thread bola!"));
						return -1;
					}*/
					break;
		
				case 27://esc
					//gotoxy(0, 1);
					//_tprintf(text("exiting"));
					//exitthread(null);
					break;
				}

		WaitForSingleObject(canMove, INFINITE);
		userInfo = gameInfo->nUsers[id];
		if (flag) {
			if (_tcscmp(gameMsg.messageInfo, TEXT("left")) == 0) {
				WaitForSingleObject(hStdoutMutex, INFINITE);
				gotoxy(userInfo.posx + userInfo.size, userInfo.posy);
				_tprintf(TEXT(" "));
				gotoxy(userInfo.posx, userInfo.posy);
				_tprintf(TEXT("%d"), id);
				ReleaseMutex(hStdoutMutex);
			}
			else if (_tcscmp(gameMsg.messageInfo, TEXT("right")) == 0) {
				WaitForSingleObject(hStdoutMutex, INFINITE);
				gotoxy(userInfo.posx - 1, userInfo.posy);
				_tprintf(TEXT(" "));
				gotoxy(userInfo.posx + userInfo.size - 1, userInfo.posy);
				_tprintf(TEXT("%d"), id);
				ReleaseMutex(hStdoutMutex);
			}
			else if(_tcscmp(gameMsg.messageInfo, TEXT("space")) == 0) {
				//do something
			}
		}
	}


	WaitForSingleObject(hStdoutMutex, INFINITE);
	gotoxy(0, 0);
	_tprintf(TEXT("Game over for user:%d"), userInfo.user_id);
	ReleaseMutex(hStdoutMutex);
	
}


DWORD WINAPI BolaThread(LPVOID param) {
	int oposx, oposy, posx=0, posy=0;
	DWORD id = ((DWORD)param);
	ball ballInfo;
	//_tprintf(TEXT("Bola[%d]-Created\n"),id);
	do {
		WaitForSingleObject(updateBalls, INFINITE);
		//_tprintf(TEXT("Bola[%d]-Updated\n"), id);
		ballInfo = gameInfo->nBalls[id];
		oposx = posx;
		oposy = posy;

		posx = ballInfo.posx;
		posy = ballInfo.posy;
	
		WaitForSingleObject(hStdoutMutex, INFINITE);
		gotoxy(oposx, oposy);
		_tprintf(TEXT(" "));
		ReleaseMutex(hStdoutMutex);

		posx = ballInfo.posx;
		posy = ballInfo.posy;
		WaitForSingleObject(hStdoutMutex, INFINITE);
		gotoxy(posx, posy);
		_tprintf(TEXT("%d"),id);
		//_tprintf(TEXT("Bola[%d]-(%d,%d)\n"), id, ballInfo.posx, ballInfo.posy);
		ReleaseMutex(hStdoutMutex);
	} while (ballInfo.status);
	WaitForSingleObject(hStdoutMutex, INFINITE);
	gotoxy(posx, posy);
	_tprintf(TEXT(" "));
	ReleaseMutex(hStdoutMutex);
	//_tprintf(TEXT("Bola[%d]-Deleted\n"), id);
	return 0;
}

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