// DLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
//DLL.cpp
#include <windows.h>
#include "DLL.h"

HANDLE hMsgShared;
//Exportar a função para ser utilizada fora da DLL
void createSharedMemory(void) {

	LARGE_INTEGER t;
	t.QuadPart = sizeof(gMsg);
	hMsgShared = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, t.HighPart, t.LowPart, MSG_SHARED_MEMORY_NAME);
	if (hMsgShared == NULL) {
		_tprintf(TEXT("Erro ao criar o objecto FileMapping para hMsgShared"));
	}
	//memCreated++;
	//mapViewOfFile();
}void mapViewOfFile(void) {
	/*gMsgFromGateway = (gMsg *)MapViewOfFile(hMsgShared, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(gMsg));
	if (gMsgFromGateway == NULL) {
		_tprintf(TEXT("Erro a abrir a vista das msg para do file em memmoria"));

	}
	if (!gMsgFromGateway->count) {
		gMsgFromGateway->count = 0;
		gMsgFromGateway->begin = 0;
		gMsgFromGateway->last = 0;
	}*/
}void writeToSharedMemory(msg newMsg) {
	/*gMsgFromGateway->nMsg[gMsgFromGateway->last] = newMsg;
	gMsgFromGateway->last = (gMsgFromGateway->last + 1) % MAX_MSG;
	gMsgFromGateway->count += 1;*/
}

msg readFromSharedMemory() {
	msg newMsg; //= gMsgFromGateway->nMsg[gMsgFromGateway->begin];
	newMsg.codigoMsg = 10;

	//gMsgFromGateway->begin = (gMsgFromGateway->begin + 1) % MAX_MSG;
	//gMsgFromGateway->count -= 1;

	return newMsg;
}

void closeSharedMemory() {
	//UnmapViewOfFile(hMsgShared);
}
int sendMessage(TCHAR user[TAM]) {	TCHAR str[TAM];
	_tprintf(TEXT("Dentro da Dll\nIntroduza uma frase:"));
	_fgetts(str, TAM, stdin);	return 0;}int Login(TCHAR user[TAM]) {	_tprintf(TEXT("Login in with:(%s)\n"),user);	return 0;}

