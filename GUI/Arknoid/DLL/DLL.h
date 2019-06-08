#pragma once

#include <windows.h>
#include <tchar.h>

#define TAM 256
#define MAX_NAME_LENGTH 250
#define _MSECOND 10000

//game
#define GAME_LEVELS 4
#define GAME_SIZE_X 800
#define GAME_SIZE_Y 600
//user
#define USER_MAX_USERS 5
#define USER_LIFES 5
#define USER_SIZE_X 100
#define USER_SIZE_Y 15
//ball
#define BALL_SPEED 5//15//25
#define BALL_MAX_BALLS 5
#define BALL_MAX_SPEED 5//time to sleep
#define BALL_SIZE 10
//bricks
#define BRICK_MAX_BRICKS 50
#define BRICK_SIZE_X 50
#define BRICK_SIZE_Y 10
//bonus
#define BONUS_DROP_SPEED 17
#define BONUS_SCORE_ADD 150 
#define BONUS_PROB_SPEED 0.5f
#define BONUS_PROB_EXTRALIFE 0.3f
#define BONUS_PROB_TRIPLE 0.2f
#define BONUS_SPEED_CHANGE 2
#define BONUS_SPEED_DURATION 10//seconds
#define BONUS_SIZE_X 20
#define BONUS_SIZE_Y 20
#define BONUS_MAX_BONUS 5//max bonus at the same time

#define USE_MSG_MUTEX TEXT("writeMsgMutex")
#define MSG_SHARED_MEMORY_NAME TEXT("MSG_SHARED_MEMORY")
#define GAME_SHARED_MEMORY_NAME TEXT("GAME_SHARED_MEMORY")
#define STDOUT_CLIENT_MUTEX_NAME TEXT("stdoutMutexClient")
#define SEMAPHORE_MEMORY_READ TEXT("memory_semaphore_read")
#define SEMAPHORE_MEMORY_WRITE TEXT("memory_semaphore_write")
#define MESSAGE_EVENT_NAME TEXT("messageEventServer")
#define MESSAGE_BROADCAST_EVENT_NAME TEXT("messageEventBroadcast")
#define LOCAL_UPDATE_GAME TEXT("updateLocalGame")
#define BALL_EVENT_NAME TEXT("ballEvent")
#define USER_MOVE_EVENT_NAME TEXT("userMoveEvent")
#define BONUS_EVENT_NAME TEXT("bonusEvent")
//local
#define LOCAL_CONNECTION_NAME TEXT("localMessage")
//Pipe
#define MAX_PIPES 5
//#define INIT_PIPE_MSG_NAME  TEXT("\\\\.\\pipe\\initPipeArknoidMsg")
//#define INIT_PIPE_GAME_NAME  TEXT("\\\\.\\pipe\\initPipeArknoidGame")
#define INIT_PIPE_MSG_NAME_ADD  TEXT("\\pipe\\initPipeArknoidMsg")
#define INIT_PIPE_GAME_NAME_ADD  TEXT("\\pipe\\initPipeArknoidGame")

//event to check if a server is running
#define CHECK_SERVER_EVENT TEXT("../arknoidServer")//this event only exists to check if a server is running


typedef struct handles {
	HANDLE hClientMsg;
	HANDLE hClientGame;
	DWORD communication;
} comuciationHandle;

typedef struct Message {
	DWORD codigoMsg, from, to;
	BOOLEAN connection;
	TCHAR messageInfo[TAM];
} msg, *pmsg;
#define BUFSIZE_MSG sizeof(msg)

typedef struct size {
	int sizex;
	int sizey;
}drawSize;

typedef struct ball {
	int id;
	int posx;
	int posy;
	int status;
	int speed;
	drawSize size;
} ball;

typedef struct bonus {
	int status;
	int type;	// 1 = speed_up | 2 = speed_down | 3 = extra_life | 4 = triple
	int posx;
	int posy;
	int speed;
	drawSize size;
} bonus;

typedef struct brick {
	int id;
	int posx;
	int posy;
	int status;
	int type;	//1 = normal | 2 = resistente | 3 = magico
	bonus brinde;
	drawSize size;
} brick;

typedef struct config {
	TCHAR file[TAM];
	//game
	int gameNumLevels;
	drawSize gameSize;
	//user
	int userMaxUsers;
	int userNumLifes;
	drawSize userSize;
	//ball
	int ballInitialSpeed;
	int ballMaxBalls;
	int ballMaxSpeed;
	drawSize ballSize;
	//bricks
	int brickMaxBricks;
	drawSize brickSize;
	//bonus
	int bonusDropSpeed;
	int bonusScoreAdd;
	float bonusProbSpeed;//probabilidade do bonus ser speed 
	float bonusProbExtraLife;//probabilidade do bonus ser vida extra
	float bonusProbTriple;//probabilidade do bonus ser triple ball
	int bonusSpeedChange;
	int bonusSpeedDuration;
	drawSize bonusSize;
} config;

typedef struct Player {
	TCHAR name[MAX_NAME_LENGTH];
	int id;//necessary? so i can associate clientsInfo to nUSers?
	int lifes;
	int posx, posy;
	DWORD score;
	drawSize size;
}user;

typedef struct {
	config myconfig;
	user nUsers[USER_MAX_USERS];
	ball nBalls[BALL_MAX_BALLS];
	brick nBricks[BRICK_MAX_BRICKS];
	TCHAR top[TAM];
	int numUsers;
	int numBalls;
	int numBricks;
	int gameStatus;
}game, *pgame;
#define BUFSIZE_GAME sizeof(game)

HANDLE messageEventClient[USER_MAX_USERS];

//Definir uma constante para facilitar a leitura do protótipo da função
//Este .h deve ser incluído no projeto que o vai usar (modo implícito)
//Esta macro é definida pelo sistema caso estejamos na DLL (<DLL_IMP>_EXPORTS definida)
//ou na app (<DLL_IMP>_EXPORTS não definida) onde DLL_IMP é o nome deste projeto
#ifdef DLL_EXPORTS
#define DLL_IMP_API __declspec(dllexport)
#else
#define DLL_IMP_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif
	//Variável global da DLL
	extern DLL_IMP_API int nDLL;
	//Funções a serem exportadas/importadas
	//msg memory
	DLL_IMP_API void createSharedMemoryMsg(void);
	DLL_IMP_API void mapViewOfFileMsg(void);
	DLL_IMP_API void sendMessageDLL(msg newMsg);
	DLL_IMP_API msg receiveMessageDLL(void);
	DLL_IMP_API void closeSharedMemoryMsg(void);
	//game memory
	DLL_IMP_API pgame createSharedMemoryGame(void);
	DLL_IMP_API void closeSharedMemoryGame(void);

	//functions
	DLL_IMP_API int Login(TCHAR user[MAX_NAME_LENGTH]);

#ifdef __cplusplus
}
#endif
