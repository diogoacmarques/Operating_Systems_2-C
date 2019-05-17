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
void createBonus(DWORD num);
void drawHelp(BOOLEAN num);
void usersMove(TCHAR move[TAM]);
void watchGame();
void endUser();
DWORD WINAPI BolaThread(LPVOID param);
DWORD WINAPI UserThread(LPVOID param);
DWORD WINAPI receiveMessageThread(LPVOID param);
DWORD WINAPI receiveBroadcast(LPVOID param);
DWORD WINAPI BrickThread(LPVOID param);
DWORD WINAPI bonusDrop(LPVOID param);

DWORD user_id;

HANDLE hTBola[MAX_BALLS],hTUserInput,hStdoutMutex,hTreceiveMessage, hTBroadcast,hTBrick, hTUserOutput, hTBonus[MAX_BONUS_AT_TIME];

HANDLE  messageEvent;

pgame gameInfo;

DWORD localGameStatus = 0;//0 = no game | 1 = playing | 2 = watching
HANDLE messageEventBroadcast;

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
	updateBalls = CreateEvent(NULL, TRUE, FALSE, BALL_EVENT_NAME);
	updateBonus = CreateEvent(NULL, FALSE, FALSE, BONUS_EVENT_NAME);
	hStdoutMutex = CreateMutex(NULL, FALSE, NULL);
	res = initializeHandles();
	if (res) {
		_tprintf(TEXT("Erro ao criar Hnaldes!"));
		//return 1;
	}
	else {
		_tprintf(TEXT("Handles iniciadas com sucesso!\n"));
	}

	//sessionId = (GetCurrentThreadId() + GetTickCount());
	//sessionId = GetCurrentThreadId();

	//Message
	createSharedMemoryMsg();
	//Game
	//gameInfo = (game *)malloc(sizeof(game));
	gameInfo = createSharedMemoryGame();

	for (int i = 0; i < MAX_BALLS; i++)
		hTBola[i] == INVALID_HANDLE_VALUE;

	do {
		
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
			_tprintf(TEXT("Login nao enviado"));
			exit(-1);
		}
		else {
			_tprintf(TEXT("Login enviado com sucesso\n"));
		}

		hTBroadcast = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveBroadcast, NULL, 0, NULL);
		if (hTBroadcast == NULL) {
			_tprintf(TEXT("Erro ao criar broadcast thread!\n"));
			return -1;
		}
		WaitForSingleObject(hTBroadcast, INFINITE);
	} while (1);

	system("cls");
	_tprintf(TEXT("3 seconds:end of user....\n"));
	closeSharedMemoryMsg();
	closeSharedMemoryGame();
	Sleep(3000);
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
			endUser();
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
	BOOLEAN logged = 0;
	do{
		//_tprintf(TEXT("Waiting for message on Broadcast thread\n"));
		WaitForSingleObject(messageEventBroadcast, INFINITE);
		inMsg = receiveMessage();
		//_tprintf(TEXT("Codigo:%d\nto:%d\nfrom:%d\ncontent:%s\n"), inMsg.codigoMsg, inMsg.to, inMsg.from, inMsg.messageInfo);
		if (inMsg.from != 254) {
			_tprintf(TEXT("Should receive from 254 but received instead from %d...\n"), inMsg.from);
			_tprintf(TEXT("Codigo:%d\nto:%d\nfrom:%d\ncontent:%s\n"), inMsg.codigoMsg,inMsg.to, inMsg.from, inMsg.messageInfo);
			Sleep(3000);
			return 0;
		}
		

		//WaitForSingleObject(hStdoutMutex, INFINITE);
		//gotoxy(0, 0);
		//_tprintf(TEXT("BROADCAST\n-to:%d    \n-from:%d     \n"), inMsg.to, inMsg.from);
		//ReleaseMutex(hStdoutMutex);
		
		if (inMsg.codigoMsg == 1 && !logged) {//successful login
			logged = 1;
			//system("cls");
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
		else if (inMsg.codigoMsg == -1 && !logged){
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
			localGameStatus = 1;
			hidecursor();
			usersMove(TEXT("init"));
			hTUserInput = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UserThread, NULL, 0, NULL);
			if (hTUserInput == NULL) {
				_tprintf(TEXT("Erro ao criar thread para o utilizador escrever!\n"));
				return -1;
			}
		}
		else if (inMsg.codigoMsg == 101) {//new ball
			DWORD tmp = _tstoi(inMsg.messageInfo);
			//_tprintf(TEXT("creating %d balls thread para o utilizador!\n"),tmp);
			createBalls(tmp);
		}
		else if (inMsg.codigoMsg == 102) {//create bricks
			hTBrick = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BrickThread, NULL, 0, NULL);
			if (hTBrick == NULL) {
				_tprintf(TEXT("Erro ao criar thread para desenhar bricks!\n"));
				return -1;
			}			
		}
		else if (inMsg.codigoMsg == 103) {
			int tmp = _tstoi(inMsg.messageInfo);
			createBonus(tmp);
		}
		else if (inMsg.codigoMsg == -110 && localGameStatus == 0) {
			_tprintf(TEXT("A game is already in progress! Would you like to watch?(Y/N):"));
			fflush(stdin);
			TCHAR resp;
			fflush(stdin);
			resp = _gettch();
			_tprintf(TEXT("\n"));
			if (resp == 'n' || resp == 'N') {
				return -1;
			}
			watchGame();
		}
		else if (inMsg.codigoMsg == 200) {
			usersMove(inMsg.messageInfo);
		}else if (inMsg.codigoMsg == -999) {
			endUser();
		}
	} while (1);
}

