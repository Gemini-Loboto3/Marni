#define _CRT_SECURE_NO_WARNINGS
#define D_SCL_SECURE_NO_WARNINGS

#include <stdafx.h>
#include "ini.h"
#include "Config.h"
#include <string>
#include <vector>
#include "dxgl\common.h"

//extern "C" DXGLCFG dxglcfg;

#define ASSIGN_U32(lpName,str,lpData,var)	if (strcmp(lpName, str) == 0) { memcpy(lpData, &var, sizeof(var)); return; }
#define ASSIGN_STR(lpName,str,lpData,var)	if (strcmp(lpName, str) == 0) { strcpy((char*)lpData, var.c_str()); return; }
#define ASSIGN_U8P(lpName,str,lpData,var)	ASSIGN_U32(lpName,str,lpData,var)

#define STORE_U32(lpName,str,lpData,var)	if (strcmp(lpName, str) == 0) { memcpy(&var, lpData, sizeof(var)); return; }
#define STORE_STR(lpName,str,lpData,var)	if (strcmp(lpName, str) == 0) var = (char*)lpData;
#define STORE_U8P(lpName,str,lpData,var)	STORE_U32(lpName,str,lpData,var)

CConfig::CConfig()
{
}

CConfig::~CConfig()
{
	FILE *ini = fopen("conf.ini", "wt+");

	fprintf(ini, "[RE1]\n"
		"Play_Number = %d\n"
		"Clear_Number = %d\n"
		"X_Size = %d\n"
		"Y_Size = %d\n"
		"Bit_Depth = %d\n"
		"FullScreen? = %d\n"
		"Display_Driver = %d\n"
		"Display_Mode = %d\n"
		//"Create_Directory = %s\n"
		//"Install_Path = %s\n"
		"Install_Flag = %d\n",
		play_cnt,
		clear_cnt,
		res_w,
		res_h,
		depth,
		fullscreen,
		vid_driver,
		vid_mode,
		//folder.c_str(),
		//path.c_str(),
		install_flg);
	fprintf(ini, "Key_Def =");
	for (size_t i = 0; i < sizeof(key_def); i++)
		fprintf(ini, " %d", key_def[i]);
	fprintf(ini, "\nJoy_Def =");
	for (size_t i = 0; i < sizeof(joy_def); i++)
		fprintf(ini, " %d", joy_def[i]);
	fprintf(ini, "\n\n");
	
	fprintf(ini, "[DXGL]\n"
		"Filter = %d\n"
		"Timer_mode = %d\n"
		"DPI_mode = %d\n",
		filter,
		timer_mode,
		dpi_mode);

	fclose(ini);
}

void CConfig::Init()
{
	ini_t *ini = ini_load("conf.ini");

	// [RE1]
	IniGetU32(ini, "RE1", "Bit_Depth", depth);
	IniGetU32(ini, "RE1", "Clear_Number", clear_cnt);
	IniGetU32(ini, "RE1", "Display_Driver", vid_driver);
	IniGetU32(ini, "RE1", "Display_Mode", vid_mode);
	IniGetU32(ini, "RE1", "FullScreen?", fullscreen);
	IniGetU32(ini, "RE1", "Play_Number", play_cnt);
	IniGetU32(ini, "RE1", "X_Size", res_w);
	IniGetU32(ini, "RE1", "Y_Size", res_h);
	IniGetU32(ini, "RE1", "Install_Flag", install_flg);
	IniGetU8P(ini, "RE1", "Joy_Def", joy_def);
	IniGetU8P(ini, "RE1", "Key_Def", key_def);
	// [DXGL]
	IniGetU32(ini, "DXGL", "Filter", filter);
	IniGetU32(ini, "DXGL", "Timer_mode", timer_mode);
	IniGetU32(ini, "DXGL", "DPI_mode", dpi_mode);

	ini_free(ini);
}

