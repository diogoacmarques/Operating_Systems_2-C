#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include <strsafe.h>
#include <aclapi.h>
#include "DLL.h"
#include "resource.h"


//threads
DWORD WINAPI localConnection(LPVOID param);
DWORD WINAPI msgPipe(LPVOID param);
DWORD WINAPI gamePipe(LPVOID param);

void WINAPI waitGameReady(LPVOID param);

//functions
void print(msg printMsg);

LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK resolveMenu(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK resolveConection(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
void resolveKey(WPARAM wParam);

void createLocalConnection();
void createRemoteConnection();

void LoginUser(TCHAR user[MAX_NAME_LENGTH]);

DWORD resolveMessage(msg inMsg);
void sendMessage(msg sendMsg);

void securityPipes(SECURITY_ATTRIBUTES * sa);
void Cleanup(PSID pEveryoneSID, PSID pAdminSID, PACL pACL, PSECURITY_DESCRIPTOR pSD);

TCHAR szProgName[] = TEXT("Client");
int connection_mode = -1;//0 = local / 1 = remote
HWND global_hWnd = NULL;

//Variaveis
	//print
int xPrint = 0, yPrint = 0;
TCHAR frase[TAM];
int numBalls = 0;

	//game
pgame gameInfo,gameUpdate;
TCHAR login[MAX_NAME_LENGTH];
DWORD client_id = -1;//identification of program so the server knows where to send info
DWORD localGameStatus = 0;

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

TCHAR userLogged[MAX_NAME_LENGTH];


//double buffer
HDC memDC = NULL;
int maxX = 0, maxY = 0;

//background
HBITMAP hBackground = NULL;
BITMAP bmBackground;
HDC hdcBackground;

//Barreira
HBITMAP hPlayerBarreira = NULL;
BITMAP bmPlayerBarreira;
HDC hdcPlayerBarreira;

//Ball
HBITMAP hBall = NULL;
BITMAP bmBall;
HDC hdcBall;

//Brick
HBITMAP hBrick = NULL;
BITMAP bmBrick;
HDC hdcBrick;

//Hard Brick
HBITMAP hHardBrick = NULL;
BITMAP bmHardBrick;
HDC hdcHardBrick;

//other
HBITMAP hBit = NULL;
HBRUSH hBrush = NULL;
HBITMAP hBmp = NULL;
BITMAP bmp;


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
		//WS_OVERLAPPEDWINDOW, // Estilo da janela (WS_OVERLAPPED= normal)
		WS_OVERLAPPED, // Estilo da janela (WS_OVERLAPPED= normal)
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
	if (!connection_mode)
		createLocalConnection();
	else
		createRemoteConnection();

	HANDLE hGameReady = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)waitGameReady, NULL, 0, NULL);
	if (hGameReady == NULL) {
		MessageBox(hWnd, TEXT("could not create waitGameReadThread"), TEXT("WARNING"), MB_OK);
		return 1;
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
	PAINTSTRUCT ps;
	TCHAR str[TAM];
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
		ReleaseDC(hWnd, hDC);

		hDC = GetDC(hWnd);

		//background
		hBackground = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/lobbyBackground.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		GetObject(hBackground, sizeof(bmBackground), &bmBackground);
		hdcBackground = CreateCompatibleDC(hDC);
		SelectObject(hdcBackground, hBackground);

		//player
		hPlayerBarreira = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/barreiratmp.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		GetObject(hPlayerBarreira, sizeof(bmPlayerBarreira), &bmPlayerBarreira);
		hdcPlayerBarreira = CreateCompatibleDC(hDC);
		SelectObject(hdcPlayerBarreira, hPlayerBarreira);

		//ball
		hBall = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/ball.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		GetObject(hBall, sizeof(bmBall), &bmBall);
		hdcBall = CreateCompatibleDC(hDC);
		SelectObject(hdcBall, hBall);
		
		//brick
		hBrick = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/brick.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		GetObject(hBrick, sizeof(bmBrick), &bmBrick);
		hdcBrick = CreateCompatibleDC(hDC);
		SelectObject(hdcBrick, hBrick);
		
		//hard brick
		hHardBrick = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/hard_brick.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		GetObject(hHardBrick, sizeof(bmHardBrick), &bmHardBrick);
		hdcHardBrick = CreateCompatibleDC(hDC);
		SelectObject(hdcHardBrick, hHardBrick);

		ReleaseDC(hWnd, hDC);
		break;
		//hBmp = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/background.bmp"),
			//IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

		//GetObject(hBmp, sizeof(bmp), &bmp);
		break;

	case WM_KEYDOWN:
		resolveKey(wParam);
		break;
	case WM_PAINT:
		SetBkMode(memDC, TRANSPARENT);
		SetTextColor((HDC)memDC, RGB(255, 255, 255));
		SetStretchBltMode(memDC, COLORONCOLOR);
		PatBlt(memDC, 0, 0, maxX, maxY, PATCOPY); // fill background
		StretchBlt(memDC, 0, 0, 800, 600, hdcBackground, 0, 0, bmBackground.bmWidth, bmBackground.bmHeight, SRCCOPY);
	
		TextOut(memDC, 0, 0, userLogged, _tcslen(userLogged));
		TextOut(memDC, 0, 15, frase, _tcslen(frase));

		if (localGameStatus == 1) {
			//users
			for (int i = 0; i < gameInfo->numUsers; i++) {
				StretchBlt(memDC, gameInfo->nUsers[i].posx, gameInfo->nUsers[i].posy, gameInfo->nUsers[i].size.sizex, gameInfo->nUsers[i].size.sizey, hdcPlayerBarreira, 0, 0, bmPlayerBarreira.bmWidth, bmPlayerBarreira.bmHeight, SRCPAINT);
			}

			//balls
			for (int i = 0; i < gameInfo->numBalls; i++) {
				StretchBlt(memDC, gameInfo->nBalls[i].posx, gameInfo->nBalls[i].posy, gameInfo->nBalls[i].size.sizex, gameInfo->nBalls[i].size.sizey, hdcBall, 0, 0, bmBall.bmWidth, bmBall.bmHeight, SRCPAINT);
			}
			
			//bricks
			for (int i = 0; i < gameInfo->numBricks; i++) {
				if(gameInfo->nBricks[i].type == 2)//hard brick
					StretchBlt(memDC, gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy, gameInfo->nBricks[i].size.sizex, gameInfo->nBricks[i].size.sizey, hdcHardBrick, 0, 0, bmHardBrick.bmWidth, bmHardBrick.bmHeight, SRCPAINT);
				else	
					StretchBlt(memDC, gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy, gameInfo->nBricks[i].size.sizex, gameInfo->nBricks[i].size.sizey, hdcBrick, 0, 0, bmBrick.bmWidth, bmBrick.bmHeight, SRCPAINT);
			}
		}
	
		hDC = BeginPaint(hWnd, &ps);
		BitBlt(hDC, 0, 0, maxX, maxY, memDC, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		break;
	case WM_COMMAND:

		switch (LOWORD(wParam)) {
		case ID_PLAY:					
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
				
				/*_tcscpy_s(tmp, TAM, TEXT("T"));
				InvalidateRect(hWnd, NULL, FALSE);	
				break;

				//barreira
				localGameStatus = 1;
				gameInfo->numUsers = 1;
				gameInfo->nUsers[0].posx = 100;
				gameInfo->nUsers[0].posy = 100;
				gameInfo->nUsers[0].size = 100;
				gameInfo->numBalls++;
				gameInfo->nBalls[0].posx = 300;
				gameInfo->nBalls[0].posy = 300;
				*/
				//MessageBeep(MB_ICONSTOP);
				_itot_s(gameInfo->numBricks, tmp, TAM, 10);//translates num to str
				MessageBox(global_hWnd, tmp, TEXT("NumBricks = "), MB_OK);
				localGameStatus = 1;
				InvalidateRect(hWnd, NULL, FALSE);	
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
			EndDialog(hWnd, 0);
			return TRUE;
		}
		else if (LOWORD(wParam) == IDC_BUTTON_REMOTE) {
			connection_mode = 1;
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
	TCHAR str[TAM];
	TCHAR tmp[TAM];
	TCHAR tmp2[TAM];
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
		if(gameInfo->nUsers[client_id].posx > 0)
			_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("left"));
		break;
	case VK_RIGHT:
	case 0x44://d key
		if (localGameStatus == 2)//watching
			return;

		//_itot_s(gameInfo->nUsers[0].posx + gameInfo->nUsers[0].size, tmp, TAM, 10);//translates num to str
		//_itot_s(gameInfo->nUsers[0].posy, tmp2, TAM, 10);//translates num to str
		//_tcscpy_s(str, TAM, TEXT("pos Before:"));
		//_tcscat_s(str, TAM, tmp);//adds
		//_tcscat_s(str, TAM, TEXT(","));//adds
		//_tcscat_s(str, TAM, tmp2);//adds
		//if(gameInfo->nUsers[0].posx > 600)
		//	MessageBox(global_hWnd, str, TEXT("pos"), MB_OK);

		if (gameInfo->nUsers[client_id].posx + gameInfo->nUsers[client_id].size.sizex < gameInfo->myconfig.gameSize.sizex)
			_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("right"));
		break;
	case VK_SPACE:
		if (localGameStatus == 2)//watching
			return;
		gameMsg.codigoMsg = 101;
		//if (gameInfo->numBalls == 0)
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
		//_tcscpy_s(frase, TAM, gameMsg.messageInfo);
		//InvalidateRect(global_hWnd, NULL, TRUE);
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
	gameReady = CreateEvent(NULL, FALSE, FALSE, LOCAL_UPDATE_GAME);
	if (messageEvent == NULL || updateBalls == NULL || updateBonus == NULL || hStdoutMutex == NULL || gameReady == NULL) {
		MessageBox(global_hWnd, TEXT("Error creating resources..."), TEXT("Resources"), MB_OK);
		PostQuitMessage(1);
	}

	//BOOLEAN res = initializeHandles();
	//if (res) {
		//print(TEXT("Erro ao criar Handles!"));
		//return 1;
	//}
	
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
		InvalidateRect(global_hWnd, NULL, FALSE);	
	} while (1);

	return 0;
}

