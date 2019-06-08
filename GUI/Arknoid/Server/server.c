#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include <strsafe.h>
#include <time.h>
#include <aclapi.h>
#include "resource.h"
#include "DLL.h"

//threads
	//pipes
DWORD WINAPI connectPipeMsg(LPVOID param);//thread for inital pipe for msg
DWORD WINAPI connectPipeGame(LPVOID param);//thread for inital pipe for game
DWORD WINAPI pipeClientMsg(LPVOID param);//thread for each client msg
DWORD WINAPI updateGamePipe(LPVOID param);//thread that updates game for eatch remote pipe
	//local
DWORD WINAPI receiveLocalMsg(LPVOID param);//thread for dll msg

	//game
DWORD WINAPI BolaThread(LPVOID param);
DWORD WINAPI bonusDrop(LPVOID param);
DWORD WINAPI movingBricks(LPVOID param);

//functions
LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK resolveMenu(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
void print(msg printMsg);
DWORD startVars();
DWORD createHandle(msg newMsg);
DWORD resolveMessage(msg inMsg);
void sendMessage(msg sendMsg);
void sendMessagePipe(msg sendMsg, HANDLE hPipe);
void createGame();
void startGame();
void endGame();
void createLevel(DWORD levelNum);
void userInit(DWORD id);
void createBalls(DWORD num);
void resetUser(DWORD id);
void resetBall(DWORD id);
void resetBrick(DWORD id);
void hitBrick(DWORD brick_id, DWORD ball_id);
void attributeBonus(DWORD client_id, DWORD brick_id);
void createBonus(DWORD brick_id);
int moveUser(DWORD id, TCHAR side[TAM]);
void updateEveryone();
int registry(user userData);
void securityPipes(SECURITY_ATTRIBUTES * sa);
void Cleanup(PSID pEveryoneSID, PSID pAdminSID, PACL pACL, PSECURITY_DESCRIPTOR pSD);


//vars
TCHAR szProgName[] = TEXT("Server Base");
HWND global_hWnd = NULL;
DWORD localNumBricks;
TCHAR inTxt[TAM];
TCHAR outTxt[TAM];
TCHAR gameState[TAM];
int ballWaitTime = 0;
TCHAR msgPipeName[TAM];
TCHAR gamePipeName[TAM];

//client related
comuciationHandle clientsInfo[USER_MAX_USERS];
DWORD tmp_client_id = 0;
HANDLE hTmpPipeMsg = NULL;
HANDLE hTmpPipeGame = NULL;

//server related
HANDLE updateRemoteGame;//event to update game for pipe users
HANDLE updateLocalGame;//event to update game for local users
HANDLE messageEventServer;//event to update server of dll msg
HANDLE hResolveMessageMutex;//handle to resolveMessage function
HANDLE writeGameMutex;//mutex to write game
DWORD server_id;
HANDLE hTBola[BALL_MAX_BALLS];
HANDLE hTBonus[BONUS_MAX_BONUS];

int maxX = 0, maxY = 0;
HDC memDC = NULL;
HBITMAP hBit = NULL;
HBRUSH hBrush = NULL;

pgame currentGame;//has game Information
pgame gameClient;//game to be sent

int counter = 0;

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	
	HWND hWnd; // hWnd é o handler da janela, gerado mais abaixo por CreateWindow()
	MSG lpMsg; // MSG é uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp; // WNDCLASSEX é uma estrutura cujos membros servem para
    // definir as características da classe da janela
    // ============================================================================
    // 1. Definição das características da janela "wcApp"
    // (Valores dos elementos da estrutura "wcApp" do tipo WNDCLASSEX)
    // ============================================================================
	wcApp.cbSize = sizeof(WNDCLASSEX); // Tamanho da estrutura WNDCLASSEX
	wcApp.hInstance = hInst; // Instância da janela actualmente exibida
    // ("hInst" é parâmetro de WinMain e vem inicializada daí)
	wcApp.lpszClassName = szProgName; // Nome da janela (neste caso = nome do programa)
	wcApp.lpfnWndProc = TrataEventos; // Endereço da função de processamento da janela
    // ("TrataEventos" foi declarada no início e encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW;// Estilo da janela: Fazer o redraw se for modificada horizontal ou verticalmente
	wcApp.hIcon = LoadIcon(NULL, IDI_APPLICATION);// "hIcon" = handler do ícon normal

	wcApp.hIconSm = LoadIcon(NULL, IDI_APPLICATION);// "hIconSm" = handler do ícon pequeno

	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW); // "hCursor" = handler do cursor (rato)

	wcApp.lpszMenuName = MAKEINTRESOURCE(IDM_MENU); // (NULL = não tem menu)
	wcApp.cbClsExtra = 0; // Livre, para uso particular
	wcApp.cbWndExtra = 0; // Livre, para uso particular

	wcApp.hbrBackground = CreateSolidBrush(RGB(98, 66, 244));


	if (!RegisterClassEx(&wcApp))
		return(0);

	hWnd = CreateWindow(
		szProgName, // Nome da janela (programa) definido acima
		TEXT("SO2 - Ficha 6 - Ex1_V2"),// Texto que figura na barra do título
		WS_OVERLAPPEDWINDOW, // Estilo da janela (WS_OVERLAPPED= normal)
		0, // Posição x pixels (default=à direita da última)
		0, // Posição y pixels (default=abaixo da última)
		GAME_SIZE_X, // Largura da janela (800)
		GAME_SIZE_Y, // Altura da janela (600)
		(HWND)HWND_DESKTOP, // handle da janela pai (se se criar uma a partir de
		// outra) ou HWND_DESKTOP se a janela for a primeira,
		// criada a partir do "desktop"
		(HMENU)NULL, // handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst, // handle da instância do programa actual ("hInst" é
		// passado num dos parâmetros de WinMain()
		0); // Não há parâmetros adicionais para a janela

	HANDLE checkExistingServer;
	//checkExistingServer = CreateEvent(NULL, FALSE, NULL, CHECK_SERVER_EVENT);
	checkExistingServer = OpenEvent(EVENT_ALL_ACCESS, TRUE, CHECK_SERVER_EVENT);
	if (!checkExistingServer == NULL) {//there is no server created
		MessageBox(hWnd, TEXT("There is a server running at this moment."), TEXT("WARNING"), MB_OK);
		return -1;
	}

	ShowWindow(hWnd, nCmdShow);
	global_hWnd = hWnd;
	UpdateWindow(hWnd); // Refrescar a janela (Windows envia à janela uma

	//-------------------------------------------------program-------------------------------------
	DWORD res;

	checkExistingServer = CreateEvent(NULL, FALSE, NULL, CHECK_SERVER_EVENT);
	writeGameMutex = CreateMutex(NULL, FALSE, NULL);
	updateRemoteGame = CreateEvent(NULL, TRUE, FALSE, NULL);
	updateLocalGame = CreateEvent(NULL, TRUE, FALSE, LOCAL_UPDATE_GAME);
	messageEventServer = CreateEvent(NULL, FALSE, FALSE, MESSAGE_EVENT_NAME);
	hResolveMessageMutex = CreateMutex(NULL, FALSE, NULL);
	if (updateRemoteGame == NULL || messageEventServer == NULL || checkExistingServer == NULL
		|| hResolveMessageMutex == NULL || updateLocalGame == NULL || writeGameMutex == NULL) {
		MessageBox(global_hWnd, TEXT("Error creating resources..."), TEXT("Resources"), MB_OK);
		
	}

	currentGame = (pgame)malloc(sizeof(game));//game that server changes
	if (currentGame == NULL) {
		MessageBox(global_hWnd, TEXT("Error reserving memory for currentGame"), TEXT("Error"), MB_ICONERROR | MB_OK);
		return -1;
	}
	//DLL(Memory)
	//Message
	createSharedMemoryMsg();
	//Game
	gameClient = createSharedMemoryGame();//game that clients sees
	

	HANDLE hTReceiveMessage = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveLocalMsg, NULL, 0, NULL);
	if (hTReceiveMessage == NULL) {
		return -1;
	}
	//pipes for initial connection

	_tcscpy_s(msgPipeName, TAM, TEXT("\\\\"));
	_tcscpy_s(gamePipeName, TAM, TEXT("\\\\"));

	_tcscat_s(msgPipeName, TAM, TEXT("."));//adds
	_tcscat_s(gamePipeName, TAM, TEXT("."));//adds

	_tcscat_s(msgPipeName, TAM, INIT_PIPE_MSG_NAME_ADD);//adds
	_tcscat_s(gamePipeName, TAM, INIT_PIPE_GAME_NAME_ADD);//adds

	MessageBox(global_hWnd, msgPipeName, TEXT("Message pipe"), MB_OK);
	MessageBox(global_hWnd, gamePipeName, TEXT("Game pipe"), MB_OK);

		//msg
	HANDLE hMsgPipe = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)connectPipeMsg, NULL, 0, NULL);
	if (hMsgPipe == NULL) {
		return -1;
	}
	//game
	HANDLE hGamePipe = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)connectPipeGame, NULL, 0, NULL);
	if (hGamePipe == NULL) {
		return -1;
	}
	//thred that updates games for remote users
	HANDLE hUpdateGame = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)updateGamePipe, NULL, 0, NULL);
	if (hUpdateGame == NULL) {
		return -1;
	}

	//starts currentGame vars
	res = startVars();
	if (res) {
		MessageBox(global_hWnd, TEXT("Error starting variables"), TEXT("Error"), MB_ICONERROR | MB_OK);
		return -1;
	}

	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg);
		DispatchMessage(&lpMsg); 
	}

	return((int)lpMsg.wParam); // Retorna sempre o parâmetro wParam da estrutura lpMsg
}

LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	HDC hDC;
	PAINTSTRUCT ps;
	TCHAR str[TAM];
	TCHAR tmp[TAM];
	switch (messg) {
	case WM_LBUTTONDOWN:
		break;

	case WM_CREATE:
		maxX = GetSystemMetrics(SM_CXSCREEN);
		maxY = GetSystemMetrics(SM_CYSCREEN);

		hDC = GetDC(hWnd);

		//create buffer
		memDC = CreateCompatibleDC(hDC);
		hBit = CreateCompatibleBitmap(hDC, maxX, maxY);
		SelectObject(memDC, hBit);
		DeleteObject(hBit);
		hBrush = CreateSolidBrush(RGB(98, 66, 244));
		SelectObject(memDC, hBrush);
		PatBlt(memDC, 0, 0, maxX, maxY, PATCOPY);
		ReleaseDC(hWnd, hDC);
		break;

	case WM_KEYDOWN:
		break;
	case WM_PAINT:
		SetBkMode(memDC, TRANSPARENT);
		SetTextColor((HDC)memDC, RGB(255, 255, 255));
		SetStretchBltMode(memDC, COLORONCOLOR);
		PatBlt(memDC, 0, 0, maxX, maxY, PATCOPY); // fill background
		
		TextOut(memDC, 0, 10, gameState, _tcslen(gameState));
		TextOut(memDC, 0, 30, inTxt, _tcslen(inTxt));
		TextOut(memDC, 0, 45, outTxt, _tcslen(outTxt));

		_tcscpy_s(str, TAM, TEXT("Info["));
		_itot_s(counter++, tmp, TAM, 10);//translates num to str
		_tcscat_s(str, TAM, tmp);//ads
		_tcscat_s(str, TAM, TEXT("]->localNumBricks="));
		_itot_s(localNumBricks, tmp, TAM, 10);//translates num to str
		_tcscat_s(str, TAM, tmp);
		TextOut(memDC, 0, 100, str, _tcslen(str));

		hDC = BeginPaint(hWnd, &ps);
		BitBlt(hDC, 0, 0, maxX, maxY, memDC, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		break;

	case WM_COMMAND:
		resolveMenu(hWnd, messg, wParam, lParam);
		break;
	case WM_CLOSE:
		/*res = MessageBox(hWnd, TEXT("Are you sure you want to quit?"), TEXT("Quit MessageBox"), MB_ICONQUESTION | MB_YESNO);
		if (res == IDYES)*/{
		closeSharedMemoryMsg();
		closeSharedMemoryGame();
		DestroyWindow(hWnd);
	}
			
		break;
	case WM_DESTROY: // Destruir a janela e terminar o programa
		PostQuitMessage(0);
		free(currentGame);
		closeSharedMemoryMsg();
		closeSharedMemoryGame();
		break;
	default:
		return DefWindowProc(hWnd, messg, wParam, lParam);
		break;
	}
	return(0);
}

LRESULT CALLBACK resolveMenu(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	TCHAR tmp[TAM];
	int num = 0;
	switch (LOWORD(wParam)) {
		case ID_CREATEGAME:
			if (currentGame->gameStatus == -1)
				createGame();
			else
				MessageBox(global_hWnd, TEXT("Cant create a game right now"), TEXT("Warning"), MB_OK);
			break;
		case ID_STARTGAME:
			if (currentGame->numUsers > 0 && currentGame->gameStatus == 0) {
				MessageBox(global_hWnd, TEXT("WILL START GAME AFTER OK"), TEXT("GAME START"), MB_OK);
				startGame();
			}
			else
				MessageBox(global_hWnd, TEXT("Cant start the game right now"), TEXT("Warning"), MB_OK);
			break;
		case ID_TOP10:
			_tcscpy_s(tmp, TAM, currentGame->top);
			for (int i = 0; i < 10; i++) {
				for (int j = 0; j < TAM; j++) {
					if (tmp[num] == '|') {
						tmp[j] = '\0';
						num++;
						break;
					}

					tmp[j] = tmp[num++];
				}
				MessageBox(hWnd, tmp, TEXT("Info:"), MB_OK);
			}
			break;
		case ID_USERSLOGGED:
			if (currentGame->numUsers == 0) {
				MessageBox(global_hWnd, TEXT("There are no users logged in"), TEXT("Info"), MB_ICONWARNING | MB_OK);
				break;
			}
			for(int i = 0;i<currentGame->numUsers;i++)
				MessageBox(global_hWnd, currentGame->nUsers[i].name, TEXT("User"), MB_OK);
			break;
		case ID_SETTINGS:
			_itot_s(currentGame->numUsers, tmp, TAM, 10);//translates num to str
			MessageBox(global_hWnd, tmp, TEXT("Number of users:"), MB_OK);
			_itot_s(currentGame->gameStatus, tmp, TAM, 10);//translates num to str
			MessageBox(global_hWnd, tmp, TEXT("GameStatus:"), MB_OK);
			for (int i = 0; i < USER_MAX_USERS; i++) {
				_itot_s(clientsInfo[i].communication, tmp, TAM, 10);//translates num to str
				MessageBox(global_hWnd, tmp, TEXT("client[i].communication = "), MB_OK);
			}
			break;
	}
	return 1;
}

void print(msg printMsg) {
	TCHAR tmp[TAM];
	TCHAR tmp2[30];
	if(printMsg.to == 254)
		_tcscpy_s(tmp, TAM, TEXT("Received->"));
	else
		_tcscpy_s(tmp, TAM, TEXT("Sent->"));

	_tcscat_s(tmp, TAM, TEXT("Info:"));
	_tcscat_s(tmp, TAM, printMsg.messageInfo);//adds

	_tcscat_s(tmp, TAM, TEXT("|Codigo:"));
	_itot_s(printMsg.codigoMsg, tmp2, 30, 10);//translates num to str
	_tcscat_s(tmp, TAM, tmp2);//ads

	_tcscat_s(tmp, TAM, TEXT("|Connection:"));
	_itot_s(printMsg.connection, tmp2, 30, 10);//translates num to str
	_tcscat_s(tmp, TAM, tmp2);//ads

	_tcscat_s(tmp, TAM, TEXT("|To:"));
	_itot_s(printMsg.to, tmp2, 30, 10);//translates num to str
	_tcscat_s(tmp, TAM, tmp2);//ads


	//MessageBox(global_hWnd, tmp, TEXT("Server - sending message"), MB_OK);
	if (printMsg.to == 254)
		_tcscpy_s(inTxt, TAM, tmp);
	else
		_tcscpy_s(outTxt, TAM, tmp);
	
	InvalidateRect(global_hWnd, NULL, FALSE);
	return;
	//MessageBeep(MB_ICONSTOP);

}

