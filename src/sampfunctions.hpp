/*
	About: rustext sampfunctions
	Author: ziggi
*/

#ifndef SAMPFUNCTIONS_H
#define SAMPFUNCTIONS_H

#include "common.hpp"

class Samp {
public:
	static amx_Function_t addr_GameTextForAll;
	static amx_Function_t addr_GameTextForPlayer;
	static amx_Function_t addr_TextDrawCreate;
	static amx_Function_t addr_TextDrawSetString;
	static amx_Function_t addr_CreatePlayerTextDraw;
	static amx_Function_t addr_PlayerTextDrawSetString;
	static amx_Function_t addr_CreateMenu;
	static amx_Function_t addr_AddMenuItem;
	static amx_Function_t addr_SetMenuColumnHeader;
};

#endif
