#include "KitsuneSwagConsole.h"

#define PROJECT_TABLENAME "Network"

static int lua_putsmyprint(lua_State *L){

	int n = lua_gettop(L);
	size_t len;
	const char * str;

	if (n > 1){

		for (int i = 1; i <= n; i++){

			str = lua_tolstring(L, i, &len);

			ASWN->TC->Puts(str,len);
			ASWN->TC->Puts("\r\n",2);
		}
	}
	else{

		str = lua_tolstring(L, 1, &len);

		ASWN->TC->Puts(str,len);
	}

	lua_pop(L, n);

	return 0;
}

static int lua_cls(lua_State *L){

	SetWindowText(ASWN->TC->hwndOutput, "");
	return 0;
}

static int lua_die(lua_State *L){

	PostMessage(ASWN->TC->hwnd, WM_CLOSE, 0, 0);
	return 0;
}

typedef struct LSock {
	SOCKET sock;
} LSock;


static LSock *toLSock(lua_State *L, int index)
{
	LSock *LS = (LSock *)lua_touserdata(L, index);
	if (LS == NULL) luaL_typerror(L, index, PROJECT_TABLENAME);
	return LS;
}

static LSock *checkLSock(lua_State *L, int index)
{
	LSock *LS;
	luaL_checktype(L, index, LUA_TUSERDATA);
	LS = (LSock *)luaL_checkudata(L, index, PROJECT_TABLENAME);
	if (LS == NULL) luaL_typerror(L, index, PROJECT_TABLENAME);
	return LS;
}

static LSock *pushLSock(lua_State *L)
{
	LSock *LS = (LSock *)lua_newuserdata(L, sizeof(LSock));
	luaL_getmetatable(L, PROJECT_TABLENAME);
	lua_setmetatable(L, -2);
	memset(LS, 0, sizeof(LSock));
	return LS;
}

static int ls__eq(lua_State *L){

	LSock * ls = toLSock(L, 1);
	int n = lua_tointeger(L, 2);
	
	lua_pushboolean(L, ls->sock == n);

	return 1;
}

static int ls__lt(lua_State *L){

	LSock * ls = toLSock(L, 1);
	int n = lua_tointeger(L, 2);

	lua_pushboolean(L, (int)ls->sock < n);

	return 1;
}

static int ls__le(lua_State *L){

	LSock * ls = toLSock(L, 1);
	int n = lua_tointeger(L, 2);

	lua_pushboolean(L, (int)ls->sock <= n);

	return 1;
}

static int ls__gc(lua_State *L){

	LSock * ls = toLSock(L, 1);

	if (ls->sock > 0){

		ASWN->Network->KillSocket(ls->sock);
		ls->sock = 0;
	}

	return 0;
}

static int ls__tostring(lua_State *L){

	char buf[100];

	sprintf(buf, "%u", toLSock(L, 1)->sock);
	lua_pushstring(L, buf);

	return 1;
}

static int lua_send(lua_State *L){


	LSock * ls = checkLSock(L, 1);
	size_t len;
	const char * str = lua_tolstring(L, 2, &len);
	SOCKET serv = ls->sock;
	SOCKET cli = lua_tointeger(L, 3);

	int n = ASWN->Network->Send(serv, str, len, cli);
	lua_pop(L, 3);
	lua_pushinteger(L, n);
	return 1;
}

static int lua_connectclient(lua_State *L){

	SOCKET n = ASWN->Network->OpenClientSocket(lua_tostring(L, 1), lua_tointeger(L,2));
	lua_pop(L, 2);

	if (n > 0){
		LSock * ls = pushLSock(L);
		ls->sock = n;
	}
	else
		lua_pushnil(L);

	return 1;
}

static int lua_connectserver(lua_State *L){

	SOCKET n = ASWN->Network->OpenServerSocket(lua_tointeger(L, 1));

	lua_pop(L, 1);

	if (n > 0){
		LSock * ls = pushLSock(L);
		ls->sock = n;
	}
	else
		lua_pushnil(L);

	return 1;
}