DWORD resolveMessage(msg inMsg) {
	//Sleep(2000);
	DWORD num;
	TCHAR tmp[TAM];
	boolean flag;
	msg outMsg;
	outMsg.codigoMsg = 0;
	outMsg.from = 254;
	outMsg.connection = inMsg.connection;
	outMsg.to = inMsg.from;
	_tcscpy_s(outMsg.messageInfo, TAM, TEXT("---"));

	if (inMsg.codigoMsg == -9999) {
		num = createHandle(inMsg);
		if (num == USER_MAX_USERS - 1) {//full
			outMsg.codigoMsg = -9999;
		}
		else if (num >= 0) {//success
			outMsg.codigoMsg = 9999;
			updateEveryone();//sends info after connecting
		}
		outMsg.to = num;
		_itot_s(num, tmp, TAM, 10);//translates num to str
		_tcscpy_s(outMsg.messageInfo, TAM, tmp);//copys client_id to messageInfo
		sendMessage(outMsg);
		print(outMsg);
		return 1;
	}

	//user
	if (inMsg.codigoMsg == 1) {//login de utilizador
		flag = 1;
		if (currentGame->gameStatus == -1) {
			outMsg.codigoMsg = -100;//no game created
			outMsg.to = inMsg.from;
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("noGameCreated"));
			sendMessage(outMsg);
			return 1;
		}
		else if (currentGame->gameStatus > 0) {
			outMsg.codigoMsg = -110;//game already in progress
			outMsg.to = inMsg.from;
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("gameInProgress"));
			sendMessage(outMsg);
			return 1;
		}
		else if (_tcscmp(inMsg.messageInfo, TEXT("nop")) == 0 || _tcscmp(inMsg.messageInfo, TEXT("")) == 0) {
			outMsg.codigoMsg = -1;//not successful
			outMsg.to = inMsg.from;
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("youAreBanned"));
			sendMessage(outMsg);
			return 1;
		}
		for (int i = 0; i < currentGame->numUsers; i++) {
			if (_tcscmp(currentGame->nUsers[i].name, inMsg.messageInfo) == 0) {
				flag = 0;
				outMsg.codigoMsg = -1;//not successful
				_tcscpy_s(outMsg.messageInfo, TAM, TEXT("alreadyHaveThisUser"));
				sendMessage(outMsg);
				break;
			}
		}
		if (flag) {
			WaitForSingleObject(writeGameMutex, INFINITE);
			_tcscpy_s(currentGame->nUsers[inMsg.from].name, MAX_NAME_LENGTH, inMsg.messageInfo);
			currentGame->nUsers[currentGame->numUsers].id = inMsg.from;
			outMsg.codigoMsg = 1;//sucesso
			currentGame->numUsers++;
			ReleaseMutex(writeGameMutex);
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("loginSucess"));
			sendMessage(outMsg);
		}

	}
	else if (inMsg.codigoMsg == 2) {//end of user
		Sleep(250);
		resetUser(inMsg.from);
		outMsg.to = inMsg.from;
		outMsg.codigoMsg = 2;
		_tcscpy_s(outMsg.messageInfo, TAM, TEXT("userEnd"));
		sendMessage(outMsg);

		int count = 0;
		for (int i = 0; i < currentGame->numUsers; i++)//counts the number of players playing
			if (currentGame->nUsers[i].id != -1)
				count++;

		if (count == 0)
			endGame();
	}
	else if (inMsg.codigoMsg == 101 && currentGame->gameStatus > 0) {
		if (currentGame->nUsers[inMsg.from].lifes > 0 && currentGame->numBalls == 0)
			createBalls(1);
	}
	else if (inMsg.codigoMsg == 200 && currentGame->gameStatus > 0) {//user trying to move
		int res;
		if (_tcscmp(inMsg.messageInfo, TEXT("right")) != 0 && _tcscmp(inMsg.messageInfo, TEXT("left")) != 0) {
			int moveToPos = _tstoi(inMsg.messageInfo);//tranlate
			if(moveToPos > currentGame->nUsers[inMsg.from].posx)
				_tcscpy_s(inMsg.messageInfo, TAM, TEXT("right"));
			else
				_tcscpy_s(inMsg.messageInfo, TAM, TEXT("left"));
			do {
				if (_tcscmp(inMsg.messageInfo, TEXT("right")) == 0 && moveToPos <= currentGame->nUsers[inMsg.from].posx){
					break;
				}
				else if (_tcscmp(inMsg.messageInfo, TEXT("left")) == 0 && moveToPos >= currentGame->nUsers[inMsg.from].posx) {
					break;
				}
				res = moveUser(inMsg.from, inMsg.messageInfo);
				if (res) {//if cant move
					break;
				}					
			} while (1);
		}else{
			res = moveUser(inMsg.from, inMsg.messageInfo);
			if (res)
				return -1;
		}
		
	}
	else if (inMsg.codigoMsg == 123) {//for tests || returns the exact same msg
		outMsg.to = inMsg.from;
		sendMessage(outMsg);
	}

	InvalidateRect(global_hWnd, NULL, FALSE);
	updateEveryone();
	return -1;
}

//thread to receive initial clients via pipe for msg
DWORD WINAPI connectPipeMsg(LPVOID param) {
	BOOLEAN fConnected = 0;
	HANDLE hPipeInit = INVALID_HANDLE_VALUE;
	HANDLE hThread;

	SECURITY_ATTRIBUTES sa;
	securityPipes(&sa);

	do {
		hPipeInit = CreateNamedPipe(msgPipeName,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE | // tipo de pipe = message
			PIPE_READMODE_MESSAGE | // com modo message-read e
			PIPE_WAIT, // bloqueante (não usar PIPE_NOWAIT nem mesmo em Asyncr)
			PIPE_UNLIMITED_INSTANCES, // max. instancias (255)
			BUFSIZE_MSG, // tam buffer output
			BUFSIZE_MSG, // tam buffer input
			2000, // time-out p/ cliente 5k milisegundos (0->default=50)
			&sa);

		if (hPipeInit == INVALID_HANDLE_VALUE) {
			MessageBox(global_hWnd, TEXT("Error intiating initial pipe for the msg"), TEXT("Error"), MB_ICONERROR | MB_OK);
			return 0;
		}

		//waits for client
		fConnected = ConnectNamedPipe(hPipeInit, NULL);//waits for client
		if (!fConnected && GetLastError() == ERROR_PIPE_CONNECTED)
			fConnected = 1;

		//MessageBox(global_hWnd, TEXT("Got a Msg Client"), TEXT("Client"), MB_OK);

		if (fConnected) {
			hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)pipeClientMsg, (LPVOID)hPipeInit, 0, NULL);
			if (hThread == NULL) {
				MessageBox(global_hWnd, TEXT("Error creating thread msg"), TEXT("Error"), MB_ICONERROR | MB_OK);
				return 0;
			}
			else {//created with success
				CloseHandle(hThread);
			}
		}
		else {
			CloseHandle(hPipeInit);
		}
	} while (1);

	return 0;
}

//thread to receive initial clients via pipe for game
DWORD WINAPI connectPipeGame(LPVOID param) {
	BOOLEAN fConnected = 0;
	HANDLE hPipeInit = INVALID_HANDLE_VALUE;

	SECURITY_ATTRIBUTES sa;
	securityPipes(&sa);

	do {

		hPipeInit = CreateNamedPipe(gamePipeName,
			PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // bloqueante (não usar PIPE_NOWAIT nem mesmo em Asyncr)
			PIPE_UNLIMITED_INSTANCES, // max. instancias (255)
			BUFSIZE_GAME, // tam buffer output
			BUFSIZE_GAME, // tam buffer input
			2000, // time-out p/ cliente 5k milisegundos (0->default=50)
			&sa);

		if (hPipeInit == INVALID_HANDLE_VALUE) {
			MessageBox(global_hWnd, TEXT("Error intiating initial pipe for the game"), TEXT("Error"), MB_ICONERROR | MB_OK);
			return 0;
		}

		//waits for client
		fConnected = ConnectNamedPipe(hPipeInit, NULL);
		if (!fConnected && GetLastError() == ERROR_PIPE_CONNECTED)
			fConnected = 1;
	
		//MessageBox(global_hWnd, TEXT("client for game"), TEXT("game"), MB_OK);

		if (fConnected) {
			clientsInfo[tmp_client_id].hClientGame = hPipeInit;//got a new client to send the game to
		}
		else {
			CloseHandle(hPipeInit);
		}

		Sleep(1000);//waits for client to be ready
		SetEvent(updateRemoteGame);//sends info after connecting
		ResetEvent(updateRemoteGame);
	} while (1);

	return 0;
}

//thread to receive each client msg
DWORD WINAPI pipeClientMsg(LPVOID param) {
	msg inMsg;
	DWORD cbBytesRead;
	DWORD resp;
	BOOLEAN fSuccess;
	HANDLE hPipe = (HANDLE)param; // a informação enviada à thread é o handle do pipe
	if (hPipe == NULL) {
		MessageBox(global_hWnd, TEXT("Error handle received by pipeClientMsg = NULL"), TEXT("Error"), MB_ICONERROR | MB_OK);
		return -1;
	}

	hTmpPipeMsg = hPipe;

	HANDLE ReadReady;
	OVERLAPPED OverlRd = { 0 };

	ReadReady = CreateEvent(
		NULL, // default security + non inheritable
		TRUE, // Reset manual, por requisito do overlapped IO => uso de ResetEvent
		FALSE, // estado inicial = not signaled
		NULL); // não precisa de nome: uso interno ao processo
	if (ReadReady == NULL) {
		MessageBox(global_hWnd, TEXT("Could not create Event ReadReady"), TEXT("Error"), MB_ICONERROR | MB_OK);
		return 1;
	}

	while (1) {
		ZeroMemory(&OverlRd, sizeof(OverlRd));
		ResetEvent(ReadReady);
		OverlRd.hEvent = ReadReady;

		fSuccess = ReadFile(//waiting for message
			hPipe, // handle para o pipe (recebido no param)
			&inMsg, // buffer para os dados a ler
			sizeof(msg), // Tamanho msg a ler
			&cbBytesRead, // número de bytes lidos
			&OverlRd); // != NULL -> é overlapped I/O

		WaitForSingleObject(ReadReady, INFINITE);
		//MessageBox(global_hWnd, TEXT("Got a message via the pipes"), TEXT("PIPE"), MB_OK);

		GetOverlappedResult(hPipe, &OverlRd, &cbBytesRead, FALSE);
		if (cbBytesRead < sizeof(msg))
			MessageBox(global_hWnd, TEXT("Didnt read all the data from the pipe!!"), TEXT("Error"), MB_ICONWARNING | MB_OK);
		
		WaitForSingleObject(hResolveMessageMutex, INFINITE);
		resp = resolveMessage(inMsg);
		ReleaseMutex(hResolveMessageMutex);
	}
	return 1;
}