void createBalls(DWORD num) {
	//create num balls
	DWORD count = 0;
	DWORD threadId;
	for (int i = 0; i < MAX_BALLS; i++) {
		//_tprintf(TEXT("BOLA[%d]\n"),i);
		if (hTBola[i] == INVALID_HANDLE_VALUE || hTBola[i] == NULL) {//se handle is available
			//_tprintf(TEXT("Found a free handle\n"));
			hTBola[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BolaThread, (LPVOID)i, 0, &threadId);
			if (hTBola[i] == NULL) {
				_tprintf(TEXT("Erro ao criar bola numero:%d!\n"), i);
				return -1;
			}
			else {
				count++;
				//_tprintf(TEXT("BolaThread Created\n"));
			}
		}

		if (count >= num)
			break;
	}

	return;
}

void createBonus(DWORD id) {
	//create num balls
	DWORD threadId;
	//_tprintf(TEXT("Creating Bonus with id:%d\n"), id);
	for (int i = 0; i < MAX_BONUS_AT_TIME; i++) {
		if (hTBonus[i] == INVALID_HANDLE_VALUE || hTBonus[i] == NULL) {//se handle is available
			//_tprintf(TEXT("Found a free handle\n"));
			hTBonus[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)bonusDrop, (LPVOID)id, 0, &threadId);
			if (hTBonus[i] == NULL) {
				_tprintf(TEXT("Erro ao criar bola numero:%d!\n"), i);
				return -1;
			}
			else {
				break;
			}
		}
	}

	return;
}




DWORD WINAPI UserThread(LPVOID param){
	Sleep(1000);
	user userInfo = gameInfo->nUsers[user_id];
	//gotoxy(0, 10);
	//_tprintf(TEXT("I have the id number:%d\n"), user_id);
	//_tprintf(TEXT("my name is %s\n"), userInfo.name);
	//_tprintf(TEXT("I am in pos(%d,%d)\n"), userInfo.posx,userInfo.posy);
	//_tprintf(TEXT("My id is'%d' and my name is size is'%d'\n"), userInfo.user_id,userInfo.size);
	TCHAR KeyPress;
	msg gameMsg;
	
	while(1) {
		gameMsg.codigoMsg = 200;
		gameMsg.from = user_id;
		gameMsg.to = 254;
		fflush(stdin);
		KeyPress = _gettch();
		//_putch(keypress);
		switch (KeyPress) {
			case 'a':
			case 'A':
				if (localGameStatus == 2)//watching
					break;
				_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("left"));
				sendMessage(gameMsg);
				break;
		
			case 'd':
			case 'D':	
				if (localGameStatus == 2)//watching
					break;
				_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("right"));
				sendMessage(gameMsg);
				break;

			case 32://sends ball
				if (localGameStatus == 2)//watching
					break;
				gameMsg.codigoMsg = 101;
				gameMsg.from = user_id;
				gameMsg.to = 254;
				_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("ball"));
				sendMessage(gameMsg);
				break;
		
			case 27://esc
				if (localGameStatus == 2) {//watching
					endUser();
					return 0;
				}
				
				gameMsg.codigoMsg = 2;
				gameMsg.from = user_id;
				gameMsg.to = 254;
				_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("exit"));
				sendMessage(gameMsg);
				return 0;
		}
	
	}	
}

