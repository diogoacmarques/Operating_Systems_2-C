#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include "DLL.h"
#include "resource.h"


//threads
DWORD WINAPI localConnection(LPVOID param);
DWORD WINAPI BrickThread(LPVOID param);
DWORD WINAPI msgPipe(LPVOID param);
DWORD WINAPI gamePipe(LPVOID param);
//functions
void print(msg printMsg);

LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK resolveMenu(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK resolveConection(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
void resolveKey(WPARAM wParam);

void createLocalConnection();
void createRemoteConnection();

void LoginUser(TCHAR user[MAX_NAME_LENGTH]);

void usersMove(TCHAR move[TAM]);

DWORD resolveMessage(msg inMsg);

void sendMessage(msg sendMsg);

TCHAR szProgName[] = TEXT("Client");
int connection_mode = -1;//0 = local / 1 = remote
HWND global_hWnd = NULL;

//bmp
HBITMAP hBmpBarreira;//[MAX_CLIENTS] = NULL;
HBITMAP hBmpBola;//[MAX_BALLS] = NULL;
HBITMAP hBmpBrick;//[MAX_BRICKS] = NULL;

//Variaveis
	//print
int xPrint = 0, yPrint = 0;
TCHAR frase[TAM];

	//window
int maxX = 0, maxY = 0;
HDC memDC = NULL;
HBITMAP hBit = NULL;
HBRUSH hBrush = NULL;
HDC tempDC = NULL;
HBITMAP hBmp = NULL;
BITMAP bmp;

	//game
pgame gameInfo;
TCHAR login[MAX_NAME_LENGTH];
DWORD client_id = -1;//identification of program so the server knows where to send info
DWORD localGameStatus = 0;
int xBitMap = 0, yBitMap = 0;
int xBall=0, yBall=0;

	//handles
HANDLE gameReady;
HANDLE  messageEvent, hStdoutMutex;
HANDLE hTMsgConnection;//has thread where receives messages
HANDLE hTBrick;
HANDLE hPipeMsg;
HANDLE hPipeGame;


	//comunication
BOOLEAN canSendMsg = TRUE;
//end of variables

//HANDLES
HANDLE hTUserInput;



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
   // ("TrataEventos" foi declarada no início e
   // encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW;// Estilo da janela: Fazer o redraw se for
   // modificada horizontal ou verticalmente
	wcApp.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LOGGO));// "hIcon" = handler do ícon normal
   //"NULL" = Icon definido no Windows
   // "IDI_AP..." Ícone "aplicação"

	//here
	wcApp.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LOGGO));// "hIconSm" = handler do ícon pequeno

	//"NULL" = Icon definido no Windows
	// "IDI_INF..." Ícon de informação
	wcApp.hCursor = LoadCursor(hInst, MAKEINTRESOURCE(IDC_INIT)); // "hCursor" = handler do cursor (rato)
   // "NULL" = Forma definida no Windows
   // "IDC_ARROW" Aspecto "seta"
	wcApp.lpszMenuName = MAKEINTRESOURCE(IDM_MENU); // Classe do menu que a janela pode ter
   // (NULL = não tem menu)
	wcApp.cbClsExtra = 0; // Livre, para uso particular
	wcApp.cbWndExtra = 0; // Livre, para uso particular

	wcApp.hbrBackground = CreateSolidBrush(RGB(244, 206, 66));

	if (!RegisterClassEx(&wcApp))
		return(0);
	// ============================================================================
	// 3. Criar a janela
	// ============================================================================

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
	// ============================================================================
	// 4. Mostrar a janela
	// ============================================================================
	if (connection_mode == -1) {
		DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_CONNECTION), NULL, resolveConection);
		if (connection_mode == -1)
			return 0;
	}

	ShowWindow(hWnd, nCmdShow); // "hWnd"= handler da janela, devolvido por
	// "CreateWindow"; "nCmdShow"= modo de exibição (p.e.
	// normal/modal); é passado como parâmetro de WinMain()
	global_hWnd = hWnd;
	UpdateWindow(hWnd); // Refrescar a janela (Windows envia à janela uma

	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg); // Pré-processamento da mensagem (p.e. obter código
	   // ASCII da tecla premida)
		DispatchMessage(&lpMsg); // Enviar a mensagem traduzida de volta ao Windows, que
	   // aguarda até que a possa reenviar à função de
	   // tratamento da janela, CALLBACK TrataEventos (abaixo)
	}

	return((int)lpMsg.wParam); // Retorna sempre o parâmetro wParam da estrutura lpMsg
}


LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	HDC hDC;
	RECT rect;
	PAINTSTRUCT ps;
	TCHAR tmp[TAM];
	TCHAR tmp2[30];
	int res;
	switch (messg) {
	case WM_LBUTTONDOWN:

		break;

	case WM_CREATE:
		maxX = GetSystemMetrics(SM_CXSCREEN);
		maxY = GetSystemMetrics(SM_CYSCREEN);

		hDC = GetDC(hWnd);

		memDC = CreateCompatibleDC(hDC);

		hBit = CreateCompatibleBitmap(hDC, maxX, maxY);

		SelectObject(memDC, hBit);

		DeleteObject(hBit);

		hBrush = CreateSolidBrush(RGB(36, 140, 117));

		SelectObject(memDC, hBrush);

		PatBlt(memDC, 0, 0, maxX, maxY, PATCOPY);

		ReleaseDC(hWnd, hDC);

		//hBmp = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/background.bmp"),
			//IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

		//GetObject(hBmp, sizeof(bmp), &bmp);
		break;

	case WM_KEYDOWN:
		resolveKey(wParam);
		break;
	case WM_PAINT:
		tempDC = CreateCompatibleDC(memDC);

		SelectObject(tempDC, hBmp);

		PatBlt(memDC, 0, 0, maxX, maxY, PATCOPY);

		//BitBlt(memDC, 0, 0, bmp.bmWidth, bmp.bmHeight, tempDC, 0, 0, SRCCOPY);

		BitBlt(memDC, xBitMap, yBitMap, bmp.bmWidth, bmp.bmHeight, tempDC, 0, 0, SRCCOPY);

		DeleteDC(tempDC);

		GetClientRect(hWnd, &rect);
		SetTextColor(memDC, RGB(255, 255, 255));
		SetBkMode(memDC, TRANSPARENT);
		rect.left = 0;
		rect.top = yPrint++;
		DrawText(memDC, frase, _tcslen(frase), &rect, DT_SINGLELINE | DT_NOCLIP);

		hDC = BeginPaint(hWnd, &ps);

		BitBlt(hDC, 0, 0, maxX, maxY, memDC, 0, 0, SRCCOPY);

		EndPaint(hWnd, &ps);
		_tcscpy_s(frase, TAM, TEXT("-"));
		break;
	case WM_COMMAND:

		switch (LOWORD(wParam)) {
		case ID_PLAY:												//NULL em vez de hWnd para a janela nao ter "prioridade"
			res = DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG_LOGIN), hWnd, resolveMenu);
			if (res == IDCANCEL || res == IDABORT)
				break;
			if (_tcscmp(login, TEXT("")) != 0) {
				MessageBox(hWnd, login, TEXT("Trying to login with:"), MB_OK);//sucesso
				for (int i = 0; i < MAX_NAME_LENGTH; i++)//preventes users from using ':' (messes up registry)
					if (login[i] == ':')
						login[i] = '\0';
			}
			LoginUser(login);
				
			break;
			case ID_WATCH:
				//MessageBeep(MB_ICONQUESTION);
				break;
			case ID_TOP10:
				//MessageBeep(MB_ICONSTOP);
				break;
			case ID_ABOUT_TYPE:
				_tcscpy_s(tmp, TAM, TEXT("This is a "));
						
				if(connection_mode == 0)
					_tcscat_s(tmp, TAM, TEXT("local "));
				else if(connection_mode == 1)
					_tcscat_s(tmp, TAM, TEXT("remote "));
				else
					_tcscat_s(tmp, TAM, TEXT("Hacked? "));

				_tcscat_s(tmp, TAM, TEXT("conection || My client id = "));
				_itot_s(client_id, tmp2, 30, 10);//translates num to str
				_tcscat_s(tmp, TAM, tmp2);//ads


				_tcscat_s(tmp, TAM, TEXT(" || BTW GameStatus = "));
				_itot_s(gameInfo->gameStatus, tmp2, 30, 10);//translates num to str
				_tcscat_s(tmp, TAM, tmp2);//ads

				MessageBox(hWnd,tmp, TEXT("Type of connection"), MB_OK);
				break;
			case ID_ABOUT_VERSION:
				MessageBox(hWnd, TEXT("Version 0.1! - Made by Diogo Marques"), TEXT("About Arknoid"), MB_OK);
				break;
		}
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

	switch (messg) {
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK) {
			GetDlgItemText(hWnd, IDC_EDIT_LOGIN, login, 100);			
			EndDialog(hWnd, 0);
			return TRUE;
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			_stprintf_s(login, 100, TEXT(""));
			EndDialog(hWnd, 0);
			return TRUE;
		}

	}

	return FALSE;
}

