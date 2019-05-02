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
void createGame();
int startGame();
void createBalls(DWORD num);
int startVars(TCHAR file[TAM]);
BOOLEAN insertSetting(TCHAR setting[TAM], DWORD value);
void resetUser(DWORD id);
void resetBrick(DWORD id);
void resetBall(DWORD id);
int registry(user userData);
int moveUser(DWORD id, TCHAR side[TAM]);
void createBrick(DWORD num);
void hitBrick(DWORD brick_id, DWORD ball_id);
void checkSettings();

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
	//TCHAR file[TAM];
	//_tcscpy_s(name, TAM, TEXT("HighScore"));
	BOOLEAN res = startVars(TEXT("config.txt"));
	if (res) {
		_tprintf(TEXT("Erro ao iniciar as variaveis!"));
	}
	
	hTReceiveMessage = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveMessageThread, NULL, 0, &threadId);
	if (hTReceiveMessage == NULL) {
		_tprintf(TEXT("Erro ao criar thread ReceiveMessage!"));
		return -1;
	}

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
		_tprintf(TEXT("5:Check Settings\n"));
		_tprintf(TEXT("6:Exit\n"));
		
		fflush(stdin);
		KeyPress = _gettch();
		_puttchar(KeyPress);
		//system("cls");

		switch (KeyPress) {
		case '1':
			createGame();	
			break;
		case '2':
			if (gameInfo->gameStatus != 0) {
				_tprintf(TEXT("Game isnt created yet!\n"));
			}
			else if (gameInfo->numUsers > 0) {
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
		case '5':
			checkSettings();
		}
		
	} while (KeyPress != '6');

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
		//_tprintf(TEXT("[%d]NewMsg(%d):\%s\n-from:%d\n-to:%d\n"), quant, newMsg.codigoMsg, newMsg.messageInfo, newMsg.from, newMsg.to);
		if (newMsg.codigoMsg == 1 && gameInfo->numUsers < MAX_USERS) {//login de utilizador
			_tprintf(TEXT("Login do Utilizador (%s)\n"), newMsg.messageInfo);
			if (gameInfo->gameStatus != 0) {
				//_tprintf(TEXT("User(%s) tried to login with no game created\n"), newMsg.messageInfo);
				newMsg.codigoMsg = -100;//no game created
				newMsg.number = -100;
				newMsg.to = 255;//broadcast
				newMsg.from = 254;
				//_tprintf(TEXT("Sent no game created\n"));
				sendMessage(newMsg);
				continue;
			}else if(_tcscmp(newMsg.messageInfo, TEXT("nop")) == 0) {//for tests
				//_tprintf(TEXT("Login do Utilizador (%s) not accepted\n"), newMsg.messageInfo);
				newMsg.codigoMsg = -1;//not successful
				newMsg.number = -1;
				newMsg.to = 255;//broadcast
				newMsg.from = 254;
				//_tprintf(TEXT("Sent user not accepted created\n"));
				sendMessage(newMsg);
				continue;
			}				
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
				newMsg.number = gameInfo->numUsers++;
				newMsg.to = 255;//broadcast
				newMsg.from = 254;
				_tprintf(TEXT("sending message with sucess from:%d\n"),newMsg.from);
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
			_tprintf(TEXT("[%d]-%s\n"), newMsg.from,newMsg.messageInfo);
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
	int i;
	msg tmpMsg;
	//moveBalls = CreateSemaphore(NULL, gameInfo->numBalls, gameInfo->numBalls, NULL);//semafore que dispara quando tem a posicao de todas as bolas.
	//for (i = 0; i < gameInfo->numBalls; i++)//preenche semaforo
	//	WaitForSingleObject(moveBalls, INFINITE);

	_tprintf(TEXT("Game started!\n"));
	tmpMsg.codigoMsg = 100;//new game
	tmpMsg.from = server_id;
	tmpMsg.to = 255; //broadcast
	_tcscpy_s(tmpMsg.messageInfo, TAM, TEXT("game started"));
	sendMessage(tmpMsg);

	for (i = 0; i < gameInfo->numUsers; i++) {
		hTUser[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)userThread, NULL, 0, NULL);
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
	//_tprintf(TEXT("out of start game!\n"));
}

