#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include <strsafe.h>
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
void assignBrick(DWORD num);
void userInit(DWORD id);
void createBalls(DWORD num);
void resetUser(DWORD id);
void resetBall(DWORD id);
void resetBrick(DWORD id);
void hitBrick(DWORD brick_id, DWORD ball_id);
void createBonus(DWORD id);
int moveUser(DWORD id, TCHAR side[TAM]);
void securityPipes(SECURITY_ATTRIBUTES * sa);
void Cleanup(PSID pEveryoneSID, PSID pAdminSID, PACL pACL, PSECURITY_DESCRIPTOR pSD);


//vars
TCHAR szProgName[] = TEXT("Server Base");
HWND global_hWnd = NULL;
DWORD localNumBricks;
TCHAR inTxt[TAM];
TCHAR outTxt[TAM];
TCHAR gameState[TAM];

//client related
comuciationHandle clientsInfo[USER_MAX_USERS];
DWORD tmp_client_id = 0;
HANDLE hTmpPipeMsg = NULL;
HANDLE hTmpPipeGame = NULL;

//server related
HANDLE updateGame;//event to update game for pipe users
HANDLE updateLocalGame;//event to update game for local users
HANDLE messageEventServer;//event to update server of dll msg
HANDLE hResolveMessageMutex;//handle to resolveMessage function
DWORD server_id;
HANDLE hTBola[BALL_MAX_BALLS];
HANDLE hTBonus[BONUS_MAX_BONUS];

int maxX = 0, maxY = 0;
HDC memDC = NULL;
HBITMAP hBit = NULL;
HBRUSH hBrush = NULL;

pgame gameInfo;//has game Information

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPCTSTR lpCmdLine, int nCmdShow) {
	
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

	ShowWindow(hWnd, nCmdShow);
	global_hWnd = hWnd;
	UpdateWindow(hWnd); // Refrescar a janela (Windows envia à janela uma

	//-------------------------------------------------program-------------------------------------
	DWORD res;
	srand((int)time(NULL));

	/*HANDLE checkExistingServer;
	checkExistingServer = CreateEvent(NULL, FALSE, NULL, CHECK_SERVER_EVENT);
	checkExistingServer = OpenEvent(NULL,NULL, CHECK_SERVER_EVENT);
	if (checkExistingServer == NULL) {//there is no server created
		checkExistingServer = CreateEvent(NULL, FALSE, NULL, CHECK_SERVER_EVENT);
	}
	else {
		MessageBox(hWnd, TEXT("There is a server running at this moment."), TEXT("WARNING"), MB_OK);
		PostQuitMessage(0);
	}
	*/

	updateGame = CreateEvent(NULL, FALSE, FALSE, NULL);
	updateLocalGame = CreateEvent(NULL, TRUE, FALSE, LOCAL_UPDATE_GAME);
	messageEventServer = CreateEvent(NULL, FALSE, FALSE, MESSAGE_EVENT_NAME);
	updateBalls = CreateEvent(NULL, TRUE, FALSE, BALL_EVENT_NAME);//updates ball of local
	updateBonus = CreateEvent(NULL, FALSE, FALSE, BONUS_EVENT_NAME);//updates bonus of local
	hResolveMessageMutex = CreateMutex(NULL, FALSE, NULL);
	if (updateGame == NULL || messageEventServer == NULL || updateBalls == NULL || updateBonus == NULL || hResolveMessageMutex == NULL || updateLocalGame == NULL) {
		MessageBox(global_hWnd, TEXT("Error creating resources..."), TEXT("Resources"), MB_OK);
		PostQuitMessage(1);
	}

	//DLL(Memory)
	//Message
	createSharedMemoryMsg();
	//Game
	gameInfo = createSharedMemoryGame();

	HANDLE hTReceiveMessage = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveLocalMsg, NULL, 0, NULL);
	if (hTReceiveMessage == NULL) {
		return -1;
	}
	//pipes for initial connection
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

	_tcscpy_s(gameInfo->myconfig.file, TAM, TEXT("none"));//file input

	//starts gameInfo vars
	res = startVars();
	if (res) {
		//_tprintf(TEXT("Erro ao iniciar as variaveis!"));
	}


	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg); // Pré-processamento da mensagem (p.e. obter código
	   // ASCII da tecla premida)
		DispatchMessage(&lpMsg); // Enviar a mensagem traduzida de volta ao Windows, que
	   // aguarda até que a possa reenviar à função de
	   // tratamento da janela, CALLBACK TrataEventos (abaixo)
	}
	// ============================================================================
	// 6. Fim do programa
	// ============================================================================
	return((int)lpMsg.wParam); // Retorna sempre o parâmetro wParam da estrutura lpMsg
}

LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	//draw
	HDC hDC;
	PAINTSTRUCT ps;

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
		//BitBlt(memDC, xBitMap, yBitMap, bmp.bmWidth, bmp.bmHeight, tempDC, 0, 0, SRCCOPY);
		ReleaseDC(hWnd, hDC);
		break;

	case WM_KEYDOWN:
		break;
	case WM_PAINT:
		SetBkMode(memDC, TRANSPARENT);
		SetTextColor((HDC)memDC, RGB(255, 255, 255));
		SetStretchBltMode(memDC, COLORONCOLOR);
		PatBlt(memDC, 0, 0, maxX, maxY, PATCOPY); // fill background
		//StretchBlt(memDC, 0, 0, 800, 700, hdcBackground, 0, 0, bmBackground.bmWidth, bmBackground.bmHeight, SRCCOPY);
		
		TextOut(memDC, 0, 10, gameState, _tcslen(gameState));
		TextOut(memDC, 0, 30, inTxt, _tcslen(inTxt));
		TextOut(memDC, 0, 45, outTxt, _tcslen(outTxt));

		hDC = BeginPaint(hWnd, &ps);
		BitBlt(hDC, 0, 0, maxX, maxY, memDC, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		break;

	case WM_COMMAND:
		resolveMenu(hWnd, messg, wParam, lParam);
		break;
	case WM_CLOSE:
		/*res = MessageBox(hWnd, TEXT("Are you sure you want to quit?"), TEXT("Quit MessageBox"), MB_ICONQUESTION | MB_YESNO);
		if (res == IDYES)*/
			DestroyWindow(hWnd);
		break;
	case WM_DESTROY: // Destruir a janela e terminar o programa
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, messg, wParam, lParam);
		break;
	}
	return(0);
}

