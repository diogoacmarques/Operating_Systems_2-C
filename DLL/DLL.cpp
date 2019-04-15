// DLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
//DLL.cpp
#include <windows.h>
#include "DLL.h"

HANDLE hMsgShared, hGameShared;

pgame dllGame;
pmsg dllMessage;

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
	//msgFromMemory = (mMsg *)MapViewOfFile(hMsgShared, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(mMsg));
	dllMessage = (pmsg)MapViewOfFile(hMsgShared, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(msg));

	if (dllMessage == NULL) {
		_tprintf(TEXT("(DLL)Eror mapping MSG memory\n"));
	}
	else {
		_tprintf(TEXT("(DLL)MSG memory sucessfully mapped!\n"));
	}

	/*if (!msgFromMemory->count) {
		msgFromMemory->count = 0;
		msgFromMemory->begin = 0;
		msgFromMemory->last = 0;
	}*/
}

void sendMessage(msg newMsg) {

	//_tprintf(TEXT("(DLL)WRTING TO MEMORY:\nCodigo:(%d)\nUser:(%s)\n"),newMsg.codigoMsg,newMsg.namePlayer);
	//msgFromMemory->nMsg[msgFromMemory->last] = newMsg;
	//msgFromMemory->last = msgFromMemory->last + 1;
	//msgFromMemory->count += 1;
	dllMessage->from = sessionId;
	*dllMessage = newMsg;
	SetEvent(messageEvent);
	return;
}

msg receiveMessage() {
	//msg newMsg = msgFromMemory->nMsg[msgFromMemory->begin];
	//msgFromMemory->begin = msgFromMemory->begin + 1;
	//msgFromMemory->count -= 1;

	return *dllMessage;
}

void closeSharedMemoryMsg() {
	UnmapViewOfFile(hMsgShared);
}

int Login(TCHAR user[MAX_NAME_LENGTH]) {
	_tprintf(TEXT("(DLL)Login in with:(%s)\n"), user);
	msg newMsg;
	newMsg.to = 0;//login e sempre para o servidor
	newMsg.codigoMsg = 1;//login
	_tcscpy_s(newMsg.messageInfo, MAX_NAME_LENGTH, user);
	sendMessage(newMsg);
	return 1;
}

//-------------------------------------------------------------------GAME------------------------------------------------------------------
pgame createSharedMemoryGame(void) {
	hGameShared = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(game), TEXT("../../GAME_SHARED_MEMORY_TMP"));
	if (hGameShared == NULL) {
		_tprintf(TEXT("Erro ao criar o objecto FileMapping para hGameShared\n"));
	}
	dllGame = (game*)MapViewOfFile(hGameShared, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(game));
	if (dllGame == NULL) {
		_tprintf(TEXT("(DLL)Eror mapping GAME memory\n"));
	}

	return dllGame;
}

void closeSharedMemoryGame() {
	UnmapViewOfFile(hGameShared);
}

//------------------------------------OTHER---------------------------------------------------------
