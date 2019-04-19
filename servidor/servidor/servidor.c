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
int startVars();
void resetUser(DWORD id);
int registry(user userData);
int moveUser(DWORD id, TCHAR side[TAM]);
void createBrick(DWORD num);
void hitBrick(DWORD brick_id, DWORD ball_id);

HANDLE moveBalls;
HANDLE hTBola[MAX_BALLS];
HANDLE hTBrick[MAX_BRICKS];
HANDLE messageEvent;

BOOLEAN continua = 1;

DWORD server_id;

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
	messageEvent = CreateEvent(NULL, FALSE, FALSE, MESSAGE_EVENT_NAME);
	gameEvent = CreateEvent(NULL,TRUE,FALSE, GAME_EVENT_NNAME);
	updateBalls = CreateEvent(NULL, TRUE, FALSE, BALL_EVENT_NAME);
	canMove = CreateEvent(NULL, FALSE, FALSE, USER_MOVE_EVENT_NAME);
	createHandles();
	//Game
	gameInfo = createSharedMemoryGame();

	//Message
	createSharedMemoryMsg();

	BOOLEAN res = startVars();
	if (res) {
		_tprintf(TEXT("Erro ao iniciar as variaveis!"));
	}
	
	hTReceiveMessage = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveMessageThread, NULL, 0, &threadId);
	if (hTReceiveMessage == NULL) {
		_tprintf(TEXT("Erro ao criar thread ReceiveMessage!"));
		return -1;
	}

	msg tmpMsg;
	user tmp_user;
	tmp_user.score = -1;
	_tcscpy_s(tmp_user.name, MAX_NAME_LENGTH, TEXT("just checking"));
	registry(tmp_user); //Creates/Checks TOP 10
	do{
		_tprintf(TEXT("--Welcome to Server--\n"));
		_tprintf(TEXT("1:Create Game\n"));
		_tprintf(TEXT("2:Start Game\n"));
		_tprintf(TEXT("3:Users Logged\n"));
		_tprintf(TEXT("4:Top 10\n"));
		_tprintf(TEXT("5:Exit\n"));
		
		fflush(stdin);
		KeyPress = _gettch();
		_puttchar(KeyPress);
		system("cls");

		switch (KeyPress) {
		case '1':
			gameInfo->gameStatus = 0;
			tmpMsg.codigoMsg = 100;//new game
			tmpMsg.from = server_id;
			tmpMsg.to = 255; //broadcast
			_tcscpy_s(tmpMsg.messageInfo, TAM, TEXT("new game created"));
			sendMessage(tmpMsg);				
			break;
		case '2':
			if (gameInfo->gameStatus == -1) {
				_tprintf(TEXT("Game isnt created yet!\n"));
			}
			else if (gameInfo->numUsers >= 0) {
				_tprintf(TEXT("Game is starting!\n"));
				startGame();
				gameInfo->gameStatus = -1; //after game ends
				system("cls");				
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
			registry(tmp_user);
			break;
		}
	} while (KeyPress != '5');

	continua = 0;
	SetEvent(messageEvent);
	//WaitForSingleObject(hTBola, INFINITE);
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
		//_tprintf(TEXT("waiting for msg...\n"));
		WaitForSingleObject(messageEvent,INFINITE);
		newMsg = receiveMessage();
		quant++;
		_tprintf(TEXT("[%d]NewMsg(%d):\%s\n-from:%d\n-to:%d\n"), quant, newMsg.codigoMsg, newMsg.messageInfo, newMsg.from, newMsg.to);
		if (newMsg.codigoMsg == 1 && gameInfo->numUsers < MAX_USERS) {//login de utilizador
			_tprintf(TEXT("Login do Utilizador (%s)\n"), newMsg.messageInfo);
			for (int i = 0; i < gameInfo->numUsers; i++) {
				if (_tcscmp(gameInfo->nUsers[i].name, newMsg.messageInfo) == 0) {
					flag = 0;
					break;
				}
			}
			if (flag) {
				_tcscpy_s(gameInfo->nUsers[gameInfo->numUsers].name, MAX_NAME_LENGTH, newMsg.messageInfo);
				gameInfo->nUsers[gameInfo->numUsers].user_id = newMsg.from;
				//_tprintf(TEXT("\nAdicionei (%s) na pos %d\n\n"),usersLogged->names[usersLogged->tam], usersLogged->tam);
				newMsg.codigoMsg = 1;//sucesso
				newMsg.number = gameInfo->numUsers++;;
				newMsg.to = 255;//broadcast
				newMsg.from = 254;
				sendMessage(newMsg);
			}
		}
		else if (newMsg.codigoMsg == 2) {//end of user
			int res = registry(gameInfo->nUsers[newMsg.from]);
			if (res) {
				_tprintf(TEXT("new high score saved!\n"));
			}
			else {
				_tprintf(TEXT("not enough for top 10!\n"));
			}
			SetEvent(gameEvent);//releases user that is waiting for input?
			ResetEvent(gameEvent);
			Sleep(250);
			resetUser(newMsg.from);
			gameInfo->numUsers--;
			newMsg.to = newMsg.from;
			newMsg.from = server_id;
			newMsg.codigoMsg = 2;
			sendMessage(newMsg);
		}
		else if (newMsg.codigoMsg == 200) {//user trying to move
			moveUser(newMsg.from, newMsg.messageInfo);
			SetEvent(canMove);
			//_tprintf(TEXT("user_pos(%d,%d)\n"), gameInfo->nUsers[0].posx, gameInfo->nUsers[0].posy);
			//_tprintf(TEXT("(moveUser)User[%d]:%s\n"), newMsg.user_id, newMsg.messageInfo);
		}
		else if (newMsg.codigoMsg == 123) {//for tests || returns the exact same msg
			newMsg.to = newMsg.from;
			newMsg.from = 254;
			sendMessage(newMsg);
		}

	} while (continua);
}