DWORD WINAPI userThread(LPVOID param) {
	DWORD id = ((DWORD)param);
	user userData;
	userData = gameInfo->nUsers[id];
	//_tprintf(TEXT("(THREAD)User:%s-id[%d]\n"),userData.name,id);
	gameInfo->nUsers[id].posx = gameInfo->myconfig.limx / 2;
	gameInfo->nUsers[id].posy = gameInfo->myconfig.limy - 2;
	gameInfo->nUsers[id].lifes = gameInfo->myconfig.inital_lifes;
	
	//_tprintf(TEXT("Waiting for a user to let us know that we can start\n"));
	Sleep(250);

	//build bricks here
	createBrick(gameInfo->myconfig.initial_bricks);

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
	DWORD posx = gameInfo->nUsers[0].posx + (gameInfo->nUsers[0].size / 2);
	DWORD posy = gameInfo->nUsers[0].posy;
	DWORD oposx, oposy, num = 0, ballScore = 0;
	boolean goingUp = 1, goingRight = (rand() % 2), flag;
	gameInfo->nBalls[id].status = 1;
	do{
		ballScore = GetTickCount();
		flag = 0;
		Sleep(gameInfo->nBalls[id].speed);
		//checks for bricks
		for (int i = 0; i < gameInfo->numBricks; i++) {
			if (!gameInfo->nBricks[i].status)
				continue;
			//_tprintf(TEXT("Ball(%d,%d) | Brick(%d,%d)\n"),posx,posy,gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy);
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
			gameInfo->nBricks[i].posx = 15;
			gameInfo->nBricks[i].posy = 3;
			gameInfo->nBricks[i].tam = 30;
			gameInfo->nBricks[i].status = 4;
		}
		else if (i == 1) {
			gameInfo->nBricks[i].posx = gameInfo->myconfig.limx - 40;
			gameInfo->nBricks[i].posy = 3;
			gameInfo->nBricks[i].tam = 15;
			gameInfo->nBricks[i].status = 5;
		}
		else if (i == 2) {
			gameInfo->nBricks[i].posx = 60;
			gameInfo->nBricks[i].posy = 3;
			gameInfo->nBricks[i].tam = 10;
			gameInfo->nBricks[i].status = 6;
		}
		else if (i == 3) {
			gameInfo->nBricks[i].posx = 5;
			gameInfo->nBricks[i].posy = 7;
			gameInfo->nBricks[i].tam = 50;
			gameInfo->nBricks[i].status = 9;
		}
		else if (i == 4) {
			gameInfo->nBricks[i].posx = gameInfo->myconfig.limx - 55;
			gameInfo->nBricks[i].posy = 7;
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
	if(gameInfo->nBalls[ball_id].speed >= gameInfo->myconfig.max_speed)
		gameInfo->nBalls[ball_id].speed -= gameInfo->myconfig.num_speed_up;
	_tprintf(TEXT("Ball speed is:%d!\n"), gameInfo->myconfig.num_speed_up);
	for (int i = 0; i < gameInfo->numUsers; i++)
		gameInfo->nUsers[i].score += 100;

}

void createGame() {
	msg tmpMsg;
	TCHAR resp;
	DWORD num;
	_tprintf(TEXT("Would you like to change defaut values for the game?(Y/N):"));
	fflush(stdin);
	resp = _gettch();
	if (resp == 'y' || resp == 'Y') {
		do {
			_tprintf(TEXT("\nNumber of initial lifes(1-10):"));
			_tscanf_s(TEXT("%d"), &num,1);
		} while (num <= 0 || num > 10);
		_tprintf(TEXT("\nNumber of initial lifes is now %d\n"),num);
	}

	gameInfo->gameStatus = 0;
	_tprintf(TEXT("New Game Created!\n"));
}

int startVars(TCHAR file[TAM]) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	int i,j;
	BOOLEAN flag;
	//Status
	gameInfo->gameStatus = -1;
	server_id = 254;
	//default config
	//game
	gameInfo->myconfig.limx = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	gameInfo->myconfig.limy = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	gameInfo->myconfig.num_levels = MAX_LEVELS;
	//Users
	gameInfo->myconfig.max_users = MAX_USERS;
	gameInfo->myconfig.inital_lifes = MAX_INIT_LIFES;
	//balls
	gameInfo->myconfig.max_balls = MAX_BALLS;
	gameInfo->myconfig.max_speed = MAX_SPEED;
	gameInfo->myconfig.num_speed_up = MAX_SPEED_BONUS;
	gameInfo->myconfig.num_speed_down = MAX_SPEED_BONUS;
	gameInfo->myconfig.duration = MAX_DURATION;
	gameInfo->myconfig.inital_ball_speed = INIT_SPEED;
	//bricks
	gameInfo->myconfig.max_bricks = MAX_BRICKS;
	gameInfo->myconfig.initial_bricks = INIT_BRICKS;
	//end of default config

	//file config
	FILE *fp;
	errno_t err;
	err = _tfopen_s(&fp,file,"r");
	_tprintf(TEXT("err = %s\n"), err);
	if (!fp) {
		_tprintf(TEXT("Error opening '%s' file\n"), file);
		return -1;
	}

	TCHAR line[TAM];
	TCHAR setting[TAM];
	TCHAR value[TAM];
	DWORD num;
	//fseek(fp, 0L, SEEK_SET);

	while (_ftscanf_s(fp, TEXT("%s"), line, _countof(line))!=EOF) {
		flag = 0;
		j = 0;
		//_tprintf(TEXT("Line[%d]=%s\n"), ++i, line);
		for (int i = 0; i < TAM; i++) {
			if (line[i] == ':') {
				setting[i] = '\0';//end of setting
				flag = 1;
				continue;
			}

			if (!flag) {
				setting[i] = line[i];
			}
			else {
				value[j] = line[i];
				j++;
				if (line[i] == "\0")
					break;
			}			
		}
		num = _tstoi(value);//tranlate
		flag = insertSetting(setting, num);
		if (flag) {
			_tprintf(TEXT("Error inserting the setting '%s' with the value (%d)\n"), setting, num);
		}
		else {
			_tprintf(TEXT("Setting:'%s' was successufuly added with the value of(%d)\n"), setting, num);
		}
	}

	//Users
	gameInfo->numUsers = 0;
	for (i = 0; i < MAX_USERS; i++) {
		resetUser(i);
	}
	//Balls
	gameInfo->numBalls = 0;
	for (i = 0; i < MAX_BALLS; i++) {
		resetBall(i);
	}
	//Brick
	gameInfo->numBricks = 0;
	for (i = 0; i < MAX_BRICKS; i++) {
		resetBrick(i);
	}

	fclose(fp);
	return 0;
}

BOOLEAN insertSetting(TCHAR setting[TAM],DWORD value) {
	if (_tcscmp(setting, TEXT("MAX_USERS")) == 0) {
		gameInfo->myconfig.max_users = value;
		return 0;
	}else if (_tcscmp(setting, TEXT("MAX_BALLS")) == 0) {
		gameInfo->myconfig.max_balls = value;
		return 0;
	}else if (_tcscmp(setting, TEXT("MAX_BRICKS")) == 0) {
		gameInfo->myconfig.max_bricks = value;
		return 0;
	}else if (_tcscmp(setting, TEXT("NUM_LEVELS")) == 0) {
		gameInfo->myconfig.num_levels = value;
		return 0;
	}else if (_tcscmp(setting, TEXT("SPEED_UP")) == 0) {
		gameInfo->myconfig.num_speed_up = value;
		return 0;
	}else if (_tcscmp(setting, TEXT("SPEED_DOWN")) == 0) {
		gameInfo->myconfig.num_speed_down = value;
		return 0;
	}else if (_tcscmp(setting, TEXT("PROB_SPEED_UP")) == 0) {
		gameInfo->myconfig.prob_speed_up = value;
		return 0;
	}else if (_tcscmp(setting, TEXT("PROB_SPEED_DOWN")) == 0) {
		gameInfo->myconfig.prob_speed_down = value;
		return 0;
	}else if (_tcscmp(setting, TEXT("DURATION_SPEED")) == 0) {
		gameInfo->myconfig.duration = value;
		return 0;
	}else if (_tcscmp(setting, TEXT("INITIAL_LIFES")) == 0) {
		gameInfo->myconfig.inital_lifes = value;
		return 0;
	}else if (_tcscmp(setting, TEXT("INITAL_BALL_SPEED")) == 0) {
		gameInfo->myconfig.inital_ball_speed = value;
		return 0;
	}else if (_tcscmp(setting, TEXT("NUM_INIT_BRICKS")) == 0) {
		gameInfo->myconfig.initial_bricks = value;
		return 0;
	}else if (_tcscmp(setting, TEXT("MAX_SPEED")) == 0) {
		gameInfo->myconfig.max_speed = value;
		return 0;
	}
	return 1;
}

void checkSettings() {
	system("cls");
	//game
	_tprintf(TEXT("------------------------------GAME------------------------------\n"));
	_tprintf(TEXT("limite x:%d\n"),gameInfo->myconfig.limx);
	_tprintf(TEXT("limite y:%d\n"),gameInfo->myconfig.limy);
	_tprintf(TEXT("Number of Levels:%d\n"),gameInfo->myconfig.num_levels);
	//users
	_tprintf(TEXT("------------------------------USERS------------------------------\n"));
	_tprintf(TEXT("Max Users:%d\n"),gameInfo->myconfig.num_levels);
	_tprintf(TEXT("Initial Lifes:%d\n"),gameInfo->myconfig.inital_lifes);
	//balls
	_tprintf(TEXT("------------------------------BALLS------------------------------\n"));
	_tprintf(TEXT("Max number of Balls:%d\n"), gameInfo->myconfig.max_balls);
	_tprintf(TEXT("Initial Lifes:%d\n"), gameInfo->myconfig.max_speed);
	_tprintf(TEXT("Number of Speed Ups:%d\n"), gameInfo->myconfig.num_speed_up);
	_tprintf(TEXT("Number of Speed Downs Lifes:%d\n"), gameInfo->myconfig.num_speed_down);
	_tprintf(TEXT("Probability of speed up:%d\n"), gameInfo->myconfig.prob_speed_up);
	_tprintf(TEXT("Probability of speed down:%d\n"), gameInfo->myconfig.prob_speed_down);
	_tprintf(TEXT("Bonus duration:%d\n"), gameInfo->myconfig.duration);
	_tprintf(TEXT("Initial ball speed:%d\n"), gameInfo->myconfig.inital_ball_speed);
	//bricks
	_tprintf(TEXT("------------------------------BRICKS------------------------------\n"));
	_tprintf(TEXT("Max number of bricks speed:%d\n"), gameInfo->myconfig.max_bricks);
	_tprintf(TEXT("Number of initial bricks:%d\n"), gameInfo->myconfig.initial_bricks);

	_tprintf(TEXT("\n\n"));
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

void resetBall(DWORD id) {
	//_tprintf(TEXT("Reseting ball %d\n"), id);
	gameInfo->nBalls[id].posx = 0;
	gameInfo->nBalls[id].posy = 0;
	gameInfo->nBalls[id].status = 0;
	gameInfo->nBalls[id].speed = gameInfo->myconfig.inital_ball_speed;
	hTBola[id] = NULL;
}

void resetBrick(DWORD id) {
	//_tprintf(TEXT("Reseting brick %d\n"), id);
	gameInfo->nBricks[id].posx = 0;
	gameInfo->nBricks[id].posy = 0;
	gameInfo->nBricks[id].status = 0;
	gameInfo->nBricks[id].tam = 0;
	hTBrick[id] = NULL;
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