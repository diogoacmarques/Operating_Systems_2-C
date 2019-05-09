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
void createBalls(DWORD num);
void drawHelp(BOOLEAN num);
DWORD WINAPI BolaThread(LPVOID param);
DWORD WINAPI UserThread(LPVOID param);
DWORD WINAPI receiveMessageThread(LPVOID param);
DWORD WINAPI receiveBroadcast(LPVOID param);
DWORD WINAPI BrickThread(LPVOID param);

DWORD user_id;

HANDLE hTBola[MAX_BALLS],hTUserInput,hStdoutMutex,hTreceiveMessage, hTBroadcast,hTBrick;

HANDLE messageEventBroadcast, messageEvent;

pgame gameInfo;

BOOLEAN gameStatus = 0;


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

	messageEventBroadcast = CreateEvent(NULL, FALSE, FALSE, MESSAGE_BROADCAST_EVENT_NAME);
	gameEvent = CreateEvent(NULL, TRUE, FALSE, GAME_EVENT_NAME);
	hStdoutMutex = CreateMutex(NULL, FALSE, STDOUT_CLIENT_MUTEX_NAME);
	updateBalls = CreateEvent(NULL, TRUE, FALSE, BALL_EVENT_NAME);
	canMove = CreateEvent(NULL, FALSE, FALSE, USER_MOVE_EVENT_NAME);
	//initializeHandles();

	//sessionId = (GetCurrentThreadId() + GetTickCount());
	//sessionId = GetCurrentThreadId();

	//Message
	createSharedMemoryMsg();
	//Game
	//gameInfo = (game *)malloc(sizeof(game));
	gameInfo = createSharedMemoryGame();

	for (int i = 0; i < MAX_BALLS; i++)
		hTBola[i] == NULL;

	do {
		system("cls");
		_tprintf(TEXT("--Welcome to Client[%d]--\n"), GetCurrentThreadId());
		_tprintf(TEXT("'exit' - Leave\n"));
		_tprintf(TEXT("LOGIN:"));
		_tscanf_s(TEXT("%s"), str, MAX_NAME_LENGTH);
		if (_tcscmp(str, TEXT("exit")) == 0 || _tcscmp(str, TEXT("fim")) == 0)
			break;
		for (int i = 0; i < MAX_NAME_LENGTH; i++)//preventes users from using ':' (messes up registry)
			if (str[i] == ':')
				str[i] = '\0';
		//_tprintf(TEXT("Trying login with:%s"),str);
		res = Login(str);
		if (!res) {
			_tprintf(TEXT("Login enviado sem sucesso"));
			exit(-1);
		}
		hTBroadcast = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveBroadcast, NULL, 0, NULL);
		if (hTBroadcast == NULL) {
			_tprintf(TEXT("Erro ao criar broadcast thread!\n"));
			return -1;
		}
		WaitForSingleObject(hTBroadcast, INFINITE);
	} while (1);

	system("cls");
	_tprintf(TEXT("end of user....\n"));
	closeSharedMemoryMsg();
	closeSharedMemoryGame();
	KeyPress = _gettch();
	return 0;
}

DWORD WINAPI receiveMessageThread(LPVOID param) {
	msg newMsg;
	int quant = 0;

	do {
		WaitForSingleObject(messageEvent, INFINITE);
		newMsg = receiveMessage();
		gotoxy(0, 0);
		_tprintf(TEXT("[%d]NewMsg(%d):%s                \n-from:%d                            \n-to:%d                         \n                             "), quant++, newMsg.codigoMsg, newMsg.messageInfo, newMsg.from, newMsg.to);
		if (newMsg.codigoMsg == 2) {//end of user
			TerminateThread(hTBroadcast, 1);
			CloseHandle(hTBroadcast);
			return 0;
		}
		else if (newMsg.codigoMsg == 123) {
			_tprintf(TEXT("----------TESTES--------------\n"));
		}
	} while (1);
}