LRESULT CALLBACK resolveConection(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	switch (messg) {
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_BUTTON_LOCAL) {
			connection_mode = 0;
			createLocalConnection();
			EndDialog(hWnd, 0);
			return TRUE;
		}
		else if (LOWORD(wParam) == IDC_BUTTON_REMOTE) {
			connection_mode = 1;
			createRemoteConnection();
			EndDialog(hWnd, 0);
			return TRUE;
		}
	case WM_CLOSE:
		EndDialog(hWnd, 0);
		return FALSE;
	}
	return FALSE;
}

void resolveKey(WPARAM wParam) {
	if (localGameStatus == -1)
		return;

	msg gameMsg;
	gameMsg.codigoMsg = 200;
	gameMsg.from = client_id;
	gameMsg.to = 254;
	_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("not"));
	switch (LOWORD(wParam)) {
	case VK_LEFT:
	case 0x41://a key
		if (localGameStatus == 2)//watching
			return;
		_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("left"));
		break;
	case VK_RIGHT:
	case 0x44://d key
		if (localGameStatus == 2)//watching
			return;
		_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("right"));
		break;
	case VK_SPACE:
		if (localGameStatus == 2)//watching
			return;
		gameMsg.codigoMsg = 101;
		_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("ball"));
		break;

	case VK_ESCAPE:
		if (localGameStatus == 2) {//watching
			//endUser();
		}

		gameMsg.codigoMsg = 2;
		_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("exit"));
		break;
	}

	if (_tcscmp(gameMsg.messageInfo, TEXT("not")) != 0) {
		sendMessage(gameMsg);
		_tcscpy_s(frase, TAM, gameMsg.messageInfo);
		InvalidateRect(global_hWnd, NULL, TRUE);
	}
		
}

void createLocalConnection() {
	TCHAR str[TAM];
	TCHAR tmp[TAM];
	_tcscpy_s(str, TAM, LOCAL_CONNECTION_NAME);
	_itot_s(GetCurrentThreadId(), tmp, TAM, 10);
	_tcscat_s(str, TAM, tmp);
	//print

	messageEvent = CreateEvent(NULL, FALSE, FALSE, str);
	updateBalls = CreateEvent(NULL, TRUE, FALSE, BALL_EVENT_NAME);
	updateBonus = CreateEvent(NULL, FALSE, FALSE, BONUS_EVENT_NAME);
	hStdoutMutex = CreateMutex(NULL, FALSE, NULL);
	gameReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (messageEvent == NULL || updateBalls == NULL || updateBonus == NULL || hStdoutMutex == NULL || gameReady == NULL) {
		MessageBox(global_hWnd, TEXT("Error creating resources..."), TEXT("Resources"), MB_OK);
		PostQuitMessage(1);
	}

	BOOLEAN res = initializeHandles();
	if (res) {
		//print(TEXT("Erro ao criar Handles!"));
		//return 1;
	}
	
	//sessionId = (GetCurrentThreadId() + GetTickCount());
	//sessionId = GetCurrentThreadId();

	//Message
	createSharedMemoryMsg();
	//Game
	//gameInfo = (game *)malloc(sizeof(game));
	gameInfo = createSharedMemoryGame();

	hTMsgConnection = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)localConnection, NULL, 0, NULL);
	if (hTMsgConnection == NULL) {
		MessageBox(global_hWnd, TEXT("Error creating local connction thread..."), TEXT("Resources"), MB_OK);
		PostQuitMessage(1);

	}

	msg tmpMsg;
	tmpMsg.codigoMsg = -9999;//new client
	tmpMsg.from = -1;
	tmpMsg.to = 254;
	_tcscpy_s(tmpMsg.messageInfo, TAM, str);
	sendMessage(tmpMsg); // lets server know of new client
	
	return;
}