LRESULT CALLBACK resolveMenu(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	TCHAR tmp[TAM];
	switch (LOWORD(wParam)) {
		case ID_CREATEGAME:
			if (gameInfo->gameStatus == -1) {
				createGame();
			}
			else
				MessageBox(global_hWnd, TEXT("Cant create a game right now"), TEXT("Warning"), MB_OK);
			break;
		case ID_STARTGAME:
			if (gameInfo->numUsers > 0 && gameInfo->gameStatus == 0) {
				MessageBox(global_hWnd, TEXT("WILL START GAME AFTER OK"), TEXT("GAME START"), MB_OK);
				startGame();
			}
			break;
		case ID_TOP10:
			assignBrick(30);
			//SetEvent(updateLocalGame);
			MessageBeep(MB_ICONSTOP);
			break;
		case ID_USERSLOGGED:
			break;
		case ID_SETTINGS:
			_itot_s(gameInfo->numUsers, tmp, TAM, 10);//translates num to str
			MessageBox(global_hWnd, tmp, TEXT("Number of users:"), MB_OK);
			_itot_s(gameInfo->gameStatus, tmp, TAM, 10);//translates num to str
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

	////_tprintf(TEXT("[%d]NewMsg(%d):\%s\n-from:%d\n-to:%d\n"), quant, inMsg.codigoMsg, inMsg.messageInfo, inMsg.from, inMsg.to);
	//init
	if (inMsg.codigoMsg == -9999) {
		num = createHandle(inMsg);
		_itot_s(num, tmp, TAM, 10);//translates num to str
		_tcscpy_s(outMsg.messageInfo, TAM, tmp);//copys client_id to messageInfo
		//MessageBox(global_hWnd, tmp, TEXT("client number:"), MB_OK);
		if (num == USER_MAX_USERS - 1) {//full
			outMsg.codigoMsg = -9999;
		}
		else if (num >= 0) {//success
			outMsg.codigoMsg = 9999;
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
		if (gameInfo->gameStatus == -1) {
			//_tprintf(TEXT("User(%s) tried to login with no game created\n"), newMsg.messageInfo);
			outMsg.codigoMsg = -100;//no game created
			outMsg.to = inMsg.from;
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("noGameCreated"));
			//MessageBox(global_hWnd, TEXT("Sent game not created"), TEXT("Response:"), MB_OK);
			sendMessage(outMsg);
			return 1;
		}
		else if (gameInfo->gameStatus == 1) {
			outMsg.codigoMsg = -110;//game already in progress
			outMsg.to = inMsg.to;
			//hClients[inMsg.from] to spectate
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("gameInProgress"));
			//_tprintf(TEXT("Sent game in progress\n"));
			sendMessage(outMsg);
			return 1;
		}
		else if (_tcscmp(inMsg.messageInfo, TEXT("nop")) == 0) {//for tests
			outMsg.codigoMsg = -1;//not successful
			outMsg.to = inMsg.from;
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("youAreBanned"));
			sendMessage(outMsg);
			return 1;
		}
		for (int i = 0; i < gameInfo->numUsers; i++) {
			if (_tcscmp(gameInfo->nUsers[i].name, inMsg.messageInfo) == 0) {
				//_tprintf(TEXT("This user is already logged in\n"));
				flag = 0;
				outMsg.codigoMsg = -1;//not successful
				////_tprintf(TEXT("Sent user not accepted created\n"));
				sendMessage(outMsg);
				break;
			}
		}
		if (flag) {
			_tcscpy_s(gameInfo->nUsers[gameInfo->numUsers].name, MAX_NAME_LENGTH, inMsg.messageInfo);
			gameInfo->nUsers[gameInfo->numUsers].id = inMsg.from;
			//_tprintf(TEXT("sending message with sucess to user_id:%d\n"), gameInfo->nUsers[gameInfo->numUsers].user_id);
			outMsg.codigoMsg = 1;//sucesso
			gameInfo->numUsers++;
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("loginSucess"));
			sendMessage(outMsg);
		}
		else {
			//already have an user like you
			outMsg.codigoMsg = -1;
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("alreadyHaveThisUser"));
			sendMessage(outMsg);
		}
	}
	else if (inMsg.codigoMsg == 2) {//end of user
		//_tprintf(TEXT("%s[%d] exited with the score of:%d\n"), gameInfo->nUsers[inMsg.from].name, gameInfo->nUsers[inMsg.from].user_id, gameInfo->nUsers[inMsg.from].score);
		/*int res = registry(gameInfo->nUsers[inMsg.from]);
		if (res) {
			//_tprintf(TEXT("new high score saved!\n"));
		}
		else {
			//_tprintf(TEXT("not enough for top 10!\n"));
		}
		*/
		Sleep(250);
		resetUser(inMsg.from);
		gameInfo->numUsers--;
		outMsg.to = inMsg.from;
		outMsg.codigoMsg = 2;
		_tcscpy_s(outMsg.messageInfo, TAM, TEXT("userEnd"));
		sendMessage(outMsg);

		if (gameInfo->numUsers == 0) {//closing game
			//_tprintf(TEXT("Reseting game...\n"));
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("End of Game"));
			outMsg.to = inMsg.from;
			outMsg.codigoMsg = -999;
			sendMessage(outMsg);

			for (int i = 0; i < BALL_MAX_BALLS; i++) {
				TerminateThread(hTBola[i], 1);
				CloseHandle(hTBola[i]);
				resetBall(i);
			}
			for (int i = 0; i < BRICK_MAX_BRICKS; i++) {
				resetBrick(i);
			}
			gameInfo->numBalls = 0;
			gameInfo->gameStatus = -1; //after game ends
			localNumBricks = 0;
			Sleep(1000);
			//_tprintf(TEXT("Game reseted!\n"));
		}
		//else
			//_tprintf(TEXT("There are still players in the game!\n"));
	}
	else if (inMsg.codigoMsg == 101 && gameInfo->gameStatus == 1) {
		if (gameInfo->nUsers[inMsg.from].lifes > 0 && gameInfo->numBalls == 0)
			createBalls(1);
	}
	else if (inMsg.codigoMsg == 200 && gameInfo->gameStatus == 1) {//user trying to move
		int res;
		if (_tcscmp(inMsg.messageInfo, TEXT("right")) != 0 && _tcscmp(inMsg.messageInfo, TEXT("left")) != 0) {
			//MessageBox(global_hWnd, inMsg.messageInfo, TEXT("user would like to move to:"), MB_OK);
			int moveToPos = _tstoi(inMsg.messageInfo);//tranlate
			if(moveToPos > gameInfo->nUsers[inMsg.from].posx)
				_tcscpy_s(inMsg.messageInfo, TAM, TEXT("right"));
			else
				_tcscpy_s(inMsg.messageInfo, TAM, TEXT("left"));
			do {
				if (_tcscmp(inMsg.messageInfo, TEXT("right")) == 0 && moveToPos <= gameInfo->nUsers[inMsg.from].posx){
					//MessageBox(global_hWnd, TEXT("end of move"), TEXT("user move"), MB_OK);
					break;
				}
				else if (_tcscmp(inMsg.messageInfo, TEXT("left")) == 0 && moveToPos >= gameInfo->nUsers[inMsg.from].posx) {
					//MessageBox(global_hWnd, TEXT("end of move"), TEXT("user move"), MB_OK);
					break;
				}

				res = moveUser(inMsg.from, inMsg.messageInfo);
				if (res) {//if cant
					break;
					//MessageBox(global_hWnd, TEXT("not sucess"), TEXT("user would like to move to:"), MB_OK);
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
	SetEvent(updateLocalGame);
	ResetEvent(updateLocalGame);
	SetEvent(updateGame);
	return -1;
}

//thread to receive initial clients via pipe for msg
DWORD WINAPI connectPipeMsg(LPVOID param) {
	BOOLEAN fConnected = 0;
	HANDLE hPipeInit = INVALID_HANDLE_VALUE;
	HANDLE hThread;
	//DWORD nSent;

	SECURITY_ATTRIBUTES sa;
	securityPipes(&sa);

	do {
		hPipeInit = CreateNamedPipe(INIT_PIPE_MSG_NAME,
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
			//_tprintf(TEXT("Erro ao iniciar pipe!"));
			return 0;
		}
		else
			//_tprintf(TEXT("Pipe created with success\n"));

		//waits for client
		fConnected = ConnectNamedPipe(hPipeInit, NULL);//waits for client
		if (!fConnected && GetLastError() == ERROR_PIPE_CONNECTED)
			fConnected = 1;

		MessageBox(global_hWnd, TEXT("Got a Msg Client"), TEXT("Client"), MB_OK);

		if (fConnected) {
			hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)pipeClientMsg, (LPVOID)hPipeInit, 0, NULL);
			if (hThread == NULL) {
				//_tprintf(TEXT("Erro a criar thread para o pipe do user\n"));
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

		hPipeInit = CreateNamedPipe(INIT_PIPE_GAME_NAME,
			PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // bloqueante (não usar PIPE_NOWAIT nem mesmo em Asyncr)
			PIPE_UNLIMITED_INSTANCES, // max. instancias (255)
			BUFSIZE_GAME, // tam buffer output
			BUFSIZE_GAME, // tam buffer input
			2000, // time-out p/ cliente 5k milisegundos (0->default=50)
			&sa);

		if (hPipeInit == INVALID_HANDLE_VALUE) {
			//_tprintf(TEXT("Erro ao iniciar pipe!"));
			return 0;
		}
		else
			//_tprintf(TEXT("Pipe for game created with success\n"));

		//waits for client
		//_tprintf(TEXT("Waiting for a client!\n"));
		fConnected = ConnectNamedPipe(hPipeInit, NULL);
		if (!fConnected && GetLastError() == ERROR_PIPE_CONNECTED)
			fConnected = 1;
		
		MessageBox(global_hWnd, TEXT("client for game"), TEXT("game"), MB_OK);

		if (fConnected) {
			clientsInfo[tmp_client_id].hClientGame = hPipeInit;//got a new client to send the game to
		}
		else {
			CloseHandle(hPipeInit);
		}

		//SetEvent(updateGame);
	} while (1);

	return 0;
}

//thread to receive each client msg
DWORD WINAPI pipeClientMsg(LPVOID param) {
	msg inMsg;
	DWORD cbBytesRead;
	//DWORD cbWritten,cbReplyBytes;
	DWORD resp;
	BOOLEAN fSuccess;
	HANDLE hPipe = (HANDLE)param; // a informação enviada à thread é o handle do pipe
	if (hPipe == NULL) {
		//_tprintf(TEXT("\nErro – o handle enviado no param da thread é nulo"));
		return -1;
	}

	hTmpPipeMsg = hPipe;//used to CreateHandle()

	HANDLE ReadReady;
	OVERLAPPED OverlRd = { 0 };

	ReadReady = CreateEvent(
		NULL, // default security + non inheritable
		TRUE, // Reset manual, por requisito do overlapped IO => uso de ResetEvent
		FALSE, // estado inicial = not signaled
		NULL); // não precisa de nome: uso interno ao processo
	if (ReadReady == NULL) {
		//_tprintf(TEXT("\nServidor: não foi possível criar o evento Read. Mais vale parar já"));
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
			//_tprintf(TEXT("\nReadFile não leu os dados todos. Erro = %d"), GetLastError()); // acrescentar lógica de encerrar a thread cliente

		//processa info recebida
		WaitForSingleObject(hResolveMessageMutex, INFINITE);
		resp = resolveMessage(inMsg);
		//SetEvent(updateGame);
		ReleaseMutex(hResolveMessageMutex);
	}

	//end client
	//removeCliente(hPipe);
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe); // Desliga servidor da instância
	CloseHandle(hPipe); // Fecha este lado desta instância
	//_tprintf(TEXT("\nThread dedicada Cliente a terminar"));
	return 1;
}

//thread to send the game to each remote user
DWORD WINAPI updateGamePipe(LPVOID param) {

	DWORD bytesWritten;
	BOOLEAN fSuccess;

	int i;
	do {
		WaitForSingleObject(updateGame, INFINITE);
		//_tprintf(TEXT("Will update game now\n"));
		for (i = 0; i < USER_MAX_USERS; i++)
			if(clientsInfo[i].communication == 0){
				//SetEvent() lets know that the game is ready to be used(should i?)
			}
			else if (clientsInfo[i].hClientGame != NULL && clientsInfo[i].communication == 1) {
				//sends game
				//_tprintf(TEXT("This is the size of the game = %d | also there are %d bricks\n"), sizeof(tmp_game),tmp_game.numBricks);
				//_tprintf(TEXT("Updating game for client number %d\n"), i);
				fSuccess = WriteFile(
					clientsInfo[i].hClientGame,
					gameInfo,
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
		//MessageBox(global_hWnd, TEXT("got event messageEventServer"), TEXT("warning"), MB_OK);
		//for (int i = 0; i < 3; i++) {//show handels
			//_itot_s(clientsInfo[i].communication, tmp, TAM, 10);//translates num to str
			//MessageBox(global_hWnd, tmp, TEXT("client[i].communication = "), MB_OK);
		//}
		inMsg = receiveMessageDLL();
		print(inMsg);
		resolveMessage(inMsg);
		ReleaseMutex(hResolveMessageMutex);
		
		//Sleep(5000);
	} while (1);
}

DWORD startVars() {
	RECT tmpRect;
	GetWindowRect(global_hWnd, &tmpRect);
	int i;
	server_id = 254;

	gameInfo->gameStatus = -1;
	//default config
	gameInfo->myconfig.file;
	//game
	gameInfo->myconfig.gameNumLevels = GAME_LEVELS;
	gameInfo->myconfig.gameSize.sizex = GAME_SIZE_X - 15;//this values are weird
	gameInfo->myconfig.gameSize.sizey = GAME_SIZE_Y  - 60;
	//user
	gameInfo->myconfig.userMaxUsers = USER_MAX_USERS;
	gameInfo->myconfig.userNumLifes = USER_LIFES;
	gameInfo->myconfig.userSize.sizex = USER_SIZE_X;
	gameInfo->myconfig.userSize.sizey = USER_SIZE_Y;
	//ball
	gameInfo->myconfig.ballInitialSpeed = BALL_SPEED;
	gameInfo->myconfig.ballMaxBalls = BALL_MAX_BALLS;
	gameInfo->myconfig.ballMaxSpeed = BALL_MAX_SPEED;
	gameInfo->myconfig.ballSize.sizex = BALL_SIZE;
	gameInfo->myconfig.ballSize.sizey = BALL_SIZE;
	//bricks
	gameInfo->myconfig.brickMaxBricks = BRICK_MAX_BRICKS;
	gameInfo->myconfig.brickSize.sizex = BRICK_SIZE_X;
	gameInfo->myconfig.brickSize.sizey = BRICK_SIZE_Y;
	//bonus
	gameInfo->myconfig.bonusSpeed = BONUS_SPEED;
	gameInfo->myconfig.bonusScoreAdd = BONUS_SCORE_ADD;
	gameInfo->myconfig.bonusProbSpeed = BONUS_PROB_SPEED;
	gameInfo->myconfig.bonusProbExtraLife = BONUS_PROB_EXTRALIFE;
	gameInfo->myconfig.bonusProbTriple = BONUS_PROB_TRIPLE;
	gameInfo->myconfig.bonusSpeedChange = BONUS_SPEED_CHANGE;
	gameInfo->myconfig.bonusSpeedDuration = BONUS_SPEED_DURATION;
	gameInfo->myconfig.bonusSize.sizex = BONUS_SIZE_X;
	gameInfo->myconfig.bonusSize.sizey = BONUS_SIZE_Y;
	//end of default config

	//Users
	gameInfo->numUsers = 0;
	for (i = 0; i < USER_MAX_USERS; i++) {
		resetUser(i);
		clientsInfo[i].hClientMsg = NULL;
		clientsInfo[i].hClientGame = NULL;
		clientsInfo[i].communication = -1;
	}
	//Balls
	gameInfo->numBalls = 0;
	for (i = 0; i < BALL_MAX_BALLS; i++) {
		resetBall(i);
	}
	//Brick
	gameInfo->numBricks = 0;
	for (i = 0; i < BRICK_MAX_BRICKS; i++) {
		resetBrick(i);
	}
	//bonus
	for(i = 0;i<BONUS_MAX_BONUS;i++)
		hTBonus[i] = NULL;

	_tcscpy_s(gameState, TAM, TEXT("Server Initiated"));
	InvalidateRect(global_hWnd, NULL, TRUE);


	TCHAR str[TAM];
	TCHAR tmp[TAM];
	TCHAR tmp2[TAM];
	_itot_s(gameInfo->myconfig.gameSize.sizex, tmp, TAM, 10);//translates num to str
	_itot_s(gameInfo->myconfig.gameSize.sizey, tmp2, TAM, 10);//translates num to str
	_tcscpy_s(str, TAM, TEXT("lmites = "));
	_tcscat_s(str, TAM, tmp);//ads
	_tcscat_s(str, TAM, TEXT(","));
	_tcscat_s(str, TAM, tmp2);//ads
	MessageBox(global_hWnd, str, TEXT("DEGUB"), MB_OK);

	return 0;
}

void resetUser(DWORD id) {
	gameInfo->nUsers[id].lifes = -1;
	_stprintf_s(gameInfo->nUsers[id].name, MAX_NAME_LENGTH, TEXT("empty"));
	gameInfo->nUsers[id].posx = -1;
	gameInfo->nUsers[id].posy = -1;
	gameInfo->nUsers[id].score = -1;
	gameInfo->nUsers[id].size.sizex = -1;
	gameInfo->nUsers[id].size.sizey = -1;
	gameInfo->nUsers[id].id = -1;
}

void resetBall(DWORD id) {
	gameInfo->nBalls[id].id = -1;
	gameInfo->nBalls[id].posx = -1;
	gameInfo->nBalls[id].posy = -1;
	gameInfo->nBalls[id].size.sizex = -1;
	gameInfo->nBalls[id].size.sizey = -1;
	gameInfo->nBalls[id].speed = -1;
	gameInfo->nBalls[id].status = -1;
	hTBola[id] = INVALID_HANDLE_VALUE;
}

void resetBrick(DWORD id) {
	gameInfo->nBricks[id].brinde.speed = -1;
	gameInfo->nBricks[id].brinde.posx = -1;
	gameInfo->nBricks[id].brinde.posy = -1;
	gameInfo->nBricks[id].brinde.size.sizex = -1;
	gameInfo->nBricks[id].brinde.size.sizey = -1;
	gameInfo->nBricks[id].brinde.status = -1;
	gameInfo->nBricks[id].brinde.type = -1;
	gameInfo->nBricks->id = -1;
	gameInfo->nBricks[id].posx = -1;
	gameInfo->nBricks[id].posy = -1;
	gameInfo->nBricks[id].size.sizex = -1;
	gameInfo->nBricks[id].size.sizey = -1;
	gameInfo->nBricks[id].status = -1;
	gameInfo->nBricks[id].type = -1;
}

void createBalls(DWORD num) {
	//create num balls
	DWORD count = 0;
	DWORD threadId;
	int i;
	for (i = 0; i < BALL_MAX_BALLS; i++) {
		if (hTBola[i] == INVALID_HANDLE_VALUE) {//se handle is available
			hTBola[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BolaThread, (LPVOID)i, 0, &threadId);
			if (hTBola[i] == INVALID_HANDLE_VALUE) {
				//_tprintf(TEXT("Erro ao criar bola numero:%d!\n"), i);
				return;
			}
			else {
				//_tprintf(TEXT("Created Ball number:%d!\n"), i);
				//hTBola[i] = UNDEFINE_ALTERNATE;
				gameInfo->numBalls++;
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

void createBonus(DWORD id) {
	for (int i = 0; i < BONUS_MAX_BONUS; i++) {
		if (hTBonus[i] == INVALID_HANDLE_VALUE || hTBonus[i] == NULL) {//se handle is available
			////_tprintf(TEXT("Found a free handle\n"));
			hTBonus[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)bonusDrop, (LPVOID)id, 0, NULL);
			if (hTBonus[i] == NULL) {
				//_tprintf(TEXT("Erro ao criar bola numero:%d!\n"), i);
				return -1;
			}
			else {
				break;
			}
		}
	}

	return;
}

DWORD WINAPI BolaThread(LPVOID param) {

	srand((int)time(NULL));
	DWORD id = ((DWORD)param);
	int numberBrick = gameInfo->numBricks;
	LARGE_INTEGER li;
	HANDLE hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
	
	gameInfo->nBalls[id].id = id;
	gameInfo->nBalls[id].status = 1;
	gameInfo->nBalls[id].speed = gameInfo->myconfig.ballInitialSpeed;
	gameInfo->nBalls[id].size.sizex = gameInfo->myconfig.ballSize.sizex;
	gameInfo->nBalls[id].size.sizey = gameInfo->myconfig.ballSize.sizey;
	DWORD posx = gameInfo->nUsers[0].posx + (gameInfo->nUsers[0].size.sizex / 2);
	DWORD posy = gameInfo->nUsers[0].posy - gameInfo->nBalls[id].size.sizey;
	DWORD ballScore = 0;
	boolean flag, goingUp = 1, goingRight = (rand() % 2);

	do {
		ballScore = GetTickCount();
		flag = 0;
		//checks for bricks
		for (int i = 0; i < numberBrick; i++) {
			if (gameInfo->nBricks[i].status <= 0)
				continue;
			//up
			if (goingUp && posy - 1 == gameInfo->nBricks[i].posy + gameInfo->nBricks[i].size.sizey) {
				if (gameInfo->nBricks[i].posx - 1 < posx && (gameInfo->nBricks[i].posx + gameInfo->nBricks[i].size.sizex) > posx) {
					//MessageBox(global_hWnd, TEXT("hit up"), TEXT("debug"), MB_OK);
					goingUp = 0;
					hitBrick(i, id);
				}
			}
			//down
			else if (!goingUp && posy + gameInfo->nBalls[id].size.sizey + 1 == gameInfo->nBricks[i].posy) {
				if (gameInfo->nBricks[i].posx < posx + 1 && (gameInfo->nBricks[i].posx + gameInfo->nBricks[i].size.sizex) > posx) {
					//MessageBox(global_hWnd, TEXT("hit down"), TEXT("debug"), MB_OK);
					goingUp = 1;
					hitBrick(i, id);
				}
			}

			//left
			if (!goingRight && posx == gameInfo->nBricks[i].posx + gameInfo->nBricks[i].size.sizex && posy < gameInfo->nBricks[i].posy + gameInfo->nBricks[i].size.sizey && posy > gameInfo->nBricks[i].posy) {
				//MessageBox(global_hWnd, TEXT("hit left"), TEXT("debug"), MB_OK);
				goingRight = 1;
				hitBrick(i, id);
			}
			//right

			else if (goingRight && posx + gameInfo->nBalls[id].size.sizey == gameInfo->nBricks[i].posx && posy < gameInfo->nBricks[i].posy + gameInfo->nBricks[i].size.sizey && posy > gameInfo->nBricks[i].posy) {
				//MessageBox(global_hWnd, TEXT("hit right"), TEXT("debug"), MB_OK);
				goingRight = 0;
				hitBrick(i, id);
			}
		}

		if (goingRight) {
			if (posx + gameInfo->nBalls[id].size.sizex < gameInfo->myconfig.gameSize.sizex ) {
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
			if (posy + gameInfo->nBalls[id].size.sizey < gameInfo->myconfig.gameSize.sizey) {

				for (int i = 0; i < gameInfo->numUsers; i++) {//checks for player
					////_tprintf(TEXT("BALL(%d,%d)\nUSER(%d-%d,%d)\n\n"),posx,posy, gameInfo->nUsers[i].posx, gameInfo->nUsers[i].posx + gameInfo->nUsers[i].size, gameInfo->nUsers[i].posy);
					if (posy + gameInfo->nBalls[id].size.sizey == gameInfo->nUsers[i].posy && (posx >= gameInfo->nUsers[i].posx && posx <= (gameInfo->nUsers[i].posx + gameInfo->nUsers[i].size.sizex))) {//atinge a barreira
						flag = 1;
						//_tprintf(TEXT("HIT player[%d]!!\n"), i);
						//MessageBox(global_hWnd, TEXT("hit player"), TEXT("debug"), MB_OK);
						break;
					}
				}
				if (flag) {
					goingUp = 1;
					posy--;
				}
				else { posy++; }

			}
			else {//se atinge o fim do mapa
				//reset ball
				//MessageBox(global_hWnd, TEXT("end of ball"), TEXT("debug"), MB_OK);
				gameInfo->nBalls[id].status = 0;//end of ball
				gameInfo->numBalls--;
			}
		}

		if (localNumBricks <= 0) {
			gameInfo->nBalls[id].status = 0;//end of ball
			gameInfo->numBalls--;
		}

		gameInfo->nBalls[id].posx = posx;
		gameInfo->nBalls[id].posy = posy;

		if (gameInfo->nBalls[id].status != 0)//if is to end
			gameInfo->nBalls[id].status = 1;

		SetEvent(updateLocalGame);
		ResetEvent(updateLocalGame);
		SetEvent(updateGame);
		//MessageBox(global_hWnd, TEXT("ball init pos + 1"), TEXT("debug"), MB_OK);
		for (int i = 0; i < gameInfo->numUsers; i++) {
			gameInfo->nUsers[i].score += (GetTickCount() - ballScore) / 100;
		}

		//Sleep(gameInfo->nBalls[id].speed);
		li.QuadPart = -(gameInfo->nBalls[id].speed) * _MSECOND; // negative = relative time | 100-nanosecond units
		if (!SetWaitableTimer(hTimer, &li, 0, NULL, NULL, 0))
			MessageBox(global_hWnd, TEXT("Could not SetWaitbleTimer"), TEXT("warning"), MB_OK);

		if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0)
			MessageBox(global_hWnd, TEXT("Could not wait for timer on ball thread"), TEXT("warning"), MB_OK);

	} while (gameInfo->nBalls[id].status);

	if (gameInfo->numBalls == 0)
		for (int i = 0; i < gameInfo->numUsers; i++) {
			gameInfo->nUsers[i].lifes--;
		}

	resetBall(id);
	return 0;
}

DWORD WINAPI bonusDrop(LPVOID param) {
	DWORD id = ((DWORD)param);

	LARGE_INTEGER li;
	HANDLE hTimer = CreateWaitableTimer(NULL, FALSE, NULL);
					
	gameInfo->nBricks[id].brinde.size.sizex = gameInfo->myconfig.bonusSize.sizex;
	gameInfo->nBricks[id].brinde.size.sizey = gameInfo->myconfig.bonusSize.sizey;
	gameInfo->nBricks[id].brinde.speed = gameInfo->myconfig.bonusSpeed;
	gameInfo->nBricks[id].brinde.posx = (gameInfo->nBricks[id].posx + (gameInfo->nBricks[id].size.sizex / 2)) - gameInfo->nBricks[id].brinde.size.sizex / 2;
	gameInfo->nBricks[id].brinde.posy = gameInfo->nBricks[id].posy + gameInfo->nBricks[id].size.sizey;
	gameInfo->nBricks[id].brinde.status = 1;
	
	do {
		for (int i = 0; i < gameInfo->numUsers; i++) {
			if ((gameInfo->nUsers[i].posy) == gameInfo->nBricks[id].brinde.posy + gameInfo->nBricks[id].brinde.size.sizey) {
				//MessageBox(global_hWnd, TEXT("cheking for user now"), TEXT("debug"), MB_OK);
				if (gameInfo->nBricks[id].brinde.posx + gameInfo->nBricks[id].brinde.size.sizex >= gameInfo->nUsers[i].posx && gameInfo->nBricks[id].brinde.posx <= (gameInfo->nUsers[i].posx + gameInfo->nUsers[i].size.sizex)) {
					gameInfo->nBricks[id].brinde.status = 0;
					SetEvent(updateLocalGame);
					ResetEvent(updateLocalGame);
					SetEvent(updateGame);
					//MessageBox(global_hWnd, TEXT("user caught the bonus"), TEXT("debug"), MB_OK);
					return 1;
				}
			}
		}
		gameInfo->nBricks[id].brinde.posy++;
		SetEvent(updateLocalGame);
		ResetEvent(updateLocalGame);
		SetEvent(updateGame);//update pipe

		li.QuadPart = -(gameInfo->nBricks[id].brinde.speed) * _MSECOND; // negative = relative time | 100-nanosecond units
		if (!SetWaitableTimer(hTimer, &li, 0, NULL, NULL, 0))
			MessageBox(global_hWnd, TEXT("Could not SetWaitbleTimer"), TEXT("warning"), MB_OK);

		if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0)
			MessageBox(global_hWnd, TEXT("Could not wait for timer on ball thread"), TEXT("warning"), MB_OK);

	} while (gameInfo->nBricks[id].brinde.posy + gameInfo->nBricks[id].brinde.size.sizey < gameInfo->myconfig.gameSize.sizey - 1);

	//MessageBox(global_hWnd, TEXT("user did not caught the bonus"), TEXT("debug"), MB_OK);
	gameInfo->nBricks[id].brinde.status = 0;
	SetEvent(updateLocalGame);
	ResetEvent(updateLocalGame);
	SetEvent(updateGame);

	return 0;
}

void hitBrick(DWORD brick_id, DWORD ball_id) {
	gameInfo->nBricks[brick_id].status--;
	if (gameInfo->nBricks[brick_id].status == 0) {
		localNumBricks--;
		for (int i = 0; i < gameInfo->numUsers; i++)
			gameInfo->nUsers[i].score += gameInfo->myconfig.bonusScoreAdd;

		if (gameInfo->nBricks[brick_id].type == 3) {//magic
			createBonus(brick_id);
		}
	}

}

int moveUser(DWORD id, TCHAR side[TAM]) {
	TCHAR str[TAM];
	TCHAR tmp[TAM];
	TCHAR tmp2[TAM];

	_itot_s(gameInfo->nUsers[id].posx, tmp, TAM, 10);//translates num to str

	if (_tcscmp(side, TEXT("right")) == 0) {
		for (int i = 0; i < gameInfo->numUsers; i++) {
			if (gameInfo->nUsers[id].posx + gameInfo->nUsers[id].size.sizex > gameInfo->myconfig.gameSize.sizex)//map
				return 1;
			if (gameInfo->nUsers[id].posx + gameInfo->nUsers[id].size.sizex == gameInfo->nUsers[i].posx)//player
				return 1;
		}
		gameInfo->nUsers[id].posx += 10;
	}
	else if (_tcscmp(side, TEXT("left")) == 0) {
		for (int i = 0; i < gameInfo->numUsers; i++) {
			if (gameInfo->nUsers[id].posx < 1)
				return 1;
			if (gameInfo->nUsers[id].posx == gameInfo->nUsers[i].posx + gameInfo->nUsers[i].size.sizex)
				return 1;
		}
		gameInfo->nUsers[id].posx -= 10;
	}

	_itot_s(gameInfo->nUsers[id].posx, tmp2, TAM, 10);//translates num to str

	_tcscpy_s(str, TAM, TEXT("pos - before:"));
	_tcscat_s(str, TAM, tmp);//ads
	_tcscat_s(str, TAM, TEXT("|after:"));
	_tcscat_s(str, TAM, tmp2);//ads

	//if(gameInfo->nUsers[id].posx > 650 || gameInfo->nUsers[id].posx < 50)
		//MessageBox(global_hWnd, str, TEXT("DEGUB"), MB_OK);
	
	return 0;
}

void sendMessage(msg sendMsg) {
	print(sendMsg);
	if (sendMsg.to == 255) {
		//MessageBox(global_hWnd, TEXT("255"), TEXT("sending to:"), MB_OK);
		for (int i = 0; i < USER_MAX_USERS; i++) {
			if (clientsInfo[i].communication == 0) {
				//MessageBox(global_hWnd, sendMsg.messageInfo, TEXT("Server - 255"), MB_OK);
				sendMessageDLL(sendMsg);
				SetEvent(clientsInfo[i].hClientMsg);
			}
			else if (clientsInfo[i].communication == 1) {
				sendMessagePipe(sendMsg, clientsInfo[i].hClientMsg);
			}//else
				//MessageBox(global_hWnd, TEXT("warning - error"), TEXT("sendMEssage"), MB_OK);
				
		}
	}
	else {
		//MessageBox(global_hWnd, TEXT("unic"), TEXT("sending to:"), MB_OK);
		if (clientsInfo[sendMsg.to].communication == 0) {
			////_tprintf(TEXT("Responding via DLL\n"));
			//MessageBox(global_hWnd, sendMsg.messageInfo, TEXT("Server - unic"), MB_OK);
			sendMessageDLL(sendMsg);
			SetEvent(clientsInfo[sendMsg.to].hClientMsg);
		}
		else {
			//_itot_s(sendMsg.to, tmp2, TAM, 10);//translates num to str
			//_tcscpy_s(tmp, TAM, TEXT("Will send via pipes to singular -> "));
			//_tcscat_s(tmp, TAM, tmp2);//ads
			//MessageBox(global_hWnd, tmp, TEXT("info"), MB_OK);
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
	////_tprintf(TEXT("Sent:'%s'\tCode=%d\tFrom=%d\tTo=%d\n"),sendMsg.messageInfo, sendMsg.codigoMsg, sendMsg.from, sendMsg.to);
	////_tprintf(TEXT("Sent codigo:(%d)\n"), sendMsg.codigoMsg);
	//if (sendMsg.codigoMsg < 0)
		//hClients[sendMsg.to] = NULL;//if invalid resets handle
}

void sendMessagePipe(msg sendMsg, HANDLE hPipe) {
	HANDLE WriteReady;
	OVERLAPPED OverlWr = { 0 };
	DWORD bytesWritten;
	BOOLEAN fSuccess;

	WriteReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (WriteReady == NULL) {
		//_tprintf(TEXT("Erro ao criar writeRead Event. Erro = %d\n"), GetLastError());
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
	//_tprintf(TEXT("Write concluido...\n"));

	GetOverlappedResult(hPipe, &OverlWr, &bytesWritten, FALSE);
	if (bytesWritten < sizeof(msg)) {
		//_tprintf(TEXT("Write File failed... | Erro = %d\n"), GetLastError());
	}

	////_tprintf(TEXT("[ESCRITOR] Enviei %d bytes ao leitor...(WriteFile)\n"), bytesWritten);
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
		//_itot_s(i, tmp, TAM, 10);//translates num to str
		//MessageBox(global_hWnd, tmp, TEXT("will now return"), MB_OK);
		return i;
	}
	else if (newMsg.connection == 1) {
		clientsInfo[i].hClientMsg = hTmpPipeMsg;
		hTmpPipeMsg = NULL;
		tmp_client_id = i;//next client for game pipe will be this one
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
	gameInfo->gameStatus = 0;//users can now join
	gameInfo->numUsers = 0;
	//MessageBox(global_hWnd, TEXT("game created"), TEXT("ALERT"), MB_OK);
	_tcscpy_s(gameState, TAM, TEXT("Game Created"));
	InvalidateRect(global_hWnd, NULL, TRUE);
}

void startGame(){
	//MessageBox(global_hWnd, TEXT("starting game"), TEXT("WARNING"), MB_OK);
	int i;
	msg tmpMsg;
	//start Users
	for (i = 0; i < gameInfo->numUsers; i++) {
		userInit(i);
	}

	//start bricks
	assignBrick(30);

	gameInfo->gameStatus = 1;
	SetEvent(updateLocalGame);//sends game
	ResetEvent(updateLocalGame);
	SetEvent(updateGame);//sends game
	Sleep(250);
	tmpMsg.codigoMsg = 100;//new game
	tmpMsg.from = server_id;
	tmpMsg.to = 255; //broadcast
	_tcscpy_s(tmpMsg.messageInfo, TAM, TEXT("gameStart"));
	sendMessage(tmpMsg);

	_tcscpy_s(gameState, TAM, TEXT("Game Started"));
	InvalidateRect(global_hWnd, NULL, TRUE);
	return;
}

void assignBrick(DWORD num) {
	//create num bricks
	int spaceToLim = 10;
	int spaceToBrick = 1;
	int oposx = spaceToLim, oposy = 10;
	for (int i = 0; i < num; i++) {

		gameInfo->nBricks[i].id = i;
		gameInfo->nBricks[i].size.sizex = gameInfo->myconfig.brickSize.sizex;
		gameInfo->nBricks[i].size.sizey = gameInfo->myconfig.brickSize.sizey;

		//type
		gameInfo->nBricks[i].type = 1 + rand() % 3;
		if (gameInfo->nBricks[i].type == 1) {//normal
			gameInfo->nBricks[i].status = 1;
		}
		else if (gameInfo->nBricks[i].type == 2) {//resistent
			gameInfo->nBricks[i].status = 2 + rand() % 3;
		}
		else if (gameInfo->nBricks[i].type == 3) {//magic
			gameInfo->nBricks[i].status = 1;
			gameInfo->nBricks[i].brinde.type = 1 + rand() % 3;
		}

		//pos
		if (!(oposx + gameInfo->nBricks[i].size.sizex + spaceToLim < gameInfo->myconfig.gameSize.sizex)) {
			oposx = spaceToLim;
			oposy += 25;
		}
		gameInfo->nBricks[i].posx = oposx;
		gameInfo->nBricks[i].posy = oposy;

		oposx += gameInfo->nBricks[i].size.sizex + spaceToBrick;

	}

	gameInfo->numBricks = num;
	localNumBricks = num;
	return;
}

void userInit(DWORD id) {
	if (id)
		gameInfo->nUsers[id].posx = gameInfo->nUsers[id - 1].posx + gameInfo->nUsers[id - 1].size.sizex + 20;
	else
		gameInfo->nUsers[id].posx = 20;

	gameInfo->nUsers[id].posy = gameInfo->myconfig.gameSize.sizey - gameInfo->myconfig.userSize.sizey - 20;
	gameInfo->nUsers[id].lifes = gameInfo->myconfig.userNumLifes;
	gameInfo->nUsers[id].size.sizex = gameInfo->myconfig.userSize.sizex;
	gameInfo->nUsers[id].size.sizey = gameInfo->myconfig.userSize.sizey;
	return;
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

