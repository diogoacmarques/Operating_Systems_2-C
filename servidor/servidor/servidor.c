#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "DLL.h"

DWORD WINAPI BolaThread(LPVOID param);
DWORD WINAPI userThread(LPVOID param);
DWORD WINAPI receiveMessageThread();
int startGame();
void createBalls(DWORD num);
int regestry_user();
int startVars(pgame gameData);
int moveUser(DWORD id, TCHAR side[TAM]);

HANDLE receiveMessageEvent,moveBalls;
HANDLE hTBola[MAX_BALLS];

BOOLEAN continua = 1;

pgame gameUpdate;
pgame gameInfo;

int _tmain(int argc, LPTSTR argv[]) {
	//DLL_tprintf(TEXT("Resultado da função da UmaString DLL: %d"), UmaString());
	DWORD threadId;
	HANDLE hTGame,hTReceiveMessage;
	TCHAR KeyPress;
	//UNICODE: Por defeito, a consola Windows não processe caracteres wide.
	//A maneira mais fácil para ter esta funcionalidade é chamar _setmode:
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	receiveMessageEvent = CreateEvent(NULL, FALSE, FALSE, SENDMESSAGEEVENT);
	gameEvent = CreateEvent(NULL,TRUE,FALSE, GAME_EVENT_NNAME);
	updateBalls = CreateEvent(NULL, TRUE, FALSE, BALL_EVENT_NAME);
	canMove = CreateEvent(NULL, FALSE, FALSE, USER_EVENT_NAME);

	//Game
	gameInfo = createSharedMemoryGame();

	//Message
	createSharedMemoryMsg();

	BOOLEAN res = startVars(gameInfo);
	if (res) {
		_tprintf(TEXT("Erro ao iniciar as variaveis!"));
	}
	
	hTReceiveMessage = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveMessageThread, NULL, 0, &threadId);
	if (hTReceiveMessage == NULL) {
		_tprintf(TEXT("Erro ao criar thread ReceiveMessage!"));
		return -1;
	}

	do{
		_tprintf(TEXT("--Welcome to Server--\n"));
		_tprintf(TEXT("1:Create Game\n"));
		_tprintf(TEXT("2:Start Game\n"));
		_tprintf(TEXT("3:Users Logged in\n"));
		_tprintf(TEXT("4:top 10\n"));
		_tprintf(TEXT("5:Exit\n"));
		
		fflush(stdin);
		KeyPress = _gettch();
		_puttchar(KeyPress);
		system("cls");

		switch (KeyPress) {
		case '1':
			gameInfo->gameStatus = 0;
			break;
		case '2':
			if (gameInfo->gameStatus == -1) {
				_tprintf(TEXT("Game isnt created yet!\n"));
			}
			else if (gameInfo->numUsers >= 0) {
				_tprintf(TEXT("Game is starting!\n"));
				startGame();
			}
			else {
				_tprintf(TEXT("No users playing have joined in.\n"));
			}
			break;
		case '3':
			_tprintf(TEXT("Users(%d)!\n"), gameInfo->numUsers);
			for (int i = 0; i < gameInfo->numUsers; i++)
				_tprintf(TEXT("User[%d]=(%s)\n"), i, gameInfo->nUsers[i].name);
			break;
		case '4':
			_tprintf(TEXT("Still in the works\n"));
			break;
		}
	} while (KeyPress != '5');

	continua = 0;
	//WaitForSingleObject(hTBola, INFINITE);
	SetEvent(receiveMessageEvent);
	WaitForSingleObject(hTReceiveMessage, INFINITE);
	closeSharedMemoryMsg();
	closeSharedMemoryGame();
	_tprintf(TEXT("[Thread Principal %d] Vou terminar..."), GetCurrentThreadId());
	return 0;
}

DWORD WINAPI receiveMessageThread() {
	gameInfo->numUsers = 0;
	msg newMsg;
	boolean flag;	
	int quant = 0;
	
	do{
		flag = 1;
		WaitForSingleObject(receiveMessageEvent,INFINITE);
		newMsg = receiveMessage();
		quant++;
		_tprintf(TEXT("[%d]NewMsg(%d):%s\n"),quant, newMsg.codigoMsg, newMsg.messageInfo);
		if (newMsg.codigoMsg==1 && gameInfo->numUsers < MAX_USERS) {//login de utilizador
			_tprintf(TEXT("Login do Utilizador (%s)\n"), newMsg.messageInfo);
			for (int i = 0; i < gameInfo->numUsers;i++) {
				if (_tcscmp(gameInfo->nUsers[i].name, newMsg.messageInfo) == 0) {
					flag = 0;
					break;
				}		
			}
			if (flag) {
				_tcscpy_s(gameInfo->nUsers[gameInfo->numUsers].name,MAX_NAME_LENGTH,newMsg.messageInfo);
				gameInfo->nUsers[gameInfo->numUsers].user_id = gameInfo->numUsers;
				//_tprintf(TEXT("\nAdicionei (%s) na pos %d\n\n"),usersLogged->names[usersLogged->tam], usersLogged->tam);
				gameInfo->numUsers++;
			}
		} else if(newMsg.codigoMsg == 200){//user trying to move
			moveUser(newMsg.user_id,newMsg.messageInfo);
			SetEvent(canMove);

			//_tprintf(TEXT("user_pos(%d,%d)\n"), gameInfo->nUsers[0].posx, gameInfo->nUsers[0].posy);
			//_tprintf(TEXT("(moveUser)User[%d]:%s\n"), newMsg.user_id, newMsg.messageInfo);
		}

	} while (continua);
}

