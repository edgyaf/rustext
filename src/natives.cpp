/*
	About: rustext natives
	Author: ziggi
*/

#include <sdk/plugin.h>
#include <sdk/amx/amx2.h>
#include <string>
#include <vector>
#include <utility>
#include "natives.hpp"
#include "common.hpp"
#include "sampfunctions.hpp"
#include "converter.hpp"
#include "russifier.hpp"

extern logprintf_t logprintf;

// Calls the original native with the text argument converted for the given russifier type.
//
// The conversion maps one character to one character, so it is done directly in the script's own string and undone
// right after the call. Nothing is allocated on the AMX heap: the previous version copied the text there, in the first
// loaded script instead of the calling one, and crashed the server when that allocation failed.
static cell CallWithConvertedText(AMX *amx, cell *params, int index, bool convert, Converter::Types type, amx_Function_t original)
{
	if (original == NULL) {
		return 0;
	}

	cell *text = NULL;
	if (amx_GetAddr(amx, params[index], &text) != AMX_ERR_NONE || text == NULL) {
		return original(amx, params);
	}

	// an empty text was never passed on
	if (*text == 0) {
		return 0;
	}

	// packed strings are passed through untouched
	if (!convert || static_cast<ucell>(*text) > UNPACKEDMAX) {
		return original(amx, params);
	}

	std::vector<std::pair<cell *, cell> > changed;

	for (cell *character = text; *character != 0; character++) {
		if ((*character & ~0xFF) != 0) {
			continue;
		}

		uint8_t code = Converter::GetCode(static_cast<uint8_t>(*character), type);
		if (code != 0 && code != *character) {
			changed.push_back(std::make_pair(character, *character));
			*character = code;
		}
	}

	cell result = original(amx, params);

	for (size_t i = 0; i < changed.size(); i++) {
		*changed[i].first = changed[i].second;
	}

	return result;
}

// native GetRussifierVersion(version[], const size = sizeof(version));
cell AMX_NATIVE_CALL Natives::GetRussifierVersion(AMX *amx, cell *params)
{
	CHECK_PARAMS(2, "GetRussifierVersion");

	amx_SetCString(amx, params[1], PLUGIN_VERSION, params[2]);
	return 1;
}

// native GetRussifierText(RussifierType:type, string[], string_return[], const size = sizeof(string_return));
cell AMX_NATIVE_CALL Natives::GetRussifierText(AMX *amx, cell *params)
{
	CHECK_PARAMS(4, "GetRussifierText");

	char *string;
	cell *dest_addr;

	int type = static_cast<int>(params[1]);
	uint32_t length = amx_GetCString(amx, params[2], string);
	amx_GetAddr(amx, params[3], &dest_addr);
	int size = static_cast<int>(params[4]);

	if (type < 0 || type >= Converter::TypesCount) {
		return 0;
	}

	Converter::Process(string, length, static_cast<Converter::Types>(type));

	amx_SetString(dest_addr, string, 0, 0, size);
	return 1;
}

// native SetPlayerRussifierType(playerid, RussifierType:type);
cell AMX_NATIVE_CALL Natives::SetPlayerRussifierType(AMX *amx, cell *params)
{
	CHECK_PARAMS(2, "SetPlayerRussifierType");

	int playerid = static_cast<int>(params[1]);
	int type = static_cast<int>(params[2]);

	if (playerid < 0 || playerid >= MAX_PLAYERS) {
		return 0;
	}

	if (type == -1) {
		Russifier::DisablePlayer(playerid);
		return 1;
	}

	if (type < 0 || type >= Converter::TypesCount) {
		return 0;
	}

	Russifier::SetPlayerType(playerid, type);
	return 1;
}

// native RussifierType:GetPlayerRussifierType(playerid);
cell AMX_NATIVE_CALL Natives::GetPlayerRussifierType(AMX *amx, cell *params)
{
	CHECK_PARAMS(1, "GetPlayerRussifierType");

	int playerid = static_cast<int>(params[1]);

	if (playerid < 0 || playerid >= MAX_PLAYERS) {
		return 0;
	}

	return Russifier::GetPlayerType(playerid);
}

// native SetDefaultRussifierType(RussifierType:type);
cell AMX_NATIVE_CALL Natives::SetDefaultRussifierType(AMX *amx, cell *params)
{
	CHECK_PARAMS(1, "SetDefaultRussifierType");

	int type = static_cast<int>(params[1]);

	if (type == -1) {
		Russifier::DisableDefault();
		return 1;
	}

	if (type < 0 || type >= Converter::TypesCount) {
		return 0;
	}

	Russifier::SetDefaultType(type);
	return 1;
}

