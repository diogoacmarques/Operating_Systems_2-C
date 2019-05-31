#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
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
//void startGame();
DWORD WINAPI startGameThread(LPVOID param);
void userInit(DWORD id);
void resetUser(DWORD id);
void resetBall(DWORD id);
void resetBrick(DWORD id);
void hitBrick(DWORD brick_id, DWORD ball_id);
int moveUser(DWORD id, TCHAR side[TAM]);


//vars
TCHAR szProgName[] = TEXT("Server Base");
HWND global_hWnd = NULL;
DWORD localNumBricks;

//client related
comuciationHandle clientsInfo[MAX_CLIENTS];
DWORD tmp_client_id = 0;
HANDLE hTmpPipeMsg = NULL;
HANDLE hTmpPipeGame = NULL;

//server related
HANDLE updateGame;//event to update game for pipe users
HANDLE messageEventServer;//event to update server of dll msg
HANDLE hResolveMessageMutex;//handle to resolveMessage function
DWORD server_id;
HANDLE hTBola[MAX_BALLS];

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
		5, // Posição x pixels (default=à direita da última)
		5, // Posição y pixels (default=abaixo da última)
		800, // Largura da janela (em pixels)
		600, // Altura da janela (em pixels)
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

	HANDLE checkExistingServer = CreateEvent(NULL, FALSE, FALSE, CHECK_SERVER_EVENT);
	//if(checkExistingServer == )
		//return 0;

	updateGame = CreateEvent(NULL, FALSE, FALSE, NULL);
	messageEventServer = CreateEvent(NULL, FALSE, FALSE, MESSAGE_EVENT_NAME);
	updateBalls = CreateEvent(NULL, TRUE, FALSE, BALL_EVENT_NAME);//updates ball of local
	updateBonus = CreateEvent(NULL, FALSE, FALSE, BONUS_EVENT_NAME);//updates bonus of local
	hResolveMessageMutex = CreateMutex(NULL, FALSE, NULL);
	if (updateGame == NULL || messageEventServer == NULL || updateBalls == NULL || updateBonus == NULL || hResolveMessageMutex == NULL) {
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
		_tprintf(TEXT("Erro ao iniciar as variaveis!"));
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

int xPos = 0, yPos = 0;
TCHAR c = '?';
TCHAR frase[200];

int maxX = 0, maxY = 0;
HDC memDC = NULL;
HBITMAP hBit = NULL;
HBRUSH hBrush = NULL;
HDC tempDC = NULL;
HBITMAP hBmp = NULL;
BITMAP bmp;

int xBitMap = 0, yBitMap = 0;
TCHAR login[100];
//print
int xPrint = 0, yPrint = 0;

LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	//draw
	HDC hDC;
	RECT rect;
	//DWORD res;
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
		GetClientRect(hWnd, &rect);
		switch (LOWORD(wParam)) {
		case VK_UP:
			yBitMap = yBitMap > 0 ? yBitMap - 10 : 0;
			break;
		case VK_DOWN:
			yBitMap = yBitMap < rect.bottom - bmp.bmHeight ? yBitMap + 10 : rect.bottom - bmp.bmHeight;
			break;
		case VK_LEFT:
			xBitMap = xBitMap > 0 ? xBitMap - 10 : 0;
			break;
		case VK_RIGHT:
			xBitMap = xBitMap < rect.right - bmp.bmWidth ? xBitMap + 10 : rect.right - bmp.bmWidth;
			break;
		case VK_SPACE:
			//draw image
			hBmp = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/background.bmp"),
				IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

			GetObject(hBmp, sizeof(bmp), &bmp);
			tempDC = CreateCompatibleDC(memDC);
			SelectObject(tempDC, hBmp);
			//PatBlt(memDC, 0, 0, maxX, maxY, PATCOPY);
			BitBlt(memDC, xBitMap, yBitMap, 150, 150, tempDC, 0, 0, SRCCOPY);//copies from tmpDC to memDC
			DeleteDC(tempDC);
			break;
		}
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	case WM_PAINT:

		GetClientRect(hWnd, &rect);
		SetTextColor(memDC, RGB(255, 255, 255));
		SetBkMode(memDC, TRANSPARENT);
		rect.left = xPrint;
		rect.top = yPrint;
		yPrint += 15;
		if (yPrint > maxY)
			yPrint = 0;
		DrawText(memDC, frase, _tcslen(frase), &rect, DT_SINGLELINE | DT_NOCLIP);

		hDC = BeginPaint(hWnd, &ps);

		BitBlt(hDC, 0, 0, maxX, maxY, memDC, 0, 0, SRCCOPY);

		EndPaint(hWnd, &ps);
		_tcscpy_s(frase, TAM, TEXT("-"));
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
			MessageBox(global_hWnd, TEXT("WILL START GAME AFTER OK"), TEXT("GAME START"), MB_OK);
			if (gameInfo->numUsers > 0 && gameInfo->gameStatus == 0) {
				_tcscpy_s(frase, TAM, TEXT("Game starting..."));
				InvalidateRect(global_hWnd, NULL, TRUE);
				//MessageBox(global_hWnd, TEXT("should've message"), TEXT("message?"), MB_OK);
				HANDLE hStartGame = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)startGameThread, NULL, 0, NULL);
			}
			break;
		case ID_TOP10:
			break;
		case ID_USERSLOGGED:
			break;
		case ID_SETTINGS:
			_itot_s(gameInfo->numUsers, tmp, TAM, 10);//translates num to str
			MessageBox(global_hWnd, tmp, TEXT("Number of users:"), MB_OK);
			_itot_s(gameInfo->gameStatus, tmp, TAM, 10);//translates num to str
			MessageBox(global_hWnd, tmp, TEXT("GameStatus:"), MB_OK);
			for (int i = 0; i < MAX_CLIENTS; i++) {
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
		_tcscpy_s(tmp, TAM, TEXT("Received-"));
	else
		_tcscpy_s(tmp, TAM, TEXT("Sent-"));

	_tcscat_s(tmp, TAM, TEXT("|Info:"));
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
	_tcscpy_s(frase, TAM, tmp);
	InvalidateRect(global_hWnd, NULL, TRUE);
	return;


	//MessageBeep(MB_ICONSTOP);

	_tcscat_s(tmp, 30, TEXT("|To:"));
	_itot_s(printMsg.to, tmp2, 30, 10);//translates num to str
	_tcscat_s(tmp, 30, tmp2);//adds
	_tcscat_s(tmp, 30, TEXT("|From:"));
	_itot_s(printMsg.from, tmp2, 30, 10);//translates num to str
	_tcscat_s(tmp, 30, tmp2);//adds
	_tcscat_s(tmp, 30, TEXT("|Via:"));
	_itot_s(printMsg.connection, tmp2, 30, 10);//translates num to str
	_tcscat_s(tmp, 30, tmp2);//adds
	
	//_tcscpy_s(frase, TAM, line);
	//InvalidateRect(global_hWnd, NULL, TRUE);
}