DWORD WINAPI receiveBroadcast(LPVOID param) {
	msg inMsg;
	TCHAR str[TAM];
	TCHAR tmp[TAM];
	do{
		WaitForSingleObject(messageEventBroadcast, INFINITE);
		inMsg = receiveMessage();
		if (inMsg.from != 254) {
			_tprintf(TEXT("Should receive from 254 but received instead from %d...\n"), inMsg.from);
			_tprintf(TEXT("to:%d\nfrom:%d\ncontent:%s\n"), inMsg.to, inMsg.from, inMsg.messageInfo);
			Sleep(3000);
			return 0;
		}
			
		//WaitForSingleObject(hStdoutMutex, INFINITE);
		//gotoxy(0, 0);
		//_tprintf(TEXT("BROADCAST\n-to:%d    \n-from:%d     \n"), inMsg.to, inMsg.from);
		//ReleaseMutex(hStdoutMutex);
		
		if (inMsg.codigoMsg == 1) {//successful login
			system("cls");
			_tcscpy_s(str, TAM, TEXT("messageEventClient"));
			//_itot_s(inMsg.number, tmp, TAM, 10);
			_tcscat_s(str, TAM, inMsg.messageInfo);
			_tprintf(TEXT("(Client) HANDLE = (%s)\n"), str);
			messageEvent = CreateEvent(NULL, FALSE, FALSE, str);
			user_id = _tstoi(inMsg.messageInfo);
			_tprintf(TEXT("Login do Utilizador (%s) efetuado com sucesso\nWaiting for server to start game..."), inMsg.messageInfo);

			hTreceiveMessage = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveMessageThread, NULL, 0, NULL);
			if (hTreceiveMessage == NULL) {
				_tprintf(TEXT("Erro ao criar thread para receber mensagens!\n"));
				return -1;
			}

		}
		else if (inMsg.codigoMsg == -1){
			_tprintf(TEXT("Server refused login with %s\nPress Any Key ..."), inMsg.messageInfo);
			TCHAR tmp;
			fflush(stdin);
			tmp = _gettch();
			return -1;
		}
		else if (inMsg.codigoMsg == -100) {
			_tprintf(TEXT("There is no game created by the server yet\nPress Any Key ...\n"));
			TCHAR tmp;
			fflush(stdin);
			tmp = _gettch();
			return -1;
		}
		else if (inMsg.codigoMsg == 100) {//new game
			_tprintf(TEXT("GAME created by server...\n"));
			WaitForSingleObject(gameEvent, INFINITE);
			gameStatus = 1;
			hidecursor();
			hTUserInput = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UserThread, NULL, 0, NULL);
			if (hTUserInput == NULL) {
				_tprintf(TEXT("Erro ao criar thread para o utilizador!\n"));
				return -1;
			}
		}
		else if (inMsg.codigoMsg == 101) {//new ball
			createBalls(_tstoi(inMsg.messageInfo));
		}
		else if (inMsg.codigoMsg == 102) {//create bricks
			hTBrick = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BrickThread, NULL, 0, NULL);
			if (hTBrick == NULL) {
				_tprintf(TEXT("Erro ao criar thread para desenhar bricks!\n"));
				return -1;
			}			
		}
		else if (inMsg.codigoMsg == -110 && gameStatus == 0) {
			_tprintf(TEXT("A game is already in progress, this still needs to be implemented so that i can see what is going on with the game...\n"));
			TCHAR tmp;
			fflush(stdin);
			tmp = _gettch();
			return -1;
		}
	} while (1);
}


void createBalls(DWORD num) {
	//create num balls
	DWORD count = 0;
	DWORD threadId;
	for (int i = 0; i < MAX_BALLS; i++) {
		if (hTBola[i] == NULL) {//se handle is available
			hTBola[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BolaThread, (LPVOID)i, 0, &threadId);
			if (hTBola[i] == NULL) {
				_tprintf(TEXT("Erro ao criar bola numero:%d!\n"), i);
				return -1;
			}
			else {
				count++;
			}
		}

		if (count >= num)
			break;
	}

	return;
}

