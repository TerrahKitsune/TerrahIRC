#include "KitsuneSwagConsole.h"

void KitsuneSwagConsole::StartCounter()
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		MessageBox(NULL, "SHITS BROKE!", "ERROR ERROR TASKETE!", NULL);

	PCFreq = double(li.QuadPart) / 1000.0;

	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;
}
double KitsuneSwagConsole::GetCounter()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - CounterStart) / PCFreq;
}

void KitsuneSwagConsole::LuaAlive()
{
	LARGE_INTEGER li;
	if (!QueryPerformanceFrequency(&li))
		MessageBox(NULL, "SHITS BROKE!", "ERROR ERROR TASKETE!", NULL);

	LuaFreq = double(li.QuadPart) / 1000.0;

	QueryPerformanceCounter(&li);
	LuaLastAlive = li.QuadPart;
}
double KitsuneSwagConsole::GetLuaAlive()
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - LuaLastAlive) / LuaFreq;
}

bool KitsuneSwagConsole::MainToLuaThreadCall(char * func, char * param, int size){


	int retries=0;
	//Critical
	UINT_PTR t = SetTimer(ASWN->TC->hwndOutput, NULL, USER_TIMER_MINIMUM, NULL);
	
	//Incase thread is suspended
	ResumeThread(this->LuaEngineThread);
	
	while (this->ExhangeMutex >= 1){

		MSG Msg;

		if (GetMessage(&Msg, ASWN->TC->hwndOutput, 0, 0) > 0)
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}

		if (retries++ >= 1000){
			this->LuaMustDie = true;
			retries = 0;
		}
	}

	KillTimer(ASWN->TC->hwndOutput, t);

	this->LuaMustDie = false;
	//Signal data being written
	this->ExhangeMutex = 1;

	this->ThreadExhange[0] = func;
	this->ThreadExhange[1] = param;
	this->ThreadExhange[2] = (char*)size;

	//Signal ready to be processed
	this->ExhangeMutex = 2;

	return true;
}

void KitsuneSwagConsole::TimeForLua(){

	if (TickFunc && TickFunc[0] != '\0'){

		double d = GetCounter();
		if (d >= MilisecondTicks){

			lua_getglobal(L, TickFunc);

			if (lua_pcall(L, 0, 0, 0) != 0){

				size_t strsize;
				const char * str = lua_tolstring(ASWN->L, -1, &strsize);

				if (ErrorMessage)delete[]ErrorMessage;
				ErrorMessage = new char[strsize + 1];

				if (!ErrorMessage){

					lua_pop(ASWN->L, 1);

					DoErrorEvent("Not enough memory!");
				}
				else{
					strcpy(ErrorMessage, str);

					lua_pop(ASWN->L, 1);

					DoErrorEvent(ErrorMessage);
				}
			}
			StartCounter();
		}	
	}	
}

char * KitsuneSwagConsole::GetWindowTextGoodEdition(HWND hwnd){

	size_t len = GetWindowTextLength(hwnd) + 1;
	void * c = new char[len];
	if (!c)return NULL;
	GetWindowText(hwnd, (TCHAR *)c, len);
	return (char*)c;
}

void KitsuneSwagConsole::DoErrorEvent(const char * msg){

	if (ErrorFunc && ErrorFunc[0] != '\0'){

		lua_getglobal(L, ErrorFunc);
		lua_pushstring(L, msg);

		if (lua_pcall(L, 1, 0, 0) != 0){

			size_t len;
			const char * str = lua_tolstring(ASWN->L, -1,&len);

			TC->Puts(str,len);
			TC->Puts("\r\n",2);
			lua_pop(ASWN->L,1);
		}
	}	
}

