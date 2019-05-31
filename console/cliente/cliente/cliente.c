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
void sendMessage(msg sendMsg);
void LoginUser(TCHAR user[MAX_NAME_LENGTH]);
DWORD resolveMessage(msg inMsg);
BOOLEAN createLocalConnection();
BOOLEAN createRemoteConnection();
void pressKey();
DWORD WINAPI receiveGamePipe(LPVOID param);
DWORD WINAPI pipeConnection(LPVOID param);
DWORD WINAPI BolaThread(LPVOID param);
DWORD WINAPI UserThread(LPVOID param);
DWORD WINAPI localConnection(LPVOID param);
DWORD WINAPI BrickThread(LPVOID param);
DWORD WINAPI bonusDrop(LPVOID param);


DWORD user_id;
DWORD client_id;

HANDLE hTBola[MAX_BALLS],hTUserInput,hStdoutMutex,hTBrick, hTBonus[MAX_BONUS_AT_TIME];

HANDLE newClient;

HANDLE  messageEvent;

pgame gameInfo;

DWORD localGameStatus = 0;//0 = no game | 1 = playing | 2 = watching

HANDLE hTMsgConnection;//thread responsible for incoming messages(local and remote)
HANDLE hTGameConnection;
DWORD connection_mode = -1;

HANDLE hPipe;

BOOLEAN canSendMsg;

HANDLE gameReady;

HANDLE xy;

DWORD quantMsg = 0;

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

	do {
		_tprintf(TEXT("What type of connection would you like?(0=local | 1=remote) > "));
		fflush(stdin);
		KeyPress = _gettch();
		_putch(KeyPress);

		_tprintf(TEXT("\n"));
		if (KeyPress == '0') {
			connection_mode = 0;
			break;
		}
		else if (KeyPress == '1') {
			connection_mode = 1;
			break;
		}
	} while (1);

	canSendMsg = TRUE;
	xy = CreateMutex(NULL, FALSE, NULL);
	gameReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	newClient = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (newClient == NULL || gameReady == NULL) {
		_tprintf(TEXT("Erro ao criar event newClient ou WriteGame. Erro = %d\n"), GetLastError());
	}

	if (connection_mode == 0)
		res = createLocalConnection();
	else if (connection_mode == 1)
		res = createRemoteConnection();

	if (res) {
		_tprintf(TEXT("Erro ao criar connections\n"));
		pressKey();
		return -1;
	}

	for (int i = 0; i < MAX_BALLS; i++)
		hTBola[i] == INVALID_HANDLE_VALUE;


	do {
		WaitForSingleObject(newClient, INFINITE);
		//system("cls");
		_tprintf(TEXT("--Welcome to Client[%d]--\n"), user_id);
		_tprintf(TEXT("'exit' - Leave\n"));
		_tprintf(TEXT("LOGIN:"));
		_tscanf_s(TEXT("%s"), str, MAX_NAME_LENGTH);
		if (_tcscmp(str, TEXT("exit")) == 0 || _tcscmp(str, TEXT("fim")) == 0)
			break;
		for (int i = 0; i < MAX_NAME_LENGTH; i++)//preventes users from using ':' (messes up registry)
			if (str[i] == ':')
				str[i] = '\0';
		//_tprintf(TEXT("Trying login with:%s"),str);
		LoginUser(str);
	} while (1);

	system("cls");
	_tprintf(TEXT("3 seconds:end of user....\n"));
	//broadcast thread
	TerminateThread(hTMsgConnection, 1);
	CloseHandle(hTMsgConnection);
	closeSharedMemoryMsg();
	closeSharedMemoryGame();
	Sleep(3000);
	return 0;
}

DWORD WINAPI localConnection(LPVOID param) {
	msg newMsg;
	int quant = 0;
	DWORD resp;

	do {
		WaitForSingleObject(messageEvent, INFINITE);
		newMsg = receiveMessageDLL();
	
		resp = resolveMessage(newMsg);

	} while (1);

	return;
	//do {
	//	WaitForSingleObject(messageEvent, INFINITE);
	//	newMsg = receiveMessageDLL();
	//	gotoxy(0, 0);
	//	_tprintf(TEXT("[%d]NewMsg(%d):%s                \n-from:%d                            \n-to:%d                         \n                             "), quant++, newMsg.codigoMsg, newMsg.messageInfo, newMsg.from, newMsg.to);
	//	if (newMsg.codigoMsg == 2) {//end of user
	//		endUser();
	//		return 0;
	//	}
	//	else if (newMsg.codigoMsg == 123) {
	//		_tprintf(TEXT("----------TESTES--------------\n"));
	//	}
	//} while (1);
}

