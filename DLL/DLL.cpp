// DLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
//DLL.cpp
#include <windows.h>
#include "DLL.h"
DWORD teste = 1;
HANDLE hMsgShared, hGameShared;
HANDLE messageEventServer = CreateEvent(NULL, FALSE, FALSE, MESSAGE_EVENT_NAME);
HANDLE messageEventBroadcast = CreateEvent(NULL, TRUE, FALSE, MESSAGE_BROADCAST_EVENT_NAME);
//HANDLE messageEventClient = CreateEvent(NULL, FALSE, FALSE, TEXT("../..messageEventClient"));

pgame dllGame;
pmsg dllMessage;

//Exportar a função para ser utilizada fora da DLL
//MSG
void createSharedMemoryMsg(void) {
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

void sendMessage(msg newMsg) {
	*dllMessage = newMsg;
	if (newMsg.to == 254) {
		//_tprintf(TEXT("(DLL)Client -> Server\n"));
		SetEvent(messageEventServer);//to server
	}
	else if (newMsg.to == 255) {
		//_tprintf(TEXT("(DLL)Server -> Broadcast\n"));
		SetEvent(messageEventBroadcast);//broadcast
		ResetEvent(messageEventBroadcast);
	}
	else {
		//_tprintf(TEXT("(DLL)Server to Client[%d]\n"), newMsg.to);
		SetEvent(messageEventClient[newMsg.to]);//specific user
	}
		
	//_tprintf(TEXT("(DLL)MSG From:%d to %d, with:%s\n"), newMsg.from, newMsg.to, newMsg.messageInfo);
	return;
}

msg receiveMessage() {

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
	sendMessage(newMsg);
	return 1;
}

void createHandles() {
	TCHAR str[TAM];
	TCHAR tmp[TAM];
	for (int i = 0; i < MAX_USERS; i++) {
		_tcscpy_s(str, TAM, TEXT("messageEventClient"));
		_itot_s(i, tmp, TAM, 10);
		_tcscat_s(str, TAM, tmp);
		//_tprintf(TEXT("(DLL)STR:(%s)\n"), str);
		messageEventClient[i] = CreateEvent(NULL, FALSE, FALSE, str);
	}
	_tcscpy_s(str, TAM, TEXT(""));
	return;
}