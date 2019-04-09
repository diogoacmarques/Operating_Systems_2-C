#pragma once
//DLL.h
// O bloco ifdef seguinte é o modo standard de criar macros que tornam a exportação de
// funções e variáveis mais simples. Todos os ficheiros neste projeto DLL são
// compilados com o símbolo DLL_IMP_EXPORTS definido. Este símbolo não deve ser definido
// em nenhum projeto que use a DLL. Desta forma, qualquer outro projeto que inclua este
// este ficheiro irá ver as funções e variáveis DLL_IMP_API como sendo importadas de uma
// DLL.
#define MAX_NAME_LENGTH 250
#define MAX_MSG 50 
#define MSG_SHARED_MEMORY_NAME TEXT("SHARED_MEMORY")

typedef struct Mensage {
	int codigoMsg;
	TCHAR namePlayer[MAX_NAME_LENGTH];
} msg;

typedef struct groupMessage {
	msg nMsg[MAX_MSG];
	int count; // indica o numero de elementos presentes
	int begin; // aponta para a posicao anterior ao primeiro elemento
	int last; // aponta para o ultimo elemento
} gMsg;

#include <windows.h>
#include <tchar.h>
//Definir uma constante para facilitar a leitura do protótipo da função
//Este .h deve ser incluído no projeto que o vai usar (modo implícito)
#define TAM 256
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
	DLL_IMP_API void createSharedMemory(void);
	DLL_IMP_API void mapViewOfFile(void);
	DLL_IMP_API void writeToSharedMemory(msg newMsg);
	DLL_IMP_API msg readFromSharedMemory(void);
	DLL_IMP_API void closeSharedMemory(void);
	DLL_IMP_API int sendMessage(TCHAR user[TAM]);
	DLL_IMP_API int Login(TCHAR user[TAM]);
#ifdef __cplusplus
}
#endif