void usersMove(TCHAR move[TAM]) {
	if (_tcscmp(move, TEXT("init")) == 0) {
		system("cls");
		drawHelp(1);
		for (int i = 0; i < gameInfo->numUsers; i++) {
			//gotoxy(0, i);
			//_tprintf(TEXT("Drwaing user %d"), i);
			WaitForSingleObject(hStdoutMutex, INFINITE);
			gotoxy(gameInfo->nUsers[i].posx, gameInfo->nUsers[i].posy);
			for(int j = 0;j< gameInfo->nUsers[i].size;j++)
				_tprintf(TEXT("%d"), i);
			ReleaseMutex(hStdoutMutex);
		}
		return;
	}

	//this trhreads displays users barreiras
	BOOLEAN flag = 0;
	DWORD user_id, j = 0;
	TCHAR usr[TAM];
	TCHAR direction[TAM];
	for (int i = 0; i < TAM; i++) {
		if (move[i] == ':') {
			usr[i] == '\0';
			flag = 1;
			continue;
		}
		else if (move[i] == '\0') {
			direction[j] = '\0';
			break;
		}

		if (!flag)
			usr[i] = move[i];
		else {
			direction[j] = move[i];
			j++;
		}		
	}
	user_id = _tstoi(usr);
	if (user_id < 0 || user_id > MAX_USERS) {
		_tprintf(TEXT("Invalido->From (%s) to user[%d]->(%s)\n"),move,user_id,direction);
		return;
	}
		
	user userinfo = gameInfo->nUsers[user_id];
	if (_tcscmp(direction, TEXT("left")) == 0) {
		WaitForSingleObject(hStdoutMutex, INFINITE);
		gotoxy(userinfo.posx + userinfo.size, userinfo.posy);
		_tprintf(TEXT(" "));
		gotoxy(userinfo.posx, userinfo.posy);
		_tprintf(TEXT("%d"), user_id);
		ReleaseMutex(hStdoutMutex);
	}
	else if (_tcscmp(direction, TEXT("right")) == 0) {
		WaitForSingleObject(hStdoutMutex, INFINITE);
		gotoxy(userinfo.posx - 1, userinfo.posy);
		_tprintf(TEXT(" "));
		gotoxy(userinfo.posx + userinfo.size - 1, userinfo.posy);
		_tprintf(TEXT("%d"), user_id);
		ReleaseMutex(hStdoutMutex);
	}
	//_tprintf(TEXT("im out\n"));
	return;
}