DWORD WINAPI localConnection(LPVOID param) {
	msg newMsg;
	int quant = 0;
	DWORD resp;

	do {
		WaitForSingleObject(messageEvent, INFINITE);
		//MessageBox(global_hWnd, TEXT("you got a message"), TEXT("client"), MB_OK);
		newMsg = receiveMessageDLL();
		resp = resolveMessage(newMsg);
	} while (1);

	return 0;
}

void createRemoteConnection() {
	BOOL fSuccess = FALSE;
	DWORD dwMode;
	while (1) {

		hPipeMsg = CreateFile(
			INIT_PIPE_MSG_NAME,
			GENERIC_READ | GENERIC_WRITE,
			0 | FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0 | FILE_FLAG_OVERLAPPED,
			NULL
		);


		if (hPipeMsg != INVALID_HANDLE_VALUE) {
			//print(TEXT("I got a valid hPipe\n"));
			break;
		}


		if (GetLastError() != ERROR_PIPE_BUSY) {
			//print(TEXT("Deu erro e nao foi de busy. Erro = %d\n"), GetLastError());
			return;
		}


		if (!WaitNamedPipe(INIT_PIPE_MSG_NAME, 10000)) {
			//print(TEXT("Waited 10 seconds and cant find a pipe, I give up...\n"));
			return;
		}

	}

	dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		hPipeMsg,
		&dwMode,
		NULL,
		NULL
	);

	if (!fSuccess) {
		//print(TEXT("SetNamedPipeHandleState falhou. Erro = %d\n"), GetLastError());
		return;
	}

	//print(TEXT("initianting pipeConnection thread\n"));
	hTMsgConnection = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)msgPipe, NULL, 0, NULL);
	if (hTMsgConnection == NULL) {
		//print(TEXT("Erro na criãção da thread para remotePipe. Erro = %d\n"), GetLastError());
		return;
	}

	
	Sleep(250);
	//MessageBox(global_hWnd, TEXT("Can i send message?"), TEXT("can i?"), MB_OK);
	//print(TEXT("sending message ...\n"));
	//MessageBox(global_hWnd, TEXT("Sending message..."), TEXT("init connection"), MB_OK);
	msg tmpMsg;
	tmpMsg.codigoMsg = -9999;
	tmpMsg.from = client_id;
	tmpMsg.to = 254;
	_tcscpy_s(tmpMsg.messageInfo, TAM, TEXT("remoteClient"));
	sendMessage(tmpMsg); // lets server know of new client
	return;
}

