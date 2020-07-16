//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains Shared Utilities that multiple files need to use
//
//=====================================================================================//

#include "cbase.h"
#include "UI_Shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

const char *GetRandomQuitString()
{
	const char *string;

	int randomizer = RandomInt(0, 22);

	switch (randomizer)
	{
	default:  string = "#QuitMessage0"; break;
	case 0: string = "#QuitMessage0"; break;
	case 1: string = "#QuitMessage1"; break;
	case 2: string = "#QuitMessage2"; break;
	case 3: string = "#QuitMessage3"; break;
	case 4: string = "#QuitMessage4"; break;
	case 5: string = "#QuitMessage5"; break;
	case 6: string = "#QuitMessage6"; break;
	case 7: string = "#QuitMessage7"; break;
	case 8: string = "#QuitMessage8"; break;
	case 9: string = "#QuitMessage9"; break;
	case 10: string = "#QuitMessage10"; break;
	case 11: string = "#QuitMessage11"; break;
	case 12: string = "#QuitMessage12"; break;
	case 13: string = "#QuitMessage13"; break;
	case 14: string = "#QuitMessage14"; break;
	case 15: string = "#QuitMessage15"; break;
	case 16: string = "#QuitMessage16"; break;
	case 17: string = "#QuitMessage17"; break;
	case 18: string = "#QuitMessage18"; break;
	case 19: string = "#QuitMessage19"; break;
	case 20: string = "#QuitMessage20"; break;
	case 21: string = "#QuitMessage21"; break;
	case 22: string = "#QuitMessage22"; break;
	}

	return string;
}