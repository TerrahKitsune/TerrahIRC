#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include "sqlite3.h"
#include <conio.h>
#include "Lua.hpp"
#include <Windows.h>
#pragma comment(lib, "lua51.lib")

#define PROJECT_TABLENAME "luasqlite"
#ifdef WIN32
#define LUA_API __declspec(dllexport)
#else
#define LUA_API
#endif