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
LRESULT CALLBACK resolveInput(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK resolveMenu(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK resolveConection(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
void resolveKey(WPARAM wParam);

int createLocalConnection();
int createRemoteConnection();

void LoginUser(TCHAR user[MAX_NAME_LENGTH]);
void logoutUser();

DWORD resolveMessage(msg inMsg);
void sendMessage(msg sendMsg);

void securityPipes(SECURITY_ATTRIBUTES * sa);
void Cleanup(PSID pEveryoneSID, PSID pAdminSID, PACL pACL, PSECURITY_DESCRIPTOR pSD);

TCHAR szProgName[] = TEXT("Client");
int connection_mode = -1;//0 = local / 1 = remote
HWND global_hWnd = NULL;
TCHAR top10[TAM];

//Variaveis
	//print

TCHAR frase[TAM];
TCHAR inTxt[TAM];
TCHAR outTxt[TAM];
int mouseX = 100, mouseY = 100;

	//game
pgame gameInfo;
TCHAR login[MAX_NAME_LENGTH];
DWORD client_id = -1;//identification of program so the server knows where to send info
DWORD userState = -1;//-1 lobby | 0 = playing | 1 = watching

	//handles
HANDLE gameReady;
HANDLE  messageEvent, hStdoutMutex;
HANDLE hTMsgConnection;//has thread where receives messages
HANDLE hTBrick;
HANDLE hPipeMsg;
HANDLE hPipeGame;
//end of variables

//HANDLES
HANDLE hTUserInput;

TCHAR userLogged[MAX_NAME_LENGTH];

//double buffer
HDC memDC = NULL;
int maxX = 0, maxY = 0;

//lobby background
HBITMAP hBackground = NULL;
BITMAP bmBackground;
HDC hdcBackground;

//level background
HBITMAP hLevelBackground = NULL;
BITMAP bmLevelBackground;
HDC hdcLevelBackground;

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

//Bonus
HBITMAP hBonus = NULL;
BITMAP bmBonus;
HDC hdcBonus;

//other
HBITMAP hBit = NULL;
HBRUSH hBrush = NULL;
HBITMAP hBmp = NULL;
BITMAP bmp;


int counter = 0;

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {

	HWND hWnd;
	MSG lpMsg;
	WNDCLASSEX wcApp; 

	wcApp.cbSize = sizeof(WNDCLASSEX); 
	wcApp.hInstance = hInst; 
	wcApp.lpszClassName = szProgName; 
	wcApp.lpfnWndProc = resolveInput;

	wcApp.style = CS_HREDRAW | CS_VREDRAW;
	wcApp.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LOGGO));

	wcApp.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LOGGO));

	wcApp.hCursor = LoadCursor(hInst, MAKEINTRESOURCE(IDC_INIT));

	wcApp.lpszMenuName = MAKEINTRESOURCE(IDM_MENU);
	wcApp.cbClsExtra = 0; 
	wcApp.cbWndExtra = 0;

	wcApp.hbrBackground = CreateSolidBrush(RGB(244, 206, 66));

	if (!RegisterClassEx(&wcApp))
		return(0);

	hWnd = CreateWindow(
		szProgName, 
		TEXT("SO2 - Ficha 6 - Ex1_V2"),
		WS_OVERLAPPED, 
		5,
		5, 
		GAME_SIZE_X, 
		GAME_SIZE_Y, 
		(HWND)HWND_DESKTOP, 
		(HMENU)NULL, 
		(HINSTANCE)hInst,
		0); 



	if (connection_mode == -1) {
		DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_CONNECTION), NULL, resolveConection);
		if (connection_mode == -1)
			return 0;
	}
	int res;
	if (!connection_mode)
		res = createLocalConnection();
	else
		res = createRemoteConnection();

	if (res == -1)
		return -1;

	_tcscpy_s(top10, TAM, TEXT("loading"));

	HANDLE hGameReady = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)waitGameReady, NULL, 0, NULL);
	if (hGameReady == NULL) {
		MessageBox(hWnd, TEXT("could not create waitGameReadThread"), TEXT("WARNING"), MB_OK);
		return 1;
	}

	ShowWindow(hWnd, nCmdShow);

	global_hWnd = hWnd;
	UpdateWindow(hWnd);

	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg);
		DispatchMessage(&lpMsg); 

	}

	return((int)lpMsg.wParam);
}