DWORD resolveMessage(msg inMsg) {
	print(inMsg);
	//Sleep(2000);
	DWORD num;
	TCHAR tmp[TAM];
	boolean flag;
	msg outMsg;
	outMsg.codigoMsg = 0;
	outMsg.from = 254;
	outMsg.connection = inMsg.connection;
	
	//_tprintf(TEXT("[%d]NewMsg(%d):\%s\n-from:%d\n-to:%d\n"), quant, inMsg.codigoMsg, inMsg.messageInfo, inMsg.from, inMsg.to);
	//init
	if (inMsg.codigoMsg == -9999) {
		num = createHandle(inMsg);
		_itot_s(num, tmp, TAM, 10);//translates num to str
		MessageBox(global_hWnd, tmp, TEXT("num attributed is"), MB_OK);
		if (num == MAX_CLIENTS - 1) {//full
			outMsg.codigoMsg = -9999;
		}
		else if (num >= 0) {//success
			outMsg.codigoMsg = 9999;
			//SetEvent(updateGame);//sends client the game
		}
		outMsg.to = num;
		_itot_s(num, tmp, TAM, 10);//translates num to str
		_tcscpy_s(outMsg.messageInfo, TAM, tmp);//copys client_id to messageInfo
		sendMessage(outMsg);
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
			_tprintf(TEXT("Sent game in progress\n"));
			sendMessage(outMsg);
			return 1;
		}
		else if (_tcscmp(inMsg.messageInfo, TEXT("nop")) == 0) {//for tests
			outMsg.codigoMsg = -1;//not successful
			outMsg.to = inMsg.from;
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("iDontLikeYou"));
			sendMessage(outMsg);
			return 1;
		}
		for (int i = 0; i < gameInfo->numUsers; i++) {
			if (_tcscmp(gameInfo->nUsers[i].name, inMsg.messageInfo) == 0) {
				_tprintf(TEXT("This user is already logged in\n"));
				flag = 0;
				outMsg.codigoMsg = -1;//not successful
				outMsg.to = inMsg.from;
				//_tprintf(TEXT("Sent user not accepted created\n"));
				sendMessage(outMsg);
				break;
			}
		}
		if (flag) {
			_tcscpy_s(gameInfo->nUsers[gameInfo->numUsers].name, MAX_NAME_LENGTH, inMsg.messageInfo);
			gameInfo->nUsers[gameInfo->numUsers].user_id = inMsg.from;
			_tprintf(TEXT("sending message with sucess to user_id:%d\n"), gameInfo->nUsers[gameInfo->numUsers].user_id);
			outMsg.codigoMsg = 1;//sucesso
			gameInfo->nUsers[gameInfo->numUsers].hConnection = clientsInfo[inMsg.from].hClientMsg;
			gameInfo->numUsers++;
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("loginSucess"));
			outMsg.to = inMsg.from;
			sendMessage(outMsg);
		}
	}
	else if (inMsg.codigoMsg == 2) {//end of user
		_tprintf(TEXT("%s[%d] exited with the score of:%d\n"), gameInfo->nUsers[inMsg.from].name, gameInfo->nUsers[inMsg.from].user_id, gameInfo->nUsers[inMsg.from].score);
		/*int res = registry(gameInfo->nUsers[inMsg.from]);
		if (res) {
			_tprintf(TEXT("new high score saved!\n"));
		}
		else {
			_tprintf(TEXT("not enough for top 10!\n"));
		}
		*/
		Sleep(250);
		resetUser(inMsg.from);
		gameInfo->numUsers--;
		outMsg.to = inMsg.from;
		outMsg.codigoMsg = 2;
		_tcscpy_s(outMsg.messageInfo, TAM, TEXT("ended user"));
		sendMessage(outMsg);

		if (gameInfo->numUsers == 0) {//closing game
			_tprintf(TEXT("Reseting game...\n"));
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("End of Game"));
			outMsg.to = inMsg.from;
			outMsg.codigoMsg = -999;
			sendMessage(outMsg);

			for (int i = 0; i < MAX_BALLS; i++) {
				TerminateThread(hTBola[i], 1);
				CloseHandle(hTBola[i]);
				resetBall(i);
			}
			for (int i = 0; i < MAX_BRICKS; i++) {
				resetBrick(i);
			}
			gameInfo->numBalls = 0;
			gameInfo->gameStatus = -1; //after game ends
			localNumBricks = 0;
			Sleep(1000);
			_tprintf(TEXT("Game reseted!\n"));
		}
		else
			_tprintf(TEXT("There are still players in the game!\n"));
	}
	else if (inMsg.codigoMsg == 101) {
		//if (gameInfo->nUsers[inMsg.from].lifes > 0 && gameInfo->numBalls == 0)
			//createBalls(1);
	}
	else if (inMsg.codigoMsg == 200) {//user trying to move
		_tprintf(TEXT("[%d]-%s\n"), inMsg.from, inMsg.messageInfo);
		//_tprintf(TEXT("\n\nIN->Codigo:(%d)\tDirection:(%s)\tFrom:%d\tTo:%d\n"), inMsg.codigoMsg, inMsg.messageInfo, inMsg.from, inMsg.to);

		if (_tcscmp(inMsg.messageInfo, TEXT("right")) != 0 && _tcscmp(inMsg.messageInfo, TEXT("left")) != 0) {
			_tprintf(TEXT("Erro!\n"));
			Sleep(3000);
		}

		int res = moveUser(inMsg.from, inMsg.messageInfo);
		if (!res) {
			SetEvent(updateGame);//update pipe
			_itot_s(inMsg.from, outMsg.messageInfo, TAM, 10);//translates user_id num to str
			_tcscat_s(outMsg.messageInfo, TAM, TEXT(":"));//adds ':'
			_tcscat_s(outMsg.messageInfo, TAM, inMsg.messageInfo);//adds direction
		}
		else
			_tcscpy_s(outMsg.messageInfo, TAM, TEXT("no"));
		outMsg.codigoMsg = 200;
		outMsg.to = 255;
		sendMessage(outMsg);
		//_tprintf(TEXT("user_pos(%d,%d)\n"), gameInfo->nUsers[0].posx, gameInfo->nUsers[0].posy);
		//_tprintf(TEXT("(moveUser)User[%d]:%s\n"), newMsg.user_id, newMsg.messageInfo);
	}
	else if (inMsg.codigoMsg == 123) {//for tests || returns the exact same msg
		outMsg.to = inMsg.from;
		sendMessage(outMsg);
	}
	return -1;
}

