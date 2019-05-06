#pragma once
//DLL.h
// O bloco ifdef seguinte � o modo standard de criar macros que tornam a exporta��o de
// fun��es e vari�veis mais simples. Todos os ficheiros neste projeto DLL s�o
// compilados com o s�mbolo DLL_IMP_EXPORTS definido. Este s�mbolo n�o deve ser definido
// em nenhum projeto que use a DLL. Desta forma, qualquer outro projeto que inclua este
// este ficheiro ir� ver as fun��es e vari�veis DLL_IMP_API como sendo importadas de uma
// DLL.


#include <windows.h>
#include <tchar.h>

#define TAM 256
#define MAX_MSG 50
//game
#define MAX_LEVELS 5
//User
#define MAX_USERS 20
#define MAX_NAME_LENGTH 250
#define MAX_INIT_LIFES 10

//bals
#define MAX_SPEED 100
#define MAX_BALLS 20
#define MAX_SPEED_BONUS 5
#define PROB_SPEED_BONUS 0.5
#define INIT_SPEED 500
#define MAX_DURATION 50000

//bricks
#define MAX_BRICKS 50
#define INIT_BRICKS 5


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
	DWORD codigoMsg, number, from, to;
	TCHAR messageInfo[TAM];
} msg, *pmsg;

typedef struct ball {
	int posx;
	int posy;
	int status;
	int speed;
} ball;

typedef struct brick {
	int posx;
	int posy;
	int tam;
	int status;
	//brinde or not
} brick;

typedef struct config {
	//game
	int limx;
	int limy;
	int num_levels;
	//user
	int max_users;
	int inital_lifes;
	//ball
	int max_balls;
	int max_speed;
	int num_speed_up;
	int num_speed_down;
	int prob_speed_up;
	int prob_speed_down;
	int duration;
	int inital_ball_speed;
	//bricks
	int max_bricks;
	int initial_bricks;
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