LRESULT CALLBACK resolveInput(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	HDC hDC;
	PAINTSTRUCT ps;
	TCHAR str[TAM];
	TCHAR tmp[TAM];
	TCHAR tmp2[TAM];
	TCHAR info[TAM];
	int res;
	msg gameMsg;
	BOOLEAN checkActiveObjects = FALSE;
	switch (messg) {
	case WM_LBUTTONDOWN:
		if (userState == 0) {
			mouseX = LOWORD(lParam);
			mouseY = HIWORD(lParam);
			mouseX -= (gameInfo->nUsers[client_id].size.sizex / 2);
			gameMsg.codigoMsg = 200;
			gameMsg.from = client_id;
			gameMsg.to = 254;
			_itot_s(mouseX, gameMsg.messageInfo, TAM, 10);
			sendMessage(gameMsg);
		}
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

		//lobby background
		hBackground = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/lobbyBackground.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		GetObject(hBackground, sizeof(bmBackground), &bmBackground);
		hdcBackground = CreateCompatibleDC(hDC);
		SelectObject(hdcBackground, hBackground);
		
		//level background
		hLevelBackground = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/level.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		GetObject(hLevelBackground, sizeof(bmLevelBackground), &bmLevelBackground);
		hdcLevelBackground = CreateCompatibleDC(hDC);
		SelectObject(hdcLevelBackground, hLevelBackground);

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
		
		//bonus
		hBonus = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/bonus.bmp"), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		GetObject(hBonus, sizeof(bmBonus), &bmBonus);
		hdcBonus = CreateCompatibleDC(hDC);
		SelectObject(hdcBonus, hBonus);

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
		//StretchBlt(memDC, 0, 0, GAME_SIZE_X, GAME_SIZE_Y, hdcBackground, 0, 0, bmBackground.bmWidth, bmBackground.bmHeight, SRCCOPY);
	
		if((userState == 0 || userState == 1) && gameInfo->gameStatus > 0){
			StretchBlt(memDC, 0, 0, GAME_SIZE_X, GAME_SIZE_Y, hdcLevelBackground, 0, 0, bmLevelBackground.bmWidth, bmLevelBackground.bmHeight, SRCCOPY);
			//game
			//users
			for (int i = 0; i < gameInfo->numUsers; i++) {
				StretchBlt(memDC, gameInfo->nUsers[i].posx, gameInfo->nUsers[i].posy, gameInfo->nUsers[i].size.sizex, gameInfo->nUsers[i].size.sizey, hdcPlayerBarreira, 0, 0, bmPlayerBarreira.bmWidth, bmPlayerBarreira.bmHeight, SRCPAINT);
			}

			//bricks
			for (int i = 0; i < gameInfo->numBricks; i++) {
				if (gameInfo->nBricks[i].status > 0) {//active
					if (gameInfo->nBricks[i].type == 2)//hard brick
						StretchBlt(memDC, gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy, gameInfo->nBricks[i].size.sizex, gameInfo->nBricks[i].size.sizey, hdcHardBrick, 0, 0, bmHardBrick.bmWidth, bmHardBrick.bmHeight, SRCPAINT);
					else
						StretchBlt(memDC, gameInfo->nBricks[i].posx, gameInfo->nBricks[i].posy, gameInfo->nBricks[i].size.sizex, gameInfo->nBricks[i].size.sizey, hdcBrick, 0, 0, bmBrick.bmWidth, bmBrick.bmHeight, SRCPAINT);
				}
				else if (gameInfo->nBricks[i].brinde.status > 0) {
					StretchBlt(memDC, gameInfo->nBricks[i].brinde.posx, gameInfo->nBricks[i].brinde.posy, gameInfo->nBricks[i].brinde.size.sizex, gameInfo->nBricks[i].brinde.size.sizey, hdcBonus, 0, 0, bmBonus.bmWidth, bmBonus.bmHeight, SRCPAINT);
					checkActiveObjects = TRUE;
				}
			}

			//balls
			if (gameInfo->numBalls == 0 && gameInfo->nUsers[0].lifes > 0 && !checkActiveObjects)
				StretchBlt(memDC, gameInfo->nUsers[0].posx + gameInfo->nUsers[0].size.sizex / 2, gameInfo->nUsers[0].posy - gameInfo->myconfig.ballSize.sizey, gameInfo->myconfig.ballSize.sizex, gameInfo->myconfig.ballSize.sizey, hdcBall, 0, 0, bmBall.bmWidth, bmBall.bmHeight, SRCPAINT);
			else
				for (int i = 0; i < gameInfo->numBalls; i++) {
					StretchBlt(memDC, gameInfo->nBalls[i].posx, gameInfo->nBalls[i].posy, gameInfo->nBalls[i].size.sizex, gameInfo->nBalls[i].size.sizey, hdcBall, 0, 0, bmBall.bmWidth, bmBall.bmHeight, SRCPAINT);
				}

			if (userState == 0) {
				//lifes
				_tcscpy_s(info, TAM, TEXT("Lifes:"));
				_itot_s(gameInfo->nUsers[client_id].lifes, tmp, TAM, 10);//translates num to str
				_tcscat_s(info, TAM, tmp);
				TextOut(memDC, 0, 400, info, _tcslen(info));

				//speed
				_tcscpy_s(info, TAM, TEXT("Speed:"));
				_itot_s(gameInfo->myconfig.ballInitialSpeed, tmp, TAM, 10);//translates num to str
				_tcscat_s(info, TAM, tmp);
				TextOut(memDC, 0, 420, info, _tcslen(info));
			}
			else if(userState == 1)
				TextOut(memDC, gameInfo->myconfig.gameSize.sizex - 90, gameInfo->myconfig.gameSize.sizey - 15, TEXT("SPECTATING"), _tcslen(TEXT("SPECTATING")));

		}
		else {
			StretchBlt(memDC, 0, 0, GAME_SIZE_X, GAME_SIZE_Y, hdcBackground, 0, 0, bmBackground.bmWidth, bmBackground.bmHeight, SRCCOPY);
			//top 10
			if (_tcscmp(gameInfo->top, TEXT("loading")) != 0 && _tcscmp(gameInfo->top, TEXT("")) != 0) {//no top 10 yet
			_tcscpy_s(info, TAM, TEXT(""));
				res = 0;
				_tcscpy_s(str, TAM, gameInfo->top);
				for (int i = 0; i < 10; i++) {
					for (int j = 0; j < TAM; j++) {
						if (str[res] == '|') {
							tmp[j] = '\0';
							res++;
							break;
						}
						tmp[j] = str[res++];
					}
					TextOut(memDC, 10, 215 + i * 20, tmp, _tcslen(tmp));
				}
			}
		}
			
				
		TextOut(memDC, 0, 20, userLogged, _tcslen(userLogged));

		_tcscpy_s(str, TAM, TEXT("Info["));
		_itot_s(counter++, tmp, TAM, 10);//translates num to str
		_tcscat_s(str, TAM, tmp);//ads
		_tcscat_s(str, TAM, TEXT("]->localGameStatus = "));//ads
		_itot_s(userState, tmp, TAM, 10);//translates num to str
		_tcscat_s(str, TAM, tmp);//ads
		_tcscat_s(str, TAM, TEXT("| GameStatus = "));
		_itot_s(gameInfo->gameStatus, tmp, TAM, 10);//translates num to str
		_tcscat_s(str, TAM, tmp);//ads
		_tcscat_s(str, TAM, TEXT("| numUsers = "));
		_itot_s(gameInfo->numUsers, tmp, TAM, 10);//translates num to str
		_tcscat_s(str, TAM, tmp);//ads
		_tcscat_s(str, TAM, TEXT("| numBricks = "));
		_itot_s(gameInfo->numBricks, tmp, TAM, 10);//translates num to str
		_tcscat_s(str, TAM, tmp);//ads
		TextOut(memDC, 0, 100, str, _tcslen(str));


		TextOut(memDC, 0, 130, inTxt, _tcslen(inTxt));
		TextOut(memDC, 0, 145, outTxt, _tcslen(outTxt));
	
		hDC = BeginPaint(hWnd, &ps);
		BitBlt(hDC, 0, 0, maxX, maxY, memDC, 0, 0, SRCCOPY);
		EndPaint(hWnd, &ps);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_HOME:
			if (userState != -1){
				gameMsg.codigoMsg = 2;
				gameMsg.connection = connection_mode;
				gameMsg.from = client_id;
				gameMsg.to = 254;
				_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("endUser"));
				sendMessage(gameMsg);
			}
			
			break;
		case ID_EXIT:
			DestroyWindow(hWnd);
			break;
		case ID_PLAY:	
			if (userState != -1)
				return;
			res = DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG_LOGIN), hWnd, resolveMenu);
			if (res == IDCANCEL || res == IDABORT)
				break;
			if (_tcscmp(login, TEXT("")) != 0) {
				for (int i = 0; i < MAX_NAME_LENGTH; i++)//preventes users from using ':' (messes up registry)
					if (login[i] == ':' || login[i] == '|')
						login[i] = '\0';
			}
			LoginUser(login);
				
			break;
			case ID_WATCH:
				if (userState != -1)
					return;
				userState = 1;
				MessageBeep(MB_ICONQUESTION);
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
				_itot_s(gameInfo->gameStatus, tmp2, TAM, 10);//translates num to str
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
		if (res == IDYES)*/{


		DestroyWindow(hWnd);

	}
			
		break;
	case WM_DESTROY: // Destruir a janela e terminar o programa
		if (connection_mode) {
			free(gameInfo);
			//ends  msg pipe thread
			TerminateThread(hTMsgConnection, 1);
			CloseHandle(hTMsgConnection);
			//ends game pipe thread
			TerminateThread(hPipeGame, 1);
			CloseHandle(hPipeGame);
		}
		else {
			closeSharedMemoryMsg();
			closeSharedMemoryGame();
		}
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
	if (userState != 0)
		return;

	msg gameMsg;
	gameMsg.codigoMsg = 200;
	gameMsg.from = client_id;
	gameMsg.to = 254;
	_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("not"));
	switch (LOWORD(wParam)) {
	case VK_LEFT:
	case 0x41://a key
		_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("left"));
		break;
	case VK_RIGHT:
	case 0x44://d key
		_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("right"));
		break;
	case VK_SPACE:
		gameMsg.codigoMsg = 101;
		_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("ball"));
		break;

	case VK_ESCAPE:
		_tcscpy_s(gameMsg.messageInfo, TAM, TEXT("exit"));
		gameMsg.codigoMsg = 2;
		break;
	}

	if (_tcscmp(gameMsg.messageInfo, TEXT("not")) != 0) {
		sendMessage(gameMsg);
	}
}

