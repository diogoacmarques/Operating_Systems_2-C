// DLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
//DLL.cpp
#include <windows.h>
#include "DLL.h"
HANDLE hMsgShared, hGameShared;

//HANDLE messageEventClient = CreateEvent(NULL, FALSE, FALSE, TEXT("../..messageEventClient"));
pgame dllGame;
pmsg dllMessage;
HANDLE messageServerDll;

//Exportar a função para ser utilizada fora da DLL
//MSG
void createSharedMemoryMsg(void) {
	messageServerDll = CreateEvent(NULL, FALSE, FALSE, MESSAGE_EVENT_NAME);

	LARGE_INTEGER t;
	t.QuadPart = sizeof(msg);
	hMsgShared = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_EXECUTE_READWRITE, t.HighPart, t.LowPart, MSG_SHARED_MEMORY_NAME);
	if (hMsgShared == NULL) {
		_tprintf(TEXT("Erro ao criar o objecto FileMapping para hMsgShared"));
	}

	mapViewOfFileMsg();
}

void mapViewOfFileMsg(void) {
	dllMessage = (pmsg)MapViewOfFile(hMsgShared, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(msg));

	if (dllMessage == NULL) {
		_tprintf(TEXT("(DLL)Eror mapping MSG memory\n"));
	}
	else {
		_tprintf(TEXT("(DLL)MSG memory sucessfully mapped!\n"));
	}
}

void sendMessageDLL(msg newMsg) {
	*dllMessage = newMsg;
	if (newMsg.to == 254) {
		SetEvent(messageServerDll);//to server
	}
	return;
}

msg receiveMessageDLL() {
	return *dllMessage;
}

void closeSharedMemoryMsg() {
	UnmapViewOfFile(hMsgShared);
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
	else {
		_tprintf(TEXT("(DLL)GAME memory sucessfully mapped!\n"));
	}

	return dllGame;
}

void closeSharedMemoryGame() {
	UnmapViewOfFile(hGameShared);
}

//------------------------------------OTHER---------------------------------------------------------
int Login(TCHAR user[MAX_NAME_LENGTH]) {
	//_tprintf(TEXT("(DLL)Login in with:(%s)\n"), user);
	msg newMsg;
	newMsg.from = -1;
	newMsg.to = 254;//login e sempre para o servidor
	newMsg.codigoMsg = 1;//login
	_tcscpy_s(newMsg.messageInfo, MAX_NAME_LENGTH, user);
	sendMessageDLL(newMsg);
	return 1;
}

