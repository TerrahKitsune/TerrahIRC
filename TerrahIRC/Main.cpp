#include "KitsuneSwagConsole.h"

KitsuneSwagConsole * ASWN;

DWORD WINAPI LuaMain(LPVOID lpParam){

	//Engien buffer
	ASWN->BUFFERSIZE = 100000;
	char * Buffer = new char[ASWN->BUFFERSIZE +1];

	while (!Buffer){

		if ((--ASWN->BUFFERSIZE) < 512){

			MessageBox(NULL, "Unable to create lua engien memory buffer!", "LAME!", NULL);
			PostMessage(ASWN->TC->hwnd, WM_CLOSE, 0, 0);
			return 0;
		}

		Buffer = new char[ASWN->BUFFERSIZE +1];
	}

	//Start event
	if (ASWN->StartScript && ASWN->StartScript[0] != 0){

		int s = luaL_loadfile(ASWN->L, ASWN->StartScript);

		if (s == 0) {

			if (lua_pcall(ASWN->L, 0, LUA_MULTRET, 0) != 0) {
				strcpy(Buffer, lua_tostring(ASWN->L, -1));
				lua_pop(ASWN->L, 1); // remove error message
				ASWN->DoErrorEvent(Buffer);
			}
		}
		else{
			ASWN->DoErrorEvent("Lua start script file not found!");
		}
	}

	ASWN->StartCounter();

	//Buisness as usual
	while (ASWN->Alive){

		ASWN->LuaAlive();

		//Thread exhange
		if (ASWN->ExhangeMutex == 2){

			ASWN->CallLuaFunction(ASWN->ThreadExhange[0], ASWN->ThreadExhange[1],(int)ASWN->ThreadExhange[2]);
			ASWN->ExhangeMutex = 0;
		}
		else if (ASWN->ExhangeMutex == 3){
			
			if (ASWN->StartScript && ASWN->StartScript[0] != 0){

				int s = luaL_loadfile(ASWN->L, ASWN->StartScript);

				if (s == 0) {

					if (lua_pcall(ASWN->L, 0, 0, 0) != 0) {
						strcpy(Buffer, lua_tostring(ASWN->L, -1));
						lua_pop(ASWN->L, 1); // remove error message
						ASWN->DoErrorEvent(Buffer);
					}
				}
				else{
					ASWN->DoErrorEvent("Lua start script file not found!");
				}
			}
			ASWN->ExhangeMutex = 0;
		}

		//Heartbeat
		if (ASWN->MilisecondTicks >= 0)
			ASWN->TimeForLua();

		//Network
		if (ASWN->Network->GetOpenSockets() > 0){

			ASWN->Network->Listen();
		}
		else{

			if (ASWN->MilisecondTicks < 0){
				SuspendThread(ASWN->LuaEngineThread);
			}
			else
				Sleep(1);
		}
	}

	//Stop event
	if (ASWN->ExitFunc && ASWN->ExitFunc[0] != 0){
		lua_getglobal(ASWN->L, ASWN->ExitFunc);

		if (lua_pcall(ASWN->L, 0, 0, 0) != 0){

			strcpy(Buffer, lua_tostring(ASWN->L, -1));
			lua_pop(ASWN->L, 1); // remove error message
			ASWN->DoErrorEvent(Buffer);
		}
	}

	delete[]Buffer;
	return 0;
}

void RecvProc(MultiSocket * MS, SOCKET To, SOCKET From, const char * data, int nSize, DWORD Flag){

	/*if (Flag == 1){

		ASWN->DoErrorEvent(data);
		return;
	}*/

	if (!ASWN->RecvFunc || ASWN->RecvFunc[0] == 0)
		return;

	lua_getglobal(ASWN->L, ASWN->RecvFunc);

	//lua_newtable(ASWN->L);
	lua_createtable(ASWN->L, 0, 5);

	lua_pushstring(ASWN->L, "Socket");
	lua_pushinteger(ASWN->L, To);
	lua_settable(ASWN->L, -3);

	lua_pushstring(ASWN->L, "From");
	lua_pushinteger(ASWN->L, From);
	lua_settable(ASWN->L, -3);

	lua_pushstring(ASWN->L, "Data");
	lua_pushlstring(ASWN->L, data,nSize);
	lua_settable(ASWN->L, -3);

	lua_pushstring(ASWN->L, "Size");
	lua_pushinteger(ASWN->L, nSize);
	lua_settable(ASWN->L, -3);

	lua_pushstring(ASWN->L, "Flag");
	lua_pushinteger(ASWN->L, Flag);
	
	lua_settable(ASWN->L, -3);

	if (lua_pcall(ASWN->L, 1, 0, 0) != 0){

		size_t strsize;
		const char * str = lua_tolstring(ASWN->L, -1, &strsize);
		if (ASWN->ErrorMessage)delete[]ASWN->ErrorMessage;
		ASWN->ErrorMessage = new char[strsize + 1];

		if (!ASWN->ErrorMessage){

			lua_pop(ASWN->L, 1);

			ASWN->DoErrorEvent("Not enough memory!");
		}
		else{
			strcpy(ASWN->ErrorMessage, str);

			lua_pop(ASWN->L, 1);

			ASWN->DoErrorEvent(ASWN->ErrorMessage);
		}
	}
}