//thread to send the game to each remote user
DWORD WINAPI updateGamePipe(LPVOID param) {

	DWORD bytesWritten;
	BOOLEAN fSuccess;

	int i;
	do {
		WaitForSingleObject(updateRemoteGame, INFINITE);
		for (i = 0; i < USER_MAX_USERS; i++)
			if (clientsInfo[i].hClientGame != NULL && clientsInfo[i].communication == 1) {
				fSuccess = WriteFile(
					clientsInfo[i].hClientGame,
					gameClient,
					sizeof(game),
					&bytesWritten,
					NULL
				);
			}
		
	} while (1);
	return 1;
}

//thread to receive msg via DLL
DWORD WINAPI receiveLocalMsg(LPVOID param) {//local communication
	msg inMsg;
	TCHAR tmp[200] = { 0 };
	do {
		WaitForSingleObject(messageEventServer, INFINITE);
		WaitForSingleObject(hResolveMessageMutex, INFINITE);
		inMsg = receiveMessageDLL();
		print(inMsg);
		resolveMessage(inMsg);
		ReleaseMutex(hResolveMessageMutex);
	} while (1);
}

DWORD startVars() {
	RECT tmpRect;
	GetWindowRect(global_hWnd, &tmpRect);
	int i;
	server_id = 254;
	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->gameStatus = -1;
	//default config
	currentGame->myconfig.file;
	//game
	currentGame->myconfig.gameNumLevels = GAME_LEVELS;
	currentGame->myconfig.gameSize.sizex = GAME_SIZE_X - 15;//this values are weird
	currentGame->myconfig.gameSize.sizey = GAME_SIZE_Y  - 60;
	//user
	currentGame->myconfig.userMaxUsers = USER_MAX_USERS;
	currentGame->myconfig.userNumLifes = USER_LIFES;
	currentGame->myconfig.userSize.sizex = USER_SIZE_X;
	currentGame->myconfig.userSize.sizey = USER_SIZE_Y;
	//ball
	currentGame->myconfig.ballInitialSpeed = BALL_SPEED;
	currentGame->myconfig.ballMaxBalls = BALL_MAX_BALLS;
	currentGame->myconfig.ballMaxSpeed = BALL_MAX_SPEED;
	currentGame->myconfig.ballSize.sizex = BALL_SIZE;
	currentGame->myconfig.ballSize.sizey = BALL_SIZE;
	//bricks
	currentGame->myconfig.brickMaxBricks = BRICK_MAX_BRICKS;
	currentGame->myconfig.brickSize.sizex = BRICK_SIZE_X;
	currentGame->myconfig.brickSize.sizey = BRICK_SIZE_Y;
	//bonus
	currentGame->myconfig.bonusDropSpeed = BONUS_DROP_SPEED;
	currentGame->myconfig.bonusScoreAdd = BONUS_SCORE_ADD;
	currentGame->myconfig.bonusProbSpeed = BONUS_PROB_SPEED;
	currentGame->myconfig.bonusProbExtraLife = BONUS_PROB_EXTRALIFE;
	currentGame->myconfig.bonusProbTriple = BONUS_PROB_TRIPLE;
	currentGame->myconfig.bonusSpeedChange = BONUS_SPEED_CHANGE;
	currentGame->myconfig.bonusSpeedDuration = BONUS_SPEED_DURATION;
	currentGame->myconfig.bonusSize.sizex = BONUS_SIZE_X;
	currentGame->myconfig.bonusSize.sizey = BONUS_SIZE_Y;
	ReleaseMutex(writeGameMutex);
	_tcscpy_s(currentGame->myconfig.file, TAM, TEXT("none"));//file input
	//end of default config

	//Users
	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->numUsers = 0;
	ReleaseMutex(writeGameMutex);
	for (i = 0; i < USER_MAX_USERS; i++) {
		resetUser(i);
		clientsInfo[i].hClientMsg = NULL;
		clientsInfo[i].hClientGame = NULL;
		clientsInfo[i].communication = -1;
	}
	//Balls
	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->numBalls = -1;
	ReleaseMutex(writeGameMutex);
	for (i = 0; i < BALL_MAX_BALLS; i++) {
		resetBall(i);
	}
	//Brick
	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->numBricks = 0;
	ReleaseMutex(writeGameMutex);
	for (i = 0; i < BRICK_MAX_BRICKS; i++) {
		resetBrick(i);
	}
	//bonus
	for(i = 0;i<BONUS_MAX_BONUS;i++)
		hTBonus[i] = NULL;

	_tcscpy_s(currentGame->top, TAM, TEXT(""));
	user userData;
	userData.score = -1;
	registry(userData);
	InvalidateRect(global_hWnd, NULL, TRUE);


	_tcscpy_s(gameState, TAM, TEXT("Server Initiated"));


	/*
	TCHAR str[TAM];
	TCHAR tmp[TAM];
	TCHAR tmp2[TAM];
	_itot_s(currentGame->myconfig.gameSize.sizex, tmp, TAM, 10);//translates num to str
	_itot_s(currentGame->myconfig.gameSize.sizey, tmp2, TAM, 10);//translates num to str
	_tcscpy_s(str, TAM, TEXT("lmites = "));
	_tcscat_s(str, TAM, tmp);//ads
	_tcscat_s(str, TAM, TEXT(","));
	_tcscat_s(str, TAM, tmp2);//ads
	MessageBox(global_hWnd, str, TEXT("DEGUB"), MB_OK);
	
	*/

	return 0;
}

void resetUser(DWORD id) {
	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->nUsers[id].lifes = -1;
	_stprintf_s(currentGame->nUsers[id].name, MAX_NAME_LENGTH, TEXT("empty"));
	currentGame->nUsers[id].posx = -1;
	currentGame->nUsers[id].posy = -1;
	currentGame->nUsers[id].score = -1;
	currentGame->nUsers[id].size.sizex = -1;
	currentGame->nUsers[id].size.sizey = -1;
	currentGame->nUsers[id].id = -1;
	ReleaseMutex(writeGameMutex);
}

void resetBall(DWORD id) {
	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->nBalls[id].id = -1;
	currentGame->nBalls[id].posx = -1;
	currentGame->nBalls[id].posy = -1;
	currentGame->nBalls[id].size.sizex = -1;
	currentGame->nBalls[id].size.sizey = -1;
	currentGame->nBalls[id].speed = -1;
	currentGame->nBalls[id].status = -1;
	hTBola[id] = INVALID_HANDLE_VALUE;
	ReleaseMutex(writeGameMutex);
}

void resetBrick(DWORD id) {
	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->nBricks[id].brinde.speed = -1;
	currentGame->nBricks[id].brinde.posx = -1;
	currentGame->nBricks[id].brinde.posy = -1;
	currentGame->nBricks[id].brinde.size.sizex = -1;
	currentGame->nBricks[id].brinde.size.sizey = -1;
	currentGame->nBricks[id].brinde.status = -1;
	currentGame->nBricks[id].brinde.type = -1;
	currentGame->nBricks->id = -1;
	currentGame->nBricks[id].posx = -1;
	currentGame->nBricks[id].posy = -1;
	currentGame->nBricks[id].size.sizex = -1;
	currentGame->nBricks[id].size.sizey = -1;
	currentGame->nBricks[id].status = -1;
	currentGame->nBricks[id].type = -1;
	ReleaseMutex(writeGameMutex);
}

void createBalls(DWORD num) {
	DWORD count = 0;
	DWORD threadId;
	int i;
	for (i = 0; i < BALL_MAX_BALLS; i++) {
		if (hTBola[i] == INVALID_HANDLE_VALUE) {//se handle is available
			hTBola[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BolaThread, (LPVOID)i, 0, &threadId);
			if (hTBola[i] == INVALID_HANDLE_VALUE) {
				MessageBox(global_hWnd, TEXT("Error creating BallThread thread"), TEXT("Error"), MB_ICONERROR | MB_OK);
				return;
			}
			else {
				WaitForSingleObject(writeGameMutex, INFINITE);
				currentGame->numBalls++;
				ReleaseMutex(writeGameMutex);
				count++;
			}
		}
		if (count >= num)
			break;
	}

	msg tmpMsg;
	TCHAR tmp[TAM];
	tmpMsg.codigoMsg = 101;
	tmpMsg.from = server_id;
	tmpMsg.to = 255;
	_itot_s(num, tmp, TAM, 10);
	_tcscpy_s(tmpMsg.messageInfo, TAM, tmp);
	sendMessage(tmpMsg);

	return;
}