int startGame() {
	HANDLE hTUser[MAX_USERS];
	DWORD threadId;
	int i;

	//moveBalls = CreateSemaphore(NULL, gameInfo->numBalls, gameInfo->numBalls, NULL);//semafore que dispara quando tem a posicao de todas as bolas.
	//for (i = 0; i < gameInfo->numBalls; i++)//preenche semaforo
	//	WaitForSingleObject(moveBalls, INFINITE);

	_tprintf(TEXT("Game started!\n"));

	for (i = 0; i < gameInfo->numUsers; i++) {
		hTUser[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)userThread, NULL, 0, &threadId);
		if (hTUser[i] == NULL) {
			_tprintf(TEXT("Erro ao criar user numero:%d!\n"), i);
			return -1;
		}
		else {
			_tprintf(TEXT("Created User number:%d!\n"), i);
		}
	}
	
	SetEvent(gameEvent);
	ResetEvent(gameEvent);//lets users know that the game started

	WaitForMultipleObjects(gameInfo->numUsers, hTUser, TRUE, INFINITE);
	_tprintf(TEXT("out of start game!\n"));
}

DWORD WINAPI userThread(LPVOID param) {
	DWORD id = ((DWORD)param);
	user userData;
	userData = gameInfo->nUsers[id];
	//_tprintf(TEXT("(THREAD)User:%s-id[%d]\n"),userData.name,id);
	gameInfo->nUsers[id].posx = gameInfo->myconfig.limx / 2;
	gameInfo->nUsers[id].posy = gameInfo->myconfig.limy - 2;
	gameInfo->nUsers[id].lifes = 3;
	
	//_tprintf(TEXT("Waiting for a user to let us know that we can start\n"));
	Sleep(250);

	//build bricks here
	createBrick(1);

	while (gameInfo->nUsers[id].lifes > 0){
		//_tprintf(TEXT("User[%d] waiting for input %d lifes\n"), id, gameInfo->nUsers[id].lifes);
		WaitForSingleObject(gameEvent, INFINITE);
		if(gameInfo->nUsers[id].lifes > 0 && gameInfo->numBalls == 0)
			createBalls(1);
	}
	_tprintf(TEXT("fim de jogo para o User. Score final de: %d\n"), gameInfo->nUsers[id].score);
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
	int i;
	for (i = 0; i < MAX_BALLS; i++) {
		if (hTBola[i] == NULL) {//se handle is available
			hTBola[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BolaThread, (LPVOID)i, 0, &threadId);
			if (hTBola[i] == NULL) {
				_tprintf(TEXT("Erro ao criar bola numero:%d!\n"), i);
				return -1;
			}
			else {
				_tprintf(TEXT("Created Ball number:%d!\n"), i);
				//hTBola[i] = UNDEFINE_ALTERNATE;
				gameInfo->numBalls++;
				count++;
			}
		}

		if (count >= num )
			break;
	}

	msg tmpMsg;
	tmpMsg.codigoMsg = 101;
	tmpMsg.from = server_id;
	tmpMsg.to = 255;
	tmpMsg.number = num;
	_tcscpy_s(tmpMsg.messageInfo, TAM, TEXT("ball"));
	sendMessage(tmpMsg);

	return;
}