DWORD WINAPI msgPipe(LPVOID param) {
	msg inMsg;
	DWORD resp;
	DWORD bytesRead = 0;
	BOOLEAN fSuccess = FALSE;
	HANDLE ReadReady;
	OVERLAPPED OverlRd = { 0 };

	ReadReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ReadReady == NULL) {
		_tprintf(TEXT("Erro na criãção do event ReadReady. Erro = %d\n"), GetLastError());
		return -1;
	}

	_tcscpy_s(frase, TAM, TEXT("connected to a msg pipe"));
	InvalidateRect(global_hWnd, NULL, TRUE);

	while (1) {
		ZeroMemory(&OverlRd, sizeof(OverlRd));
		OverlRd.hEvent = ReadReady;
		ResetEvent(ReadReady);

		//MessageBox(global_hWnd, TEXT("will now wait for a pipe msg..."), TEXT("WAIT..."), MB_OK);
		fSuccess = ReadFile(
			hPipeMsg,
			&inMsg,
			sizeof(msg),
			&bytesRead,
			&OverlRd
		);

		WaitForSingleObject(ReadReady, INFINITE);//wait for read to be complete
		//_tprintf(TEXT("Read Done...\n"));

		//MessageBox(global_hWnd, TEXT("Got a message!!!!"), TEXT("Got messsage"), MB_OK);

		GetOverlappedResult(hPipeMsg, &OverlRd, &bytesRead, FALSE);
		if (bytesRead < sizeof(msg)) {
			_tprintf(TEXT("Read File failed... | Erro = %d\n"), GetLastError());
		}

		//_tprintf(TEXT("I got a message from pipe:\n"));
		//_tprintf(TEXT("Codigo:%d\nto:%d\nfrom:%d\ncontent:%s\n"), inMsg.codigoMsg, inMsg.to, inMsg.from, inMsg.messageInfo);
		//WaitForSingleObject(hStdoutMutex, INFINITE);
		//gotoxy(0, 13);
		//_tprintf(TEXT("MSG->COD(%d)-'%s'"), inMsg.codigoMsg,inMsg.messageInfo);
		//ReleaseMutex(hStdoutMutex);

		resp = resolveMessage(inMsg);
	}
	return 1;
}

DWORD WINAPI gamePipe(LPVOID param) {

	HANDLE gamePipe;
	//BOOLEAN res;
	DWORD bytesRead;// dwMode;
	HANDLE ReadReady;
	BOOLEAN fSuccess = FALSE;
	OVERLAPPED OverlRd = { 0 };


	while (1) {
		_tprintf(TEXT("Create file for game\n"));
		gamePipe = CreateFile(
			INIT_PIPE_GAME_NAME,
			GENERIC_READ,
			0,
			NULL,
			OPEN_EXISTING,
			0 | FILE_FLAG_OVERLAPPED,
			NULL
		);


		if (gamePipe != INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("I got a valid hPipe\n"));
			break;
		}


		if (GetLastError() != ERROR_PIPE_BUSY) {
			_tprintf(TEXT("Deu erro e nao foi de busy. Erro = %d\n"), GetLastError());
			return -1;
		}


		if (!WaitNamedPipe(INIT_PIPE_GAME_NAME, 10000)) {
			_tprintf(TEXT("Waited 10 seconds and cant find a pipe, I give up...\n"));
			return -1;
		}

	}

	gameInfo = (pgame)malloc(sizeof(game));
	if (gameInfo == NULL) {
		return -1;
	}


	ReadReady = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (ReadReady == NULL) {
		_tprintf(TEXT("Erro na criãção do event ReadReady. Erro = %d\n"), GetLastError());
		return -1;
	}

	int quant = 0;

	while (1) {
		//_tprintf(TEXT("Waiting to receive game..\n"));
		ZeroMemory(&OverlRd, sizeof(OverlRd));
		OverlRd.hEvent = ReadReady;
		ResetEvent(ReadReady);

		fSuccess = ReadFile(
			gamePipe,
			gameInfo,
			sizeof(game),
			&bytesRead,
			&OverlRd
		);

		WaitForSingleObject(ReadReady, INFINITE);//sssswait for read to be complete
		//_tprintf(TEXT("Read Done...\n"));

		GetOverlappedResult(gamePipe, &OverlRd, &bytesRead, FALSE);
		if (bytesRead < sizeof(game)) {
			_tprintf(TEXT("Read File failed... | Erro = %d\n"), GetLastError());
		}

		SetEvent(gameReady);
		ResetEvent(gameReady);

		//WaitForSingleObject(hStdoutMutex,INFINITE);
		//gotoxy(0, 10);
		//_tprintf(TEXT("GameStatus = %d\n"),gameInfo->gameStatus);
		//_tprintf(TEXT("[%d]Udated\n->Bricks:%d\n->Player_0:(%d,%d)"),quant++,gameInfo->numBricks,gameInfo->nUsers[0].posx, gameInfo->nUsers[0].posy);
		//ReleaseMutex(hStdoutMutex);

	}

	CloseHandle(gamePipe);
	Sleep(200);
	return 0;
}

