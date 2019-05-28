//Base.c
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
	//local
DWORD WINAPI receiveLocalMsg(LPVOID param);//thread for dll msg

//functions
LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK resolveMenu(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK print(TCHAR line[TAM]);
DWORD startVars();
void resetUser(DWORD id);
void resetBall(DWORD id);
void resetBrick(DWORD id);


//vars
TCHAR szProgName[] = TEXT("Server Base");
HWND global_hWnd = NULL;

//client related
comuciationHandle clientsInfo[MAX_CLIENTS];
DWORD tmp_client_id;
HANDLE hPipeClient;

//server related
HANDLE updateGame;//event to update game for pipe users
HANDLE messageEventServer;//event to update server of dll msg
HANDLE hResolveMessageMutex;//handle to resolveMessage function
DWORD server_id;
HANDLE hTBola[MAX_BALLS];

pgame gameInfo;//has game Information


int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {
	
	HWND hWnd; // hWnd � o handler da janela, gerado mais abaixo por CreateWindow()
	MSG lpMsg; // MSG � uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp; // WNDCLASSEX � uma estrutura cujos membros servem para
    // definir as caracter�sticas da classe da janela
    // ============================================================================
    // 1. Defini��o das caracter�sticas da janela "wcApp"
    // (Valores dos elementos da estrutura "wcApp" do tipo WNDCLASSEX)
    // ============================================================================
	wcApp.cbSize = sizeof(WNDCLASSEX); // Tamanho da estrutura WNDCLASSEX
	wcApp.hInstance = hInst; // Inst�ncia da janela actualmente exibida
    // ("hInst" � par�metro de WinMain e vem inicializada da�)
	wcApp.lpszClassName = szProgName; // Nome da janela (neste caso = nome do programa)
	wcApp.lpfnWndProc = TrataEventos; // Endere�o da fun��o de processamento da janela
    // ("TrataEventos" foi declarada no in�cio e encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW;// Estilo da janela: Fazer o redraw se for modificada horizontal ou verticalmente
	wcApp.hIcon = LoadIcon(NULL, IDI_APPLICATION);// "hIcon" = handler do �con normal

	wcApp.hIconSm = LoadIcon(NULL, IDI_APPLICATION);// "hIconSm" = handler do �con pequeno

	wcApp.hCursor = LoadCursor(NULL, IDC_ARROW); // "hCursor" = handler do cursor (rato)

	wcApp.lpszMenuName = MAKEINTRESOURCE(IDM_MENU); // (NULL = n�o tem menu)
	wcApp.cbClsExtra = 0; // Livre, para uso particular
	wcApp.cbWndExtra = 0; // Livre, para uso particular

	wcApp.hbrBackground = CreateSolidBrush(RGB(98, 66, 244));

	if (!RegisterClassEx(&wcApp))
		return(0);

	hWnd = CreateWindow(
		szProgName, // Nome da janela (programa) definido acima
		TEXT("SO2 - Ficha 6 - Ex1_V2"),// Texto que figura na barra do t�tulo
		WS_OVERLAPPEDWINDOW, // Estilo da janela (WS_OVERLAPPED= normal)
		5, // Posi��o x pixels (default=� direita da �ltima)
		5, // Posi��o y pixels (default=abaixo da �ltima)
		800, // Largura da janela (em pixels)
		600, // Altura da janela (em pixels)
		(HWND)HWND_DESKTOP, // handle da janela pai (se se criar uma a partir de
		// outra) ou HWND_DESKTOP se a janela for a primeira,
		// criada a partir do "desktop"
		(HMENU)NULL, // handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst, // handle da inst�ncia do programa actual ("hInst" �
		// passado num dos par�metros de WinMain()
		0); // N�o h� par�metros adicionais para a janela
	ShowWindow(hWnd, nCmdShow);
	global_hWnd = hWnd;
	UpdateWindow(hWnd); // Refrescar a janela (Windows envia � janela uma

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
		print(TEXT("Erro ao criar recursos para o jogo!"));
		Sleep(5000);
		PostQuitMessage(1);
	}

	//Message
	createSharedMemoryMsg();
	//Game
	gameInfo = createSharedMemoryGame();

	_tcscpy_s(gameInfo->myconfig.file, TAM, TEXT("none"));//file input

	//starts gameInfo vars
	res = startVars();
	if (res) {
		_tprintf(TEXT("Erro ao iniciar as variaveis!"));
	}

	//pipes for initial connection
		//msg
	//HANDLE hMainPipe = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)connectPipeMsg, NULL, 0, NULL);
	//if (hMainPipe == NULL) {
	//	_tprintf(TEXT("Erro ao criar thread hMainPipe!"));
	//	return -1;
	//}
	//game
	//HANDLE hUpdateGame = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)connectPipeGame, NULL, 0, NULL);
	//if (hUpdateGame == NULL) {
	//	_tprintf(TEXT("Erro ao criar thread hMainPipe!"));
	//	return -1;
	//}

	//memory
	HANDLE hTReceiveMessage = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)receiveLocalMsg, NULL, 0, NULL);
	if (hTReceiveMessage == NULL) {
		_tprintf(TEXT("Erro ao criar thread ReceiveMessage!"));
		return -1;
	}	

	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg); // Pr�-processamento da mensagem (p.e. obter c�digo
	   // ASCII da tecla premida)
		DispatchMessage(&lpMsg); // Enviar a mensagem traduzida de volta ao Windows, que
	   // aguarda at� que a possa reenviar � fun��o de
	   // tratamento da janela, CALLBACK TrataEventos (abaixo)
	}
	// ============================================================================
	// 6. Fim do programa
	// ============================================================================
	return((int)lpMsg.wParam); // Retorna sempre o par�metro wParam da estrutura lpMsg
}
// ============================================================================
// FUN��O DE PROCESSAMENTO DA JANELA
// Esta fun��o pode ter um nome qualquer: Apenas � neces�rio que na inicializa��o da
// estrutura "wcApp", feita no in�cio de WinMain(), se identifique essa fun��o. Neste
// caso "wcApp.lpfnWndProc = WndProc"
//
// WndProc recebe as mensagens enviadas pelo Windows (depois de lidas e pr�-processadas
// no loop "while" da fun��o WinMain()
//
// Par�metros:
// hWnd O handler da janela, obtido no CreateWindow()
// messg Ponteiro para a estrutura mensagem (ver estrutura em 5. Loop...
// wParam O par�metro wParam da estrutura messg (a mensagem)
// lParam O par�metro lParam desta mesma estrutura
//
// NOTA:Estes par�metros est�o aqui acess�veis o que simplifica o acesso aos seus valores
//
// A fun��o EndProc � sempre do tipo "switch..." com "cases" que descriminam a mensagem
// recebida e a tratar. Estas mensagens s�o identificadas por constantes (p.e.
// WM_DESTROY, WM_CHAR, WM_KEYDOWN, WM_PAINT...) definidas em windows.h
//============================================================================
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
	DWORD res;
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
		BitBlt(memDC, xBitMap, yBitMap, bmp.bmWidth, bmp.bmHeight, tempDC, 0, 0, SRCCOPY);
		ReleaseDC(hWnd, hDC);

		break;
	case WM_KEYDOWN:
		GetClientRect(hWnd, &rect);
		switch (LOWORD(wParam)) {
		case VK_UP:
			yBitMap = yBitMap > 0 ? yBitMap - 10 : 0;
			break;
		case VK_DOWN:
			yBitMap = yBitMap < rect.bottom - bmp.bmHeight ? yBitMap + 10 : rect.bottom - bmp.bmHeight;
			//yBitMap += 10;
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
		//tempDC = CreateCompatibleDC(memDC);

		//SelectObject(tempDC, hBmp);

		//PatBlt(memDC, 0, 0, maxX, maxY, PATCOPY);

		//BitBlt(memDC, 0, 0, bmp.bmWidth, bmp.bmHeight, tempDC, 0, 0, SRCCOPY);
		

		//DeleteDC(tempDC);

		GetClientRect(hWnd, &rect);
		SetTextColor(memDC, RGB(255, 255, 255));
		SetBkMode(memDC, TRANSPARENT);
		rect.left = xPrint;
		rect.top = yPrint++;
		DrawText(memDC, frase, _tcslen(frase), &rect, DT_SINGLELINE | DT_NOCLIP);

		hDC = BeginPaint(hWnd, &ps);

		BitBlt(hDC, 0, 0, maxX, maxY, memDC, 0, 0, SRCCOPY);

		EndPaint(hWnd, &ps);

		break;
	case WM_COMMAND:
		resolveMenu(hWnd, messg, wParam, lParam);
		break;
	case WM_CLOSE:
		res = MessageBox(hWnd, TEXT("Are you sure you want to quit?"), TEXT("Quit MessageBox"), MB_ICONQUESTION | MB_YESNO);
		if (res == IDYES)
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
	switch (LOWORD(wParam)) {
		case ID_CREATEGAME:
			break;
		case ID_STARTGAME:
			break;
		case ID_TOP10:
			MessageBeep(MB_ICONSTOP);
			break;
		case ID_USERSLOGGED:
			break;
		case ID_SETTINGS:
			break;
	}
	return;
}