int startGame() {
	HANDLE hTUser[MAX_USERS];
	DWORD threadId;
	int i;

	moveBalls = CreateSemaphore(NULL, gameInfo->numBalls, gameInfo->numBalls, NULL);//semafore que dispara quando tem a posicao de todas as bolas.
	for (i = 0; i < gameInfo->numBalls; i++)//preenche semaforo
		WaitForSingleObject(moveBalls, INFINITE);

	_tprintf(TEXT("Game started!\n"));
	_tprintf(TEXT("[Server] Number of balls = %d!\n"), gameInfo->numBalls);

	for (i = 0; i < gameInfo->numUsers; i++) {
		hTUser[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)userThread, NULL, 0, &threadId);
		Sleep(1000);
		if (hTUser[i] == NULL) {
			_tprintf(TEXT("Erro ao criar user numero:%d!\n"), i);
			return -1;
		}
		else {
			_tprintf(TEXT("Created User number:%d!\n"), i);
		}
	}


	Sleep(3000);

	createBalls(1);
	SetEvent(gameEvent);
	ResetEvent(gameEvent);
	WaitForMultipleObjects(gameInfo->numBalls, hTBola, TRUE, INFINITE);
}

DWORD WINAPI userThread(LPVOID param) {
	DWORD id = ((DWORD)param);
	user userData;
	userData = gameInfo->nUsers[id];
	_tprintf(TEXT("(THREAD)User:%s-id[%d]\n"),userData.name,id);
	gameInfo->nUsers[id].posx = gameInfo->myconfig.limx / 2;
	gameInfo->nUsers[id].posy = gameInfo->myconfig.limy - 2;
	gameInfo->nUsers[id].lifes = 3;
	Sleep(5000);
	return 0;
}

int moveUser(DWORD id, TCHAR side[TAM]) {
	//_tprintf(TEXT("(moveUser)User[%d]:%s\n"),id,side);
	if (_tcscmp(side, TEXT("right")) == 0) {
		if (gameInfo->nUsers[id].posx + gameInfo->nUsers[id].size >= gameInfo->myconfig.limx)
			return;
		gameInfo->nUsers[id].posx++;
	}
	else if (_tcscmp(side, TEXT("left")) == 0) {
		if (gameInfo->nUsers[id].posx < 1)
			return;
		gameInfo->nUsers[id].posx--;
	}
	return;
}

void createBalls(DWORD num){
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
				_tprintf(TEXT("Created Ball number:%d!\n"), i);
				gameInfo->numBalls++;
				count++;
			}
		}

		if (num >= count)
			break;
	}

	return;
}