void createBonus(DWORD brick_id) {
	drawSize tmpStruct;//not actually for size (using it to pass 2 int variables)
	for (int i = 0; i < BONUS_MAX_BONUS; i++) {
		if (hTBonus[i] == INVALID_HANDLE_VALUE || hTBonus[i] == NULL) {//se handle is available
			tmpStruct.sizex = i;//thread id
			tmpStruct.sizey = brick_id;//brick id
			hTBonus[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)bonusDrop, (LPVOID)&tmpStruct, 0, NULL);
			Sleep(5);//just enough so that the thread can save values;
			if (hTBonus[i] == NULL) {
				MessageBox(global_hWnd, TEXT("Error creating bonusDrop thread"), TEXT("Error"), MB_ICONERROR | MB_OK);
				break;
			}
			else
				break;
			
		}
	}
	return;
}

DWORD WINAPI BolaThread(LPVOID param) {

	srand((int)time(NULL));
	DWORD id = ((DWORD)param);
	int numberBrick = currentGame->numBricks;
	LARGE_INTEGER li;
	HANDLE hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->nBalls[id].id = id;
	currentGame->nBalls[id].status = 1;
	currentGame->nBalls[id].speed = currentGame->myconfig.ballInitialSpeed;
	currentGame->nBalls[id].size.sizex = currentGame->myconfig.ballSize.sizex;
	currentGame->nBalls[id].size.sizey = currentGame->myconfig.ballSize.sizey;
	ReleaseMutex(writeGameMutex);

	//waits random time so that all balls arent on top of each other
	if (currentGame->numBalls != 1) {
		ballWaitTime += 100;
		Sleep(ballWaitTime);
		ballWaitTime -= 100;
	}

	DWORD posx = currentGame->nUsers[0].posx + (currentGame->nUsers[0].size.sizex / 2);
	DWORD posy = currentGame->nUsers[0].posy - currentGame->nBalls[id].size.sizey;
	DWORD ballScore = 0;
	boolean flag, goingUp = 1, goingRight = (rand() % 2);
	do {
		ballScore = GetTickCount();
		flag = 0;
		//checks for bricks
		for (int i = 0; i < numberBrick; i++) {
			if (currentGame->nBricks[i].status <= 0)
				continue;


			if (posy - 1 == currentGame->nBricks[i].posy) {//tests
				hitBrick(i, id);
				goingUp = 0;
				posy++;
				continue;
			}



			//up
			if (goingUp) {
				if (posy - 1 == currentGame->nBricks[i].posy + currentGame->nBricks[i].size.sizey && currentGame->nBricks[i].posx <= posx && (currentGame->nBricks[i].posx + currentGame->nBricks[i].size.sizex) >= posx) {
					//MessageBox(global_hWnd, TEXT("hit up"), TEXT("debug"), MB_OK);
					goingUp = 0;
					hitBrick(i, id);
					continue;
				}
			}
			//down
			else{
				if (posy + currentGame->nBalls[id].size.sizey + 1 == currentGame->nBricks[i].posy && currentGame->nBricks[i].posx <= posx && (currentGame->nBricks[i].posx + currentGame->nBricks[i].size.sizex) >= posx) {
					//MessageBox(global_hWnd, TEXT("hit down"), TEXT("debug"), MB_OK);
					goingUp = 1;
					hitBrick(i, id);
					continue;
				}
			}

			if (goingRight) {
				if (posx + currentGame->nBalls[id].size.sizex + 1 == currentGame->nBricks[i].posx && 
					posy + currentGame->nBalls[id].size.sizey >= currentGame->nBricks[i].posy && posy <= currentGame->nBricks[i].posy + currentGame->nBricks[i].size.sizey) {
					goingRight = 0;
					hitBrick(i, id);
					continue;
				}

				//for some reason inside the brick
				/*if (posx + currentGame->nBalls[id].size.sizex > currentGame->nBricks[i].posx && posx < currentGame->nBricks[i].posx + currentGame->nBricks[i].size.sizex &&
					posy + currentGame->nBalls[id].size.sizey > currentGame->nBricks[i].posy && posy < currentGame->nBricks[i].posy + currentGame->nBricks[i].size.sizey)
					goingRight;*/

			}
			//left
			else{
				if (posx - 1 == currentGame->nBricks[i].posx && posy + currentGame->nBalls[id].size.sizey >= currentGame->nBricks[i].posy && 
					posy <= currentGame->nBricks[i].posy + currentGame->nBricks[i].size.sizey) {
					goingRight = 1;
					hitBrick(i, id);
					continue;
				}
			}

			/*if (posx + currentGame->nBalls[id].size.sizex > currentGame->nBricks[i].posx //direta da bola vs esquerda do tijolo
				&& posx < currentGame->nBricks[i].posx + currentGame->nBricks[i].size.sizex//esquerda da bola vs direita do tijolo
				&& posy < currentGame->nBricks[i].posy + currentGame->nBricks[i].size.sizey //cimo da bola vs baixo do tijolo
				&& posy + currentGame->nBalls[id].size.sizey > currentGame->nBricks[i].posy) {//baixo da bola vs cima do tijolo
				goingRight = !goingRight;
				goingUp = !goingUp;
				hitBrick(i, id);
				MessageBox(global_hWnd, TEXT("inside a brick"), TEXT("INFO"), MB_OK);
			}*/
		}

		//ball movement
		if (goingRight) {
			if (posx + currentGame->nBalls[id].size.sizex < currentGame->myconfig.gameSize.sizex ) {
				posx++;
			}
			else {
				posx--;
				goingRight = 0;
				//MessageBox(global_hWnd, TEXT("hit wall right"), TEXT("debug"), MB_OK);
			}
		}
		else {
			if (posx > 0) {
				posx--;
			}
			else {
				posx++;
				goingRight = 1;
			}
		}

		if (goingUp) {
			if (posy > 0) {
				posy--;
			}
			else {
				posy++;
				goingUp = 0;
			}
		}
		else {
			if (posy + currentGame->nBalls[id].size.sizey < currentGame->myconfig.gameSize.sizey) {

				for (int i = 0; i < currentGame->numUsers; i++) {//checks for player
					if (posy + currentGame->nBalls[id].size.sizey == currentGame->nUsers[i].posy && (posx >= currentGame->nUsers[i].posx && posx <= (currentGame->nUsers[i].posx + currentGame->nUsers[i].size.sizex))) {//atinge a barreira
						flag = 1;
						break;
					}
				}
				if (flag) {
					goingUp = 1;
					posy--;
				}
				else { posy++; }

			}
			else {//end of map
				//reset ball
				//MessageBox(global_hWnd, TEXT("end of ball"), TEXT("debug"), MB_OK);
				WaitForSingleObject(writeGameMutex, INFINITE);
				currentGame->nBalls[id].status = 0;
				currentGame->numBalls--;
				ReleaseMutex(writeGameMutex);
			}
		}

		if (localNumBricks <= 0) {
			WaitForSingleObject(writeGameMutex, INFINITE);
			currentGame->nBalls[id].status = 0;
			currentGame->numBalls--;
			ReleaseMutex(writeGameMutex);
		}
		else if (localNumBricks == -1)
			return;
		WaitForSingleObject(writeGameMutex, INFINITE);
		currentGame->nBalls[id].posx = posx;
		currentGame->nBalls[id].posy = posy;
		ReleaseMutex(writeGameMutex);

		if (currentGame->nBalls[id].status != 0) {
			WaitForSingleObject(writeGameMutex, INFINITE);
			currentGame->nBalls[id].status = 1;
			ReleaseMutex(writeGameMutex);
		}
			

		updateEveryone();
		//MessageBox(global_hWnd, TEXT("ball init pos + 1"), TEXT("debug"), MB_OK);
		WaitForSingleObject(writeGameMutex, INFINITE);
		for (int i = 0; i < currentGame->numUsers; i++)
			currentGame->nUsers[i].score += (GetTickCount() - ballScore) / 100;
		ReleaseMutex(writeGameMutex);
		

		//Sleep(currentGame->nBalls[id].speed);
		li.QuadPart = -(currentGame->nBalls[id].speed) * _MSECOND; // negative = relative time | 100-nanosecond units
		if (!SetWaitableTimer(hTimer, &li, 0, NULL, NULL, 0))
			MessageBox(global_hWnd, TEXT("Could not SetWaitbleTimer"), TEXT("warning"), MB_OK);

		if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0)
			MessageBox(global_hWnd, TEXT("Could not wait for timer on ball thread"), TEXT("warning"), MB_OK);

	} while (currentGame->nBalls[id].status);

	if (currentGame->numBalls == 0) {
		if (!localNumBricks) {
			currentGame->numBalls = 0;
			if (currentGame->gameStatus < currentGame->myconfig.gameNumLevels) {
				currentGame->gameStatus++;//next level
				createLevel(currentGame->gameStatus);
			}
			else {
				endGame();
			}
			
		}
		else {
			WaitForSingleObject(writeGameMutex, INFINITE);
			for (int i = 0; i < currentGame->numUsers; i++)
				currentGame->nUsers[i].lifes--;
			ReleaseMutex(writeGameMutex);
		}
	}
		
	resetBall(id);
	return 0;
}