void CConfig::GetKey(char *lpName, void *lpData)
{
	ASSIGN_U32(lpName, "Play_Number",		lpData, play_cnt);
	ASSIGN_U32(lpName, "Clear_Number",		lpData, clear_cnt);
	ASSIGN_U32(lpName, "X_Size",			lpData, res_w);
	ASSIGN_U32(lpName, "Y_Size",			lpData, res_h);
	ASSIGN_U32(lpName, "Bit_Depth",			lpData, depth);
	ASSIGN_U32(lpName, "Fullscreen?",		lpData, fullscreen);
	ASSIGN_U32(lpName, "Display_Driver",	lpData, vid_driver);
	ASSIGN_U32(lpName, "Display_Mode",		lpData, vid_mode);
	ASSIGN_U32(lpName, "Install_Flag",		lpData, install_flg);
	//ASSIGN_STR(lpName, "Install_Path",		lpData, path);
	//ASSIGN_STR(lpName, "Create_Directory",	lpData, folder);
	ASSIGN_U8P(lpName, "Key_Def",			lpData, key_def);
	ASSIGN_U8P(lpName, "Joy_Def",			lpData, joy_def);
}

void CConfig::SetKey(char *lpName, void *lpData)
{
	STORE_U32(lpName, "Play_Number",		lpData, play_cnt);
	STORE_U32(lpName, "Clear_Number",		lpData, clear_cnt);
	STORE_U32(lpName, "X_Size",				lpData, res_w);
	STORE_U32(lpName, "Y_Size",				lpData, res_h);
	STORE_U32(lpName, "Bit_Depth",			lpData, depth);
	STORE_U32(lpName, "Fullscreen?",		lpData, fullscreen);
	STORE_U32(lpName, "Display_Driver",		lpData, vid_driver);
	STORE_U32(lpName, "Display_Mode",		lpData, vid_mode);
	STORE_U32(lpName, "Install_Flag",		lpData, install_flg);
	STORE_STR(lpName, "Install_Path",		lpData, path);
	STORE_STR(lpName, "Create_Directory",	lpData, folder);
	STORE_U8P(lpName, "Key_Def",			lpData, key_def);
	STORE_U8P(lpName, "Joy_Def",			lpData, joy_def);
}

void CConfig::IniGetU32(ini_t *ini, char* key, char *name, u32 &dst)
{
	auto got = ini_get(ini, key, name);

	if (!got) dst = 0;
	else dst = atoi(got);
}

void CConfig::IniGetStr(ini_t *ini, char* key, char *name, std::string &dst)
{
	auto got = ini_get(ini, key, name);
	if (!got) dst.clear();
	else dst = got;
}

void CConfig::IniGetU8P(ini_t *ini, char* key, char *name, u8 *dst)
{
	std::string got = ini_get(ini, key, name);
	char *tok = strtok((char*)got.c_str(), " ");

	while (tok)
	{
		*dst++ = atoi(tok);
		tok = strtok(NULL, " ");
	}
}

// reverse string tokenizer
char *rstrtok(char *s, char delim)
{
	static char *sstart,
		*scur;
	// entering here for the first time
	if (s)
	{
		sstart = s;
		size_t len = strlen(s);
		// replace delimi with EOS
		for (size_t i = 0; i < len; i++)
			if (s[i] == delim)
				s[i] = '\0';
		scur = &s[len - 1];
	}
	else scur--;

	// grab the beginning of a tokenized string
	while (*scur != '\0')
	{
		// no more tokens left to process
		if (scur == sstart)
			return scur;
		if (scur < sstart)
			return NULL;
		scur--;
	}

	return scur + 1;
}

void CConfig::ReconstructPath(char *pszStr)
{
#if 0
	size_t len = strlen(pszStr);
	char *str[2];
	char *str_e = &pszStr[len - 1];


	// find the first two backslashes
	size_t i = len - 1;
	for (int s = 0; s < 2; s++)
	{
		for (; i >= 0; i--)
		{
			if (pszStr[i] == '\\')
			{
				str[s] = &pszStr[i--];
				break;
			}
		}

		// this case is not really possible
		if (i == 0) str[1] = pszStr;
	}

	std::string sz = pszStr;

	path=sz.substr(0, str[1] - pszStr + 1);
	folder=sz.substr(str[1] - pszStr + 1, str[0] - str[1] - 1);
#else
	char *sdup = _strdup(pszStr);
	char *t = rstrtok(sdup, '\\');
	path.clear();
	for (int i = 0; t; i++)
	{
		switch (i)
		{
		case 0:
			break;
		case 1:		// folder name
			folder = t;
			break;
		default:	// path
			path = std::string(t)+ "\\" + path;
		}

		t = rstrtok(NULL, '\\');
	}
	free(sdup);
#endif
}