DWORD resolveMessage(msg inMsg) {
	canSendMsg = TRUE;
	//WaitForSingleObject(hStdoutMutex, INFINITE);
	//gotoxy(0, 0);
	//_tprintf(TEXT("\n\n[%d]Codigo:%d\nto:%d\nfrom:%d\ncontent:%s\n"),quantMsg++, inMsg.codigoMsg, inMsg.to, inMsg.from, inMsg.messageInfo);
	//ReleaseMutex(hStdoutMutex);
	if (inMsg.codigoMsg == 9999) {
		_tprintf(TEXT("New client allowed\n"));
		client_id = _tstoi(inMsg.messageInfo);
		_tprintf(TEXT("initianting receiveGamePipe thread\n"));
		//thread responsible for game updates
		if (connection_mode) {//if is via pipes then opens up receivegamepipe
			hTGameConnection = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveGamePipe, NULL, 0, NULL);
			if (hTGameConnection == NULL) {
				_tprintf(TEXT("Erro na criãção da thread para remotePipe for game. Erro = %d\n"), GetLastError());
				pressKey();
				return 1;
			}
		}
		
		SetEvent(newClient);
		return;
	}

	TCHAR str[TAM];
	TCHAR tmp[TAM];
	BOOLEAN logged = 0;
	if (inMsg.from != 254) {
		_tprintf(TEXT("Should receive from 254 but received instead from %d...\n"), inMsg.from);
		_tprintf(TEXT("Codigo:%d\nto:%d\nfrom:%d\ncontent:%s\n"), inMsg.codigoMsg, inMsg.to, inMsg.from, inMsg.messageInfo);
		Sleep(3000);
		return 0;
	}
	//WaitForSingleObject(hStdoutMutex, INFINITE);
	//gotoxy(0, 0);
	//_tprintf(TEXT("BROADCAST\n-to:%d    \n-from:%d     \n"), inMsg.to, inMsg.from);
	//ReleaseMutex(hStdoutMutex);

	if (inMsg.codigoMsg == 1 && !logged) {//successful login
		logged = 1;
		_tprintf(TEXT("Login do Utilizador (%s) efetuado com sucesso\nWaiting for server to start game..."), inMsg.messageInfo);
	}
	else if (inMsg.codigoMsg == -1 && !logged) {
		_tprintf(TEXT("Server refused login with %s\n"), inMsg.messageInfo);
		pressKey();
		endUser();
		return -1;
	}
	else if (inMsg.codigoMsg == -100) {
		_tprintf(TEXT("There is no game created by the server yet\n"));
		pressKey();
		endUser();
		return -1;
	}
	else if (inMsg.codigoMsg == 100) {//new game
		_tprintf(TEXT("\nGAME created by server... Status = %d\n"),gameInfo->gameStatus);
		localGameStatus = 1;
		hidecursor();
		system("cls");
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
	}
	else if (inMsg.codigoMsg == -999) {
		endUser();
	}
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
		gameMsg.from = client_id;
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
		//_tprintf(TEXT("Iniciating %d players\n"), gameInfo->numUsers);
		//_tprintf(TEXT("Player 0 pos(%d,%d)\n"), gameInfo->nUsers[0].posx,gameInfo->nUsers[0].posy);
		//Sleep(5000);
		//system("cls");
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
	else if (_tcscmp(move, TEXT("no")) == 0)
		return;

	if (connection_mode == 1) {
		WaitForSingleObject(gameReady, INFINITE);//waits for info to come from pipe
	}

	//this function displays users barreiras
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
		Sleep(2000);
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

	WaitForSingleObject(hStdoutMutex, INFINITE);
	gotoxy(0, 15);
	_tprintf(TEXT("User[%d]-pos(%d,%d)"),user_id, userinfo.posx, userinfo.posy);
	ReleaseMutex(hStdoutMutex);

	return;
}

