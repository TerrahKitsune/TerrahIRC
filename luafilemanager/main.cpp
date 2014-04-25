#include "luafilemanager.h"

extern "C" {
	int LUA_API luaopen_luafilemanager(lua_State *L);
}

typedef struct LFM {
	CFileManager * MGR;
	char * filter;
} LFM;

static LFM *toLFM(lua_State *L, int index)
{
	LFM *LS = (LFM *)lua_touserdata(L, index);
	if (LS == NULL) luaL_typerror(L, index, PROJECT_TABLENAME);
	return LS;
}

static LFM *checkLFM(lua_State *L, int index)
{
	LFM *LS;
	luaL_checktype(L, index, LUA_TUSERDATA);
	LS = (LFM *)luaL_checkudata(L, index, PROJECT_TABLENAME);
	if (LS == NULL) luaL_typerror(L, index, PROJECT_TABLENAME);
	return LS;
}

static LFM *pushLFM(lua_State *L)
{
	LFM *LS = (LFM *)lua_newuserdata(L, sizeof(LFM));
	luaL_getmetatable(L, PROJECT_TABLENAME);
	lua_setmetatable(L, -2);
	memset(LS, 0, sizeof(LFM));
	return LS;
}

static int ls__gc(lua_State *L){

	LFM * ls = toLFM(L, 1);

	if (ls->MGR)
		delete ls->MGR;

	if (ls->filter)
		delete[]ls->filter;

	return 0;
}

static int ls__tostring(lua_State *L){

	lua_pushstring(L, toLFM(L, 1)->filter == NULL ? "" : toLFM(L, 1)->filter);
	return 1;
}

static int lua_file(lua_State *L){

	LFM * data = pushLFM(L);

	data->filter = NULL;
	data->MGR = new CFileManager();

	return 1;
}