void sendMessage(msg sendMsg) {
	HANDLE WriteReady;
	OVERLAPPED OverlWr = { 0 };
	DWORD bytesWritten;
	BOOLEAN fSuccess;
	
	if (canSendMsg)
		canSendMsg = FALSE;
	else
		return;

	sendMsg.connection = connection_mode;
	print(sendMsg);

	//MessageBox(global_hWnd, TEXT("Trynig to send message AGAIN..."), TEXT("MessageBox"), MB_OK);

	if (connection_mode == 0) {
		sendMessageDLL(sendMsg);
	}
	else if (connection_mode == 1) {
		WriteReady = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (WriteReady == NULL) {
			_tprintf(TEXT("Erro ao criar writeRead Event. Erro = %d\n"), GetLastError());
			return;
		}

		//_tprintf(TEXT("Ligaçao establecida...\n"));

		ZeroMemory(&OverlWr, sizeof(OverlWr));
		ResetEvent(WriteReady);
		OverlWr.hEvent = WriteReady;


		fSuccess = WriteFile(
			hPipeMsg,
			&sendMsg,
			sizeof(msg),
			&bytesWritten,
			&OverlWr
		);

		WaitForSingleObject(WriteReady, INFINITE);

		GetOverlappedResult(hPipeMsg, &OverlWr, &bytesWritten, FALSE);
		if (bytesWritten < sizeof(msg)) {
			_tprintf(TEXT("Write File failed... | Erro = %d\n"), GetLastError());
		}

		//_tprintf(TEXT("[ESCRITOR] Enviei %d bytes ao leitor...(WriteFile)\n"), bytesWritten);

	}
}

DWORD resolveMessage(msg inMsg) {
	canSendMsg = TRUE;
	print(inMsg);
	TCHAR tmp_info[TAM];
	//WaitForSingleObject(hStdoutMutex, INFINITE);
	//gotoxy(0, 0);
	//_tprintf(TEXT("\n\n[%d]Codigo:%d\nto:%d\nfrom:%d\ncontent:%s\n"),quantMsg++, inMsg.codigoMsg, inMsg.to, inMsg.from, inMsg.messageInfo);
	//ReleaseMutex(hStdoutMutex);
	_itot_s(inMsg.codigoMsg, tmp_info, TAM, 10);
	MessageBox(global_hWnd,inMsg.messageInfo,tmp_info, MB_OK);//sucesso
	if (inMsg.codigoMsg == 9999) {//first connection
		_tprintf(TEXT("New client allowed\n"));
		client_id = _tstoi(inMsg.messageInfo);
		if (connection_mode) {//if is via pipes then opens up receivegamepipe
			hPipeGame = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)gamePipe, NULL, 0, NULL);
			if (hPipeGame == NULL) {
				return 1;
			}
		}
		return 1;
	}
	else if (inMsg.codigoMsg == -9999) {
		MessageBox(global_hWnd, TEXT("NOT ALLOWED"), TEXT("client"), MB_OK);
		DestroyWindow(global_hWnd);//not destroying...
	}

	BOOLEAN logged = 0;
	if (inMsg.from != 254) {
		_tprintf(TEXT("Should receive from 254 but received instead from %d...\n"), inMsg.from);
		_tprintf(TEXT("Codigo:%d\nto:%d\nfrom:%d\ncontent:%s\n"), inMsg.codigoMsg, inMsg.to, inMsg.from, inMsg.messageInfo);
		Sleep(3000);
		return 0;
	}
	//WaitForSingleObject(hStdoutMutex, INFINITE);
	//gotoxy(0, 0);
	//_tprintf(TEXT("BROADCAST\n-to:%d    \n-from:%d     \n"), inMsg.to, inMsg.from);
	//ReleaseMutex(hStdoutMutex);

	if (inMsg.codigoMsg == 1 && !logged) {//successful login
		logged = 1;
		_tcscpy_s(frase, TAM, TEXT("You are now in the game, waiting for server to start..."));
		InvalidateRect(global_hWnd, NULL, TRUE);
	}
	else if (inMsg.codigoMsg == -1 && !logged) {
		_tprintf(TEXT("Server refused login with %s\n"), inMsg.messageInfo);
		//endUser();
		return -1;
	}
	else if (inMsg.codigoMsg == -100) {
		_tprintf(TEXT("There is no game created by the server yet\n"));
		//endUser();
		return -1;
	}
	else if (inMsg.codigoMsg == 100) {//start game
		_tcscpy_s(frase, TAM, TEXT("game started by the server"));
		//invalid rect will be done insde usersMoves
		localGameStatus = 1;
		usersMove(TEXT("init"));
	}
	else if (inMsg.codigoMsg == 101) {//new ball
		DWORD tmp = _tstoi(inMsg.messageInfo);
		//_tprintf(TEXT("creating %d balls thread para o utilizador!\n"),tmp);
		//createBalls(tmp);
	}
	else if (inMsg.codigoMsg == 102) {//create bricks
		hTBrick = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BrickThread, NULL, 0, NULL);
		if (hTBrick == NULL) {
			_tprintf(TEXT("Erro ao criar thread para desenhar bricks!\n"));
			return -1;
		}
	}
	else if (inMsg.codigoMsg == 103) {
		int tmp = _tstoi(inMsg.messageInfo);
		//createBonus(tmp);
	}
	else if (inMsg.codigoMsg == -110 && localGameStatus == 0) {
		//_tprintf(TEXT("A game is already in progress! Would you like to watch?(Y/N):"));
		//TCHAR resp;
		
		//if (resp == 'n' || resp == 'N') {
		//	return -1;
		//}
		//watchGame();
	}
	else if (inMsg.codigoMsg == 200) {
		usersMove(inMsg.messageInfo);
	}
	else if (inMsg.codigoMsg == -999) {
		//endUser();
	}
	return -1;
}

