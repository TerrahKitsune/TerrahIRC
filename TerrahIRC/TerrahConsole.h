#pragma once
#include "resource.h"
#include "MultiSocket.h"
#include "TerrahStack.h"

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

struct PrintMSG{

	char * str;
	int len;
};

class TerrahConsole
{
public:
	TerrahConsole(HINSTANCE hInstance, int screensplit, int MaxLetters, LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam));
	~TerrahConsole();
	int MessageLoop();
	//Should run in your winproc
	void HandleResize(UINT msg);
	void Puts(const char * txt, int len);
	char * Gets(char * inbuf, int max);
	void RawPuts(const char * txt, int len);
	bool ApplyMsgToConsole();

	WNDPROC InputDefProc;
	WNDCLASSEX wc;
	HWND hwnd;
	HWND hwndOutput;
	HWND hwndInput;
	MSG Msg;

	unsigned long TypeSetSize;
	unsigned long consolebuffersize;
	char * consolebuffer;
	char * printmsgbuffer;
	long printmsglen;

	int TextMax;
	TerrahStack<PrintMSG*> * msgs;
};

