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
void readGame();
void createBalls(DWORD num);
DWORD WINAPI BolaThread(LPVOID param);
DWORD WINAPI UserThread(LPVOID param);
DWORD WINAPI receiveMessageThread(LPVOID param);
DWORD WINAPI receiveBroadcast(LPVOID param);

DWORD user_id;

HANDLE hTBola[MAX_BALLS],hTUserInput,hStdoutMutex,hTreceiveMessage;

HANDLE messageEventBroadcast, messageEvent, hTBroadcast;

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

	messageEventBroadcast = CreateEvent(NULL, FALSE, FALSE, MESSAGE_BROADCAST_EVENT_NAME);
	gameEvent = CreateEvent(NULL, TRUE, FALSE, GAME_EVENT_NNAME);
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

	system("cls");
	_tprintf(TEXT("--Welcome to Client[%d]--\n"), GetCurrentThreadId());
	_tprintf(TEXT("LOGIN:"));
	_tscanf_s(TEXT("%s"), str, MAX_NAME_LENGTH);
	Login(str);
	hTBroadcast = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveBroadcast, NULL, 0, NULL);
	if (hTBroadcast == NULL) {
		_tprintf(TEXT("Erro ao criar broadcast thread!\n"));
		return -1;
	}
	WaitForSingleObject(hTBroadcast, INFINITE);


	/*do {
		_tprintf(TEXT("--Welcome to Client[%d]--\n"),GetCurrentThreadId());
		_tprintf(TEXT("1:Login?\n"));
		_tprintf(TEXT("2:Send Message\n"));
		_tprintf(TEXT("3:Play game\n"));
		_tprintf(TEXT("4:Check var gData\n"));
		_tprintf(TEXT("5:Exit\n"));

		fflush(stdin);
		KeyPress = _gettch();
		_puttchar(KeyPress);
		//system("cls");

		switch (KeyPress) {
		case '1':
			
			break;
		case '2':
			//_tprintf(TEXT("Mensagem:"));
			//_tscanf_s(TEXT("%s"), newMsg.messageInfo, TAM);
			//newMsg.codigoMsg = 123;
			//newMsg.to = 254;
			//newMsg.from = user_id;

			//sendMessage(newMsg);
			break;
		case '3':
			//newMsg.codigoMsg = 10;//new game
			//_tcscpy_s(newMsg.messageInfo, MAX_NAME_LENGTH, userLogged);
			//sendMessage(newMsg);
			break;
		case '4':
			break;
		}
	} while (KeyPress != '5');
	*/
	closeSharedMemoryMsg();
	closeSharedMemoryGame();
	return 0;
}

DWORD WINAPI receiveMessageThread(LPVOID param) {
	msg newMsg;
	int quant = 0;

	do {
		WaitForSingleObject(messageEvent, INFINITE);
		newMsg = receiveMessage();
		_tprintf(TEXT("[%d]NewMsg(%d):%s\n-from:%d\n-to:%d\n"), quant++, newMsg.codigoMsg, newMsg.messageInfo, newMsg.from, newMsg.to);

	} while (1);
}

DWORD WINAPI receiveBroadcast(LPVOID param) {
	msg inMsg;
	TCHAR str[TAM];
	TCHAR tmp[TAM];
	
	do{
		WaitForSingleObject(messageEventBroadcast, INFINITE);
		inMsg = receiveMessage();
		WaitForSingleObject(hStdoutMutex, INFINITE);
		gotoxy(0, 1);
		_tprintf(TEXT("BROADCAST\n-to:%d    \n-from:%d     \n"), inMsg.to, inMsg.from);
	
		ReleaseMutex(hStdoutMutex);
		

		if (inMsg.codigoMsg == 2) {//unsuccessful login
			_tcscpy_s(str, TAM, TEXT("messageEventClient"));
			_itot_s(inMsg.number, tmp, TAM, 10);
			_tcscat_s(str, TAM, tmp);
			_tprintf(TEXT("(Client) HANDLE = (%s)\n"), str);
			messageEvent = CreateEvent(NULL, FALSE, FALSE, str);
			user_id = inMsg.number;
			_tprintf(TEXT("Login do Utilizador (%s) efetuado com sucesso\n"), inMsg.messageInfo);

			hTreceiveMessage = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveMessageThread, NULL, 0, NULL);
			if (hTreceiveMessage == NULL) {
				_tprintf(TEXT("Erro ao criar thread para receber mensagens!\n"));
				return -1;
			}

		}
		else if (inMsg.codigoMsg == 100) {//new game
			_tprintf(TEXT("GAME created by server...\n"));
			readGame();
		}
		else if (inMsg.codigoMsg == 101) {//new ball
			createBalls(inMsg.number);
		}
	} while (gameInfo->nUsers[user_id].lifes);
}

void readGame() {
	DWORD threadId;
	WaitForSingleObject(gameEvent, INFINITE);
	hidecursor();
	system("cls");

	hTUserInput = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UserThread, NULL, 0, &threadId);
	if (hTUserInput == NULL) {
		_tprintf(TEXT("Erro ao criar thread para o utilizador!\n"));
		return -1;
	}

	return 0;
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
					SetEvent(gameEvent);
					ResetEvent(gameEvent);
					break;
		
				case 27://esc
					//gotoxy(0, 1);
					//_tprintf(text("exiting"));
					//exitthread(null);
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
		gotoxy(0, 0);
		_tprintf(TEXT("LIFES:%d"), gameInfo->nUsers[user_id].lifes);
		//_tprintf(TEXT("Bola[%d]-(%d,%d)\n"), id, ballInfo.posx, ballInfo.posy);
		ReleaseMutex(hStdoutMutex);
	} while (ballInfo.status);
	WaitForSingleObject(hStdoutMutex, INFINITE);
	gotoxy(posx, posy);
	_tprintf(TEXT(" "));
	ReleaseMutex(hStdoutMutex);
	//_tprintf(TEXT("Bola[%d]-Deleted\n"), id);
	hTBola[id] = NULL;
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