void PushIntoToStack(lua_State *L, WIN32_FIND_DATA*f,LFM * data){

	if (!f){
		lua_pushnil(L);
		return;
	}

	lua_createtable(L, 0, 8);

	lua_pushstring(L, "AltName");
	lua_pushstring(L, f->cAlternateFileName);
	lua_settable(L, -3);

	lua_pushstring(L, "Name");
	lua_pushstring(L, f->cFileName);
	lua_settable(L, -3);

	lua_pushstring(L, "Attributes");
	lua_pushinteger(L, f->dwFileAttributes);
	lua_settable(L, -3);

	lua_pushstring(L, "CreationTime");
	lua_pushinteger(L, data->MGR->FileTimeToUnixTime(&f->ftCreationTime));
	lua_settable(L, -3);
	
	lua_pushstring(L, "LastAccessTime");
	lua_pushinteger(L, data->MGR->FileTimeToUnixTime(&f->ftLastAccessTime));
	lua_settable(L, -3);

	lua_pushstring(L, "LastWriteTime");
	lua_pushinteger(L, data->MGR->FileTimeToUnixTime(&f->ftLastWriteTime));
	lua_settable(L, -3);

	ULONGLONG FileSize = f->nFileSizeHigh;
	FileSize <<= sizeof(f->nFileSizeHigh) * 8; // Push by count of bits
	FileSize |= f->nFileSizeLow;

	lua_pushstring(L, "Size");
	lua_pushinteger(L, FileSize);
	lua_settable(L, -3);

	lua_pushstring(L, "Folder");
	lua_pushboolean(L, f->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
	lua_settable(L, -3);
}

static int lua_getfirst(lua_State *L){

	LFM * data = checkLFM(L, 1);

	if (!data->MGR){
		lua_pushnil(L);
		return 1;
	}

	size_t len;
	const char * filter = luaL_checklstring(L, 2, &len);
	lua_pop(L, 2);
	if (data->filter)
		delete[]data->filter;
	data->filter = new char[len + 1];

	if (!data->filter){
		lua_pushnil(L);
		return 1;
	}

	data->filter[len] = 0;
	strncpy(data->filter, filter, len);

	WIN32_FIND_DATA * f = data->MGR->GetFirstNextFile(data->filter,true);

	PushIntoToStack(L, f, data);

	return 1;
}

static int lua_getnext(lua_State *L){

	LFM * data = checkLFM(L, 1);
	lua_pop(L, 1);

	if (!data->MGR){
		lua_pushnil(L);
		return 1;
	}

	PushIntoToStack(L, data->MGR->GetFirstNextFile(data->filter, false), data);

	return 1;
}

static void stackDump(lua_State *L) {
	int i = lua_gettop(L);
	FILE * f = fopen("stack.txt", "a+");
	fprintf(f," ----------------  Stack Dump ----------------\n");
	while (i) {
		int t = lua_type(L, i);
		switch (t) {
		case LUA_TSTRING:
			fprintf(f, "%d:`%s'\n", i, lua_tostring(L, i));
			break;
		case LUA_TBOOLEAN:
			fprintf(f, "%d: %s\n", i, lua_toboolean(L, i) ? "true" : "false");
			break;
		case LUA_TNUMBER:
			fprintf(f, "%d: %g\n", i, lua_tonumber(L, i));
			break;
		default: fprintf(f, "%d: %s\n", i, lua_typename(L, t)); break;
		}
		i--;
	}
	fprintf(f,"--------------- Stack Dump Finished ---------------\n");
	fclose(f);
}

int _stdcall FMTrav(CFileManager * pThis, const char * str){

	//lua_pop((lua_State*)pThis->CustomData, 1);
	
	lua_pushvalue((lua_State*)pThis->CustomData, 2);

	lua_pushstring((lua_State*)pThis->CustomData, str);

	/*stackDump((lua_State*)pThis->CustomData);

	if (!lua_isfunction((lua_State*)pThis->CustomData, 2)){
		//stackDump((lua_State*)pThis->CustomData);
		luaL_error((lua_State*)pThis->CustomData, "Wasnt func");
	}*/

	if (lua_pcall((lua_State*)pThis->CustomData, 1, 1, 0) != 0){
		luaL_error((lua_State*)pThis->CustomData, lua_tostring((lua_State*)pThis->CustomData, -1));
		return 1;
	}

	int n = lua_tonumber((lua_State*)pThis->CustomData, 1);
	lua_pop((lua_State*)pThis->CustomData, 1);
	return n;
}

static int lua_Traversal(lua_State *L){

	LFM * data = checkLFM(L, 1);
	const char * str = luaL_checkstring(L, 3);

	if (!lua_isfunction(L, 2)){
		luaL_error(L, "function expected!");
		return 0;
	}

	data->MGR->CustomData = L;
	lua_pop(L, 1);
	int n = data->MGR->RunForeach(FMTrav, str, true);
	data->MGR->CustomData = NULL;

	lua_pop(L, 2);
	lua_pushinteger(L, n);
	//stackDump(L);
	return 1;
}

static int lua_getLogicalDrives(lua_State *L){

	LFM * data = checkLFM(L, 1);
	lua_pop(L, 1);

	if (!data->MGR){
		lua_pushnil(L);
		return 1;
	}

	lua_newtable(L);
	int i = 0;
	char letter[2] = { 0 };
	letter[0] = data->MGR->GetFirstNextDriveLetter(true);
	while (letter[0] > 0){

		lua_pushstring(L, letter);
		lua_rawseti(L, -2, ++i );

		letter[0] = data->MGR->GetFirstNextDriveLetter(false);
	}

	return 1;
}

int LUA_API luaopen_luafilemanager(lua_State *L) {

	struct luaL_reg driver[] = {
		{ "File", lua_file },
		{ "GetFirst", lua_getfirst },
		{ "GetNext", lua_getnext },
		{ "GetLogicalDrives", lua_getLogicalDrives },
		{ "FileTraversal", lua_Traversal },
		{ NULL, NULL },
	};

	struct luaL_reg ls_meta[] = {
		{ "__gc", ls__gc },
		{ "__tostring", ls__tostring },
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

	lua_pop(L, 1);

	return 1;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){

	if (fdwReason == DLL_PROCESS_DETACH){
	
		;
	}

	return true;
}