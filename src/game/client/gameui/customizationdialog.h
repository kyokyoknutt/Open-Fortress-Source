//
//
// TODO: REMOVE THIS FILE WHEN THE LOADOUT IS ADDED!
//
//

//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CUSTOMIZATIONDIALOG_H
#define CUSTOMIZATIONDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyDialog.h"


//-----------------------------------------------------------------------------
// Purpose: Holds all the game option pages
//-----------------------------------------------------------------------------
class CCustomizationDialog : public vgui::PropertyDialog
{
	DECLARE_CLASS_SIMPLE( CCustomizationDialog, vgui::PropertyDialog );

public:
	CCustomizationDialog(vgui::Panel *parent );
	~CCustomizationDialog();

	void Run();
	virtual void Activate();

	MESSAGE_FUNC( OnGameUIHidden, "GameUIHidden" );	// called when the GameUI is hidden
};


#define OPTIONS_MAX_NUM_ITEMS 15

struct OptionData_t;

#endif // OPTIONSDIALOG_H