DWORD WINAPI BrickThread(LPVOID param) {
	//_tprintf(TEXT("should create %d bricks"), gameInfo->numBricks);
	/*brick localBricks[MAX_BRICKS];

	if (connection_mode == 1) {
		do {
			WaitForSingleObject(gameReady, INFINITE);
			if (gameInfo->numBricks > 0 && gameInfo->numBricks < MAX_BRICKS)
				break;
		} while (1);
	}

	int numBricks = gameInfo->numBricks;
	//draws intially all bricks
	for (int i = 0; i < numBricks; i++) {
		WaitForSingleObject(hStdoutMutex, INFINITE);

		if (gameInfo->nBricks[i].status <= 0 && localGameStatus == 2) {
			ReleaseMutex(hStdoutMutex);
			continue;
		}

		gotoxy(gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy);
		for (int j = 0; j < gameInfo->nBricks[i].tam; j++)
			_tprintf(TEXT("%d"), gameInfo->nBricks[i].status);
		ReleaseMutex(hStdoutMutex);
	}

	//updates
	do {
		WaitForSingleObject(updateBalls, INFINITE);
		//numBricks = gameInfo->numBricks;
		for (int i = 0; i < numBricks; i++)
			if (localBricks[i].status != gameInfo->nBricks[i].status) {
				if (gameInfo->nBricks[i].status == 0) {//end of brick life
					WaitForSingleObject(hStdoutMutex, INFINITE);
					gotoxy(gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy);
					for (int j = 0; j < gameInfo->nBricks[i].tam; j++)
						_tprintf(TEXT(" "));
					ReleaseMutex(hStdoutMutex);
				}
				else {
					WaitForSingleObject(hStdoutMutex, INFINITE);
					gotoxy(gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy);
					for (int j = 0; j < gameInfo->nBricks[i].tam; j++)
						_tprintf(TEXT("%d"), gameInfo->nBricks[i].status);
					ReleaseMutex(hStdoutMutex);
					localBricks[i] = gameInfo->nBricks[i];
				}

			}
	} while (gameInfo->numBricks);
	*/
	return 0;
}