static int lua_closesocket(lua_State *L){

	LSock * ls = checkLSock(L, 1);

	if (ls->sock>0)
		ASWN->Network->KillSocket(ls->sock);

	lua_pop(L, 1);
	return 0;
}
static int lua_stoi(lua_State *L){
	LSock * ls = checkLSock(L, 1);
	lua_pop(L, 1);
	lua_pushinteger(L, ls->sock);
	return 1;
}

static int lua_getinfo(lua_State *L){

	LSock * ls = checkLSock(L, 1);
	SOCKET sock = ls->sock;
	lua_pop(L, 1);

	if (ASWN->Network->SocketsLen > 0)
		lua_newtable(L);
	else{
		lua_pushnil(L);
		return 1;
	}

	for (int n = 0; n < ASWN->Network->SocketsLen; n++){

		if (ASWN->Network->Sockets[n]->Socket == sock){

			lua_pushstring(L, "IP");
			lua_pushstring(L, ASWN->Network->Sockets[n]->TargetServer);
			lua_settable(ASWN->L, -3);

			lua_pushstring(L, "Port");
			lua_pushinteger(L, ASWN->Network->Sockets[n]->port);
			lua_settable(ASWN->L, -3);

			lua_pushstring(L, "Type");
			lua_pushinteger(L, ASWN->Network->Sockets[n]->isServer ? 1 : 0);
			lua_settable(ASWN->L, -3);

			if (ASWN->Network->Sockets[n]->isServer && ASWN->Network->Sockets[n]->SCSize){

				char MsgBuff[NI_MAXHOST];

				lua_newtable(L);

				for (int i = 0; i < ASWN->Network->Sockets[n]->SCLength; i++){

					lua_pushinteger(L, ASWN->Network->Sockets[n]->SC[i]->socket);
					sprintf(MsgBuff, "%s:%d", ASWN->Network->Sockets[n]->SC[i]->IP, ASWN->Network->Sockets[n]->SC[i]->port);
					lua_pushstring(L, MsgBuff);
					lua_settable(ASWN->L, -3);
				}			
				lua_setfield(L, -2, "Clients");
			}

			return 1;
		}
		else if (ASWN->Network->Sockets[n]->isServer){

			for (int i = 0; i < ASWN->Network->Sockets[n]->SCLength; i++){

				if (ASWN->Network->Sockets[n]->SC[i]->socket == sock){
					
					lua_pushstring(L, "IP");
					lua_pushstring(L, ASWN->Network->Sockets[n]->SC[i]->IP);
					lua_settable(ASWN->L, -3);

					lua_pushstring(L, "Port");
					lua_pushinteger(L, ASWN->Network->Sockets[n]->SC[i]->port);
					lua_settable(ASWN->L, -3);

					lua_pushstring(L, "Type");
					lua_pushinteger(L, 2);
					lua_settable(ASWN->L, -3);

					lua_pushstring(L, "Server");
					lua_pushinteger(L, ASWN->Network->Sockets[n]->port);
					lua_settable(ASWN->L, -3);

					return 1;
				}
			}
		}
	}

	return 1;
}

