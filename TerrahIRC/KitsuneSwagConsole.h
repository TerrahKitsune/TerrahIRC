#pragma once
#include "TerrahConsole.h"
#include <conio.h>
#include "Lua.hpp"
#include "IniFile.h"
#include <Windows.h>
#pragma comment(lib, "lua51.lib")

LRESULT CALLBACK subEditProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class KitsuneSwagConsole{
public:
	KitsuneSwagConsole(){ memset(this, 0, sizeof(KitsuneSwagConsole)); }

	TerrahConsole * TC;
	MultiSocket * Network;
	lua_State *L;
	bool EnterIsDown;
	char * StartScript;
	char * RecvFunc;
	char * SendFunc;
	char * TickFunc;
	char * ExitFunc;
	char * ErrorFunc;
	int MilisecondTicks;
	int ScreenSplit;
	int MsgLoopTimer;
	int MaxConsoleLetters;
	UINT_PTR timerId;
	int BUFFERSIZE;
	double PCFreq;
	__int64 CounterStart;
	double LuaFreq;
	__int64 LuaLastAlive;
	HANDLE LuaEngineThread;
	DWORD LuaThreadData;
	DWORD ExhangeMutex;
	bool Alive;
	char * ThreadExhange[3];
	char * prevlua;
	char * ErrorMessage;
	bool LuaMustDie;
	bool ErrorFlag;
	int SocketBufferSize;
	FILE * tmpfile;
	bool MainToLuaThreadCall(char * func, char * param, int size);

	void StartCounter();
	double GetCounter();

	void LuaAlive();
	double GetLuaAlive();

	void DoErrorEvent(const char * msg);
	char * GetWindowTextGoodEdition(HWND hwnd);
	bool CallLuaFunction(char * func, char * param, int size);
	void LoadAllShitWeNeed();
	void SaveAllShit();
	void TimeForLua();
	void LoadKSCLuaLib();
	
};

extern KitsuneSwagConsole * ASWN;