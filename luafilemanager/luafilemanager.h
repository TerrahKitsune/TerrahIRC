#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <conio.h>
#include "Lua.hpp"
#include <Windows.h>
#include "FileManager.h"
#pragma comment(lib, "lua51.lib")

#define PROJECT_TABLENAME "luafm"
#ifdef WIN32
#define LUA_API __declspec(dllexport)
#else
#define LUA_API
#endif