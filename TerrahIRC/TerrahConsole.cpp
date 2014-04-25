#include "TerrahConsole.h"

TerrahConsole::TerrahConsole(HINSTANCE hInstance, int screensplit, int MaxLetters,LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)){

	memset(this, 0, sizeof(TerrahConsole));
	this->TypeSetSize = screensplit;
	consolebuffersize = (MaxLetters * 2) + 1;
	consolebuffer = new char[consolebuffersize];
	if (consolebuffer)memset(consolebuffer, 0, consolebuffersize);
	msgs = new TerrahStack<PrintMSG*>;
	//Step 1: Registering the Window Class
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)); //LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wc.lpszClassName = "TerrahWindow";
	wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window Registration Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	// Step 2: Creating the Window
	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		"TerrahWindow",
		"Kitsune Swag Console",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 500, 500,
		NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return;
	}

	HGDIOBJ hfDefault;
	this->hwndOutput = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | ES_AUTOHSCROLL,
		0, 0, 100, 100, hwnd, (HMENU)IDD_FORMVIEW, GetModuleHandle(NULL), NULL);
	if (this->hwndOutput == NULL)
		MessageBox(hwnd, "Could not create edit box.", "Error", MB_OK | MB_ICONERROR);

	hfDefault = GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(this->hwndOutput, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
	//SetWindowText(this->hwndOutput, consolebuffer);

	RECT rcClient;

	GetClientRect(this->hwnd, &rcClient); 
	rcClient.bottom = rcClient.bottom - TypeSetSize;
	SetWindowPos(this->hwndOutput, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);

	this->hwndInput = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
		0, 0, 100, 100, hwnd, (HMENU)IDD_FORMVIEW1, GetModuleHandle(NULL), NULL);
	if (this->hwndInput == NULL)
		MessageBox(hwnd, "Could not create edit box.", "Error", MB_OK | MB_ICONERROR);

	GetClientRect(this->hwnd, &rcClient);
	rcClient.bottom = rcClient.bottom - TypeSetSize;
	SetWindowPos(this->hwndInput, NULL, 0, rcClient.bottom, rcClient.right, TypeSetSize, SWP_NOZORDER);

	SendMessage(hwndOutput, EM_SETLIMITTEXT, (consolebuffersize - 1) / 2, 0);
	TextMax = SendMessage(hwndOutput, EM_GETLIMITTEXT, 0, 0);

	ShowWindow(hwnd, 1);
	UpdateWindow(hwnd);
}

TerrahConsole::~TerrahConsole(){

	if (consolebuffer)
		delete[]consolebuffer;
	if (printmsgbuffer)
		delete[]printmsgbuffer;

	if (msgs)
		delete msgs;
}

void TerrahConsole::HandleResize(UINT msg){

	if (this && msg == WM_SIZE){
		
		RECT rcClient;

		GetClientRect(this->hwnd, &rcClient);
		rcClient.bottom = rcClient.bottom - TypeSetSize;
		SetWindowPos(this->hwndOutput, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOZORDER);
		SetWindowPos(this->hwndInput, NULL, 0, rcClient.bottom, rcClient.right, TypeSetSize, SWP_NOZORDER);
	}
}

char * TerrahConsole::Gets(char * inbuf, int max){

	GetWindowText(this->hwndInput, inbuf, max);

	return inbuf;
}

void TerrahConsole::Puts(const char * txt, int len){

	if (!txt)return;

	PrintMSG * n = new PrintMSG;
	if (!n)return;

	n->str = new char[len + 1];
	if (!n->str)return;
	strncpy(n->str, txt, len);
	n->str[len] = 0;
	n->len = len;

	if (!msgs->Push(n)){
		delete[]n->str;
		delete n;
	}
}

void TerrahConsole::RawPuts(const char * txt, int len){

	if (!txt || !IsWindowVisible(hwnd) )return;

	// get new length to determine buffer size
	int subLen = GetWindowTextLength(this->hwndOutput);
	int outLength = subLen + len + 1;

	// get buffer to hold current and new text
	char * buf = (char *)this->consolebuffer;

	if (this->consolebuffersize < (unsigned)outLength){

		delete[]buf;
		buf = new char[outLength * sizeof(char)];

		if (!buf) {

			this->consolebuffer = NULL;
			this->consolebuffersize = 0;

			SetWindowText(this->hwndOutput, "");
			buf = new char[len + 1];
			if (!buf){
				MessageBox(NULL, "No memory!", "No memory!", 0);
				return;
			}
			else{
				this->consolebuffer = buf;
				this->consolebuffersize = len + 1;
			}
		}
		else{
			this->consolebuffer = buf;
			this->consolebuffersize = outLength;
		}
	}

	// get existing text from edit control and put into buffer
	GetWindowText(this->hwndOutput, buf, outLength);

	// append the newText to the buffer
	strcat(&buf[subLen], txt);

	if (subLen > TextMax){

		char * Target = buf;
		char * Source = &buf[outLength - TextMax];
		memcpy(Target, Source, TextMax);
	}

	// Set the text in the edit control
	SetWindowText(this->hwndOutput, buf);

	//Scroll to the bottom /console style
	SendNotifyMessage(this->hwndOutput, LOWORD(WM_VSCROLL), SB_BOTTOM, 0);

}

bool TerrahConsole::ApplyMsgToConsole(){

	if (msgs->IsEmpty())
		return true;

	EnterCriticalSection(&msgs->CriticalSection);

	TerrahStack<PrintMSG*>::TSNode<PrintMSG*> * node = msgs->root;
	msgs->root = NULL;

	LeaveCriticalSection(&msgs->CriticalSection);

	TerrahStack<PrintMSG*>::TSNode<PrintMSG*> * temp = node;

	while (temp){

		if (temp->Next == NULL)
			break;

		temp = temp->Next;
	}

	PrintMSG * n;

	node = temp;

	int LetsPeek = 0;
	bool ok = true;
	while (node){

		if (ok)
			RawPuts(node->Data->str, node->Data->len);

		temp = node;
		node = node->Prev;

		delete []temp->Data->str;
		delete temp->Data;
		delete temp;
	
		if (LetsPeek++ >= 12 && ok){
			LetsPeek = 0;
			//if (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE) > 0){
			if (GetMessage(&Msg, NULL, 0, 0) > 0){
				TranslateMessage(&Msg);
				DispatchMessage(&Msg);
			}
			else
				ok = false;
		}
	}

	return ok;
}

int TerrahConsole::MessageLoop(){

	if (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
		return ApplyMsgToConsole();
	}
	return false;
}

BOOL CALLBACK AboutDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:

		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwnd, IDOK);
			break;
		case IDCANCEL:
			EndDialog(hwnd, IDCANCEL);
			break;
		}
		break;
	default:
		return FALSE;
	}
	return TRUE;
}