DWORD WINAPI bonusDrop(LPVOID param) {
	drawSize* tmpStruct = (drawSize*)param;
	DWORD bonusThreadId = tmpStruct->sizex;
	DWORD brick_id = tmpStruct->sizey;

	LARGE_INTEGER li;
	HANDLE hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->nBricks[brick_id].brinde.size.sizex = currentGame->myconfig.bonusSize.sizex;
	currentGame->nBricks[brick_id].brinde.size.sizey = currentGame->myconfig.bonusSize.sizey;
	currentGame->nBricks[brick_id].brinde.speed = currentGame->myconfig.bonusDropSpeed;
	currentGame->nBricks[brick_id].brinde.posx = (currentGame->nBricks[brick_id].posx + (currentGame->nBricks[brick_id].size.sizex / 2)) - currentGame->nBricks[brick_id].brinde.size.sizex / 2;
	currentGame->nBricks[brick_id].brinde.posy = currentGame->nBricks[brick_id].posy + currentGame->nBricks[brick_id].size.sizey;
	currentGame->nBricks[brick_id].brinde.status = 1;
	ReleaseMutex(writeGameMutex);
	
	do {
		for (int i = 0; i < currentGame->numUsers; i++) {
			if ((currentGame->nUsers[i].posy) == currentGame->nBricks[brick_id].brinde.posy + currentGame->nBricks[brick_id].brinde.size.sizey) {
				if (currentGame->nBricks[brick_id].brinde.posx + currentGame->nBricks[brick_id].brinde.size.sizex >= currentGame->nUsers[i].posx && currentGame->nBricks[brick_id].brinde.posx <= (currentGame->nUsers[i].posx + currentGame->nUsers[i].size.sizex)) {
					WaitForSingleObject(writeGameMutex, INFINITE);
					currentGame->nBricks[brick_id].brinde.status = 0;
					ReleaseMutex(writeGameMutex);
					attributeBonus(i,brick_id);
					updateEveryone();
					CloseHandle(hTBonus[bonusThreadId]);
					hTBonus[bonusThreadId] = NULL;
					//MessageBox(global_hWnd, TEXT("user caught the bonus"), TEXT("debug"), MB_OK);
					return 1;
				}
			}
		}
		WaitForSingleObject(writeGameMutex, INFINITE);
		currentGame->nBricks[brick_id].brinde.posy++;
		ReleaseMutex(writeGameMutex);
		updateEveryone();

		li.QuadPart = -(currentGame->nBricks[brick_id].brinde.speed) * _MSECOND; // negative = relative time | 100-nanosecond units
		if (!SetWaitableTimer(hTimer, &li, 0, NULL, NULL, 0))
			MessageBox(global_hWnd, TEXT("Could not SetWaitbleTimer"), TEXT("warning"), MB_OK);

		if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0)
			MessageBox(global_hWnd, TEXT("Could not wait for timer on ball thread"), TEXT("warning"), MB_OK);

	} while (currentGame->nBricks[brick_id].brinde.posy + currentGame->nBricks[brick_id].brinde.size.sizey < currentGame->myconfig.gameSize.sizey - 1);

	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->nBricks[brick_id].brinde.status = 0;
	ReleaseMutex(writeGameMutex);
	CloseHandle(hTBonus[bonusThreadId]);
	hTBonus[bonusThreadId] = NULL;
	updateEveryone();
	return 0;
}

void hitBrick(DWORD brick_id, DWORD ball_id) {
	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->nBricks[brick_id].status--;
	ReleaseMutex(writeGameMutex);
	if (currentGame->nBricks[brick_id].status == 0) {
		localNumBricks--;
		InvalidateRect(global_hWnd, NULL, TRUE);
		WaitForSingleObject(writeGameMutex, INFINITE);
		for (int i = 0; i < currentGame->numUsers; i++)
			currentGame->nUsers[i].score += currentGame->myconfig.bonusScoreAdd;
		ReleaseMutex(writeGameMutex);
		if (currentGame->nBricks[brick_id].type == 3) {//magic
			createBonus(brick_id);
		}
	}
	ReleaseMutex(writeGameMutex);
}

int moveUser(DWORD id, TCHAR side[TAM]) {
	if (_tcscmp(side, TEXT("right")) == 0) {
		for (int i = 0; i < currentGame->numUsers; i++) {
			if (currentGame->nUsers[id].posx + currentGame->nUsers[id].size.sizex > currentGame->myconfig.gameSize.sizex)//map
				return 1;
			if (currentGame->nUsers[id].posx + currentGame->nUsers[id].size.sizex == currentGame->nUsers[i].posx)//player
				return 1;
		}
		WaitForSingleObject(writeGameMutex, INFINITE);
		currentGame->nUsers[id].posx += 10;
		ReleaseMutex(writeGameMutex);
	}
	else if (_tcscmp(side, TEXT("left")) == 0) {
		for (int i = 0; i < currentGame->numUsers; i++) {
			if (currentGame->nUsers[id].posx < 1)
				return 1;
			if (currentGame->nUsers[id].posx == currentGame->nUsers[i].posx + currentGame->nUsers[i].size.sizex)
				return 1;
		}
		WaitForSingleObject(writeGameMutex, INFINITE);
		currentGame->nUsers[id].posx -= 10;
		ReleaseMutex(writeGameMutex);
	}
	
	return 0;
}

void sendMessage(msg sendMsg) {
	print(sendMsg);
	if (sendMsg.to == 255) {
		for (int i = 0; i < USER_MAX_USERS; i++) {
			if (clientsInfo[i].communication == 0) {
				sendMessageDLL(sendMsg);
				SetEvent(clientsInfo[i].hClientMsg);
			}
			else if (clientsInfo[i].communication == 1) {
				sendMessagePipe(sendMsg, clientsInfo[i].hClientMsg);
			}//else
				//MessageBox(global_hWnd, TEXT("sendMessage() -> this one doesnt exist"), TEXT("warning"), MB_ICONWARNING | MB_OK);
				
		}
	}
	else {
		if (clientsInfo[sendMsg.to].communication == 0) {
			sendMessageDLL(sendMsg);
			SetEvent(clientsInfo[sendMsg.to].hClientMsg);
		}
		else {
			sendMessagePipe(sendMsg, clientsInfo[sendMsg.to].hClientMsg);
		}
	}
	
	if (sendMsg.to == (USER_MAX_USERS - 1)) {//responded that is full, resets space for other
		if (sendMsg.connection == 0)
			CloseHandle(clientsInfo[USER_MAX_USERS - 1].hClientMsg);
		else {
			CloseHandle(clientsInfo[USER_MAX_USERS - 1].hClientMsg);
			CloseHandle(clientsInfo[USER_MAX_USERS - 1].hClientGame);
		}
		clientsInfo[USER_MAX_USERS - 1].communication = -1;
	}
}

void sendMessagePipe(msg sendMsg, HANDLE hPipe) {
	HANDLE WriteReady;
	OVERLAPPED OverlWr = { 0 };
	DWORD bytesWritten;
	BOOLEAN fSuccess;

	WriteReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (WriteReady == NULL) {
		MessageBox(global_hWnd, TEXT("Error creating event WriteReady"), TEXT("Error"), MB_ICONERROR | MB_OK);
		return;
	}
	ZeroMemory(&OverlWr, sizeof(OverlWr));
	ResetEvent(WriteReady);
	OverlWr.hEvent = WriteReady;

	fSuccess = WriteFile(
		hPipe,
		&sendMsg,
		sizeof(msg),
		&bytesWritten,
		&OverlWr
	);

	WaitForSingleObject(WriteReady, INFINITE);
	//write done

	GetOverlappedResult(hPipe, &OverlWr, &bytesWritten, FALSE);
	if (bytesWritten < sizeof(msg)) {
		MessageBox(global_hWnd, TEXT("WriteFile failed, didnt write all of the bytes..."), TEXT("Error"), MB_ICONWARNING | MB_OK);
	}
	return;
}

DWORD createHandle(msg newMsg) {
	BOOLEAN flag = TRUE;
	int i;
	for (i = 0; i < USER_MAX_USERS - 1; i++) {//saves one to respond to clients that is full
		if (clientsInfo[i].communication == -1) {
			flag = FALSE;
			break;
		}
	}

	//i = MAX_CLIENTS - 1;//testing if server is full
	
	if (flag) {
		MessageBox(global_hWnd,TEXT("Could not find an available handle"), TEXT("Warning"), MB_OK);
		return -1;
	}

	if (newMsg.connection == 0) {
		clientsInfo[i].hClientMsg = CreateEvent(NULL, FALSE, FALSE, newMsg.messageInfo);
		clientsInfo[i].hClientGame = NULL;
		clientsInfo[i].communication = 0;
		return i;
	}
	else if (newMsg.connection == 1) {
		clientsInfo[i].hClientMsg = hTmpPipeMsg;
		hTmpPipeMsg = NULL;
		tmp_client_id = i;//next client for game pipe will be this on
		clientsInfo[i].communication = 1;
		return i;
	}

	return -1;
}