//thread to receive initial clients via pipe for msg
DWORD WINAPI connectPipeMsg(LPVOID param) {
	BOOLEAN fConnected = 0;
	HANDLE hPipeInit = INVALID_HANDLE_VALUE;
	HANDLE hThread;

	//DWORD nSent;

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
			NULL);

		if (hPipeInit == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("Erro ao iniciar pipe!"));
			return 0;
		}
		else
			_tprintf(TEXT("Pipe created with success\n"));

		//waits for client
		fConnected = ConnectNamedPipe(hPipeInit, NULL);//waits for client
		if (!fConnected && GetLastError() == ERROR_PIPE_CONNECTED)
			fConnected = 1;

		MessageBox(global_hWnd, TEXT("Got a Msg Client"), TEXT("Client"), MB_OK);

		if (fConnected) {
			hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)pipeClientMsg, (LPVOID)hPipeInit, 0, NULL);
			if (hThread == NULL) {
				_tprintf(TEXT("Erro a criar thread para o pipe do user\n"));
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
	//DWORD nSent;

	/*HANDLE sendGamePipe = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)updateGamePipe, NULL, 0, NULL);
	if (sendGamePipe == NULL) {
		_tprintf(TEXT("Error createing updateGamePipe!\n"));
		return -1;
	}*/


	do {

		hPipeInit = CreateNamedPipe(INIT_PIPE_GAME_NAME,
			PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // bloqueante (não usar PIPE_NOWAIT nem mesmo em Asyncr)
			PIPE_UNLIMITED_INSTANCES, // max. instancias (255)
			BUFSIZE_GAME, // tam buffer output
			BUFSIZE_GAME, // tam buffer input
			2000, // time-out p/ cliente 5k milisegundos (0->default=50)
			NULL);

		if (hPipeInit == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("Erro ao iniciar pipe!"));
			return 0;
		}
		else
			_tprintf(TEXT("Pipe for game created with success\n"));

		//waits for client
		_tprintf(TEXT("Waiting for a client!\n"));
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

		SetEvent(updateGame);
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
		_tprintf(TEXT("\nErro – o handle enviado no param da thread é nulo"));
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
		_tprintf(TEXT("\nServidor: não foi possível criar o evento Read. Mais vale parar já"));
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
		MessageBox(global_hWnd, TEXT("Got a message via the pipes"), TEXT("PIPE"), MB_OK);

		GetOverlappedResult(hPipe, &OverlRd, &cbBytesRead, FALSE);
		if (cbBytesRead < sizeof(msg))
			_tprintf(TEXT("\nReadFile não leu os dados todos. Erro = %d"), GetLastError()); // acrescentar lógica de encerrar a thread cliente

		//processa info recebida
		WaitForSingleObject(hResolveMessageMutex, INFINITE);
		resp = resolveMessage(inMsg);
		ReleaseMutex(hResolveMessageMutex);
	}

	//end client
	//removeCliente(hPipe);
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe); // Desliga servidor da instância
	CloseHandle(hPipe); // Fecha este lado desta instância
	_tprintf(TEXT("\nThread dedicada Cliente a terminar"));
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
		for (i = 0; i < MAX_CLIENTS; i++)
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
		MessageBox(global_hWnd, TEXT("got event messageEventServer"), TEXT("warning"), MB_OK);
		for (int i = 0; i < MAX_CLIENTS; i++) {//show handels
			_itot_s(clientsInfo[i].communication, tmp, TAM, 10);//translates num to str
			MessageBox(global_hWnd, tmp, TEXT("client[i].communication = "), MB_OK);
		}
		inMsg = receiveMessageDLL();
		WaitForSingleObject(hResolveMessageMutex, INFINITE);
		resolveMessage(inMsg);
		ReleaseMutex(hResolveMessageMutex);
		
		//Sleep(5000);
	} while (1);
}

