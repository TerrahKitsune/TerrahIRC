/***************************************************************************
    IniFile.cpp - implementation of the CIniFile class.
    Copyright (C) 2005 Jeroen Broekhuizen (jeroen@nwnx.org) and
	Ingmar Stieger (papillon@nwnx.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 ***************************************************************************/
#define _CRT_SECURE_NO_WARNINGS

#include "IniFile.h"
#include <stdlib.h>
#include <stdio.h>
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIniFile::CIniFile(LPCSTR lpszFileName)
{
	// profile functions require complete path
	_fullpath (m_szFileName, lpszFileName, MAX_PATH);
}

CIniFile::~CIniFile()
{
}

void CIniFile::WriteInteger(char* szSection, char* szKey, int iValue)
{
	char szValue[255];
	sprintf(szValue, "%d", iValue);
	WritePrivateProfileString(szSection, szKey, szValue, m_szFileName);
}
void CIniFile::WriteFloat(char* szSection, char* szKey, float fltValue)
{
	char szValue[255];
	sprintf(szValue, "%f", fltValue);
	WritePrivateProfileString(szSection, szKey, szValue, m_szFileName);
}
void CIniFile::WriteBoolean(char* szSection, char* szKey, bool bolValue)
{
	char szValue[255];
	sprintf(szValue, "%s", bolValue ? "True" : "False");
	WritePrivateProfileString(szSection, szKey, szValue, m_szFileName);
}
void CIniFile::WriteString(char* szSection, char* szKey, char* szValue)
{
	WritePrivateProfileString(szSection, szKey, szValue, m_szFileName);
}

int CIniFile::ReadInteger (LPCSTR lpszSection, LPCSTR lpszKey, int iDefault)
{
	// read integer value from ini-file
	return GetPrivateProfileInt (lpszSection, lpszKey, iDefault, m_szFileName);
}

long CIniFile::ReadLong (LPCSTR lpszSection, LPCSTR lpszKey, long lDefault)
{
	// read long value from ini-file
	char buffer[256];
	
	ReadString(lpszSection, lpszKey, buffer, 256, "");
	if (_stricmp(buffer, "") != 0) 
		return atol(buffer);	
	else
		return lDefault;
}

void CIniFile::ReadString (LPCSTR lpszSection, LPCSTR lpszKey, LPSTR lpszBuffer, int Size, LPCSTR lpszDefault)
{
	// read string value
	GetPrivateProfileString (lpszSection, lpszKey, lpszDefault, lpszBuffer, Size, m_szFileName);
}

bool CIniFile::ReadBool(LPCSTR lpszSection, LPCSTR lpszKey, bool iDefault)
{
	char buffer[256];
	ReadString(lpszSection, lpszKey, buffer, 256, "");
	if( buffer[0]=='\0' )return iDefault;
	else if ((_stricmp(buffer, "true") == 0) || 
		(_stricmp(buffer, "yes") == 0) ||
		(_stricmp(buffer, "1") == 0))
	{
		return true;
	}

	return false;
}