void createRemoteConnection() {
	BOOL fSuccess = FALSE;
	DWORD dwMode;

	SECURITY_ATTRIBUTES sa;
	securityPipes(&sa);

	while (1) {

		hPipeMsg = CreateFile(
			INIT_PIPE_MSG_NAME,
			GENERIC_READ | GENERIC_WRITE,
			0 | FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0 | FILE_FLAG_OVERLAPPED,
			&sa
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
	//InvalidateRect(global_hWnd, NULL, TRUE);

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
	SECURITY_ATTRIBUTES sa;
	securityPipes(&sa);

	while (1) {
		_tprintf(TEXT("Create file for game\n"));
		gamePipe = CreateFile(
			INIT_PIPE_GAME_NAME,
			GENERIC_READ,
			0,
			NULL,
			OPEN_EXISTING,
			0 | FILE_FLAG_OVERLAPPED,
			&sa
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
	//_itot_s(inMsg.codigoMsg, tmp_info, TAM, 10);
	//MessageBox(global_hWnd,inMsg.messageInfo,tmp_info, MB_OK);//sucesso
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
		MessageBox(global_hWnd, TEXT("SUCESSO NO LOGIN"), TEXT("info"), MB_OK);
		_tcscpy_s(frase, TAM, TEXT("You are now in the game, waiting for server to start..."));
		//InvalidateRect(global_hWnd, NULL, TRUE);
	}
	else if (inMsg.codigoMsg == -1 && !logged) {
		_tprintf(TEXT("Server refused login with %s\n"), inMsg.messageInfo);
		_tcscpy_s(userLogged, TAM, TEXT(""));
		//endUser();
		return -1;
	}
	else if (inMsg.codigoMsg == -100) {
		_tcscpy_s(userLogged, TAM, TEXT("Game hasnt been created by the server yet"));
		//endUser();
		return -1;
	}
	else if (inMsg.codigoMsg == 100) {//start game
		_tcscpy_s(frase, TAM, TEXT("game started by the server"));
		localGameStatus = 1;
		//usersMove(inMsg.messageInfo);
	}
	else if (inMsg.codigoMsg == 101) {//new ball
		//DWORD tmp = _tstoi(inMsg.messageInfo);
		//_tprintf(TEXT("creating %d balls thread para o utilizador!\n"),tmp);
		//createBalls(tmp);
	}
	else if (inMsg.codigoMsg == 102) {//create bricks

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
		//usersMove(inMsg.messageInfo);
	}
	else if (inMsg.codigoMsg == -999) {
		//endUser();
	}
	return -1;
}

void LoginUser(TCHAR user[MAX_NAME_LENGTH]) {
	msg newMsg;
	newMsg.from = client_id;
	newMsg.to = 254;//login e sempre para o servidor
	newMsg.codigoMsg = 1;//login
	_tcscpy_s(newMsg.messageInfo, MAX_NAME_LENGTH, user);
	_tcscpy_s(userLogged, MAX_NAME_LENGTH, user);
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
	//InvalidateRect(global_hWnd, NULL, TRUE);
	return;
}

void WINAPI waitGameReady(LPVOID param) {
	while (1) {
		WaitForSingleObject(gameReady, INFINITE);//waits for game to be ready to use
		//gameInfo = gameUpdate;
		InvalidateRect(global_hWnd, NULL, FALSE);	
	}
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