// native RussifierType:GetDefaultRussifierType();
cell AMX_NATIVE_CALL Natives::GetDefaultRussifierType(AMX *amx, cell *params)
{
	CHECK_PARAMS(0, "GetDefaultRussifierType");

	return Russifier::GetDefaultType();
}

// native GameTextForAll(const string[], time, style);
cell AMX_NATIVE_CALL Natives::GameTextForAll(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "GameTextForAll");

	return CallWithConvertedText(amx, params, 1, Russifier::IsDefaultEnabled(), Russifier::GetDefaultType(), Samp::addr_GameTextForAll);
}

// native GameTextForPlayer(playerid, const string[], time, style);
cell AMX_NATIVE_CALL Natives::GameTextForPlayer(AMX *amx, cell *params)
{
	CHECK_PARAMS(4, "GameTextForPlayer");

	int playerid = static_cast<int>(params[1]);

	return CallWithConvertedText(amx, params, 2, Russifier::IsPlayerEnabled(playerid) || Russifier::IsDefaultEnabled(),
		Russifier::GetPlayerType(playerid), Samp::addr_GameTextForPlayer);
}

// TextDrawCreate(Float:x, Float:y, text[]);
cell AMX_NATIVE_CALL Natives::TextDrawCreate(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "TextDrawCreate");

	return CallWithConvertedText(amx, params, 3, Russifier::IsDefaultEnabled(), Russifier::GetDefaultType(), Samp::addr_TextDrawCreate);
}

// TextDrawSetString(Text:text, string[]);
cell AMX_NATIVE_CALL Natives::TextDrawSetString(AMX *amx, cell *params)
{
	CHECK_PARAMS(2, "TextDrawSetString");

	return CallWithConvertedText(amx, params, 2, Russifier::IsDefaultEnabled(), Russifier::GetDefaultType(), Samp::addr_TextDrawSetString);
}

// CreatePlayerTextDraw(playerid, Float:x, Float:y, text[]);
cell AMX_NATIVE_CALL Natives::CreatePlayerTextDraw(AMX *amx, cell *params)
{
	CHECK_PARAMS(4, "CreatePlayerTextDraw");

	int playerid = static_cast<int>(params[1]);

	return CallWithConvertedText(amx, params, 4, Russifier::IsPlayerEnabled(playerid) || Russifier::IsDefaultEnabled(),
		Russifier::GetPlayerType(playerid), Samp::addr_CreatePlayerTextDraw);
}

// PlayerTextDrawSetString(playerid, PlayerText:text, string[]);
cell AMX_NATIVE_CALL Natives::PlayerTextDrawSetString(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "PlayerTextDrawSetString");

	int playerid = static_cast<int>(params[1]);

	return CallWithConvertedText(amx, params, 3, Russifier::IsPlayerEnabled(playerid) || Russifier::IsDefaultEnabled(),
		Russifier::GetPlayerType(playerid), Samp::addr_PlayerTextDrawSetString);
}

// CreateMenu(title[], columns, Float:x, Float:y, Float:col1width, Float:col2width);
cell AMX_NATIVE_CALL Natives::CreateMenu(AMX *amx, cell *params)
{
	CHECK_PARAMS(6, "CreateMenu");

	return CallWithConvertedText(amx, params, 1, Russifier::IsDefaultEnabled(), Russifier::GetDefaultType(), Samp::addr_CreateMenu);
}

// AddMenuItem(Menu:menuid, column, title[]);
cell AMX_NATIVE_CALL Natives::AddMenuItem(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "AddMenuItem");

	return CallWithConvertedText(amx, params, 3, Russifier::IsDefaultEnabled(), Russifier::GetDefaultType(), Samp::addr_AddMenuItem);
}

// SetMenuColumnHeader(menuid, column, text[]);
cell AMX_NATIVE_CALL Natives::SetMenuColumnHeader(AMX *amx, cell *params)
{
	CHECK_PARAMS(3, "SetMenuColumnHeader");

	return CallWithConvertedText(amx, params, 3, Russifier::IsDefaultEnabled(), Russifier::GetDefaultType(), Samp::addr_SetMenuColumnHeader);
}