void NastyHook(DWORD targetAddr, DWORD newAddr){

	unsigned char* func = (unsigned char*)targetAddr;

	DWORD oldProtection;
	VirtualProtect(func, 7, PAGE_EXECUTE_READWRITE, &oldProtection);

	func[0] = 0xb8; // mov eax, our_addr
	func[1] = newAddr & 0xff;
	func[2] = (newAddr >> 8) & 0xff;
	func[3] = (newAddr >> 16) & 0xff;
	func[4] = (newAddr >> 24) & 0xff;
	func[5] = 0xff; // jmp eax
	func[6] = 0xe0;

	VirtualProtect(func, 7, oldProtection, &oldProtection);
}

int _cdecl xprintf(const char * _format, ...){

	char buffer[102400];
	va_list args;
	va_start(args, _format);
	vsprintf(buffer, _format, args);
	va_end(args);
	ASWN->TC->Puts(buffer, strlen(buffer));
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine, int nCmdShow)
{
	
	//Init main class; see KSC*.cpp and KitsuneSwagConsole.h/cpp
	ASWN = new KitsuneSwagConsole();

	ASWN->Alive = true;
	ASWN->LuaEngineThread = CreateThread(NULL, 0, LuaMain, NULL, CREATE_SUSPENDED, &ASWN->LuaThreadData);

	if (ASWN->LuaEngineThread == NULL ){

		MessageBox(NULL,"FAILED TO CREATE LUA ENGINE!", "LAME!", NULL);
		return 0;
	}

	ASWN->LoadAllShitWeNeed();
	lua_State *L = lua_open();
	ASWN->L = L;

	luaL_openlibs(L);
	ASWN->LoadKSCLuaLib();

	ASWN->EnterIsDown = false;
	ASWN->TC = new TerrahConsole(hInstance, ASWN->ScreenSplit, ASWN->MaxConsoleLetters, WndProc);
	ASWN->TC->InputDefProc = (WNDPROC)SetWindowLongPtr(ASWN->TC->hwndInput, GWLP_WNDPROC, (LONG_PTR)subEditProc);
	NastyHook((DWORD)&printf, (DWORD)&xprintf);
	MultiSocket TS(&RecvProc);

	ASWN->Network = &TS;

	/*if( ASWN->MsgLoopTimer <= 0 )
		ASWN->timerId = SetTimer(ASWN->TC->hwnd, NULL, USER_TIMER_MINIMUM, NULL);
	else
		ASWN->timerId = SetTimer(ASWN->TC->hwnd, NULL, ASWN->MsgLoopTimer, NULL);*/

	ASWN->timerId = SetTimer(ASWN->TC->hwnd, NULL, USER_TIMER_MINIMUM, NULL);

	int Message = 0;
	int n = 0;

	ResumeThread(ASWN->LuaEngineThread);

	while (ASWN->TC->MessageLoop()){

		//Lua keep-alive anti-hang
		if (ASWN->MsgLoopTimer > 0 && ASWN->Network->GetSocketsReady() > 0){

			if (!ASWN->LuaMustDie && ASWN->GetLuaAlive() > ASWN->MsgLoopTimer){
				ASWN->LuaMustDie = true;
				ASWN->LuaAlive();
			}
		}
		else
			ASWN->LuaAlive();
	}

	ASWN->Alive = false;
	DWORD status = WaitForSingleObject(ASWN->LuaEngineThread, 10000);

	if (status == WAIT_TIMEOUT || status == WAIT_FAILED){
		ASWN->LuaMustDie = true;
		ResumeThread(ASWN->LuaEngineThread);
		WaitForSingleObject(ASWN->LuaEngineThread, INFINITE);
	}

	lua_close(L);
	int ret = ASWN->TC->Msg.wParam;
	delete ASWN->TC;
	delete ASWN;
	fclose(stdout);
	remove("stdout.txt");
	return ret;
}

