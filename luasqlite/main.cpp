#include "luasqlite.h"
#include "TerrahStack.h"
#include <stdio.h>

CRITICAL_SECTION SQLSECT;

extern "C" {
	int LUA_API luaopen_luasqlite(lua_State *L);
}

typedef struct LSQLite {
	sqlite3	* dbptr;
	sqlite3_stmt * stmt;
	char * db;
} LSQLite;

typedef struct SQLS {
	sqlite3	* dbptr;
	sqlite3_stmt * stmt;
	char * sql;
	int ret;
	bool hasSomeoneWaiting;
	bool isPrep;
} SQLS;

HANDLE WorkThread;
bool WTActive;
bool WTAlive;
FILE * Log;

TerrahStack<SQLS*> SQLStack;

DWORD WINAPI SQLThreadSpin(LPVOID lpParam){

	SQLS * cursor;

	while (WTAlive){

		EnterCriticalSection(&SQLSECT);
		WTActive = true;

		if (Log){

			fprintf(Log, "ENTER!\n");
			fflush(Log);
		}

		cursor = SQLStack.Pop();

		while (cursor){

			if (cursor->isPrep)
				cursor->ret = sqlite3_prepare_v2(cursor->dbptr, cursor->sql, -1, &cursor->stmt, 0);
			else
				cursor->ret = sqlite3_exec(cursor->dbptr, cursor->sql, 0, 0, 0);
			
			if (Log){

				fprintf(Log, "%C %C %i -> %s\n", cursor->hasSomeoneWaiting ? 'W' : ' ', cursor->isPrep ? 'P' : 'C', cursor->ret, cursor->sql);
				fflush(Log);
			}

			if (!cursor->hasSomeoneWaiting){

				delete[]cursor->sql;
				delete cursor;
			}
			else
				cursor->hasSomeoneWaiting = false;

			cursor = SQLStack.Pop();
		}

		if (Log){

			fprintf(Log, "LEAVE!\n");
			fflush(Log);
		}

		WTActive = false;
		LeaveCriticalSection(&SQLSECT);
		SuspendThread(WorkThread);
	}
	return 0;
}

int ExecuteSQL(sqlite3	* dbptr, const char * sql, sqlite3_stmt * stmt, bool isPrep, bool waitForRet){

	SQLS * sqls = new SQLS;
	if (!sqls)return 0;

	sqls->sql = new char[strlen(sql) + 1];
	if (!sqls->sql)return 0;

	sqls->dbptr = dbptr;
	sqls->stmt = stmt;
	sqls->isPrep = isPrep;
	sqls->hasSomeoneWaiting = waitForRet;
	strcpy(sqls->sql, sql);
	SQLStack.Push(sqls);
	
	if (!WTActive){
		ResumeThread(WorkThread);
	}

	if (waitForRet){

		while (sqls->hasSomeoneWaiting){

			EnterCriticalSection(&SQLSECT);
			LeaveCriticalSection(&SQLSECT);
		}

		int nRet = sqls->ret;

		delete[]sqls->sql;
		delete sqls;

		return nRet;
	}

	return 0;
}


static LSQLite *toLSQLite(lua_State *L, int index)
{
	LSQLite *LS = (LSQLite *)lua_touserdata(L, index);
	if (LS == NULL) luaL_typerror(L, index, PROJECT_TABLENAME);
	return LS;
}

static LSQLite *checkLSQLite(lua_State *L, int index)
{
	LSQLite *LS;
	luaL_checktype(L, index, LUA_TUSERDATA);
	LS = (LSQLite *)luaL_checkudata(L, index, PROJECT_TABLENAME);
	if (LS == NULL) luaL_typerror(L, index, PROJECT_TABLENAME);
	return LS;
}

static LSQLite *pushLSQLite(lua_State *L)
{
	LSQLite *LS = (LSQLite *)lua_newuserdata(L, sizeof(LSQLite));
	luaL_getmetatable(L, PROJECT_TABLENAME);
	lua_setmetatable(L, -2);
	memset(LS, 0, sizeof(LSQLite));
	return LS;
}

static int ls__gc(lua_State *L){
	
	LSQLite * ls = toLSQLite(L, 1);

	if (ls->db){
		delete[]ls->db;
		ls->db = NULL;
	}

	if (ls->dbptr)
		sqlite3_close_v2(ls->dbptr);

	ls->dbptr = NULL;

	return 0;
}

static int ls__tostring(lua_State *L){

	lua_pushstring(L, toLSQLite(L, 1)->db);
	return 1;
}