DWORD startVars() {

	int i;

	//Status
	gameInfo->gameStatus = -1;
	server_id = 254;
	//default config
	//game
	gameInfo->myconfig.limx = maxX + 1;
	gameInfo->myconfig.limy = maxY + 1;
	gameInfo->myconfig.num_levels = MAX_LEVELS;
	gameInfo->myconfig.score_up = 100;
	//Users
	gameInfo->myconfig.max_users = MAX_USERS;
	gameInfo->myconfig.inital_lifes = MAX_INIT_LIFES;
	//balls
	gameInfo->myconfig.max_balls = MAX_BALLS;
	gameInfo->myconfig.max_speed = MAX_SPEED;
	gameInfo->myconfig.num_speed_up = MAX_SPEED_BONUS;
	gameInfo->myconfig.num_speed_down = MAX_SPEED_BONUS;
	gameInfo->myconfig.prob_speed_up = PROB_SPEED_BONUS;
	gameInfo->myconfig.prob_speed_down = PROB_SPEED_BONUS;
	gameInfo->myconfig.duration = MAX_DURATION;
	gameInfo->myconfig.inital_ball_speed = INIT_SPEED;
	//bricks
	gameInfo->myconfig.max_bricks = MAX_BRICKS;
	gameInfo->myconfig.initial_bricks = INIT_BRICKS;
	//end of default config


	for (i = 0; i < MAX_CLIENTS; i++) {

		clientsInfo[i].hClientMsg = NULL;
		clientsInfo[i].hClientGame = NULL;
		clientsInfo[i].communication = -1;

		//_itot_s(clientsInfo[i].communication, tmp, TAM, 10);//translates num to str
		//MessageBox(global_hWnd, tmp, TEXT("client[i].communication = "), MB_OK);
	}
	//Users
	gameInfo->numUsers = 0;
	for (i = 0; i < MAX_USERS; i++) {
		resetUser(i);
	}
	//Balls
	gameInfo->numBalls = 0;
	for (i = 0; i < MAX_BALLS; i++) {
		resetBall(i);
	}
	//Brick
	gameInfo->numBricks = 0;
	for (i = 0; i < MAX_BRICKS; i++) {
		resetBrick(i);
	}

	return 0;
}

