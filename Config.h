#pragma once
#include <string>
#include "ini.h"

class CConfig
{
public:
	CConfig();
	~CConfig();

	void Init();

	void GetKey(char *lpName, void *lpData);
	void SetKey(char *lpName, void *lpData);

	u32 play_cnt,
		clear_cnt,
		res_w,
		res_h,
		depth,
		fullscreen,
		vid_driver,
		vid_mode,
		install_flg;
	std::string path,
		folder;
	u8 key_def[32],
		joy_def[128];

	// dxgl stuff
	u32 filter,
		timer_mode,
		dpi_mode;

	void ReconstructPath(char *pszStr);

private:
	void IniGetU32(ini_t *ini, char* key, char *name, u32 &dst);
	void IniGetStr(ini_t *ini, char* key, char *name, std::string &dst);
	void IniGetU8P(ini_t *ini, char* key, char *name, u8 *dst);
};

