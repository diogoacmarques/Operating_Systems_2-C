#pragma once

#include <windows.h>
#include <tchar.h>

#define TAM 256
#define MAX_NAME_LENGTH 250
#define _MSECOND 10000

//game
#define GAME_LEVELS 5
#define GAME_SIZE_X 800
#define GAME_SIZE_Y 600
//user
#define USER_MAX_USERS 5
#define USER_LIFES 5
#define USER_SIZE_X 100
#define USER_SIZE_Y 15
//ball
#define BALL_SPEED 25
#define BALL_MAX_BALLS 5
#define BALL_MAX_SPEED 5//time to sleep
#define BALL_SIZE_X 15
#define BALL_SIZE_Y 15
//bricks
#define BRICK_MAX_BRICKS 50
#define BRICK_SIZE_X 50
#define BRICK_SIZE_Y 10
//bonus
#define BONUS_SCORE_ADD 150 
#define BONUS_PROB_SPEED 0.5
#define BONUS_PROB_EXTRALIFE 0.3
#define BONUS_PROB_TRIPLE 0.2
#define BONUS_SPEED_CHANGE 5
#define BONUS_SPEED_DURATION 10//seconds
#define BONUS_SIZE_X 10
#define BONUS_SIZE_Y 10


//#define MAX_USERS 5
//#define DEFAULT_USER_LIFES 5

//#define MAX_INIT_LIFES 3
//#define DEFAULT_SIZE_USER_X 50
//#define DEFAULT_SIZE_USER_Y 15
//
//
////bals
//#define MAX_SPEED 150
//#define MAX_BALLS 5
//#define MAX_SPEED_BONUS 50
//#define PROB_SPEED_BONUS 0.5
//#define BALL_DEFAULT_SPEED 20
//#define MAX_DURATION 50000
//#define BALL_DEFAULT_SIZE 15
//
////bricks
//#define MAX_BRICKS 50
//#define INIT_BRICKS 30
//#define BRICK_DEFAULT_SIZE 50
//
//#define MAX_BONUS_AT_TIME 

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
#define INIT_PIPE_MSG_NAME  TEXT("\\\\.\\pipe\\initPipeArknoidMsg")
#define INIT_PIPE_GAME_NAME  TEXT("\\\\.\\pipe\\initPipeArknoidGame")

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
	int id;//necessary?
	int lifes;
	DWORD score;
	int posx, posy;
	DWORD connection_mode;//not being used?
	HANDLE hConnection;//not being used?
	drawSize size;
}user;

typedef struct {
	config myconfig;
	user nUsers[USER_MAX_USERS];
	ball nBalls[BALL_MAX_BALLS];
	brick nBricks[BRICK_MAX_BRICKS];
	int numUsers;
	int numBalls;
	int numBricks;
	int gameStatus;
}game, *pgame;
#define BUFSIZE_GAME sizeof(game)

HANDLE updateBalls, updateBonus;
HANDLE messageEventClient[USER_MAX_USERS];

HANDLE messageBroadcastDll;

//Definir uma constante para facilitar a leitura do prot�tipo da fun��o
//Este .h deve ser inclu�do no projeto que o vai usar (modo impl�cito)
//Esta macro � definida pelo sistema caso estejamos na DLL (<DLL_IMP>_EXPORTS definida)
//ou na app (<DLL_IMP>_EXPORTS n�o definida) onde DLL_IMP � o nome deste projeto
#ifdef DLL_EXPORTS
#define DLL_IMP_API __declspec(dllexport)
#else
#define DLL_IMP_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C"
{
#endif
	//Vari�vel global da DLL
	extern DLL_IMP_API int nDLL;
	//Fun��es a serem exportadas/importadas
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