static int lua_sqliteopen(lua_State *L) {
	
	const char * db = luaL_checkstring(L, 1);
	lua_pop(L,1);

	sqlite3	* dbptr;

	if (sqlite3_open(db, &dbptr) != SQLITE_OK){

		lua_pushnil(L);
		return 1;
	}

	LSQLite * ls = pushLSQLite(L);

	ls->dbptr = dbptr;
	ls->db = new char[strlen(db) + 1];

	strcpy(ls->db,db);

	return 1;
}

static int lua_sqliteclose(lua_State *L) {

	EnterCriticalSection(&SQLSECT);

	LSQLite * ls = checkLSQLite(L,1);
	lua_pop(L, 1);

	if (ls->db){
		delete[]ls->db;
		ls->db = NULL;
	}

	if (ls->dbptr)
		lua_pushinteger(L, sqlite3_close_v2(ls->dbptr) );
	else
		lua_pushinteger(L, SQLITE_OK);

	ls->dbptr = NULL;

	LeaveCriticalSection(&SQLSECT);
	return 1;
}

static int lua_sqliteexecute(lua_State *L) {

	LSQLite * ls = checkLSQLite(L, 1);
	const char * str = luaL_checkstring(L, 2);
	int nShouldWait = lua_toboolean(L, 3);
	lua_pop(L, 3);

	if (ls->dbptr == NULL){

		lua_pushinteger(L, -1);
		return 1;
	}

	if (ls->stmt){
		sqlite3_finalize(ls->stmt);
		ls->stmt = NULL;
	}

	lua_pushinteger(L, ExecuteSQL(ls->dbptr, str, NULL, false, nShouldWait > 0));
	
	return 1;
}


static int lua_sqlitequerry(lua_State *L) {

	LSQLite * ls = checkLSQLite(L, 1);
	const char * str = luaL_checkstring(L, 2);
	lua_pop(L, 2);

	if (ls->dbptr == NULL){

		lua_pushinteger(L, -1);
		return 1;
	}

	if (ls->stmt){
		sqlite3_finalize(ls->stmt);
		ls->stmt = NULL;
	}

	lua_pushinteger(L, ExecuteSQL(ls->dbptr, str, ls->stmt, true, true));
	
	return 1;
}

static int lua_sqlitefetch(lua_State *L) {

	LSQLite * ls = checkLSQLite(L, 1);
	lua_pop(L, 1);

	if (ls->dbptr == NULL){

		lua_pushinteger(L, -1);
		return 1;
	}

	lua_pushinteger(L, sqlite3_step(ls->stmt));

	return 1;
}

static int lua_sqliteget(lua_State *L) {

	LSQLite * ls = checkLSQLite(L, 1);
	lua_pop(L, 1);

	if (ls->dbptr == NULL){

		lua_pushnil(L);
		return 1;
	}
	
	int cnt = sqlite3_column_count(ls->stmt);

	lua_createtable(L, 0, cnt);

	for (int column = 0; column < cnt; column++){

		lua_pushstring(L, sqlite3_column_name(ls->stmt, column));

		switch (sqlite3_column_type(ls->stmt, column)){

			case SQLITE_INTEGER: lua_pushinteger(L, sqlite3_column_int(ls->stmt, column)); break;
			case SQLITE_FLOAT: lua_pushnumber(L, sqlite3_column_double(ls->stmt, column)); break;
			case SQLITE_TEXT: lua_pushstring(L, (const char*)sqlite3_column_text(ls->stmt, column)); break;
			case SQLITE_BLOB: lua_pushlstring(L, (const char*)sqlite3_column_blob(ls->stmt, column), sqlite3_column_bytes(ls->stmt, column)); break;
			default: lua_pushnil(L); break;

		}

		lua_settable(L, -3);
	}

	return 1;
}

char * hlp_escp(const char * str){

	if (!str)
		return NULL;
	size_t len = strlen(str);

	int numb = 0;
	char * escd = new char[(len * 2)+1];
	for (unsigned int n = 0; n < len; n++){

		if (str[n] == '\''){
			escd[n + numb] = '\'';
			escd[n + (++numb)] = '\'';
		}
		else{
			escd[n + numb] = str[n];
		}
	}

	escd[len + numb] = 0;

	return escd;	
}

