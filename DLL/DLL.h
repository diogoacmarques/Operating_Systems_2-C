#pragma once
//DLL.h
// O bloco ifdef seguinte é o modo standard de criar macros que tornam a exportação de
// funções e variáveis mais simples. Todos os ficheiros neste projeto DLL são
// compilados com o símbolo DLL_IMP_EXPORTS definido. Este símbolo não deve ser definido
// em nenhum projeto que use a DLL. Desta forma, qualquer outro projeto que inclua este
// este ficheiro irá ver as funções e variáveis DLL_IMP_API como sendo importadas de uma
// DLL.


#include <windows.h>
#include <tchar.h>

#define TAM 256
#define MAX_MSG 50
#define _MSECOND 10000
//game
#define MAX_LEVELS 5
//User
#define MAX_USERS 20
#define MAX_NAME_LENGTH 250
#define MAX_INIT_LIFES 3

//bals
#define MAX_SPEED 150
#define MAX_BALLS 20
#define MAX_SPEED_BONUS 50
#define PROB_SPEED_BONUS 0.5
#define INIT_SPEED 500
#define MAX_DURATION 50000

//bricks
#define MAX_BRICKS 50
#define INIT_BRICKS 30


#define MSG_SHARED_MEMORY_NAME TEXT("../../MSG_SHARED_MEMORY")
#define GAME_SHARED_MEMORY_NAME TEXT("../../GAME_SHARED_MEMORY")
#define STDOUT_CLIENT_MUTEX_NAME TEXT("../../stdoutMutexClient")
#define SEMAPHORE_MEMORY_READ TEXT("../../memory_semaphore_read")
#define SEMAPHORE_MEMORY_WRITE TEXT("../../memory_semaphore_write")
#define MESSAGE_EVENT_NAME TEXT("../../messageEventServer")
#define MESSAGE_BROADCAST_EVENT_NAME TEXT("../../messageEventBroadcast")
#define GAME_EVENT_NAME TEXT("../../gameEvent")
#define BALL_EVENT_NAME TEXT("../../ballEvent")
#define USER_MOVE_EVENT_NAME TEXT("../../userMoveEvent")


typedef struct Message {
	DWORD codigoMsg, from, to;
	TCHAR messageInfo[TAM];
} msg, *pmsg;

typedef struct ball {
	int id;
	int posx;
	int posy;
	int status;
	int speed;
} ball;

typedef struct brick {
	int id;
	int posx;
	int posy;
	int tam;
	int status;
	int type;	//1 = normal | 2 = resistente | 3 = magico
	int bonus;	// 1 = speed_up | 2 = speed_down | 3 = extra_life | 4 = triple
} brick;

typedef struct config {
	TCHAR file[TAM];
	//game
	int limx;
	int limy;
	int num_levels;
	//user
	int max_users;
	int inital_lifes;
	//ball
	int inital_ball_speed;
	int max_balls;
	int max_speed;
	//bricks
	int max_bricks;
	int initial_bricks;
	//bonus
	int score_up;
	int prob_speed_up;
	int prob_speed_down;
	int num_speed_up;
	int num_speed_down;
	int duration;
} config;

typedef struct Player {
	TCHAR name[MAX_NAME_LENGTH];
	int user_id;
	int lifes;
	DWORD score;
	int size;
	int posx, posy;
}user;

typedef struct {
	config myconfig;
	user nUsers[MAX_USERS];
	ball nBalls[MAX_BALLS];
	brick nBricks[MAX_BRICKS];
	int numUsers;
	int numBalls;
	int numBricks;
	int gameStatus;
}game, *pgame;

HANDLE canMove, gameEvent, updateBalls;
HANDLE messageEventClient[MAX_USERS];


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
	DLL_IMP_API void sendMessage(msg newMsg);
	DLL_IMP_API msg receiveMessage(void);
	DLL_IMP_API void closeSharedMemoryMsg(void);
	//game memory
	DLL_IMP_API pgame createSharedMemoryGame(void);
	DLL_IMP_API void closeSharedMemoryGame(void);

	//functions
	DLL_IMP_API int Login(TCHAR user[MAX_NAME_LENGTH]);
	DLL_IMP_API void createHandles(void);

#ifdef __cplusplus
}
#endif
