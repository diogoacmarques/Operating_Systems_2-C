#pragma once
//DLL.h
// O bloco ifdef seguinte � o modo standard de criar macros que tornam a exporta��o de
// fun��es e vari�veis mais simples. Todos os ficheiros neste projeto DLL s�o
// compilados com o s�mbolo DLL_IMP_EXPORTS definido. Este s�mbolo n�o deve ser definido
// em nenhum projeto que use a DLL. Desta forma, qualquer outro projeto que inclua este
// este ficheiro ir� ver as fun��es e vari�veis DLL_IMP_API como sendo importadas de uma
// DLL.
#define TAM 256
#define MAX_NAME_LENGTH 250
#define MAX_MSG 50
#define MSG_SHARED_MEMORY_NAME TEXT("MSG_SHARED_MEMORY")
#define GAME_SHARED_MEMORY_NAME TEXT("GAME_SHARED_MEMORY")
#define SEMAPHORE_MEMORY_READ TEXT("../../memory_semaphore_read")
#define SEMAPHORE_MEMORY_WRITE TEXT("../../memory_semaphore_write")
#define SENDMESSAGEEVENT TEXT("../../sendMessageEvent")

typedef struct Message {
	int codigoMsg;
	TCHAR messageInfo[TAM];
} msg;

typedef struct memoryMessage {
	msg nMsg[MAX_MSG];
	int count; // indica o numero de elementos presentes
	int begin; // aponta para a posicao anterior ao primeiro elemento
	int last; // aponta para o ultimo elemento
} mMsg;

typedef struct gameInfo {
	DWORD posy, posx; //map info
}gameData;

#include <windows.h>
#include <tchar.h>
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
	DLL_IMP_API void createSharedMemoryGame(void);
	DLL_IMP_API void mapViewOfFileGame(void);
	DLL_IMP_API void sendGame(msg newMsg);
	DLL_IMP_API gameData receiveGame(void);
	DLL_IMP_API void closeSharedMemoryGame(void);
	//functions
	DLL_IMP_API void createSharedMemory(void);
	DLL_IMP_API void closeSharedMemory(void);
	DLL_IMP_API int Login(TCHAR user[MAX_NAME_LENGTH]);

#ifdef __cplusplus
}
#endif
