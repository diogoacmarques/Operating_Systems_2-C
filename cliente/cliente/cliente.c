#include <windows.h>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX 256

void gotoxy(int x, int y);
DWORD WINAPI Bola(LPVOID param);
DWORD WINAPI Barreira(LPVOID param);
void hidecursor();

HANDLE stdoutMutex;

typedef struct gameInfo {
	DWORD limx, limy; //map info
	DWORD baPosx, baPosy,baTam;//barreira info
}data,*pdata;
			
int _tmain(int argc, LPTSTR argv[]) {
	DWORD threadId;
	HANDLE hTBarreira;
	//UNICODE: Por defeito, a consola Windows não processe caracteres wide.
	//A maneira mais fácil para ter esta funcionalidade é chamar _setmode:
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif
	pdata game = (pdata)malloc(sizeof(data));
	if (!game) {
		printf("Erro ao reserver memoria estrutura\n");
		return -1;
	}
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	game->limx = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	game->limy = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	stdoutMutex = CreateMutex(NULL, FALSE, NULL);
	hidecursor();
	hTBarreira = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Barreira, (LPVOID)game, 0, &threadId);
	if (hTBarreira == NULL) {
		_tprintf(TEXT("Erro ao criar thread Barreira"));
		return -1;
	}
	WaitForSingleObject(hTBarreira, INFINITE);
	free(game);
	CloseHandle(stdoutMutex);
	_tprintf(TEXT("[Thread Principal %d] Vou terminar..."),GetCurrentThreadId());
	return 0;
}

DWORD WINAPI Barreira(LPVOID param) {
	pdata gameInfo = ((pdata)param);
	gameInfo->baTam = 15;
	gameInfo->baPosx = gameInfo->limx / 2, gameInfo->baPosy = gameInfo->limy - 2;

	WaitForSingleObject(stdoutMutex, INFINITE);
	gotoxy(gameInfo->baPosx, gameInfo->baPosy);
	for (int i = 0; i < gameInfo->baTam; i++) {
		_tprintf(TEXT("O"));
	}
	ReleaseMutex(stdoutMutex);

	TCHAR KeyPress;
	HANDLE hTBola;
	DWORD threadId;
	while (1) {
		KeyPress = _getch();
		
		//_putch(KeyPress);
		switch (KeyPress) {
		case 'a':
		case 'A':
		case 'K'://left arrow
			if (gameInfo->baPosx <= 0)
				break;
			WaitForSingleObject(stdoutMutex,INFINITE);
			gotoxy(gameInfo->baPosx + gameInfo->baTam - 1, gameInfo->baPosy);
			_tprintf(TEXT(" "));
			gameInfo->baPosx--;
			gotoxy(gameInfo->baPosx, gameInfo->baPosy);
			_tprintf(TEXT("O"));
			ReleaseMutex(stdoutMutex);
			break;

		case 'd':
		case 'D':
		case 'M':
		//right arrow
			if (gameInfo->baPosx + gameInfo->baTam >= gameInfo->limx)
				break;
			WaitForSingleObject(stdoutMutex, INFINITE);
			gotoxy(gameInfo->baPosx, gameInfo->baPosy);
			_tprintf(TEXT(" "));
			gameInfo->baPosx++;
			gotoxy(gameInfo->baPosx + gameInfo->baTam-1, gameInfo->baPosy);
			_tprintf(TEXT("O"));
			ReleaseMutex(stdoutMutex);
			break;
		case 32:
			WaitForSingleObject(stdoutMutex, INFINITE);
			gotoxy(0, gameInfo->limy / 2);
			for (int i = 0; i < gameInfo->limx; i++) {
				_tprintf(TEXT(" "));
			}
			ReleaseMutex(stdoutMutex);
			hTBola = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Bola, (LPVOID)gameInfo, 0, &threadId);
			if (hTBola == NULL) {
				_tprintf(TEXT("Erro ao criar thread Bola!"));
				return -1;
			}
			break;

		case 27://ESC
			gotoxy(0, 1);
			_tprintf(TEXT("EXITING"));
			ExitThread(NULL);
			break;
		}
	}
}

DWORD WINAPI Bola(LPVOID param) {
	pdata gameInfo = ((pdata)param);
	srand((int)time(NULL));
	boolean goingUp = 1, goingRight = 1;
	DWORD posx = gameInfo->limx/2, posy = gameInfo->limy/2, oposx, oposy;
	//goingUp = (rand() % 2);
	goingRight = (rand() % 2);
	WaitForSingleObject(stdoutMutex, INFINITE);
	gotoxy(posx, posy);
	_tprintf(TEXT("B"));
	ReleaseMutex(stdoutMutex);
	while (1) {
		//Sleep(30);
		Sleep(100);
		oposx = posx;
		oposy = posy;
		if (goingRight){
			if (posx < gameInfo->limx-1) {
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
			if (posy < gameInfo->limy - 1) {// se nao atinge o limite do mapa
				if (posy == gameInfo->baPosy - 1 && (posx >= gameInfo->baPosx && posx <= (gameInfo->baPosx + gameInfo->baTam))) {//atinge a barreira
					posy--;
					goingUp = 1;
				}else
					posy++;
			}
			else {//se atinge o fim do mapa
				WaitForSingleObject(stdoutMutex, INFINITE);
				gotoxy(gameInfo->limx/3, gameInfo->limy/2);
				_tprintf(TEXT("Game Over! Press Space for another ball!\n"));
				ReleaseMutex(stdoutMutex);
				ExitThread(NULL);
				posy--;
				goingUp = 1;
			}
		}



		/*gotoxy(5, 4);
		_tprintf(TEXT("Barreira entre:(%d,%d) at %d\n"),gameInfo->baPosx,gameInfo->baPosx + gameInfo->baTam,gameInfo->baPosy);
		gotoxy(5, 5);
		_tprintf(TEXT("Right:%d\n"), goingRight);
		gotoxy(5,5);
		_tprintf(TEXT("Right:%d\n"),goingRight);
		gotoxy(5, 6);
		_tprintf(TEXT("Up:%d\n"), goingUp);
		gotoxy(5, 7);
		_tprintf(TEXT("posx:%d\n"), posx);
		gotoxy(5, 8);
		_tprintf(TEXT("posy:%d\n"), posy);*/

		WaitForSingleObject(stdoutMutex, INFINITE);
		gotoxy(oposx, oposy);
		_tprintf(TEXT(" "));
		gotoxy(posx, posy);
		_tprintf(TEXT("B"));
		ReleaseMutex(stdoutMutex);

	}
	
}

void hidecursor()
{
	HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO info;
	info.dwSize = 100;
	info.bVisible = FALSE;
	SetConsoleCursorInfo(consoleHandle, &info);
}

void gotoxy(int x, int y) {
	static HANDLE hStdout = NULL;
	COORD coord;
	coord.X = x;
	coord.Y = y;
	if (!hStdout)
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(hStdout, coord);
}