int createLocalConnection() {
	HANDLE checkExistingServer;
	//checkExistingServer = CreateEvent(NULL, FALSE, NULL, CHECK_SERVER_EVENT);
	checkExistingServer = OpenEvent(EVENT_ALL_ACCESS, TRUE, CHECK_SERVER_EVENT);
	if (checkExistingServer == NULL) {//there is no server created
		MessageBox(global_hWnd, TEXT("There is not a server running at this moment."), TEXT("WARNING"), MB_ICONWARNING | MB_OK);
		return -1;
	}

	TCHAR str[TAM];
	TCHAR tmp[TAM];
	_tcscpy_s(str, TAM, LOCAL_CONNECTION_NAME);
	_itot_s(GetCurrentThreadId(), tmp, TAM, 10);
	_tcscat_s(str, TAM, tmp);

	messageEvent = CreateEvent(NULL, FALSE, FALSE, str);
	hStdoutMutex = CreateMutex(NULL, FALSE, NULL);
	gameReady = CreateEvent(NULL, TRUE, FALSE, LOCAL_UPDATE_GAME);
	if (messageEvent == NULL || hStdoutMutex == NULL || gameReady == NULL) {
		MessageBox(global_hWnd, TEXT("Error creating resources..."), TEXT("Resources"), MB_OK);
		return -1;
	}

	//Message
	createSharedMemoryMsg();
	//Game
	gameInfo = createSharedMemoryGame();

	hTMsgConnection = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)localConnection, NULL, 0, NULL);
	if (hTMsgConnection == NULL) {
		MessageBox(global_hWnd, TEXT("Error creating local connction thread..."), TEXT("Resources"), MB_OK);
		return -1;

	}

	msg tmpMsg;
	tmpMsg.codigoMsg = -9999;//new client
	tmpMsg.from = -1;
	tmpMsg.to = 254;
	_tcscpy_s(tmpMsg.messageInfo, TAM, str);
	sendMessage(tmpMsg); // lets server know of new client

	return 0;
}