DWORD WINAPI UserThread(LPVOID param){
	user userInfo = gameInfo->nUsers[user_id];
	system("cls");
	drawHelp(1);
	WaitForSingleObject(hStdoutMutex, INFINITE);
	gotoxy(userInfo.posx, userInfo.posy);
	for (int i = 0; i < userInfo.size; i++) {
		_tprintf(TEXT("%d"), user_id);
	}
	ReleaseMutex(hStdoutMutex);

	TCHAR KeyPress;
	msg gameMsg;
	BOOLEAN flag;
	gameMsg.codigoMsg = 200;
	gameMsg.from = user_id;
	gameMsg.to = 254;
	while(1) {
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
				if (gameInfo->numBalls == 0 &&  gameInfo->nUsers[user_id].lifes > 0) {
					drawHelp(0);
					SetEvent(gameEvent);
					ResetEvent(gameEvent);
				}
				break;
		
			case 27://esc
				if (gameInfo->nUsers[user_id].lifes == 0) {
					gameMsg.codigoMsg = 2;
					gameMsg.from = user_id;
					gameMsg.to = 254;
					_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("exit"));
					sendMessage(gameMsg);
					return 0;
				}
				break;
		}

		if (flag) {
			WaitForSingleObject(canMove, INFINITE);
			userInfo = gameInfo->nUsers[user_id];
			if (_tcscmp(gameMsg.messageInfo, TEXT("left")) == 0) {
				WaitForSingleObject(hStdoutMutex, INFINITE);
				gotoxy(userInfo.posx + userInfo.size, userInfo.posy);
				_tprintf(TEXT(" "));
				gotoxy(userInfo.posx, userInfo.posy);
				_tprintf(TEXT("%d"), user_id);
				ReleaseMutex(hStdoutMutex);
			}
			else if (_tcscmp(gameMsg.messageInfo, TEXT("right")) == 0) {
				WaitForSingleObject(hStdoutMutex, INFINITE);
				gotoxy(userInfo.posx - 1, userInfo.posy);
				_tprintf(TEXT(" "));
				gotoxy(userInfo.posx + userInfo.size - 1, userInfo.posy);
				_tprintf(TEXT("%d"), user_id);
				ReleaseMutex(hStdoutMutex);
			}
			else if(_tcscmp(gameMsg.messageInfo, TEXT("space")) == 0) {
				//do something
			}
		}
		
	}	
}


DWORD WINAPI BolaThread(LPVOID param) {
	int oposx, oposy, posx=0, posy=0;
	DWORD id = ((DWORD)param);
	ball ballInfo;
	hidecursor();
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
		gotoxy(0, gameInfo->myconfig.limy-1);
		_tprintf(TEXT("LIFES:%d  SCORE:%d      BALL(%d,%d)   Speed=%d        "), gameInfo->nUsers[user_id].lifes,gameInfo->nUsers[user_id].score,posx,posy,gameInfo->nBalls[0].speed);
		//_tprintf(TEXT("Bola[%d]-(%d,%d)\n"), id, ballInfo.posx, ballInfo.posy);
		ReleaseMutex(hStdoutMutex);
	} while (ballInfo.status);
	WaitForSingleObject(hStdoutMutex, INFINITE);
	gotoxy(posx, posy);
	_tprintf(TEXT(" "));
	ReleaseMutex(hStdoutMutex);
	//_tprintf(TEXT("Bola[%d]-Deleted\n"), id);
	hTBola[id] = NULL;
	drawHelp(1);
	return 0;
}

DWORD WINAPI BrickThread(LPVOID param) {
	brick localBricks[MAX_BRICKS];
	int numBricks = gameInfo->numBricks;
	for (int i = 0; i < numBricks; i++)
		localBricks[i] = gameInfo->nBricks[i];
	//draws intially all bricks
	//draws intially all bricks
	for (int i = 0; i < gameInfo->numBricks; i++) {
		WaitForSingleObject(hStdoutMutex, INFINITE);
		gotoxy(gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy);
		for (int j = 0; j < gameInfo->nBricks[i].tam; j++) 
			_tprintf(TEXT("%d"), gameInfo->nBricks[i].status);
		ReleaseMutex(hStdoutMutex);
		}

	//updates
	do {
		WaitForSingleObject(updateBalls, INFINITE);
		numBricks = gameInfo->numBricks;
		for (int i = 0; i < numBricks; i++)
			if (localBricks[i].status != gameInfo->nBricks[i].status) {
				if (gameInfo->nBricks[i].status == 0) {//end of brick life
					WaitForSingleObject(hStdoutMutex, INFINITE);
					gotoxy(gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy);
					for (int j = 0; j < gameInfo->nBricks[i].tam; j++)
						_tprintf(TEXT(" "));
					ReleaseMutex(hStdoutMutex);
				}
				else {
					WaitForSingleObject(hStdoutMutex, INFINITE);
					gotoxy(gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy);
					for (int j = 0; j < gameInfo->nBricks[i].tam; j++)
						_tprintf(TEXT("%d"), gameInfo->nBricks[i].status);
					ReleaseMutex(hStdoutMutex);
					localBricks[i] = gameInfo->nBricks[i];
				}
				
			}
	} while (gameInfo->numBricks);
	
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

void drawHelp(BOOLEAN num) {
	WaitForSingleObject(hStdoutMutex, INFINITE);
	gotoxy(0, 15);
	if (num) {
		_tprintf(TEXT("A/D - Move\n"));
		_tprintf(TEXT("SPACE - New Ball\n"));
		_tprintf(TEXT("ESC - EXIT\n"));
	}
	else {
		_tprintf(TEXT("                        \n                      \n\                      "));
	}
	ReleaseMutex(hStdoutMutex);
	return;
}