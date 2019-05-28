//Base.c
#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include "DLL.h"
#include "resource.h"

LRESULT CALLBACK print(TCHAR frase[TAM]);

LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ResolveMenu(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ResolveConection(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);

void createLocalConnection();
LRESULT CALLBACK createRemoteConnection();

void sendMessage(msg sendMsg);

TCHAR szProgName[] = TEXT("Base");
int connection_mode = -1;//0 = local / 1 = remote
HWND global_hWnd = NULL;


//variaveis
BOOLEAN canSendMsg;

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {

	if (connection_mode == -1) {
		DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_CONNECTION), NULL, ResolveConection);
		if (connection_mode == -1)
			return;
			//DestroyWindow(hWnd);
	}
			

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

	//here
	//wcApp.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcApp.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));

	// "hbrBackground" = handler para "brush" de pintura do fundo da janela. Devolvido por
	// "GetStockObject".Neste caso o fundo será branco
	// ============================================================================
	// 2. Registar a classe "wcApp" no Windows
	// ============================================================================
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
	ShowWindow(hWnd, nCmdShow); // "hWnd"= handler da janela, devolvido por
   // "CreateWindow"; "nCmdShow"= modo de exibição (p.e.
   // normal/modal); é passado como parâmetro de WinMain()
	global_hWnd = hWnd;
	UpdateWindow(hWnd); // Refrescar a janela (Windows envia à janela uma
   // mensagem para pintar, mostrar dados, (refrescar)…
   // ============================================================================
   // 5. Loop de Mensagens
   // ============================================================================
   // O Windows envia mensagens às janelas (programas). Estas mensagens ficam numa fila de
   // espera até que GetMessage(...) possa ler "a mensagem seguinte"
   // Parâmetros de "getMessage":
   // 1)"&lpMsg"=Endereço de uma estrutura do tipo MSG ("MSG lpMsg" ja foi declarada no
   // início de WinMain()):
   // HWND hwnd handler da janela a que se destina a mensagem
   // UINT message Identificador da mensagem
   // WPARAM wParam Parâmetro, p.e. código da tecla premida
   // LPARAM lParam Parâmetro, p.e. se ALT também estava premida
   // DWORD time Hora a que a mensagem foi enviada pelo Windows
   // POINT pt Localização do mouse (x, y)
   // 2)handle da window para a qual se pretendem receber mensagens (=NULL se se pretendem
   // receber as mensagens para todas as janelas pertencentes à thread actual)
   // 3)Código limite inferior das mensagens que se pretendem receber
   // 4)Código limite superior das mensagens que se pretendem receber
   // NOTA: GetMessage() devolve 0 quando for recebida a mensagem de fecho da janela,
   // terminando então o loop de recepção de mensagens, e o programa


	canSendMsg = TRUE;

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
// ============================================================================
// FUNÇÃO DE PROCESSAMENTO DA JANELA
// Esta função pode ter um nome qualquer: Apenas é necesário que na inicialização da
// estrutura "wcApp", feita no início de WinMain(), se identifique essa função. Neste
// caso "wcApp.lpfnWndProc = WndProc"
//
// WndProc recebe as mensagens enviadas pelo Windows (depois de lidas e pré-processadas
// no loop "while" da função WinMain()
//
// Parâmetros:
// hWnd O handler da janela, obtido no CreateWindow()
// messg Ponteiro para a estrutura mensagem (ver estrutura em 5. Loop...
// wParam O parâmetro wParam da estrutura messg (a mensagem)
// lParam O parâmetro lParam desta mesma estrutura
//
// NOTA:Estes parâmetros estão aqui acessíveis o que simplifica o acesso aos seus valores
//
// A função EndProc é sempre do tipo "switch..." com "cases" que descriminam a mensagem
// recebida e a tratar. Estas mensagens são identificadas por constantes (p.e.
// WM_DESTROY, WM_CHAR, WM_KEYDOWN, WM_PAINT...) definidas em windows.h
//============================================================================

//printf variables
int xPos = 0, yPos = 0;


int xPrint = 0, yPrint = 0;
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



//arknoid variarbles
HANDLE  messageEvent, hStdoutMutex;

HANDLE gameInfo;//has game information

HANDLE hTMsgConnection;//has thread where receives messages
//end of variables

LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	HDC hDC;
	RECT rect;
	DWORD res;
	PAINTSTRUCT ps;
	msg testesMsg;
	switch (messg) {
	case WM_LBUTTONDOWN:

		xPos = LOWORD(lParam); // Alternativa!
		yPos = HIWORD(lParam); // Alternativa!

		InvalidateRect(hWnd, NULL, TRUE);

		break;

	case WM_CREATE:
		maxX = GetSystemMetrics(SM_CXSCREEN);
		maxY = GetSystemMetrics(SM_CYSCREEN);

		hDC = GetDC(hWnd);

		memDC = CreateCompatibleDC(hDC);

		hBit = CreateCompatibleBitmap(hDC, maxX, maxY);

		SelectObject(memDC, hBit);

		DeleteObject(hBit);

		hBrush = CreateSolidBrush(RGB(70, 150, 70));

		SelectObject(memDC, hBrush);

		PatBlt(memDC, 0, 0, maxX, maxY, PATCOPY);

		ReleaseDC(hWnd, hDC);

		hBmp = (HBITMAP)LoadImage(NULL, TEXT("../assets/imgs/background.bmp"),
			IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

		GetObject(hBmp, sizeof(bmp), &bmp);
		break;

	case WM_KEYDOWN:
		GetClientRect(hWnd, &rect);
		switch (LOWORD(wParam)){
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
				testesMsg.codigoMsg = -9999;
				testesMsg.connection = 0;
				testesMsg.from = 20;
				testesMsg.to = 254;
				_tcscpy_s(testesMsg.messageInfo,TAM,TEXT("ola"));
				sendMessage(testesMsg);
				break;
			}
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	case WM_PAINT:
		tempDC = CreateCompatibleDC(memDC);

		SelectObject(tempDC, hBmp);

		PatBlt(memDC, 0, 0, maxX, maxY, PATCOPY);

		//BitBlt(memDC, 0, 0, bmp.bmWidth, bmp.bmHeight, tempDC, 0, 0, SRCCOPY);
		BitBlt(memDC, xBitMap, yBitMap, maxX, maxY, tempDC, 0, 0, SRCCOPY);

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

		break;
	case WM_COMMAND:

		switch (LOWORD(wParam)) {
		case ID_PLAY:												//NULL em vez de hWnd para a janela nao ter "prioridade"
			if (!DialogBox(NULL, MAKEINTRESOURCE(IDD_DIALOG_LOGIN), hWnd, ResolveMenu))
				if (_tccmp(login, TEXT("")) != 0)
					MessageBox(hWnd, login, TEXT("Login do User"), NULL);//sucesso
			break;
			case ID_WATCH:
				//MessageBeep(MB_ICONQUESTION);
				break;
			case ID_TOP10:
				//MessageBeep(MB_ICONSTOP);
				break;
			case ID_ABOUT_TYPE:
				if(connection_mode == 0)
					MessageBox(hWnd, TEXT("This is a local connection"), TEXT("About Connection Type"), NULL);
				else if(connection_mode == 1)
					MessageBox(hWnd, TEXT("This is a remote connection"), TEXT("About Connection Type"), NULL);
				else
					MessageBox(hWnd, TEXT("This hacked connection,shouldnt be possible"), TEXT("About Connection Type"), NULL);
				break;
			case ID_ABOUT_VERSION:
				MessageBox(hWnd, TEXT("Version 0.1! - Made by Diogo Marques"), TEXT("About Arknoid"), NULL);
				break;
		}
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

LRESULT CALLBACK ResolveMenu(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {

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

LRESULT CALLBACK ResolveConection(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
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

void createLocalConnection() {
	TCHAR str[TAM];
	TCHAR tmp[TAM];
	_tcscpy_s(str, TAM, LOCAL_CONNECTION_NAME);
	_itot_s(GetCurrentThreadId(), tmp, TAM, 10);
	_tcscat_s(str, TAM, tmp);
	//print
	print(str);

	messageEvent = CreateEvent(NULL, FALSE, FALSE, str);
	updateBalls = CreateEvent(NULL, TRUE, FALSE, BALL_EVENT_NAME);
	updateBonus = CreateEvent(NULL, FALSE, FALSE, BONUS_EVENT_NAME);
	hStdoutMutex = CreateMutex(NULL, FALSE, NULL);
	if (messageEvent == NULL || updateBalls == NULL || updateBonus == NULL || hStdoutMutex == NULL) {
		print(TEXT("Erro ao criar recursos!"));
		return 0;
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

	/*hTMsgConnection = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)localConnection, NULL, 0, NULL);
	if (hTMsgConnection == NULL) {
		print(TEXT("Erro na criãção da thread para local message thread. Erro = %d\n"), GetLastError());
		return 1;
	}*/

	msg tmpMsg;
	tmpMsg.codigoMsg = -9999;//new client
	tmpMsg.from = -1;
	tmpMsg.to = 254;
	_tcscpy_s(tmpMsg.messageInfo, TAM, str);
	sendMessage(tmpMsg); // lets server know of new client
	
	return 0;
}

LRESULT CALLBACK createRemoteConnection() {
	/*HANDLE hPipeTmp;
	BOOL fSuccess = FALSE;
	DWORD dwMode;
	DWORD dwThreadId = 0;
	while (1) {

		print(TEXT("CreateFile...\n"));
		hPipeTmp = CreateFile(
			INIT_PIPE_MSG_NAME,
			GENERIC_READ | GENERIC_WRITE,
			0 | FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			0 | FILE_FLAG_OVERLAPPED,
			NULL
		);


		if (hPipeTmp != INVALID_HANDLE_VALUE) {
			print(TEXT("I got a valid hPipe\n"));
			break;
		}


		if (GetLastError() != ERROR_PIPE_BUSY) {
			print(TEXT("Deu erro e nao foi de busy. Erro = %d\n"), GetLastError());
			return 1;
		}


		if (!WaitNamedPipe(INIT_PIPE_MSG_NAME, 10000)) {
			print(TEXT("Waited 10 seconds and cant find a pipe, I give up...\n"));
			return FALSE;
		}

	}

	dwMode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		hPipeTmp,
		&dwMode,
		NULL,
		NULL
	);

	if (!fSuccess) {
		print(TEXT("SetNamedPipeHandleState falhou. Erro = %d\n"), GetLastError());
		return -1;
	}

	print(TEXT("initianting pipeConnection thread\n"));
	hTMsgConnection = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)pipeConnection, (LPVOID)hPipeTmp, 0, NULL);
	if (hTMsgConnection == NULL) {
		print(TEXT("Erro na criãção da thread para remotePipe. Erro = %d\n"), GetLastError());
		return 1;
	}

	Sleep(250);
	print(TEXT("sending message ...\n"));
	msg tmpMsg;
	tmpMsg.codigoMsg = -9999;
	tmpMsg.from = user_id;
	tmpMsg.to = 254;
	_tcscpy_s(tmpMsg.messageInfo, TAM, TEXT("remoteClient"));
	sendMessage(tmpMsg); // lets server know of new client
	print(TEXT("message sent sending message ...\n"));
	return 0;*/
}

void sendMessage(msg sendMsg) {
	HANDLE WriteReady;
	OVERLAPPED OverlWr = { 0 };
	DWORD bytesWritten;
	BOOLEAN fSuccess;

	//if (canSendMsg)
		//canSendMsg = FALSE;
	//else
		//return;

	if (connection_mode == 0) {
		sendMsg.connection = 0;
		sendMessageDLL(sendMsg);
	}
	else if (connection_mode == 1) {
		/*sendMsg.connection = 1;
		WriteReady = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (WriteReady == NULL) {
			_tprintf(TEXT("Erro ao criar writeRead Event. Erro = %d\n"), GetLastError());
			return 0;
		}

		//_tprintf(TEXT("Ligaçao establecida...\n"));

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

		GetOverlappedResult(hPipe, &OverlWr, &bytesWritten, FALSE);
		if (bytesWritten < sizeof(msg)) {
			_tprintf(TEXT("Write File failed... | Erro = %d\n"), GetLastError());
		}*/

		//_tprintf(TEXT("[ESCRITOR] Enviei %d bytes ao leitor...(WriteFile)\n"), bytesWritten);

	}
}

LRESULT CALLBACK print(TCHAR line[TAM]) {

	_tcscpy_s(frase, TAM, line);
	InvalidateRect(global_hWnd, NULL, TRUE);

}

