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
DWORD WINAPI Bola();
void hidecursor();
								
int _tmain(int argc, LPTSTR argv[]) {
	DWORD threadId, posx = 0, posy = 0, oposx, oposy,sizex,sizey;
	_TCHAR KeyPress;
	HANDLE hT;
	//UNICODE: Por defeito, a consola Windows não processe caracteres wide.
	//A maneira mais fácil para ter esta funcionalidade é chamar _setmode:
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif	

	hT = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Bola,NULL, 0, &threadId);
	WaitForSingleObject(hT, INFINITE);
	return 0;

	gotoxy(posx, posy);
	_tprintf(TEXT("X"));
	gotoxy(posx, posy);
	while(1){
		KeyPress = _getch();
		oposx = posx;
		oposy = posy;
		//_putch(KeyPress);
		switch (KeyPress) {
			case 'a':
			case 'A':
				gotoxy(oposx, oposy);
				_tprintf(TEXT(" "));
				posx--;
				gotoxy(posx,posy);
				_tprintf(TEXT("X"));
				gotoxy(posx, posy);
				break;

			case 'd':
			case 'D':
				gotoxy(oposx, oposy);
				_tprintf(TEXT(" "));
				posx++;
				gotoxy(posx, posy);
				_tprintf(TEXT("X"));
				gotoxy(posx, posy);
				break;

			case 27:
				gotoxy(0, 1);
				_tprintf(TEXT("EXITING"));
				return 0;
				break;
		}
		
	
		
	}
	
	_tprintf(TEXT("[Thread Principal %d] Vou terminar..."),GetCurrentThreadId());
	return 0;
}

DWORD WINAPI Bola() {
	srand((int)time(NULL));
	boolean goingUp = 1, goingRight = 1;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	DWORD limx = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	DWORD limy = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	DWORD posx = limx/2, posy = limy/2, oposx, oposy;
	goingUp = (rand() % 2);
	goingRight = (rand() % 2);
	//_tprintf(TEXT("lim(%d,%d)"), limx, limy);
	hidecursor();
	gotoxy(posx, posy);
	_tprintf(TEXT("."));
	_getch();
	while (1) {
		Sleep(20);
		oposx = posx;
		oposy = posy;
		if (goingRight){
			if (posx < limx-1) {
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
			if (posy < limy - 1) {
				posy++;
			}
			else {
				posy--;
				goingUp = 1;
			}
		}

		/*gotoxy(5,5);
		_tprintf(TEXT("Right:%d\n"),goingRight);
		gotoxy(5, 6);
		_tprintf(TEXT("Up:%d\n"), goingUp);
		gotoxy(5, 7);
		_tprintf(TEXT("posx:%d\n"), posx);
		gotoxy(5, 8);
		_tprintf(TEXT("posy:%d\n"), posy);*/

		gotoxy(oposx, oposy);
		_tprintf(TEXT(" "));
		gotoxy(posx, posy);
		_tprintf(TEXT("."));

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