LRESULT CALLBACK print(TCHAR line[TAM]) {
	_tcscpy_s(frase, TAM, line);
	InvalidateRect(global_hWnd, NULL, TRUE);
}

//thread to receive initial clients via pipe for msg
DWORD WINAPI connectPipeMsg(LPVOID param) {
	BOOLEAN fConnected = 0;
	HANDLE hPipeInit = INVALID_HANDLE_VALUE;
	HANDLE hThread;

	DWORD nSent;

	msg buf;

	do {

		hPipeInit = CreateNamedPipe(INIT_PIPE_MSG_NAME,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE | // tipo de pipe = message
			PIPE_READMODE_MESSAGE | // com modo message-read e
			PIPE_WAIT, // bloqueante (n�o usar PIPE_NOWAIT nem mesmo em Asyncr)
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
		fConnected = ConnectNamedPipe(hPipeInit, NULL);
		if (!fConnected && GetLastError() == ERROR_PIPE_CONNECTED)
			fConnected = 1;

		_tprintf(TEXT("Got a client...\n"));

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
	DWORD nSent;

	/*HANDLE sendGamePipe = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)updateGamePipe, NULL, 0, NULL);
	if (sendGamePipe == NULL) {
		_tprintf(TEXT("Error createing updateGamePipe!\n"));
		return -1;
	}*/

	_tprintf(TEXT("iniciating pipe game!\n"));

	do {

		hPipeInit = CreateNamedPipe(INIT_PIPE_GAME_NAME,
			PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, // bloqueante (n�o usar PIPE_NOWAIT nem mesmo em Asyncr)
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

		_tprintf(TEXT("Got a client for game...\n"));

		if (fConnected) {
			clientsInfo[tmp_client_id].hClientGame = hPipeInit;//got a new client to send the game to
		}
		else {
			CloseHandle(hPipeInit);
		}

	} while (1);

	return 0;
}

//thread for each client pipe
DWORD WINAPI pipeClientMsg(LPVOID param) {
	msg inMsg;
	DWORD cbBytesRead, cbReplyBytes, cbWritten, resp;
	BOOLEAN fSuccess;
	HANDLE hPipe = (HANDLE)param; // a informa��o enviada � thread � o handle do pipe
	if (hPipe == NULL) {
		_tprintf(TEXT("\nErro � o handle enviado no param da thread � nulo"));
		return -1;
	}

	hPipeClient = hPipe;

	HANDLE ReadReady;
	OVERLAPPED OverlRd = { 0 };

	ReadReady = CreateEvent(
		NULL, // default security + non inheritable
		TRUE, // Reset manual, por requisito do overlapped IO => uso de ResetEvent
		FALSE, // estado inicial = not signaled
		NULL); // n�o precisa de nome: uso interno ao processo
	if (ReadReady == NULL) {
		_tprintf(TEXT("\nServidor: n�o foi poss�vel criar o evento Read. Mais vale parar j�"));
		return 1;
	}

	while (1) {
		ZeroMemory(&OverlRd, sizeof(OverlRd));
		ResetEvent(ReadReady);
		OverlRd.hEvent = ReadReady;

		fSuccess = ReadFile(
			hPipe, // handle para o pipe (recebido no param)
			&inMsg, // buffer para os dados a ler
			sizeof(msg), // Tamanho msg a ler
			&cbBytesRead, // n�mero de bytes lidos
			&OverlRd); // != NULL -> � overlapped I/O

		WaitForSingleObject(ReadReady, INFINITE);
		_tprintf(TEXT("\n[SERVER]Got a message\n"));
		GetOverlappedResult(hPipe, &OverlRd, &cbBytesRead, FALSE); // sem WAIT
		if (cbBytesRead < sizeof(msg))
			_tprintf(TEXT("\nReadFile n�o leu os dados todos. Erro = %d"), GetLastError()); // acrescentar l�gica de encerrar a thread cliente

		//processa info recebida
		WaitForSingleObject(hResolveMessageMutex, INFINITE);
		//resp = resolveMessage(inMsg);
		ReleaseMutex(hResolveMessageMutex);
	}

	//end client
	//removeCliente(hPipe);
	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe); // Desliga servidor da inst�ncia
	CloseHandle(hPipe); // Fecha este lado desta inst�ncia
	_tprintf(TEXT("\nThread dedicada Cliente a terminar"));
	return 1;
}

//thread to receive msg via DLL
DWORD WINAPI receiveLocalMsg(LPVOID param) {//local communication
	msg inMsg;
	do {
		WaitForSingleObject(messageEventServer, INFINITE);
		inMsg = receiveMessageDLL();
		WaitForSingleObject(hResolveMessageMutex, INFINITE);
		//resolveMessage(inMsg);
		ReleaseMutex(hResolveMessageMutex);
	} while (1);
}

DWORD startVars() {

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	int i;

	//Status
	gameInfo->gameStatus = -1;
	server_id = 254;
	//default config
	//game
	gameInfo->myconfig.limx = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	gameInfo->myconfig.limy = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
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

	for (i = 0; i < MAX_USERS; i++) {
		clientsInfo[i].hClientMsg = NULL;
		clientsInfo[i].hClientGame = NULL;
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
	gameInfo->nUsers[id].connection_mode = NULL;
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
