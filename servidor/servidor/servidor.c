#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "DLL.h"

DWORD WINAPI Bola(LPVOID param);
DWORD WINAPI receiveMessageThread(LPVOID param);

HANDLE canRead;
HANDLE canWrite;
HANDLE receiveMessageEvent;

BOOLEAN continua = 1;

int _tmain(int argc, LPTSTR argv[]) {
	//DLL_tprintf(TEXT("Resultado da função da UmaString DLL: %d"), UmaString());
	DWORD threadId;
	HANDLE hTBola,hTReceiveMessage;
	TCHAR KeyPress;
	//UNICODE: Por defeito, a consola Windows não processe caracteres wide.
	//A maneira mais fácil para ter esta funcionalidade é chamar _setmode:
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif
	canWrite = CreateSemaphore(NULL, 1, 1, SEMAPHORE_MEMORY_WRITE);
	canRead = CreateSemaphore(NULL, 0, 1, SEMAPHORE_MEMORY_READ);

	receiveMessageEvent = CreateEvent(NULL, FALSE, FALSE, SENDMESSAGEEVENT);

	createSharedMemory();
	hTReceiveMessage = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveMessageThread, NULL, 0, &threadId);
	if (hTReceiveMessage == NULL) {
		_tprintf(TEXT("Erro ao criar thread ReceiveMessage!"));
		return -1;
	}

	do{
		_tprintf(TEXT("--Welcome to Server--\n"));
		_tprintf(TEXT("1:New Game\n"));
		_tprintf(TEXT("2:Let client see\n"));
		_tprintf(TEXT("3:Read\n"));
		_tprintf(TEXT("4:Exit\n"));
		
		fflush(stdin);
		KeyPress = _gettch();
		_puttchar(KeyPress);
		system("cls");

		switch (KeyPress) {
		case '1':
			hTBola = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Bola, NULL, 0, &threadId);
			if (hTBola == NULL) {
				_tprintf(TEXT("Erro ao criar thread Bola!"));
				return -1;
			}
			break;
		case '2':
			_tprintf(TEXT("2 is empty\n"));
			break;
		case '3':
			_tprintf(TEXT("3 is empty...\n"));
			break;
		}
	} while (KeyPress != '4');

	continua = 0;
	//WaitForSingleObject(hTBola, INFINITE);
	SetEvent(receiveMessageEvent);
	WaitForSingleObject(hTReceiveMessage, INFINITE);
	closeSharedMemory();
	_tprintf(TEXT("[Thread Principal %d] Vou terminar..."), GetCurrentThreadId());
	return 0;
}

DWORD WINAPI receiveMessageThread(LPVOID param) {
	msg newMsg;
	do{
		WaitForSingleObject(receiveMessageEvent,INFINITE);
		newMsg = receiveMessage();
		if (newMsg.codigoMsg) {//login de utilizador
			_tprintf(TEXT("Login do Utilizador (%s)\n"), newMsg.messageInfo);
		} else {//nova mensagem
			_tprintf(TEXT("NewMsg(%d):%s\n"), newMsg.codigoMsg, newMsg.messageInfo);
		}
	} while (continua);
}

DWORD WINAPI Bola(LPVOID param) {
	srand((int)time(NULL));
	gameData game;
	game.status = 0;
	DWORD posx, posy , oposx, oposy;
	boolean goingUp = 1, goingRight = (rand() % 2);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	game.limx = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	game.limy = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	game.posx = 0;
	game.posy = 0;
	posx = game.limx / 2;
	posy = game.limy / 2;

	//_tcscpy_s(content.namePlayer, LOCALE_NAME_MAX_LENGTH, "diogo");
	int num = 0;
	
	while (1) {
		//Sleep(30);
		Sleep(1000);
		oposx = posx;
		oposy = posy;
		if (goingRight) {
			if (posx < game.limx - 1) {
				posx++;
			}
			else {
				posx--;
				goingRight = 0;
			}
		}
		else {
			if (posx > 0) {
				posx--;
			}
			else {
				posx++;
				goingRight = 1;
			}
		}

		if (goingUp) {
			if (posy > 0) {
				posy--;
			}
			else {
				posy++;
				goingUp = 0;
			}
		}
		else {
			if (posy < game.limy - 1) {// se nao atinge o limite do mapa
				//if (posy == game->baPosy - 1 && (posx >= gameInfo->baPosx && posx <= (gameInfo->baPosx + gameInfo->baTam))) {//atinge a barreira
				//	posy--;
				//	goingUp = 1;
				//}
				//else
				//	posy++;
			}
			else {//se atinge o fim do mapa
				game.status = -1;
				game.posx = 0;
				game.posy = 0;
				sendGame(game);
				ReleaseSemaphore(canRead, 1, NULL);
				_tprintf(TEXT("Game Over!\n"));
				ExitThread(NULL);
				posy--;
				goingUp = 1;
			}
		}
		
		game.posx = posx;
		game.posy = posy;
		game.status = 1;

		//_tprintf(TEXT("[%d]waiting-"),num++);
		WaitForSingleObject(canWrite, INFINITE);
		_tprintf(TEXT("[%d]wrote"), num++);
		gData = &game;
		ReleaseSemaphore(canRead, 1, NULL);
		//_tprintf(TEXT("escrevi\n"));
	}
	return 0;
}