DWORD WINAPI BolaThread(LPVOID param) {
	DWORD id = ((DWORD)param);
	srand((int)time(NULL));
	DWORD posx = gameInfo->myconfig.limx/2, posy = 7, oposx, oposy,num=0,ballScore = 0;
	boolean goingUp = 1, goingRight = (rand() % 2), flag;
	goingRight = 1;
	gameInfo->nBalls[id].status = 1;
	do{
		ballScore = GetTickCount();
		flag = 0;
		Sleep(gameInfo->nBalls[id].speed);
		//checks for bricks
		for (int i = 0; i < gameInfo->numBricks; i++) {
			_tprintf(TEXT("Ball(%d,%d) | Brick(%d,%d)\n"),posx,posy,gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy);
			//up
			if (goingUp && posy - 1 == gameInfo->nBricks[i].posy) {

				//straight up
				if (gameInfo->nBricks[i].posx - 1 < posx && (gameInfo->nBricks[i].posx + gameInfo->nBricks[i].tam) > posx) {
					_tprintf(TEXT("Hit up\n"), posy - 1, gameInfo->nBricks[i].posy);
					goingUp = 0;
					hitBrick(i, id);
				}
				//up-left
				else if (!goingRight && posx  == (gameInfo->nBricks[i].posx + gameInfo->nBricks[i].tam)) {
					_tprintf(TEXT("Hit up-left\n"));
					goingUp = 0;
					goingRight = 1;
					hitBrick(i, id);
				}
				//up-right
				else if(goingRight && posx+1 == gameInfo->nBricks[i].posx){
					_tprintf(TEXT("Hit up-right\n"));
					goingUp = 0;
					goingRight = 0;
					hitBrick(i, id);
				}
				
				
			}
			else if(!goingUp && posy + 1 == gameInfo->nBricks[i].posy){
				//down
				if (gameInfo->nBricks[i].posx < posx + 1 && (gameInfo->nBricks[i].posx + gameInfo->nBricks[i].tam) > posx) {
					_tprintf(TEXT("Hit down\n"), posy + 1, gameInfo->nBricks[i].posy);
					goingUp = 1;
					hitBrick(i, id);
				}
				//down-left
				else if (!goingRight && posx == (gameInfo->nBricks[i].posx+gameInfo->nBricks[i].tam)) {
					_tprintf(TEXT("Hit down-left\n"));
					goingUp = 1;
					goingRight = 1;
					hitBrick(i, id);
				}
				//down-right
				else if (goingRight && posx + 1 == gameInfo->nBricks[i].posx) {
					_tprintf(TEXT("Hit down-right\n"));
					goingUp = 1;
					goingRight = 0;
					hitBrick(i, id);
				}

				
				
			}
			
			//left
			if (!goingRight && posy == gameInfo->nBricks[i].posy && posx == (gameInfo->nBricks[i].posx + gameInfo->nBricks[i].tam)) {
				_tprintf(TEXT("Hit left\n"));
				goingRight = 1;
				hitBrick(i, id);
			
			}
			//right
			else if (goingRight && posy == gameInfo->nBricks[i].posy && posx + 1 == gameInfo->nBricks[i].posx) {
				_tprintf(TEXT("Hit right\n"));
				goingRight = 0;
				hitBrick(i, id);
			}
			
			
		}

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

				for (int i = 0; i < gameInfo->numUsers; i++){//checks for player
					//_tprintf(TEXT("BALL(%d,%d)\nUSER(%d-%d,%d)\n\n"),posx,posy, gameInfo->nUsers[i].posx, gameInfo->nUsers[i].posx + gameInfo->nUsers[i].size, gameInfo->nUsers[i].posy);
					if (posy == gameInfo->nUsers[i].posy - 1 && (posx >= gameInfo->nUsers[i].posx && posx <= (gameInfo->nUsers[i].posx + gameInfo->nUsers[i].size))) {//atinge a barreira
						flag = 1;
						_tprintf(TEXT("HIT player!!\n"));	
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
				gameInfo->nBalls[id].status = 0;//end of ball
				gameInfo->nBalls[id].posx = 0;
				gameInfo->nBalls[id].posy = 0;
				gameInfo->numBalls--;
			}
		}

		gameInfo->nBalls[id].posx = posx;
		gameInfo->nBalls[id].posy = posy;

		if (gameInfo->nBalls[id].status != 0)//if is to end
			gameInfo->nBalls[id].status = 1;		

		//ReleaseSemaphore(moveBalls, 1, &num);
		//_tprintf(TEXT("ball[%d];num=%d!\n"),id,num);
		//_tprintf(TEXT("Ball[%d]!\n"),id);
		//if (num == gameInfo->numBalls - 1) {
			//_tprintf(TEXT("Balls moved(%d,%d)\n"),posx,posy);
			SetEvent(updateBalls);
			ResetEvent(updateBalls);

		
		for (int i = 0; i < gameInfo->numUsers; i++) {
			gameInfo->nUsers[i].score += (GetTickCount() - ballScore) / 100;
		}
		//}
	} while (gameInfo->nBalls[id].status);
	
	if(gameInfo->numBalls == 0)
		for (int i = 0; i < gameInfo->numUsers; i++) {
			gameInfo->nUsers[i].lifes--;
		}
			
	hTBola[id] = NULL;
	_tprintf(TEXT("End of Ball %d!\n"), id);

	/*if (gameInfo->numBalls == 0) {
		if (gameInfo->nUsers[0].lifes) {
			gameInfo->nUsers[0].lifes--;
			createBalls(1);
		}	
	}*/

	return 0;
}