bool KitsuneSwagConsole::CallLuaFunction(char * func, char * param, int size){

	if (func  && func[0] != '\0' && param){

		lua_getglobal(L, func);
		lua_pushlstring(L, param, size);

		if (lua_pcall(L, 1, 0, 0) != 0){

			size_t strsize;
			const char * str = lua_tolstring(ASWN->L, -1,&strsize);
			if (ErrorMessage)delete[]ErrorMessage;
			ErrorMessage = new char[strsize + 1];

			if (!ErrorMessage){

				lua_pop(ASWN->L, 1);

				DoErrorEvent("Not enough memory!");
			}
			else{
				strcpy(ErrorMessage, str);

				lua_pop(ASWN->L, 1);

				DoErrorEvent(ErrorMessage);
			}

			this->ExhangeMutex = 0;
			return false;
		}
	}
	else if(!func && param && param[0]!='\0'){

		//this->TC->Puts(param);
		int err = luaL_loadbuffer(this->L, param, size, "USER LUA");

		if (err == LUA_ERRSYNTAX){
			DoErrorEvent("LUA: Syntax error!");
		}
		else if (err == LUA_ERRMEM){
			DoErrorEvent("LUA: Memory error!");
		}
		else if (lua_pcall(this->L, 0, LUA_MULTRET, 0) != 0) {
			
			size_t strsize;
			const char * str = lua_tolstring(ASWN->L, -1, &strsize);
			if (ErrorMessage)delete[]ErrorMessage;
			ErrorMessage = new char[strsize + 1];

			if (!ErrorMessage){

				lua_pop(ASWN->L, 1);

				DoErrorEvent("Not enough memory!");
			}
			else{
				strcpy(ErrorMessage, str);

				lua_pop(ASWN->L, 1);

				DoErrorEvent(ErrorMessage);
			}

			this->ExhangeMutex = 0;
			return false;
		}
	}
	this->ExhangeMutex = 0;
	return true;
}

void KitsuneSwagConsole::LoadAllShitWeNeed(){

	CIniFile ini("swag_options.ini");
	char Buffer[255];

	ini.ReadString("LUA", "StartScript", Buffer, 255, "");
	StartScript = new char[strlen(Buffer) + 1];
	strcpy(StartScript, Buffer);

	ini.ReadString("LUA", "Recv", Buffer, 255, "");
	RecvFunc = new char[strlen(Buffer) + 1];
	strcpy(RecvFunc, Buffer);

	ini.ReadString("LUA", "Send", Buffer, 255, "");
	SendFunc = new char[strlen(Buffer) + 1];
	strcpy(SendFunc, Buffer);

	ini.ReadString("LUA", "Tick", Buffer, 255, "");
	TickFunc = new char[strlen(Buffer) + 1];
	strcpy(TickFunc, Buffer);

	ini.ReadString("LUA", "Exit", Buffer, 255, "");
	ExitFunc = new char[strlen(Buffer) + 1];
	strcpy(ExitFunc, Buffer);

	ini.ReadString("LUA", "Error", Buffer, 255, "");
	ErrorFunc = new char[strlen(Buffer) + 1];
	strcpy(ErrorFunc, Buffer);

	MsgLoopTimer = ini.ReadLong("LUA", "MsgLoopTimer", 10000);

	ScreenSplit = ini.ReadLong("LUA", "ScreenSplit", 60);
	MilisecondTicks = ini.ReadLong("LUA", "LuaMSTicks", 1000);
	MaxConsoleLetters = ini.ReadLong("LUA", "ConsoleBufferMax", 32767);

	if (MaxConsoleLetters < 0)MaxConsoleLetters = 0;
}

void KitsuneSwagConsole::SaveAllShit(){

	CIniFile ini("swag_options.ini");
	ini.WriteString("LUA", "StartScript", StartScript);
	ini.WriteString("LUA", "Recv", RecvFunc);
	ini.WriteString("LUA", "Send", SendFunc);
	ini.WriteString("LUA", "Tick", TickFunc);
	ini.WriteString("LUA", "Exit", ExitFunc);
	ini.WriteString("LUA", "Error", ErrorFunc);

	ini.WriteInteger("LUA", "MsgLoopTimer", MsgLoopTimer);

	ini.WriteInteger("LUA", "ScreenSplit", ScreenSplit);
	ini.WriteInteger("LUA", "LuaMSTicks", MilisecondTicks);

	if (MaxConsoleLetters < 0)MaxConsoleLetters = 0;
	ini.WriteInteger("LUA", "ConsoleBufferMax", MaxConsoleLetters);
}