void resetUser(DWORD id) {
	//_tprintf(TEXT("Reseting user %d\n"), id);
	_stprintf_s(gameInfo->nUsers[id].name, MAX_NAME_LENGTH, TEXT("empty"));
	gameInfo->nUsers[id].connection_mode = -1;
	gameInfo->nUsers[id].hConnection = NULL;
	gameInfo->nUsers[id].user_id = -1;
	gameInfo->nUsers[id].lifes = 3;
	gameInfo->nUsers[id].score = 0;
	gameInfo->nUsers[id].size = 10;
	gameInfo->nUsers[id].posx = 0;
	gameInfo->nUsers[id].posy = 0;

}

void resetBall(DWORD id) {
	//_tprintf(TEXT("Reseting ball %d\n"), id);
	gameInfo->nBalls[id].id = -1;
	gameInfo->nBalls[id].posx = 0;
	gameInfo->nBalls[id].posy = 0;
	gameInfo->nBalls[id].status = 0;
	gameInfo->nBalls[id].speed = gameInfo->myconfig.inital_ball_speed;
	hTBola[id] = INVALID_HANDLE_VALUE;
}

void resetBrick(DWORD id) {
	//_tprintf(TEXT("Reseting brick %d\n"), id);
	gameInfo->nBricks->id = -1;
	gameInfo->nBricks[id].posx = 0;
	gameInfo->nBricks[id].posy = 0;
	gameInfo->nBricks[id].status = 0;
	gameInfo->nBricks[id].tam = 0;
	gameInfo->nBricks[id].type = 0;
	gameInfo->nBricks[id].brinde.status = 0;
	gameInfo->nBricks[id].brinde.type = 0;
	gameInfo->nBricks[id].brinde.posx = 0;
	gameInfo->nBricks[id].brinde.posy = 0;
}