DWORD WINAPI BolaThread(LPVOID param) {
	int oposx, oposy, posx=0, posy=0;
	DWORD id = ((DWORD)param);
	ball ballInfo;
	hidecursor();
	DWORD count = 0;
	//_tprintf(TEXT("Bola[%d]-Created\n"),id);
	do {
		WaitForSingleObject(updateBalls, INFINITE);
		WaitForSingleObject(hStdoutMutex, INFINITE);
		gotoxy(0, 0);
		_tprintf(TEXT("[%d]Bola[%d]-Updated\n"),count++, id);
		ReleaseMutex(hStdoutMutex);
		
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
		gotoxy(gameInfo->myconfig.limx - 10, gameInfo->myconfig.limy - 1);
		if (localGameStatus == 1) {
			_tprintf(TEXT("[PLAYING]"));
		}
		else if(localGameStatus == 2){
			_tprintf(TEXT("[WATCHING]"));
		}
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
	hTBola[id] = INVALID_HANDLE_VALUE;
	return 0;
}

DWORD WINAPI BrickThread(LPVOID param) {
	brick localBricks[MAX_BRICKS];
	int numBricks = gameInfo->numBricks;
	//draws intially all bricks
	for (int i = 0; i < numBricks; i++) {
		WaitForSingleObject(hStdoutMutex, INFINITE);

		if (gameInfo->nBricks[i].status <= 0 && localGameStatus == 2) {
			ReleaseMutex(hStdoutMutex);
			continue;	
		}

		gotoxy(gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy);
		for (int j = 0; j < gameInfo->nBricks[i].tam; j++) 
			_tprintf(TEXT("%d"), gameInfo->nBricks[i].status);
		ReleaseMutex(hStdoutMutex);
		}

	//updates
	do {
		WaitForSingleObject(updateBalls, INFINITE);
		//numBricks = gameInfo->numBricks;
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
	return;
	WaitForSingleObject(hStdoutMutex, INFINITE);
	gotoxy(0, 15);
	if (num) {
		_tprintf(TEXT("A/D - Move\n"));
		_tprintf(TEXT("SPACE - New Ball\n"));
		_tprintf(TEXT("ESC - EXIT\n"));
		_tprintf(TEXT("NUM OF PLAYERS:%d\n"),gameInfo->numUsers);
	}
	else {
		_tprintf(TEXT("                        \n                      \n\                      \n                                     "));
	}
	ReleaseMutex(hStdoutMutex);
	return;
}

void watchGame() {
	localGameStatus = 2;
	hidecursor();
	WaitForSingleObject(hStdoutMutex, INFINITE);
	gotoxy(0,0);
	ReleaseMutex(hStdoutMutex);
	usersMove(TEXT("init"));
	
	if (gameInfo->numBalls > 0)
		createBalls(gameInfo->numBalls);

	hTBrick = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BrickThread, NULL, 0, NULL);
	if (hTBrick == NULL) {
		_tprintf(TEXT("Erro ao criar thread para desenhar bricks!\n"));
		return -1;
	}

	hTUserInput = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UserThread, NULL, 0, NULL);
	if (hTUserInput == NULL) {
		_tprintf(TEXT("Erro ao criar thread para o utilizador escrever!\n"));
		return -1;
	}

}


void endUser() {
	//broadcast thread
	TerminateThread(hTBroadcast, 1);
	CloseHandle(hTBroadcast);

	//message thread
	TerminateThread(hTreceiveMessage, 1);
	CloseHandle(hTreceiveMessage);

	//brick thread
	TerminateThread(hTBrick, 1);
	CloseHandle(hTBrick);

	//ball thread
	for (int i = 0; i < gameInfo->numBalls; i++){
		TerminateThread(hTBola[i], 1);
		CloseHandle(hTBola[i]);
		hTBola[i] = INVALID_HANDLE_VALUE;
	}

	localGameStatus = 0;
}

DWORD WINAPI bonusDrop(LPVOID param) {
	DWORD id = ((DWORD)param);
	int oposy,posy = -1;
	int posx = gameInfo->nBricks[id].brinde.posx;
	int count = 0;
	//WaitForSingleObject(hStdoutMutex, INFINITE);
	//gotoxy(0, gameInfo->myconfig.limy / 2);
	//_tprintf(TEXT("Created bonus with id:%d!"), id);
	//ReleaseMutex(hStdoutMutex);
	do {
		oposy = posy;
		WaitForSingleObject(updateBonus,INFINITE);

		if (gameInfo->nBricks[id].brinde.posy == oposy) {
			WaitForSingleObject(hStdoutMutex, INFINITE);
			gotoxy(0, 9);
			_tprintf(TEXT("[%d]should not update bonus pos(%d,%d)!"), count++,posx,oposy);
			ReleaseMutex(hStdoutMutex);
			continue;
		}

		posy = gameInfo->nBricks[id].brinde.posy;

		WaitForSingleObject(hStdoutMutex, INFINITE);
		gotoxy(0, 10);
		_tprintf(TEXT("[%d]should update bonus pos(%d,%d)!"),count++,posx,oposy);
		gotoxy(posx, oposy);
		_tprintf(TEXT(" "));
		gotoxy(posx, gameInfo->nBricks[id].brinde.posy);
		_tprintf(TEXT("B"));
		ReleaseMutex(hStdoutMutex);

	} while (gameInfo->nBricks[id].brinde.status);

	WaitForSingleObject(hStdoutMutex, INFINITE);
	gotoxy(posx, oposy);
	_tprintf(TEXT(" "));
	gotoxy(0, 11);
	_tprintf(TEXT("End of bonus %d!"), id);
	ReleaseMutex(hStdoutMutex);

	return;
}