void createBrick(DWORD num) {
	//create num brick
	DWORD count = 0;
	DWORD threadId;
	int i;
	for (i = 0; i < MAX_BRICKS; i++) {
		if (i == 0) {
			gameInfo->nBricks[i].posx = 72;
			gameInfo->nBricks[i].posy = 3;
			gameInfo->nBricks[i].tam = 3;
			gameInfo->nBricks[i].status = 1;
		}
		else if (i == 1) {
			gameInfo->nBricks[i].posx = gameInfo->myconfig.limx - 20;
			gameInfo->nBricks[i].posy = 3;
			gameInfo->nBricks[i].tam = 15;
			gameInfo->nBricks[i].status = 2;
		}
		else if (i == 2) {
			gameInfo->nBricks[i].posx = 80;
			gameInfo->nBricks[i].posy = 3;
			gameInfo->nBricks[i].tam = 10;
			gameInfo->nBricks[i].status = 3;
		}
		else if (i == 3) {
			gameInfo->nBricks[i].posx = 5;
			gameInfo->nBricks[i].posy = 10;
			gameInfo->nBricks[i].tam = 50;
			gameInfo->nBricks[i].status = 9;
		}
		else if (i == 4) {
			gameInfo->nBricks[i].posx = gameInfo->myconfig.limx - 55;
			gameInfo->nBricks[i].posy = 10;
			gameInfo->nBricks[i].tam = 50;
			gameInfo->nBricks[i].status = 9;
		}
		gameInfo->numBricks++;
		count++;
		if (count >= num)
			break;
	}


	_tprintf(TEXT("created %d bricks!\n"), count);
	msg tmpMsg;
	tmpMsg.codigoMsg = 102;
	tmpMsg.from = server_id;
	tmpMsg.to = 255;
	tmpMsg.number = num;
	_tcscpy_s(tmpMsg.messageInfo, TAM, TEXT("brick"));
	sendMessage(tmpMsg);
	return;
}

void hitBrick(DWORD brick_id,DWORD ball_id) {
	gameInfo->nBricks[brick_id].status--;
	if(gameInfo->nBalls[ball_id].speed >= 100)
		gameInfo->nBalls[ball_id].speed -= 50;
	for (int i = 0; i < gameInfo->numUsers; i++)
		gameInfo->nUsers[i].score += 100;

}


int startVars() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	int i;
	//Status
	gameInfo->gameStatus = -1;
	//sessionId = (GetCurrentThreadId() + GetTickCount());
	server_id = 254;
	//Config
	gameInfo->myconfig.limx = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	gameInfo->myconfig.limy = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

	_tprintf(TEXT("Lim(%d,%d)\n"), gameInfo->myconfig.limx, gameInfo->myconfig.limy);

	//Users
	gameInfo->numUsers = 0;
	for (i = 0; i < MAX_USERS; i++) {
		resetUser(i);
	}
	//Balls
	gameInfo->numBalls = 0;
	for (i = 0; i < MAX_BALLS; i++) {
		gameInfo->nBalls[i].posx = 0;
		gameInfo->nBalls[i].posy = 0;
		gameInfo->nBalls[i].status = 0;
		gameInfo->nBalls[i].speed = 1500;
		hTBola[i] = NULL;
	}
	//Brick
	gameInfo->numBricks = 0;
	for (i = 0; i < MAX_BRICKS; i++) {
		gameInfo->nBricks[i].posx = 0;
		gameInfo->nBricks[i].posy = 0;
		gameInfo->nBricks[i].status = 0;
		gameInfo->nBricks[i].tam = 0;
		hTBrick[i] = NULL;
	}

	return 0;
}