DWORD WINAPI BolaThread(LPVOID param) {
	int oposx, oposy, posx=0, posy=0;
	DWORD id = ((DWORD)param);
	ball ballInfo;
	hidecursor();
	DWORD count = 0;
	BOOLEAN flag;
	//_tprintf(TEXT("Bola[%d]-Created\n"),id);
	WaitForSingleObject(hStdoutMutex, INFINITE);
	gotoxy(gameInfo->myconfig.limx - 10, gameInfo->myconfig.limy - 1);
	if (localGameStatus == 1) {
		_tprintf(TEXT("[PLAYING]"));
	}
	else if (localGameStatus == 2) {
		_tprintf(TEXT("[WATCHING]"));
	}
	ReleaseMutex(hStdoutMutex);

	do {
		if(connection_mode == 0)
			WaitForSingleObject(updateBalls, INFINITE);
		else if(connection_mode == 1)
			WaitForSingleObject(gameReady, INFINITE);
					
		ballInfo = gameInfo->nBalls[id];

		if (ballInfo.posx == posx && ballInfo.posy == posy) {
			flag = FALSE;
		}
		else {
			flag = TRUE;
		}
			
		if (flag) {
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
			_tprintf(TEXT("%d"), id);
			gotoxy(0, gameInfo->myconfig.limy - 1);
			_tprintf(TEXT("LIFES:%d  SCORE:%d      BALL(%d,%d)   Speed=%d        "), gameInfo->nUsers[user_id].lifes, gameInfo->nUsers[user_id].score, posx, posy, gameInfo->nBalls[0].speed);
			ReleaseMutex(hStdoutMutex);
		}
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
	//_tprintf(TEXT("should create %d bricks"), gameInfo->numBricks);
	brick localBricks[MAX_BRICKS];

	if (connection_mode == 1) {
		do {
			WaitForSingleObject(gameReady, INFINITE);
			if (gameInfo->numBricks > 0 && gameInfo->numBricks < MAX_BRICKS)
				break;
		} while (1);
	}
	
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
	//WaitForSingleObject(hStdoutMutex, INFINITE);
	//gotoxy(0,0);
	//ReleaseMutex(hStdoutMutex);
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
	//brick thread
	if (hTBrick != NULL) {
		TerminateThread(hTBrick, 1);
		CloseHandle(hTBrick);
	}
	hTBrick = NULL;

	//ball thread
	for (int i = 0; i < gameInfo->numBalls; i++){
		TerminateThread(hTBola[i], 1);
		CloseHandle(hTBola[i]);
		hTBola[i] = INVALID_HANDLE_VALUE;
	}

	localGameStatus = 0;
	SetEvent(newClient);
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

void sendMessage(msg sendMsg){
	//_tprintf(TEXT("Sending -> Codigo:%d\nto:%d\nfrom:%d\ncontent:%s\n"), sendMsg.codigoMsg, sendMsg.to, sendMsg.from, sendMsg.messageInfo);
	HANDLE WriteReady;
	OVERLAPPED OverlWr = { 0 };
	DWORD bytesWritten;
	BOOLEAN fSuccess;

	if (canSendMsg)
		canSendMsg = FALSE;
	else
		return;

	if (connection_mode == 0) {
		sendMsg.connection = 0;
			sendMessageDLL(sendMsg);
	}		
	else if (connection_mode == 1) {
		sendMsg.connection = 1;
		WriteReady = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (WriteReady == NULL) {
			_tprintf(TEXT("Erro ao criar writeRead Event. Erro = %d\n"), GetLastError());
			return 0;
		}

		//_tprintf(TEXT("Ligaçao establecida...\n"));

		ZeroMemory(&OverlWr, sizeof(OverlWr));
		ResetEvent(WriteReady);
		OverlWr.hEvent = WriteReady;

	
		fSuccess = WriteFile(
			hPipe,
			&sendMsg,
			sizeof(msg),
			&bytesWritten,
			&OverlWr
		);

		WaitForSingleObject(WriteReady, INFINITE);

		GetOverlappedResult(hPipe, &OverlWr, &bytesWritten, FALSE);
		if (bytesWritten < sizeof(msg)) {
			_tprintf(TEXT("Write File failed... | Erro = %d\n"), GetLastError());
		}

		//_tprintf(TEXT("[ESCRITOR] Enviei %d bytes ao leitor...(WriteFile)\n"), bytesWritten);

	}
}

BOOLEAN createRemoteConnection() {
	HANDLE hPipeTmp;
	BOOL fSuccess = FALSE;
	DWORD dwMode;
	DWORD dwThreadId = 0;
	while (1) {

		_tprintf(TEXT("CreateFile...\n"));
		hPipeTmp = CreateFile(
			INIT_PIPE_MSG_NAME,
			GENERIC_READ | GENERIC_WRITE,
			0 | FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0 | FILE_FLAG_OVERLAPPED,
			NULL
		);


		if (hPipeTmp != INVALID_HANDLE_VALUE) {
		_tprintf(TEXT("I got a valid hPipe\n"));
			break;
		}	


		if (GetLastError() != ERROR_PIPE_BUSY) {
			_tprintf(TEXT("Deu erro e nao foi de busy. Erro = %d\n"),GetLastError());
			return 1;
		}


		if (!WaitNamedPipe(INIT_PIPE_MSG_NAME, 10000)) {
			_tprintf(TEXT("Waited 10 seconds and cant find a pipe, I give up...\n"));
			return FALSE;
		}

	}
	
	dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		hPipeTmp,
		&dwMode,
		NULL,
		NULL
	);

	if (!fSuccess) {
		_tprintf(TEXT("SetNamedPipeHandleState falhou. Erro = %d\n"),GetLastError());
		return -1;
	}

	_tprintf(TEXT("initianting pipeConnection thread\n"));
	hTMsgConnection = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)pipeConnection, (LPVOID)hPipeTmp, 0, NULL);
	if (hTMsgConnection == NULL) {
		_tprintf(TEXT("Erro na criãção da thread para remotePipe. Erro = %d\n"), GetLastError());
		return 1;
	}

	Sleep(250);
	_tprintf(TEXT("sending message ...\n"));
	msg tmpMsg;
	tmpMsg.codigoMsg = -9999;
	tmpMsg.from = user_id;
	tmpMsg.to = 254;
	_tcscpy_s(tmpMsg.messageInfo, TAM, TEXT("remoteClient"));
	sendMessage(tmpMsg); // lets server know of new client
	_tprintf(TEXT("message sent sending message ...\n"));
	return 0;
}

