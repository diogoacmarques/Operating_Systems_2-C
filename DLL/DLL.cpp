// DLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
//DLL.cpp
#include <windows.h>
#include "DLL.h"

HANDLE hMsgShared, hGameShared,messageEvent = CreateEvent(NULL,FALSE,FALSE, SENDMESSAGEEVENT);

//Exportar a função para ser utilizada fora da DLL
//MSG
void createSharedMemoryMsg(void) {
	LARGE_INTEGER t;
	t.QuadPart = sizeof(mMsg);
	hMsgShared = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, t.HighPart, t.LowPart, MSG_SHARED_MEMORY_NAME);
	if (hMsgShared == NULL) {
		_tprintf(TEXT("Erro ao criar o objecto FileMapping para hMsgShared"));
	}
	mapViewOfFileMsg();
}

void mapViewOfFileMsg(void) {
	msgFromMemory = (mMsg *)MapViewOfFile(hMsgShared, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(mMsg));

	if (msgFromMemory == NULL) {_tprintf(TEXT("(DLL)Erro a mapear a vista das msg para do file em memmoria"));}
	else {_tprintf(TEXT("(DLL)createSharedMemory + mapViewOfFile = sucesso!\n"));}

	if (!msgFromMemory->count) {
		msgFromMemory->count = 0;
		msgFromMemory->begin = 0;
		msgFromMemory->last = 0;
	}
}

void sendMessage(msg newMsg) {
	//_tprintf(TEXT("(DLL)WRTING TO MEMORY:\nCodigo:(%d)\nUser:(%s)\n"),newMsg.codigoMsg,newMsg.namePlayer);
	
	msgFromMemory->nMsg[msgFromMemory->last] = newMsg;
	msgFromMemory->last = (msgFromMemory->last + 1) % MAX_MSG;
	msgFromMemory->count += 1;
	SetEvent(messageEvent);
	return;
}

msg receiveMessage() {
	msg newMsg = msgFromMemory->nMsg[msgFromMemory->begin];
	msgFromMemory->begin = (msgFromMemory->begin + 1) % MAX_MSG;
	msgFromMemory->count -= 1;

	return newMsg;
}

void closeSharedMemoryMsg() {
	UnmapViewOfFile(hMsgShared);
}

int Login(TCHAR user[MAX_NAME_LENGTH]) {
	_tprintf(TEXT("Login in with:(%s)\n"), user);
	msg newMsg;
	newMsg.codigoMsg = 1;//login
	_tcscpy_s(newMsg.messageInfo, MAX_NAME_LENGTH, user);
	sendMessage(newMsg);
	return 1;
}

//-------------------------------------------------------------------GAME------------------------------------------------------------------
void createSharedMemoryGame(void) {
	LARGE_INTEGER t;
	t.QuadPart = sizeof(gameData);
	hGameShared = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, t.HighPart, t.LowPart, GAME_SHARED_MEMORY_NAME);
	if (hGameShared == NULL) {
		_tprintf(TEXT("Erro ao criar o objecto FileMapping para hGameShared"));
	}
	mapViewOfFileGame();
}

void mapViewOfFileGame(void) {
	gData = (gameData*)MapViewOfFile(hGameShared, FILE_MAP_WRITE, 0, 0, sizeof(gameData));

	if (gData == NULL) { _tprintf(TEXT("(DLL)Erro a mapear a vista do game para do file em memmoria")); }
	else { _tprintf(TEXT("(DLL)createSharedMemoryGame + mapViewOfFileGame = sucesso!\n")); }
}

void sendGame(gameData newGame) {;
	//_tprintf(TEXT("(DLL)WRTING TO MEMORY:\nCodigo:(%d)\nUser:(%s)\n"),newMsg.codigoMsg,newMsg.namePlayer);
	return;
}

gameData receiveGame() {
	gameData newGame;
	newGame.posx = 0;
	newGame.posy = 0;
	return newGame;
}

void closeSharedMemoryGame() {
	UnmapViewOfFile(hMsgShared);
}


//------------------------------------ALL---------------------------------------------------------
void checkVar() {
	_tprintf(TEXT("(DLL)gData=(%d,%d)\n"),gData->posx,gData->posy);
}

void createSharedMemory() {
	createSharedMemoryMsg();
	createSharedMemoryGame();
}

void closeSharedMemory(void) {
	UnmapViewOfFile(hMsgShared);
	UnmapViewOfFile(hGameShared);

}