static int lua_getall(lua_State *L){

	if (ASWN->Network->SocketsLen > 0)
		lua_newtable(L);
	else{
		lua_pushnil(L);
		return 1;
	}

	char MsgBuff[NI_MAXHOST];

	for (int n = 0; n < ASWN->Network->SocketsLen; n++){

		if (ASWN->Network->Sockets[n]->isServer){

			lua_pushinteger(L, ASWN->Network->Sockets[n]->Socket);
			sprintf(MsgBuff, "*:%d", ASWN->Network->Sockets[n]->port);
			lua_pushstring(L, MsgBuff);
			lua_settable(ASWN->L, -3);

			for (int i = 0; i < ASWN->Network->Sockets[n]->SCLength; i++){
				
				lua_pushinteger(L, ASWN->Network->Sockets[n]->SC[i]->socket);
				sprintf(MsgBuff, "%s:%d", ASWN->Network->Sockets[n]->SC[i]->IP,ASWN->Network->Sockets[n]->SC[i]->port);
				lua_pushstring(L, MsgBuff);
				lua_settable(ASWN->L, -3);
			}
		}
		else{
			lua_pushinteger(L, ASWN->Network->Sockets[n]->Socket);
			sprintf(MsgBuff, "%s:%d", ASWN->Network->Sockets[n]->TargetServer, ASWN->Network->Sockets[n]->port);
			lua_pushstring(L, MsgBuff);
			lua_settable(ASWN->L, -3);
		}
	}

	return 1;
}

void luadiehook(lua_State* L, lua_Debug *ar)
{
	if (ASWN->LuaMustDie)
	{

		char message[255];
		sprintf(message, "Application closed lua execution! (Timeout: %ld)", ASWN->MsgLoopTimer);	
		luaL_error(L, message);
		ASWN->LuaMustDie = false;
	}
}

static int lua_SetVisible(lua_State *L){

	ShowWindow(ASWN->TC->hwnd, lua_tointeger(L,1));
	lua_pop(L, 1);
	return 0;
}

static int lua_GetVisible(lua_State *L){

	lua_pushinteger(L, IsWindowVisible(ASWN->TC->hwnd));
	return 1;
}

static int lua_Focus(lua_State *L){

	if (IsWindowVisible(ASWN->TC->hwnd))
		SwitchToThisWindow(ASWN->TC->hwnd,0);
	return 0;
}

void RegFunc(const char * name, lua_CFunction func ){

	lua_getglobal(ASWN->L, "KSC");

	if (!lua_istable(ASWN->L, -1)){

		lua_createtable(ASWN->L, 0, 1);
		lua_setglobal(ASWN->L, "KSC");
		lua_pop(ASWN->L, 1);

		lua_getglobal(ASWN->L, "KSC");
	}

	lua_pushstring(ASWN->L, name);
	lua_pushcfunction(ASWN->L, func);
	lua_settable(ASWN->L,-3);
	lua_pop(ASWN->L, 1);
}

void KitsuneSwagConsole::LoadKSCLuaLib(){

	lua_sethook(L, luadiehook, LUA_MASKCOUNT, 10000);
	lua_register(L, "print", lua_putsmyprint);

	RegFunc( "Clear", lua_cls);
	RegFunc( "Exit", lua_die);
	RegFunc( "Close", lua_closesocket);
	RegFunc( "GetAll", lua_getall); 
	RegFunc( "GetInfo", lua_getinfo);
	RegFunc("Client", lua_connectclient);
	RegFunc("Server", lua_connectserver);
	RegFunc("SetVisible", lua_SetVisible);
	RegFunc("GetVisible", lua_GetVisible);
	RegFunc("Focus", lua_Focus);

	struct luaL_reg driver[] = {
		{ "Send", lua_send },
		{ "Close", lua_closesocket },
		{ "AsInt", lua_stoi },
		{ NULL, NULL },
	};

	struct luaL_reg ls_meta[] = {
		{ "__gc", ls__gc },
		{ "__tostring", ls__tostring },
		{ "__eq", ls__eq },
		{ "__lt", ls__lt },
		{ "__le", ls__le },
		//{ "__add", Foo_add },
		{ 0, 0 }
	};

	luaL_openlib(L, PROJECT_TABLENAME, driver, 0);

	luaL_newmetatable(L, PROJECT_TABLENAME);
	luaL_openlib(L, 0, ls_meta, 0);
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);

	lua_setglobal(ASWN->L, PROJECT_TABLENAME);
}