DWORD WINAPI pipeConnection(LPVOID param) {
	msg inMsg;
	DWORD resp;
	DWORD bytesRead = 0;
	BOOLEAN fSuccess = FALSE;
	hPipe = ((DWORD)param);
	HANDLE ReadReady;
	OVERLAPPED OverlRd = { 0 };

	ReadReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ReadReady == NULL) {
		_tprintf(TEXT("Erro na criãção do event ReadReady. Erro = %d\n"), GetLastError());
		return;
	}

	while (1) {
		ZeroMemory(&OverlRd, sizeof(OverlRd));
		OverlRd.hEvent = ReadReady;
		ResetEvent(ReadReady);

		fSuccess = ReadFile(
			hPipe,
			&inMsg,
			sizeof(msg),
			&bytesRead,
			&OverlRd
		);

		WaitForSingleObject(ReadReady, INFINITE);//wait for read to be complete
		//_tprintf(TEXT("Read Done...\n"));

		GetOverlappedResult(hPipe, &OverlRd, &bytesRead, FALSE);
		if (bytesRead < sizeof(msg)) {
			_tprintf(TEXT("Read File failed... | Erro = %d\n"), GetLastError());
		}

		//_tprintf(TEXT("I got a message from pipe:\n"));
		//_tprintf(TEXT("Codigo:%d\nto:%d\nfrom:%d\ncontent:%s\n"), inMsg.codigoMsg, inMsg.to, inMsg.from, inMsg.messageInfo);
		//WaitForSingleObject(hStdoutMutex, INFINITE);
		//gotoxy(0, 13);
		//_tprintf(TEXT("MSG->COD(%d)-'%s'"), inMsg.codigoMsg,inMsg.messageInfo);
		//ReleaseMutex(hStdoutMutex);

		resp = resolveMessage(inMsg);
	}
}

