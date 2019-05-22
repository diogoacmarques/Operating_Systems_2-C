//Base.c
#include <windows.h>
#include <tchar.h>
#include <windowsx.h>
#include "resource.h"


LRESULT CALLBACK TrataEventos(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ResolveMenu(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ResolveConection(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
TCHAR szProgName[] = TEXT("Base");
int connection_mode = -1;//0 = local / 1 = remote

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {

	if (connection_mode == -1) {
		DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG_CONNECTION), NULL, ResolveConection);
		if (connection_mode == -1)
			return;
			//DestroyWindow(hWnd);
	}
			

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
   // ("TrataEventos" foi declarada no in�cio e
   // encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW;// Estilo da janela: Fazer o redraw se for
   // modificada horizontal ou verticalmente
	wcApp.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LOGGO));// "hIcon" = handler do �con normal
   //"NULL" = Icon definido no Windows
   // "IDI_AP..." �cone "aplica��o"

	//here
	wcApp.hIconSm = LoadIcon(hInst, MAKEINTRESOURCE(IDI_LOGGO));// "hIconSm" = handler do �con pequeno

	//"NULL" = Icon definido no Windows
	// "IDI_INF..." �con de informa��o
	wcApp.hCursor = LoadCursor(hInst, MAKEINTRESOURCE(IDC_INIT)); // "hCursor" = handler do cursor (rato)
   // "NULL" = Forma definida no Windows
   // "IDC_ARROW" Aspecto "seta"
	wcApp.lpszMenuName = MAKEINTRESOURCE(IDM_MENU); // Classe do menu que a janela pode ter
   // (NULL = n�o tem menu)
	wcApp.cbClsExtra = 0; // Livre, para uso particular
	wcApp.cbWndExtra = 0; // Livre, para uso particular

	//here
	//wcApp.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcApp.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));

	// "hbrBackground" = handler para "brush" de pintura do fundo da janela. Devolvido por
	// "GetStockObject".Neste caso o fundo ser� branco
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
// ============================================================================
// 4. Mostrar a janela
// ============================================================================
	ShowWindow(hWnd, nCmdShow); // "hWnd"= handler da janela, devolvido por
   // "CreateWindow"; "nCmdShow"= modo de exibi��o (p.e.
   // normal/modal); � passado como par�metro de WinMain()
	UpdateWindow(hWnd); // Refrescar a janela (Windows envia � janela uma
   // mensagem para pintar, mostrar dados, (refrescar)�
   // ============================================================================
   // 5. Loop de Mensagens
   // ============================================================================
   // O Windows envia mensagens �s janelas (programas). Estas mensagens ficam numa fila de
   // espera at� que GetMessage(...) possa ler "a mensagem seguinte"
   // Par�metros de "getMessage":
   // 1)"&lpMsg"=Endere�o de uma estrutura do tipo MSG ("MSG lpMsg" ja foi declarada no
   // in�cio de WinMain()):
   // HWND hwnd handler da janela a que se destina a mensagem
   // UINT message Identificador da mensagem
   // WPARAM wParam Par�metro, p.e. c�digo da tecla premida
   // LPARAM lParam Par�metro, p.e. se ALT tamb�m estava premida
   // DWORD time Hora a que a mensagem foi enviada pelo Windows
   // POINT pt Localiza��o do mouse (x, y)
   // 2)handle da window para a qual se pretendem receber mensagens (=NULL se se pretendem
   // receber as mensagens para todas as janelas pertencentes � thread actual)
   // 3)C�digo limite inferior das mensagens que se pretendem receber
   // 4)C�digo limite superior das mensagens que se pretendem receber
   // NOTA: GetMessage() devolve 0 quando for recebida a mensagem de fecho da janela,
   // terminando ent�o o loop de recep��o de mensagens, e o programa
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

TCHAR login[100];
LRESULT CALLBACK TrataEventos(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	RECT rect;
	TCHAR text_clicks[50];
	int res;
	PAINTSTRUCT ps;
	switch (messg) {
	case WM_CREATE:
		break;
	case WM_COMMAND:

		switch (LOWORD(wParam)) {
		case ID_PLAY:													//NULL em vez de hWnd para a janela nao ter "prioridade"
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
// "PostQuitMessage(Exit Status)"
		PostQuitMessage(0);
		break;
	default:
		// Neste exemplo, para qualquer outra mensagem (p.e. "minimizar","maximizar","restaurar")
		// n�o � efectuado nenhum processamento, apenas se segue o "default" do Windows
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