DWORD WINAPI BolaThread(LPVOID param) {
	DWORD id = ((DWORD)param);
	srand((int)time(NULL));
	DWORD posx = gameInfo->myconfig.limx/2, posy = gameInfo->myconfig.limy / 2, oposx, oposy,num=0;
	boolean goingUp = 1, goingRight = (rand() % 2), flag;
	goingRight = 0;
	do{
		flag = 0;
		Sleep(250);
		//Sleep(1000);
		oposx = posx;
		oposy = posy;
		if (goingRight) {
			if (posx < gameInfo->myconfig.limx - 1) {
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
			if (posy < gameInfo->myconfig.limy - 1) {// se nao atinge o limite do mapa

				for (int i = 0; i < gameInfo->numUsers; i++) {
					//_tprintf(TEXT("BALL(%d,%d)\nUSER(%d-%d,%d)\n\n"),posx,posy, gameInfo->nUsers[i].posx, gameInfo->nUsers[i].posx + gameInfo->nUsers[i].size, gameInfo->nUsers[i].posy);
					if (posy == gameInfo->nUsers[i].posy - 1 && (posx >= gameInfo->nUsers[i].posx && posx <= (gameInfo->nUsers[i].posx + gameInfo->nUsers[i].size))) {//atinge a barreira
						flag = 1;
						_tprintf(TEXT("HIT!!\n"), id);	
						break;
					}
				}

				if (flag) {
					goingUp = 1;
					posy--;
				}
				else { posy++; }
				
			}
			else {//se atinge o fim do mapa
				//reset ball
				gameInfo->nBalls[id].status = -1;//end of ball
				gameInfo->nBalls[id].posx = 0;
				gameInfo->nBalls[id].posy = 0;
				gameInfo->numBalls--;
				_tprintf(TEXT("End of Ball %d!\n"),id);
			}
		}
		
		gameInfo->nBalls[id].posx = posx;
		gameInfo->nBalls[id].posy = posy;

		if (gameInfo->nBalls[id].status == -1)//if is to end
			gameInfo->nBalls[id].status = 0;
		else
			gameInfo->nBalls[id].status = 1;

		//ReleaseSemaphore(moveBalls, 1, &num);
		//_tprintf(TEXT("ball[%d];num=%d!\n"),id,num);
		//_tprintf(TEXT("Ball[%d]!\n"),id);
		//if (num == gameInfo->numBalls - 1) {
			//_tprintf(TEXT("Balls moved(%d,%d)\n"),posx,posy);
			SetEvent(updateBalls);
			ResetEvent(updateBalls);
		//}
	} while (gameInfo->nBalls[id].status);
	
	CloseHandle(hTBola[id]);
	hTBola[id] = NULL;

	if (gameInfo->numBalls == 0) {
		if (gameInfo->nUsers[0].lifes) {
			gameInfo->nUsers[0].lifes--;
			createBalls(1);
		}
			
	}
	return 0;
}



int startVars(pgame gameData) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	int i;
	//Config
	gameData->myconfig.limx = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	gameData->myconfig.limy = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	//Users
	gameData->numUsers = 0;
	for (i = 0; i < MAX_USERS; i++) {
		_stprintf_s(gameInfo->nUsers[i].name, MAX_NAME_LENGTH, TEXT("empty"));
		gameData->nUsers[i].user_id = -1;
		gameData->nUsers[i].lifes = 3;
		gameData->nUsers[i].score = 0;
		gameData->nUsers[i].size = 30;
		gameData->nUsers[i].posx = 0;
		gameData->nUsers[i].posy = 0;
	}
	//Balls
	gameData->numBalls = 0;
	for (i = 0; i < MAX_BALLS; i++) {
		gameData->nBalls[i].posx = 0;
		gameData->nBalls[i].posy = 0;
		gameData->nBalls[i].status = 0;
		hTBola[i] = NULL;
	}

	gameData->gameStatus = -1;

	return 0;
}

int regestry_user() {

	//HKEY chave;
	//DWORD regOutput, regVersion, regSize;

	//if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\SO2_TP"), 0, NULL,
	//	REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &chave, &regOutput) != ERROR_SUCCESS) {
	//	_tprintf(TEXT("Erro ao criar/abrir chave (%d)\n"), GetLastError());
	//	return -1;
	//}

	//if (regOutput == REG_CREATED_NEW_KEY) {
	//	_tprintf(TEXT("Chave: (HKEY_CURRENT_USER\\Software\\SO2_TP) - Criada\n"));
	//	regVersion = 1;
	//	DWORD score;
	//	RegSetValueEx(chave, TEXT("Versao"), 0, REG_DWORD, (LPBYTE)&regVersion, sizeof(DWORD));
	//	for (int i = 0; i < usersLogged->tam; i++)
	//		if (_tcscmp(usersLogged->names[i], TEXT("empty") != 0)) {
	//			score = 1000 + i;
	//			RegSetValueEx(chave, usersLogged->names[i], 0, REG_DWORD, (LPBYTE)&score, sizeof(DWORD));
	//		}

	//	_tprintf(TEXT("Valores Guardados\n"));
	//}
	//else if (regOutput == REG_OPENED_EXISTING_KEY) {
	//	tamanho = 20;
	//	RegQueryValueEx(chave, TEXT("Autor"), NULL, NULL, (LPBYTE)autor,&tamanho);
	//	autor[tamanho / sizeof(TCHAR)] = '\0';
	//	tamanho = sizeof(versao);
	//	RegQueryValueEx(chave, TEXT("Versao"), NULL, NULL, (LPBYTE)&versao,	&tamanho);
	//	versao++;
	//	RegSetValueEx(chave, TEXT("Versao"), 0, REG_DWORD, (LPBYTE)&versao,	sizeof(DWORD));
	//	_stprintf_s(str, TAM, TEXT("Autor:%s Versão:%d\n"), autor, versao);
	//	_tprintf(TEXT("Lido do Registry:%s\n"), str);

	//}
	//		RegCloseKey(chave)
}