BOOLEAN createLocalConnection() {
	TCHAR str[TAM];
	TCHAR tmp[TAM];
	_tcscpy_s(str, TAM, LOCAL_CONNECTION_NAME);
	_itot_s(GetCurrentThreadId(), tmp, TAM, 10);
	_tcscat_s(str, TAM, tmp);
	_tprintf(TEXT("HANDLE = (%s)\n"), str);
	messageEvent = CreateEvent(NULL, FALSE, FALSE, str);
	updateBalls = CreateEvent(NULL, TRUE, FALSE, BALL_EVENT_NAME);
	updateBonus = CreateEvent(NULL, FALSE, FALSE, BONUS_EVENT_NAME);
	hStdoutMutex = CreateMutex(NULL, FALSE, NULL);
	if (messageEvent == NULL || updateBalls == NULL || updateBonus == NULL || hStdoutMutex == NULL) {
		_tprintf(TEXT("Erro ao criar recursos!\n"));
		return 0;
	}
	BOOLEAN res = initializeHandles();
	if (res) {
		_tprintf(TEXT("Erro ao criar Handles!\n"));
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

	hTMsgConnection = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)localConnection, NULL, 0, NULL);
	if (hTMsgConnection == NULL) {
		_tprintf(TEXT("Erro na criãção da thread para local message thread. Erro = %d\n"), GetLastError());
		return 1;
	}

	msg tmpMsg;
	tmpMsg.codigoMsg = -9999;
	tmpMsg.from = user_id;
	tmpMsg.to = 254;
	_tcscpy_s(tmpMsg.messageInfo, TAM, str);
	sendMessage(tmpMsg); // lets server know of new client

	return 0;
}

void LoginUser(TCHAR user[MAX_NAME_LENGTH]) {
	msg newMsg;
	newMsg.from = client_id;
	newMsg.to = 254;//login e sempre para o servidor
	newMsg.codigoMsg = 1;//login
	_tcscpy_s(newMsg.messageInfo, MAX_NAME_LENGTH, user);
	sendMessage(newMsg);
	return 1;
}

void pressKey() {
	_tprintf(TEXT("Press Any Key ...\n"));
	TCHAR tmp;
	fflush(stdin);
	tmp = _gettch();
}

DWORD WINAPI receiveGamePipe(LPVOID param) {

	HANDLE gamePipe;
	BOOLEAN res;
	DWORD bytesRead, dwMode;
	HANDLE ReadReady;
	BOOLEAN fSuccess = FALSE;
	OVERLAPPED OverlRd = { 0 };


	//from here
	while (1) {
		_tprintf(TEXT("Create file for game\n"));
		gamePipe = CreateFile(
			INIT_PIPE_GAME_NAME,
			GENERIC_READ,
			0,
			NULL,
			OPEN_EXISTING,
			0 | FILE_FLAG_OVERLAPPED,
			NULL
		);


		if (gamePipe != INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("I got a valid hPipe\n"));
			break;
		}


		if (GetLastError() != ERROR_PIPE_BUSY) {
			_tprintf(TEXT("Deu erro e nao foi de busy. Erro = %d\n"), GetLastError());
			return;
		}


		if (!WaitNamedPipe(INIT_PIPE_GAME_NAME, 10000)) {
			_tprintf(TEXT("Waited 10 seconds and cant find a pipe, I give up...\n"));
			return;
		}

	}

	gameInfo = (pgame)malloc(sizeof(game));

	ReadReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ReadReady == NULL) {
		_tprintf(TEXT("Erro na criãção do event ReadReady. Erro = %d\n"), GetLastError());
		return;
	}
	//to here

	int quant = 0;

	while (1) {
		//_tprintf(TEXT("Waiting to receive game..\n"));
		ZeroMemory(&OverlRd, sizeof(OverlRd));
		OverlRd.hEvent = ReadReady;
		ResetEvent(ReadReady);

		fSuccess = ReadFile(
			gamePipe,
			gameInfo,
			sizeof(game),
			&bytesRead,
			&OverlRd
		);

		WaitForSingleObject(ReadReady, INFINITE);//sssswait for read to be complete
		//_tprintf(TEXT("Read Done...\n"));

		GetOverlappedResult(hPipe, &OverlRd, &bytesRead, FALSE);
		if (bytesRead < sizeof(game)) {
			_tprintf(TEXT("Read File failed... | Erro = %d\n"), GetLastError());
		}

		SetEvent(gameReady);
		ResetEvent(gameReady);

		//WaitForSingleObject(hStdoutMutex,INFINITE);
		//gotoxy(0, 10);
		//_tprintf(TEXT("GameStatus = %d\n"),gameInfo->gameStatus);
		//_tprintf(TEXT("[%d]Udated\n->Bricks:%d\n->Player_0:(%d,%d)"),quant++,gameInfo->numBricks,gameInfo->nUsers[0].posx, gameInfo->nUsers[0].posy);
		//ReleaseMutex(hStdoutMutex);
		
	}

	CloseHandle(gamePipe);
	Sleep(200);
	return 0;
}