static int lua_sqliteinserttable(lua_State *L){

	LSQLite * ls = checkLSQLite(L, 1);
	const char * tbl = luaL_checkstring(L, 2);
	const char * trailing = lua_tostring(L, 4);

	if (trailing == NULL)
		trailing = "";
	else
		lua_pop(L, 1);

	char fields[32768];
	char values[65536];
	char complete[98305];

	sprintf(fields, "REPLACE INTO %s (", tbl);
	strcpy(values, "VALUES (");

	luaL_checktype(L, 3, LUA_TTABLE);

	lua_pushnil(L);
	bool isFirst = true;

	size_t f_cursor = strlen(fields);
	size_t v_cursor = strlen(values);
	char * escp=NULL;

	while (lua_next(L, 3) != 0)
	{
		if (lua_isstring(L, -1)){

			f_cursor = strlen(fields);
			v_cursor = strlen(values);

			if (isFirst){

				escp = hlp_escp(lua_tostring(L, -1));

				sprintf(&fields[f_cursor], "%s", lua_tostring(L, -2));
				sprintf(&values[v_cursor], "'%s'", escp);

				isFirst = false;
			}
			else{

				escp = hlp_escp(lua_tostring(L, -1));

				sprintf(&fields[f_cursor], ",%s", lua_tostring(L, -2));
				sprintf(&values[v_cursor], ",'%s'", escp);
			}

			if (escp){
				delete[]escp;
				escp = NULL;
			}

		}
		else if (lua_isnumber(L, -1)){

			f_cursor = strlen(fields);
			v_cursor = strlen(values);

			if (isFirst){

				sprintf(&fields[f_cursor], "%s", lua_tostring(L, -2));
				sprintf(&values[v_cursor], "'%s'", lua_tostring(L, -2));

				isFirst = false;
			}
			else{
				sprintf(&fields[f_cursor], ",%s", lua_tostring(L, -1));
				sprintf(&values[v_cursor], ",%f", lua_tonumber(L, -1));
			}
		}

		lua_pop(L, 1);
	}

	strcat(fields,")");
	strcat(values, ")");

	sprintf(complete, "%s %s %s", fields, values, trailing);

	/*FILE * out = fopen("test.txt", "w");
	fputs(complete,out);
	fclose(out);*/

	lua_pop(L, 3);

	ExecuteSQL(ls->dbptr, complete, NULL, false, false);
	lua_pushstring(L,"");

	return 1;
}

static int lua_sqliteescape(lua_State *L) {

	luaL_checktype(L, -1, LUA_TSTRING);
	size_t len;
	const char * str = lua_tolstring(L, -1, &len);
	lua_pop(L, 1);

	int numb = 0;
	char * escd = new char[(len * 2)];
	for (unsigned int n = 0; n < len; n++){

		if (str[n] == '\''){
			escd[n + numb] = '\'';
			escd[n + (++numb)] = '\'';
		}
		else{
			escd[n + numb] = str[n];
		}
	}

	lua_pushlstring(L, escd,len+numb);
	delete[]escd;
	return 1;
}

static int lua_logsql(lua_State *L) {

	int n = lua_tointeger(L,1);
	lua_pop(L, 1);

	if (n == 0 && Log){
		fclose(Log);
		Log = NULL;
	}
	else{
		Log = fopen("SQLITELOG.txt", "a+");
	}
	return 0;
}

static int lua_waitsql(lua_State *L) {

	while (WTActive){

		EnterCriticalSection(&SQLSECT);
		LeaveCriticalSection(&SQLSECT);
	}
	
	return 0;
}

static int lua_sqliteerror(lua_State *L) {

	LSQLite * ls = checkLSQLite(L, 1);
	lua_pop(L, 1);
	lua_pushstring(L, sqlite3_errmsg(ls->dbptr));
	return 1;
}
int LUA_API luaopen_luasqlite(lua_State *L) {
	
	struct luaL_reg driver[] = {
		{ "Open", lua_sqliteopen },
		{ "Close", lua_sqliteclose },
		{ "Execute", lua_sqliteexecute },
		{ "Querry", lua_sqlitequerry },
		{ "Fetch", lua_sqlitefetch },
		{ "Get", lua_sqliteget },
		{ "Error", lua_sqliteerror },
		{ "Escape", lua_sqliteescape },
		{ "Insert", lua_sqliteinserttable },
		{ "Log", lua_logsql },
		{ "Wait", lua_waitsql },
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

	InitializeCriticalSection(&SQLSECT);
	WorkThread = CreateThread(NULL, NULL, &SQLThreadSpin, NULL, CREATE_SUSPENDED,NULL);
	WTActive = false;
	WTAlive = true;
	Log = NULL;
	return 1;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved){

	if (fdwReason == DLL_PROCESS_DETACH){

		int n = 0;
		while (WTActive){
			Sleep(1000);
			if (n++ >= 10)
				break;
		}
		CloseHandle(WorkThread);
		DeleteCriticalSection(&SQLSECT);
		if (Log)
			fclose(Log);
	}

	return true;
}