void createBalls(DWORD num) {
	//create num balls
	DWORD count = 0;
	DWORD threadId;
	int i;
	_tcscpy_s(frase, TAM, TEXT("Creating Ball"));
	InvalidateRect(global_hWnd, NULL, TRUE);
	for (i = 0; i < MAX_BALLS; i++) {
		if (hTBola[i] == INVALID_HANDLE_VALUE) {//se handle is available
			hTBola[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BolaThread, (LPVOID)i, 0, &threadId);
			if (hTBola[i] == INVALID_HANDLE_VALUE) {
				_tprintf(TEXT("Erro ao criar bola numero:%d!\n"), i);
				return;
			}
			else {
				_tprintf(TEXT("Created Ball number:%d!\n"), i);
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
	//create num balls
	/*DWORD threadId;
	//_tprintf(TEXT("Creating Bonus with id:%d\n"), id);
	for (int i = 0; i < MAX_BONUS_AT_TIME; i++) {
		if (hTBonus[i] == INVALID_HANDLE_VALUE || hTBonus[i] == NULL) {//se handle is available
			//_tprintf(TEXT("Found a free handle\n"));
			hTBonus[i] = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)bonusDrop, (LPVOID)id, 0, &threadId);
			if (hTBonus[i] == NULL) {
				_tprintf(TEXT("Erro ao criar bola numero:%d!\n"), i);
				return -1;
			}
			else {
				break;
			}
		}
	}

	return;*/
}

DWORD WINAPI BolaThread(LPVOID param) {

	DWORD id = ((DWORD)param);
	LARGE_INTEGER li;
	HANDLE hTimer = CreateWaitableTimer(NULL, FALSE, NULL);

	int numberBrick = gameInfo->numBricks;
	srand((int)time(NULL));
	gameInfo->nBalls[id].id = id;
	DWORD posx = gameInfo->nUsers[0].posx + (gameInfo->nUsers[0].size / 2);
	DWORD posy = gameInfo->nUsers[0].posy;
	DWORD oposx, oposy, num = 0, ballScore = 0;
	boolean flag, goingUp = 1, goingRight = (rand() % 2);
	gameInfo->nBalls[id].status = 1;
	do {
		ballScore = GetTickCount();
		flag = 0;
		//checks for bricks
		for (int i = 0; i < numberBrick; i++) {
			if (gameInfo->nBricks[i].status <= 0)
				continue;
			//_tprintf(TEXT("Ball(%d,%d) | Brick(%d,%d)\n"),posx,posy,gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy);
			//up
			if (goingUp && posy - 1 == gameInfo->nBricks[i].posy) {

				//straight up
				if (gameInfo->nBricks[i].posx - 1 < posx && (gameInfo->nBricks[i].posx + gameInfo->nBricks[i].tam) > posx) {
					//_tprintf(TEXT("Hit up\n"), posy - 1, gameInfo->nBricks[i].posy);
					goingUp = 0;
					hitBrick(i, id);
				}
				//up-left
				else if (!goingRight && posx == (gameInfo->nBricks[i].posx + gameInfo->nBricks[i].tam)) {
					_tprintf(TEXT("Hit up-left\n"));
					goingUp = 0;
					goingRight = 1;
					hitBrick(i, id);
				}
				//up-right
				else if (goingRight && posx + 1 == gameInfo->nBricks[i].posx) {
					_tprintf(TEXT("Hit up-right\n"));
					goingUp = 0;
					goingRight = 0;
					hitBrick(i, id);
				}

			}
			else if (!goingUp && posy + 1 == gameInfo->nBricks[i].posy) {
				//down
				if (gameInfo->nBricks[i].posx < posx + 1 && (gameInfo->nBricks[i].posx + gameInfo->nBricks[i].tam) > posx) {
					//_tprintf(TEXT("Hit down\n"), posy + 1, gameInfo->nBricks[i].posy);
					goingUp = 1;
					hitBrick(i, id);
				}
				//down-left
				else if (!goingRight && posx == (gameInfo->nBricks[i].posx + gameInfo->nBricks[i].tam)) {
					_tprintf(TEXT("Hit down-left\n"));
					goingUp = 1;
					goingRight = 1;
					hitBrick(i, id);
				}
				//down-right
				else if (goingRight && posx + 1 == gameInfo->nBricks[i].posx) {
					_tprintf(TEXT("Hit down-right\n"));
					goingUp = 1;
					goingRight = 0;
					hitBrick(i, id);
				}

			}

			//left
			if (!goingRight && posy == gameInfo->nBricks[i].posy && posx == (gameInfo->nBricks[i].posx + gameInfo->nBricks[i].tam)) {
				_tprintf(TEXT("Hit left\n"));
				goingRight = 1;
				hitBrick(i, id);

			}
			//right
			else if (goingRight && posy == gameInfo->nBricks[i].posy && posx + 1 == gameInfo->nBricks[i].posx) {
				_tprintf(TEXT("Hit right\n"));
				goingRight = 0;
				hitBrick(i, id);
			}
		}

		oposx = posx;
		oposy = posy;
		if (goingRight) {
			if (posx < (gameInfo->myconfig.limx - 1)) {
				posx++;
			}
			else {
				posx--;
				goingRight = 0;
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
			if (posy < gameInfo->myconfig.limy - 1) {// se nao atinge o limite do mapa

				for (int i = 0; i < gameInfo->numUsers; i++) {//checks for player
					//_tprintf(TEXT("BALL(%d,%d)\nUSER(%d-%d,%d)\n\n"),posx,posy, gameInfo->nUsers[i].posx, gameInfo->nUsers[i].posx + gameInfo->nUsers[i].size, gameInfo->nUsers[i].posy);
					if (posy == gameInfo->nUsers[i].posy - 1 && (posx >= gameInfo->nUsers[i].posx && posx <= (gameInfo->nUsers[i].posx + gameInfo->nUsers[i].size))) {//atinge a barreira
						flag = 1;
						_tprintf(TEXT("HIT player[%d]!!\n"), i);
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

		SetEvent(updateBalls);//local
		ResetEvent(updateBalls);
		SetEvent(updateGame);//pipe

		for (int i = 0; i < gameInfo->numUsers; i++) {
			gameInfo->nUsers[i].score += (GetTickCount() - ballScore) / 100;
		}


		//Sleep(gameInfo->nBalls[id].speed);
		li.QuadPart = -(gameInfo->nBalls[id].speed) * _MSECOND; // negative = relative time | 100-nanosecond units
		if (!SetWaitableTimer(hTimer, &li, 0, NULL, NULL, 0))
		{
			_tprintf(TEXT("SetWaitableTimer failed (%d)\n"), GetLastError());
		}

		if (WaitForSingleObject(hTimer, INFINITE) != WAIT_OBJECT_0)
			_tprintf(TEXT("WaitForSingleObject waitableTimer failed (%d)\n"), GetLastError());


	} while (gameInfo->nBalls[id].status);

	if (gameInfo->numBalls == 0)
		for (int i = 0; i < gameInfo->numUsers; i++) {
			gameInfo->nUsers[i].lifes--;
		}

	resetBall(id);
	hTBola[id] = INVALID_HANDLE_VALUE;
	_tprintf(TEXT("End of Ball %d!\n"), id);

	/*if (gameInfo->numBalls == 0) {
		if (gameInfo->nUsers[0].lifes) {
			gameInfo->nUsers[0].lifes--;
			createBalls(1);
		}
	}*/

	return 0;
}

DWORD WINAPI bonusDrop(LPVOID param) {
	DWORD id = ((DWORD)param);
	_tprintf(TEXT("bonus created!\n"));
	gameInfo->nBricks[id].brinde.status = 1;
	gameInfo->nBricks[id].brinde.type = 1;

	//_tprintf(TEXT("Sending message that bonus with id:%d was created!\n"),id);
	msg tmpMsg;
	TCHAR tmp[TAM];
	tmpMsg.codigoMsg = 103;
	_itot_s(id, tmp, TAM, 10);
	_tcscpy_s(tmpMsg.messageInfo, TAM, tmp);
	tmpMsg.from = server_id;
	tmpMsg.to = 255;
	sendMessage(tmpMsg);


	do {
		Sleep(1000);
		for (int i = 0; i < gameInfo->numUsers; i++) {
			if ((gameInfo->nUsers[i].posy - 1) == gameInfo->nBricks[id].brinde.posy) {
				_tprintf(TEXT("cheking user [%s]!\n"), gameInfo->nUsers[i].name);
				if (gameInfo->nBricks[id].brinde.posx >= gameInfo->nUsers[i].posx && gameInfo->nBricks[id].brinde.posx <= (gameInfo->nUsers[i].posx + gameInfo->nUsers[i].size)) {
					gameInfo->nBricks[id].brinde.status = 0;
					SetEvent(updateBonus);//update local
					ResetEvent(updateBonus);
					SetEvent(updateGame);//update pipe
					_tprintf(TEXT("%s caught the bonus[%d]!\n"), gameInfo->nUsers[i].name, id);
					//addBonus(id, i);
					return 1;
				}

			}
		}

		gameInfo->nBricks[id].brinde.posy++;
		SetEvent(updateBonus);//update local
		ResetEvent(updateBonus);
		SetEvent(updateGame);//update pipe
	} while (gameInfo->nBricks[id].brinde.posy < gameInfo->myconfig.limy - 1);


	_tprintf(TEXT("End of bonus[%d]!\n"), id);
	gameInfo->nBricks[id].brinde.status = 0;
	SetEvent(updateBonus);//update local
	ResetEvent(updateBonus);
	SetEvent(updateGame);//update pipe

	return 0;
}

void hitBrick(DWORD brick_id, DWORD ball_id) {
	//_tprintf(TEXT("This initalBrick[%d] -> Status:%d\n\n"), gameInfo->nBricks[brick_id].id, gameInfo->nBricks[brick_id].status);
	gameInfo->nBricks[brick_id].status--;
	if (gameInfo->nBricks[brick_id].status == 0) {
		localNumBricks--;
		for (int i = 0; i < gameInfo->numUsers; i++)
			gameInfo->nUsers[i].score += gameInfo->myconfig.score_up;

		if (gameInfo->nBricks[brick_id].type == 3) {//magic
			//creates thread that drops brinds
			createBonus(brick_id);
		}
	}

}

int moveUser(DWORD id, TCHAR side[TAM]) {
	if (_tcscmp(side, TEXT("right")) == 0) {
		for (int i = 0; i < gameInfo->numUsers; i++) {
			if (gameInfo->nUsers[id].posx + gameInfo->nUsers[id].size >= gameInfo->myconfig.limx)//checks limits of map
				return 1;
			if (gameInfo->nUsers[id].posx + gameInfo->nUsers[id].size == gameInfo->nUsers[i].posx)
				return 1;
		}
		gameInfo->nUsers[id].posx++;
	}
	else if (_tcscmp(side, TEXT("left")) == 0) {
		for (int i = 0; i < gameInfo->numUsers; i++) {
			if (gameInfo->nUsers[id].posx < 1)
				return 1;
			if (gameInfo->nUsers[id].posx == gameInfo->nUsers[i].posx + gameInfo->nUsers[i].size)
				return 1;
		}
		gameInfo->nUsers[id].posx--;
	}
	return 0;
}

void sendMessage(msg sendMsg) {
	print(sendMsg);
	if (sendMsg.to == 255) {
		MessageBox(global_hWnd, TEXT("255"), TEXT("sending to:"), MB_OK);
		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (clientsInfo[i].communication == 0) {
				MessageBox(global_hWnd, sendMsg.messageInfo, TEXT("Server - 255"), MB_OK);
				sendMessageDLL(sendMsg);
				SetEvent(clientsInfo[i].hClientMsg);
			}
			else if (clientsInfo[i].communication == 1) {
				sendMessagePipe(sendMsg, clientsInfo[i].hClientMsg);
			}else
				MessageBox(global_hWnd, TEXT("warning - error"), TEXT("sendMEssage"), MB_OK);
				
		}
	}
	else {
		MessageBox(global_hWnd, TEXT("unic"), TEXT("sending to:"), MB_OK);
		if (clientsInfo[sendMsg.to].communication == 0) {
			//_tprintf(TEXT("Responding via DLL\n"));
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
	
	if (sendMsg.to == (MAX_CLIENTS - 1)) {//responded that is full, resets space for other
		if (sendMsg.connection == 0)
			CloseHandle(clientsInfo[MAX_CLIENTS - 1].hClientMsg);
		else {
			CloseHandle(clientsInfo[MAX_CLIENTS - 1].hClientMsg);
			CloseHandle(clientsInfo[MAX_CLIENTS - 1].hClientGame);
		}
		clientsInfo[MAX_CLIENTS - 1].communication = -1;
	}
	//_tprintf(TEXT("Sent:'%s'\tCode=%d\tFrom=%d\tTo=%d\n"),sendMsg.messageInfo, sendMsg.codigoMsg, sendMsg.from, sendMsg.to);
	//_tprintf(TEXT("Sent codigo:(%d)\n"), sendMsg.codigoMsg);
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
		_tprintf(TEXT("Erro ao criar writeRead Event. Erro = %d\n"), GetLastError());
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
	_tprintf(TEXT("Write concluido...\n"));

	GetOverlappedResult(hPipe, &OverlWr, &bytesWritten, FALSE);
	if (bytesWritten < sizeof(msg)) {
		_tprintf(TEXT("Write File failed... | Erro = %d\n"), GetLastError());
	}

	//_tprintf(TEXT("[ESCRITOR] Enviei %d bytes ao leitor...(WriteFile)\n"), bytesWritten);
	return;
}

DWORD createHandle(msg newMsg) {
	TCHAR tmp[TAM];
	BOOLEAN flag = TRUE;
	int i;
	for (i = 0; i < MAX_CLIENTS - 1; i++) {//saves one to respond to clients that is full
		//_itot_s(clientsInfo[i].communication, tmp, TAM, 10);//translates num to str
		//MessageBox(global_hWnd, tmp, TEXT("client[i].communication = "), MB_OK);
		//if (clientsInfo[i].hClientMsg == NULL) {
		if (clientsInfo[i].communication == -1) {
			flag = FALSE;
			break;
		}
	}

	int num_temp = i;
	_itot_s(i, tmp, TAM, 10);//translates num to str
	MessageBox(global_hWnd, tmp, TEXT("final:"), MB_OK);

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
	//_tprintf(TEXT("Would you like to change defaut values for the game?(Y/N):"));
	//fflush(stdin);
	//resp = _gettch();
	//if (resp == 'y' || resp == 'Y') {
		/*int res = readFromFile();
		if (res) {
			_tprintf(TEXT("Error changing default values"));
		}
		else {
			_tprintf(TEXT("Default values changed wiht success!"));
		}*/
	//}

	gameInfo->gameStatus = 0;
	gameInfo->numUsers = 0;
	_tcscpy_s(frase, TAM, TEXT("New game created"));
	InvalidateRect(global_hWnd, NULL, TRUE);
}

DWORD WINAPI startGameThread(LPVOID param){
	int i;
	msg tmpMsg;
	for (i = 0; i < gameInfo->numUsers; i++) {
		userInit(i);
	}

	gameInfo->gameStatus = 1;
	SetEvent(updateGame);
	Sleep(250);
	tmpMsg.codigoMsg = 100;//new game
	tmpMsg.from = server_id;
	tmpMsg.to = 255; //broadcast
	_tcscpy_s(tmpMsg.messageInfo, TAM, TEXT("gameStart"));
	sendMessage(tmpMsg);
	
	Sleep(1000);
	_tcscpy_s(frase, TAM, TEXT("Game started"));
	InvalidateRect(global_hWnd, NULL, TRUE);
	//create bricks
	//assignBrick(gameInfo->myconfig.initial_bricks);
	return 1;
}


void userInit(DWORD id) {
	if (id != 0) {
		gameInfo->nUsers[id].posx = gameInfo->nUsers[id - 1].posx + gameInfo->nUsers[id].size + 5;
	}
	else {
		gameInfo->nUsers[id].posx = 5;
	}

	gameInfo->nUsers[id].posy = gameInfo->myconfig.limy - 2;
	gameInfo->nUsers[id].lifes = gameInfo->myconfig.inital_lifes;

	//f_tprintf(TEXT("(User[%d]->%s | pos(%d,%d)\n"), id, gameInfo->nUsers[id].name, gameInfo->nUsers[id].posx, gameInfo->nUsers[id].posy);

	return;
}