void createGame() {
	//TCHAR resp;
	////_tprintf(TEXT("Would you like to change defaut values for the game?(Y/N):"));
	//fflush(stdin);
	//resp = _gettch();
	//if (resp == 'y' || resp == 'Y') {
		/*int res = readFromFile();
		if (res) {
			//_tprintf(TEXT("Error changing default values"));
		}
		else {
			//_tprintf(TEXT("Default values changed wiht success!"));
		}*/
	//}
	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->gameStatus = 0;//users can now join
	currentGame->numUsers = 0;
	currentGame->numBalls = 0;
	ReleaseMutex(writeGameMutex);
	_tcscpy_s(gameState, TAM, TEXT("Game Created"));
	InvalidateRect(global_hWnd, NULL, TRUE);
}

void startGame(){
	//MessageBox(global_hWnd, TEXT("starting game"), TEXT("WARNING"), MB_OK);
	int i;
	msg tmpMsg;
	//start Users
	for (i = 0; i < currentGame->numUsers; i++) {
		userInit(i);
	}

	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->gameStatus = 4;
	ReleaseMutex(writeGameMutex);
	createLevel(currentGame->gameStatus);
	tmpMsg.codigoMsg = 100;//new game
	tmpMsg.from = server_id;
	tmpMsg.to = 255; //broadcast
	//Sleep(1000);
	updateEveryone();
	_tcscpy_s(tmpMsg.messageInfo, TAM, TEXT("gameStart"));
	sendMessage(tmpMsg);

	_tcscpy_s(gameState, TAM, TEXT("Game Started"));
	InvalidateRect(global_hWnd, NULL, TRUE);
	return;
}

void endGame() {
	msg outMsg;
	int top = -1; 
	//reseting game
	localNumBricks = -1;
	for (int i = 0; i < currentGame->numUsers; i++) {
		if (currentGame->nUsers[i].id == -1)
			continue;
		top = registry(currentGame->nUsers[i]);
		resetUser(i);
		if(top)
			MessageBox(global_hWnd, TEXT("New Top 10"), TEXT("Info"), MB_OK);
	}

	for (int i = 0; i < BRICK_MAX_BRICKS; i++) {
		resetBrick(i);
	}

	WaitForSingleObject(writeGameMutex, INFINITE);
	currentGame->numBalls = 0;
	currentGame->gameStatus = -1; //after game ends
	localNumBricks = 0;
	ReleaseMutex(writeGameMutex);

	_tcscpy_s(outMsg.messageInfo, TAM, TEXT("endOfGame"));
	outMsg.to = 255;
	outMsg.codigoMsg = 2;
	outMsg.from = server_id;
	sendMessage(outMsg);

	updateEveryone();

	MessageBox(global_hWnd, TEXT("Game Reseted"), TEXT("Info"), MB_ICONINFORMATION | MB_OK);

	_tcscpy_s(gameState, TAM, TEXT("Game Reseted"));
	InvalidateRect(global_hWnd, NULL, TRUE);
}

void createLevel(DWORD levelNum) {
	//create level
	//MessageBox(global_hWnd, TEXT("will create the level now"), TEXT("debug"), MB_OK);
	srand((int)time(NULL));
	int num = 0;
	int spaceToLim = 10;
	int spaceToBrick = 1;
	float randNum;
	HANDLE hMoveBricksThread;
	WaitForSingleObject(writeGameMutex, INFINITE);//this allows for the thread(movingBricks) only start when everything is done
	if (levelNum == 1)
		num = 15;
	else if (levelNum == 2)
		num = 30;
	else if (levelNum == 3) {
		hMoveBricksThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)movingBricks, (LPVOID)levelNum, 0, NULL);
		if (hMoveBricksThread == NULL) {
			MessageBox(global_hWnd,TEXT("Could not create movingbricks thread"),TEXT("Warning"), MB_ICONWARNING | MB_OK);
			return;
		}
		num = 20;
		spaceToBrick = 15;
		spaceToLim = 15;
	}
	else if (levelNum == 4) {
		hMoveBricksThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)movingBricks, (LPVOID)levelNum, 0, NULL);
		if (hMoveBricksThread == NULL) {
			MessageBox(global_hWnd, TEXT("Could not create movingbricks thread"), TEXT("Warning"), MB_ICONWARNING | MB_OK);
			return;
		}
		num = 12;
		spaceToBrick = 15;
		spaceToLim = 25;
	}

	int oposx = spaceToLim, oposy = 10;

	for (int i = 0; i < num; i++) {
		currentGame->nBricks[i].id = i;
		currentGame->nBricks[i].size.sizex = currentGame->myconfig.brickSize.sizex;
		currentGame->nBricks[i].size.sizey = currentGame->myconfig.brickSize.sizey;

		//type
		currentGame->nBricks[i].type = 1;//1 + rand() % 3;
		if (currentGame->nBricks[i].type == 1) {//normal
			currentGame->nBricks[i].status = 1;
		}
		else if (currentGame->nBricks[i].type == 2) {//resistent
			currentGame->nBricks[i].status = 2 + rand() % 3;
		}
		else if (currentGame->nBricks[i].type == 3) {//magic
			currentGame->nBricks[i].status = 1;
			randNum = 1 + rand() % 10;//random number between 1 and 10
			randNum = randNum / 10;
			if (randNum <= currentGame->myconfig.bonusProbSpeed) {
				currentGame->nBricks[i].brinde.type = 1;
				//MessageBox(global_hWnd, TEXT("speed"), TEXT("DEGUB"), MB_OK);
			}	
			else if (randNum <= currentGame->myconfig.bonusProbSpeed + currentGame->myconfig.bonusProbExtraLife) {
				currentGame->nBricks[i].brinde.type = 2;
				//MessageBox(global_hWnd, TEXT("extra life"), TEXT("DEGUB"), MB_OK);
			}	
			else {
				currentGame->nBricks[i].brinde.type = 3;
				//MessageBox(global_hWnd, TEXT("triple"), TEXT("DEGUB"), MB_OK);
			}
			
		}

		//pos
		
			if (!(oposx + currentGame->nBricks[i].size.sizex + spaceToLim < currentGame->myconfig.gameSize.sizex)) {
				oposx = spaceToLim;
				oposy += 25;
			}
			currentGame->nBricks[i].posx = oposx;
			currentGame->nBricks[i].posy = oposy;

			oposx += currentGame->nBricks[i].size.sizex + spaceToBrick;

			if (levelNum == 4) {
				oposx = spaceToLim;
				oposy += 25;
				levelNum = 0;
			}
		
	}
	currentGame->numBricks = num;
	ReleaseMutex(writeGameMutex);

	localNumBricks = num;
	updateEveryone();
	return;
}

void userInit(DWORD id) {
	WaitForSingleObject(writeGameMutex, INFINITE);
	if (id)
		currentGame->nUsers[id].posx = currentGame->nUsers[id - 1].posx + currentGame->nUsers[id - 1].size.sizex + 20;
	else
		currentGame->nUsers[id].posx = 20;

	currentGame->nUsers[id].posy = currentGame->myconfig.gameSize.sizey - currentGame->myconfig.userSize.sizey - 20;
	currentGame->nUsers[id].lifes = currentGame->myconfig.userNumLifes;
	currentGame->nUsers[id].size.sizex = currentGame->myconfig.userSize.sizex;
	currentGame->nUsers[id].size.sizey = currentGame->myconfig.userSize.sizey;
	currentGame->nUsers[id].score = 0;

	ReleaseMutex(writeGameMutex);
	return;
}