void usersMove(TCHAR move[TAM]) {
	hBmp = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/barreiratmp.bmp"),IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	xBitMap = maxX / 2;
	yBitMap = maxY / 2;
	hBmpBarreira = hBmp;
	GetObject(hBmp, sizeof(bmp), &bmp);
	//InvalidateRect(global_hWnd, NULL, TRUE);
	return;
	if (_tcscmp(move, TEXT("init")) == 0) {
		for (int i = 0; i < gameInfo->numUsers; i++) {
			WaitForSingleObject(hStdoutMutex, INFINITE);
			hBmpBarreira = hBmp = (HBITMAP)LoadImage(NULL, TEXT("barreiratmp.bmp"),
				IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
			xBitMap = gameInfo->nUsers[i].posx;
			yBitMap = gameInfo->nUsers[i].posy;
			hBmp = hBmpBarreira;
			GetObject(hBmp, sizeof(bmp), &bmp);
			InvalidateRect(global_hWnd, NULL, TRUE);
			//invalidaterect
			//gotoxy(gameInfo->nUsers[i].posx, gameInfo->nUsers[i].posy);
			ReleaseMutex(hStdoutMutex);
		}
		return;
	}
	else if (_tcscmp(move, TEXT("no")) == 0)
		return;

	if (connection_mode == 1) {
		WaitForSingleObject(gameReady, INFINITE);//waits for info to come from pipe
	}

	//this function displays users barreiras
	BOOLEAN flag = 0;
	DWORD user_id, j = 0;
	TCHAR usr[TAM];
	TCHAR direction[TAM];
	for (int i = 0; i < TAM; i++) {
		if (move[i] == ':') {
			usr[i] = '\0';
			flag = 1;
			continue;
		}
		else if (move[i] == '\0') {
			direction[j] = '\0';
			break;
		}

		if (!flag)
			usr[i] = move[i];
		else {
			direction[j] = move[i];
			j++;
		}
	}
	user_id = _tstoi(usr);
	if (user_id < 0 || user_id > MAX_USERS) {
		_tprintf(TEXT("Invalido->From (%s) to user[%d]->(%s)\n"), move, user_id, direction);
		Sleep(2000);
		return;
	}

	user userinfo = gameInfo->nUsers[user_id];
	WaitForSingleObject(hStdoutMutex, INFINITE);
	//gotoxy(userinfo.posx + userinfo.size, userinfo.posy);
	xBitMap = userinfo.posx;
	yBitMap = userinfo.posy;
	hBmp = hBmpBarreira;
	GetObject(hBmp, sizeof(bmp), &bmp);
	InvalidateRect(global_hWnd, NULL, TRUE);
	ReleaseMutex(hStdoutMutex);

	return;
	if (_tcscmp(direction, TEXT("left")) == 0) {
		WaitForSingleObject(hStdoutMutex, INFINITE);
		//gotoxy(userinfo.posx + userinfo.size, userinfo.posy);
		xBitMap = userinfo.posx;
		yBitMap = userinfo.posy;
		hBmp = hBmpBarreira;
		GetObject(hBmp, sizeof(bmp), &bmp);
		InvalidateRect(global_hWnd, NULL, TRUE);
		ReleaseMutex(hStdoutMutex);
	}
	else if (_tcscmp(direction, TEXT("right")) == 0) {
		WaitForSingleObject(hStdoutMutex, INFINITE);
		//gotoxy(userinfo.posx - 1, userinfo.posy);
		xBitMap = userinfo.posx;
		yBitMap = userinfo.posy;
		hBmp = hBmpBarreira;
		GetObject(hBmp, sizeof(bmp), &bmp);
		InvalidateRect(global_hWnd, NULL, TRUE);
		ReleaseMutex(hStdoutMutex);
	}

	//WaitForSingleObject(hStdoutMutex, INFINITE);
	//gotoxy(0, 15);
	//_tprintf(TEXT("User[%d]-pos(%d,%d)"), user_id, userinfo.posx, userinfo.posy);
	//ReleaseMutex(hStdoutMutex);

	return;
}

void LoginUser(TCHAR user[MAX_NAME_LENGTH]) {
	msg newMsg;
	newMsg.from = client_id;
	newMsg.to = 254;//login e sempre para o servidor
	newMsg.codigoMsg = 1;//login
	_tcscpy_s(newMsg.messageInfo, MAX_NAME_LENGTH, user);
	sendMessage(newMsg);
	return;
}

void print(msg printMsg) {

	TCHAR tmp[TAM];
	TCHAR tmp2[30];
	if (printMsg.from == 254)
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

}