void resetUser(DWORD id) {
	//_tprintf(TEXT("Reseting user %d\n"), id);
	_stprintf_s(gameInfo->nUsers[id].name, MAX_NAME_LENGTH, TEXT("empty"));
	gameInfo->nUsers[id].user_id = -1;
	gameInfo->nUsers[id].lifes = 3;
	gameInfo->nUsers[id].score = 0;
	gameInfo->nUsers[id].size = 10;
	gameInfo->nUsers[id].posx = 0;
	gameInfo->nUsers[id].posy = 0;
}

int registry(user userData) {
	//_tprintf(TEXT("Saving user %s with the score of %d\n"), userData.name, userData.score);
	HKEY chave;
	DWORD regOutput, regVersion, regSize;
	TCHAR info[TAM];
	TCHAR name[TAM];
	TCHAR tmp[TAM];
	TCHAR user_name[TAM];
	TCHAR tmp_2[TAM];
	DWORD score = 0, tam = MAX_NAME_LENGTH, user_score;
	int flag, flag_2;
	BOOLEAN value = 0;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\SO2_TP"), 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &chave, &regOutput) != ERROR_SUCCESS) {
		_tprintf(TEXT("Erro ao criar/abrir chave (%d)\n"), GetLastError());
		return -1;
	}
 
	if (regOutput == REG_CREATED_NEW_KEY) {//if there is no scores saved
		//_tprintf(TEXT("Chave: (HKEY_CURRENT_USER\\Software\\SO2_TP) - Criada\n"));
		for (int i = 0; i < 10; i++) {
			_tcscpy_s(name, TAM, TEXT("HighScore"));
			_itot_s(i, info, TAM, 10);
			_tcscat_s(name, TAM, info);
			_stprintf_s(info, MAX_NAME_LENGTH, TEXT("none:"));
			_tcscat_s(info, TAM, TEXT("0"));
			RegSetValueEx(chave, name, 0, REG_SZ, (LPBYTE)("%s", info), _tcslen(("%s", info)) * sizeof(TCHAR));
		}
		
		_tprintf(TEXT("TOP 10 Created\n"));
		return 1;
	}
	else if (regOutput == REG_OPENED_EXISTING_KEY) {//if there are scores
		//_tprintf(TEXT("Chave: (HKEY_CURRENT_USER\\Software\\SO2_TP) - Aberta\n"));
		for (int i = 0; i < 10; i++) {
			tam = MAX_NAME_LENGTH;
			flag_2 = 0;
			_tcscpy_s(name, TAM, TEXT("HighScore"));
			_itot_s(i, info, TAM, 10);
			_tcscat_s(name, TAM, info);
			RegQueryValueEx(chave,name, NULL, NULL, (LPBYTE)info, &tam);
			_tprintf(TEXT("%s - %s\n"),name,info);
			if (userData.score == -1)
				continue;
			for(flag = 0;flag<MAX_NAME_LENGTH;flag++){
			//while(info[flag]!=':'){//copy name
				if (info[flag] == ':')
					break;
				user_name[flag] = info[flag];
			}
			user_name[flag] = '\0';//end of user_name
			flag++;
			for (; flag < MAX_NAME_LENGTH; flag++) {
			//while (info[flag] != '\0') {//copy score
				if (info[flag] == '\0')
					break;
				tmp[flag_2] = info[flag];
				flag_2++;
			}
			tmp[flag_2] = '\0';//end of score
			score = _tstoi(tmp);//tranlate
			//_tprintf(TEXT("TOP[%d]%s:%d\n"),i+1, user_name, score);
			//_tprintf(TEXT("NOVO:%dvs%d:OLD\n"),userData.score,score);
			if (userData.score > score) {
				value = 1;
				_tcscpy_s(tmp, TAM, userData.name);
				_tcscat_s(tmp, TAM, TEXT(":"));
				_itot_s(userData.score, tmp_2, TAM, 10);
				_tcscat_s(tmp, TAM, tmp_2);
				RegSetValueEx(chave, name, 0, REG_SZ, (LPBYTE)("%s", tmp), _tcslen(("%s", tmp)) * sizeof(TCHAR));
				userData.score = score;
				_tcscpy_s(userData.name, MAX_NAME_LENGTH ,user_name);	
			}
		
		}
		//_tprintf(TEXT("Valores Lidos\n"));
	}
	RegCloseKey(chave);
	return value;
}