DWORD WINAPI localConnection(LPVOID param) {
	msg newMsg;
	int quant = 0;
	DWORD resp;

	do {
		WaitForSingleObject(messageEvent, INFINITE);
		newMsg = receiveMessageDLL();
		resp = resolveMessage(newMsg);
		//InvalidateRect(global_hWnd, NULL, FALSE);	
	} while (1);

	return 0;
}

int createRemoteConnection() {
	BOOL fSuccess = FALSE;
	DWORD dwMode;

	SECURITY_ATTRIBUTES sa;
	securityPipes(&sa);

	gameInfo = (pgame)malloc(sizeof(game));
	if (gameInfo == NULL) {
		return -1;
	}

	_tcscpy_s(gameInfo->top, TAM, TEXT("loading"));
	gameInfo->gameStatus = -1;
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
			return -1;
		}


		if (!WaitNamedPipe(INIT_PIPE_MSG_NAME, 10000)) {
			//print(TEXT("Waited 10 seconds and cant find a pipe, I give up...\n"));
			return -1;
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
		return -1;
	}

	//print(TEXT("initianting pipeConnection thread\n"));
	hTMsgConnection = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)msgPipe, NULL, 0, NULL);
	if (hTMsgConnection == NULL) {
		//print(TEXT("Erro na criãção da thread para remotePipe. Erro = %d\n"), GetLastError());
		return -1;
	}

	gameReady = CreateEvent(NULL, TRUE, FALSE, LOCAL_UPDATE_GAME);
	if (gameReady == NULL) {
		//print(TEXT("Erro na criãção da thread para remotePipe. Erro = %d\n"), GetLastError());
		return -1;
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
	return 0;
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
	print(inMsg);

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

	if (inMsg.codigoMsg == 1) {//successful login
		logged = 1;
		MessageBox(global_hWnd, TEXT("SUCESSO NO LOGIN"), TEXT("info"), MB_OK);
		userState = 0;
		_tcscpy_s(frase, TAM, TEXT("You are now in the game, waiting for server to start..."));
		//InvalidateRect(global_hWnd, NULL, TRUE);
	}
	else if (inMsg.codigoMsg == -1) {
		_tcscpy_s(frase, TAM, TEXT("Server denied login"));
		_tcscpy_s(userLogged, TAM, TEXT(""));
		//endUser();
		return -1;
	}
	else if (inMsg.codigoMsg == 2) {
		logoutUser();
	}
	else if (inMsg.codigoMsg == -100) {
		_tcscpy_s(userLogged, TAM, TEXT(""));
		_tcscpy_s(frase, TAM, TEXT("Game hasnt been created by the server yet"));
		//endUser();
		return -1;
	}
	else if (inMsg.codigoMsg == 100) {//start game
		_tcscpy_s(frase, TAM, TEXT("game started by the server"));
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
	else if (inMsg.codigoMsg == -110) {
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
		//reset local game
	}
	return -1;
}

void LoginUser(TCHAR user[MAX_NAME_LENGTH]) {
	//MessageBox(hWnd, user, TEXT("Trying to login with:"), MB_OK);//sucesso
	msg newMsg;
	newMsg.from = client_id;
	newMsg.to = 254;//login e sempre para o servidor
	newMsg.codigoMsg = 1;//login
	_tcscpy_s(newMsg.messageInfo, MAX_NAME_LENGTH, user);
	_tcscpy_s(userLogged, MAX_NAME_LENGTH, user);
	sendMessage(newMsg);
	return;
}

void logoutUser() {
	MessageBox(global_hWnd, TEXT("reseting user now"), TEXT("info"), MB_OK);
	userState = -1;
	_tcscpy_s(userLogged, TAM, TEXT(""));
	InvalidateRect(global_hWnd, NULL, FALSE);
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

	if (printMsg.from == 254)
		_tcscpy_s(outTxt, TAM, tmp);
	else
		_tcscpy_s(inTxt, TAM, tmp);
	//MessageBox(global_hWnd, tmp, TEXT("Server - sending message"), MB_OK);
	InvalidateRect(global_hWnd, NULL, FALSE);
	return;
}

void WINAPI waitGameReady(LPVOID param) {
	while (1) {
		WaitForSingleObject(gameReady, INFINITE);//waits for game to be ready to use
		if (userState == -1)
			Sleep(1500);
		else {
			_tcscpy_s(top10, TAM, gameInfo->top);
			InvalidateRect(global_hWnd, NULL, FALSE);
		}
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