int registry(user userData) {
	//_tprintf(TEXT("Saving user %s with the score of %d\n"), userData.name, userData.score);
	HKEY chave;
	DWORD regOutput;
	TCHAR info[TAM];
	TCHAR name[TAM];
	TCHAR tmp[TAM];
	TCHAR user_name[TAM];
	TCHAR tmp_2[TAM];
	DWORD score = 0, tam = MAX_NAME_LENGTH;
	int flag, flag_2;
	BOOLEAN value = 0;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\SO2_TP"), 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &chave, &regOutput) != ERROR_SUCCESS) {
		MessageBox(global_hWnd, TEXT("Error Creating/Opening they key"), TEXT("Warning"), MB_ICONWARNING | MB_OK);
		return -1;
	}

	if (regOutput == REG_CREATED_NEW_KEY) {//if there is no scores saved
		for (int i = 0; i < 10; i++) {
			_tcscpy_s(name, TAM, TEXT("HighScore"));
			_itot_s(i, info, TAM, 10);
			_tcscat_s(name, TAM, info);
			_stprintf_s(info, MAX_NAME_LENGTH, TEXT("none:"));
			_tcscat_s(info, TAM, TEXT("0"));
			RegSetValueEx(chave, name, 0, REG_SZ, (LPBYTE)("%s", info), _tcslen(("%s", info)) * sizeof(TCHAR));
		}
		MessageBox(global_hWnd, TEXT("Top 10 Created"), TEXT("Info"), MB_ICONINFORMATION | MB_OK);
		return 1;
	}
	else if (regOutput == REG_OPENED_EXISTING_KEY) {//if there are scores
		WaitForSingleObject(writeGameMutex, INFINITE);
		ReleaseMutex(writeGameMutex);
		for (int i = 0; i < 10; i++) {
			tam = MAX_NAME_LENGTH;
			flag_2 = 0;
			_tcscpy_s(name, TAM, TEXT("HighScore"));
			_itot_s(i, info, TAM, 10);
			_tcscat_s(name, TAM, info);
			RegQueryValueEx(chave, name, NULL, NULL, (LPBYTE)info, &tam);
			if (userData.score == -1) {
				//MessageBox(global_hWnd, info, name, MB_OK);	
				_tcscpy_s(tmp, TAM, name);
				_tcscat_s(tmp, TAM, TEXT(" ->  "));
				_tcscat_s(tmp, TAM, info);
				_tcscat_s(tmp, TAM, TEXT("|"));
				WaitForSingleObject(writeGameMutex, INFINITE);
				_tcscat_s(currentGame->top, TAM, tmp);
				ReleaseMutex(writeGameMutex);
				continue;
			}

			for (flag = 0; flag < MAX_NAME_LENGTH; flag++) {
				if (info[flag] == ':')
					break;
				user_name[flag] = info[flag];
			}
			user_name[flag] = '\0';//end of user_name
			flag++;
			for (; flag < MAX_NAME_LENGTH; flag++) {
				if (info[flag] == '\0')
					break;
				tmp[flag_2] = info[flag];
				flag_2++;
			}
			tmp[flag_2] = '\0';//end of score
			score = _tstoi(tmp);//tranlate

			if (userData.score > score) {
				value = 1;
				_tcscpy_s(tmp, TAM, userData.name);
				_tcscat_s(tmp, TAM, TEXT(":"));
				_itot_s(userData.score, tmp_2, TAM, 10);
				_tcscat_s(tmp, TAM, tmp_2);
				RegSetValueEx(chave, name, 0, REG_SZ, (LPBYTE)("%s", tmp), _tcslen(("%s", tmp)) * sizeof(TCHAR));
				userData.score = score;
				_tcscpy_s(userData.name, MAX_NAME_LENGTH, user_name);
			}
		}
	}
	RegCloseKey(chave);
	return value;
}

void securityPipes(SECURITY_ATTRIBUTES * sa)
{

	PSECURITY_DESCRIPTOR pSD;
	PACL pAcl;
	EXPLICIT_ACCESS ea;
	PSID pEveryoneSID = NULL, pAdminSID = NULL;
	SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
	TCHAR str[256];

	pSD = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR,
		SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (pSD == NULL) {
		_tprintf(TEXT("Erro LocalAlloc!!!"));
		return;
	}
	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION)) {
		_tprintf(TEXT("Erro IniSec!!!"));
		return;
	}

	// Create a well-known SID for the Everyone group.
	if (!AllocateAndInitializeSid(&SIDAuthWorld, 1, SECURITY_WORLD_RID,
		0, 0, 0, 0, 0, 0, 0, &pEveryoneSID))
	{
		_stprintf_s(str, 256, TEXT("AllocateAndInitializeSid() error %u"), GetLastError());
		_tprintf(str);
		Cleanup(pEveryoneSID, pAdminSID, NULL, pSD);
	}
	else
		_tprintf(TEXT("AllocateAndInitializeSid() for the Everyone group is OK"));

	ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));

	ea.grfAccessPermissions = GENERIC_READ | GENERIC_WRITE;
	ea.grfAccessMode = SET_ACCESS;
	ea.grfInheritance = SUB_CONTAINERS_AND_OBJECTS_INHERIT;
	ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
	ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
	ea.Trustee.ptstrName = (LPTSTR)pEveryoneSID;

	if (SetEntriesInAcl(1, &ea, NULL, &pAcl) != ERROR_SUCCESS) {
		_tprintf(TEXT("Erro SetAcl!!!"));
		return;
	}

	if (!SetSecurityDescriptorDacl(pSD, TRUE, pAcl, FALSE)) {
		_tprintf(TEXT("Erro IniSec!!!"));
		return;
	}

	sa->nLength = sizeof(*sa);
	sa->lpSecurityDescriptor = pSD;
	sa->bInheritHandle = TRUE;
}

void Cleanup(PSID pEveryoneSID, PSID pAdminSID, PACL pACL, PSECURITY_DESCRIPTOR pSD)
{
	if (pEveryoneSID)
		FreeSid(pEveryoneSID);
	if (pAdminSID)
		FreeSid(pAdminSID);
	if (pACL)
		LocalFree(pACL);
	if (pSD)
		LocalFree(pSD);
}

void updateEveryone() {
	WaitForSingleObject(writeGameMutex, INFINITE);
	*gameClient = *currentGame;
	ReleaseMutex(writeGameMutex);

	SetEvent(updateRemoteGame);
	ResetEvent(updateRemoteGame);

	SetEvent(updateLocalGame);
	ResetEvent(updateLocalGame);

	return;
}

void attributeBonus(DWORD client_id, DWORD brick_id) {
	WaitForSingleObject(writeGameMutex, INFINITE);
	if (currentGame->nBricks[brick_id].brinde.type == 1) {//speed up
		if (currentGame->myconfig.ballInitialSpeed - currentGame->myconfig.bonusSpeedChange >= BALL_MAX_SPEED) {
			currentGame->myconfig.ballInitialSpeed -= currentGame->myconfig.bonusSpeedChange;

			for (int i = 0; i < currentGame->numBalls; i++)
				currentGame->nBalls[i].speed = currentGame->myconfig.ballInitialSpeed;
		}

	}else if (currentGame->nBricks[brick_id].brinde.type == 2) {//extra life
		currentGame->nUsers[client_id].lifes++;//only the one that caught it
	}
	else if (currentGame->nBricks[brick_id].brinde.type == 3) {//triple
		if(currentGame->numBalls > 0)
			createBalls(3);
		else
			for (int i = 0; i < currentGame->numUsers; i++)
				currentGame->nUsers[i].lifes++;

	}
	ReleaseMutex(writeGameMutex);
	return;
}

DWORD WINAPI movingBricks(LPVOID param) {
	WaitForSingleObject(writeGameMutex, INFINITE);//waits for the level to created
	ReleaseMutex(writeGameMutex);
	DWORD level = ((DWORD)param);
	LARGE_INTEGER li;
	HANDLE hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	srand((int)time(NULL));
	int brick_id = 0;
	int goRight = 0;;
	do {
		do {
			if (level == 3) {
				brick_id = 1 + rand() % currentGame->numBricks;//choses a random brick
				if (currentGame->nBricks[brick_id].status <= 0)
					continue;
				goRight = 1 + rand() % 2;//choses a random direction
			}
			else if (level == 4) {
				if (currentGame->nBricks[brick_id].status <= 0)
					return;
			}
			for (int i = 0; i < 10; i++) {
				if (goRight) {
					if (currentGame->nBricks[brick_id].posx + currentGame->myconfig.brickSize.sizex + 1 == currentGame->nBricks[brick_id + 1].posx || //if there is a brick
						currentGame->nBricks[brick_id].posx + currentGame->myconfig.brickSize.sizex + 1 == currentGame->myconfig.gameSize.sizex) {//end of map
							if (level == 3)
								break;
							else if (level == 4)
								goRight = 0;
					}
					else {
						WaitForSingleObject(writeGameMutex, INFINITE);
						currentGame->nBricks[brick_id].posx++;
						ReleaseMutex(writeGameMutex);
					}
						
				}
				//left
				else {
					if (currentGame->nBricks[brick_id].posx - 1 == currentGame->nBricks[brick_id + 1].posx + currentGame->myconfig.brickSize.sizex || //if there is a brick
						currentGame->nBricks[brick_id].posx + 1 == 0) {//end of map
							if (level == 3)
								break;
							else if (level == 4)
								goRight = 1;
					}
					else {
						WaitForSingleObject(writeGameMutex, INFINITE);
						currentGame->nBricks[brick_id].posx--;
						ReleaseMutex(writeGameMutex);
					}			
				}
			}
			break;//did what had to be done
		} while (1);
		updateEveryone();
		//wait to move another brick
		li.QuadPart = -(250) * _MSECOND; // negative = relative time | 100-nanosecond units
		if (!SetWaitableTimer(hTimer, &li, 0, NULL, NULL, 0))
			MessageBox(global_hWnd, TEXT("Could not SetWaitbleTimer"), TEXT("warning"), MB_OK);

		if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0)
			MessageBox(global_hWnd, TEXT("Could not wait for timer on moving brick thread"), TEXT("warning"), MB_OK);

	} while (localNumBricks);

	//MessageBox(global_hWnd, TEXT("exiting moveing brick thread"), TEXT("DEGUB"), MB_OK);

	return 0;
}