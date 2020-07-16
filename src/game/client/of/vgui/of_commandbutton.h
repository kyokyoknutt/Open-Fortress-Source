//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_COMMANDBUTTON_H
#define OF_COMMANDBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include "of_editablebutton.h"

class CTFCommandButton : public CTFEditableButton
{
private:
	DECLARE_CLASS_SIMPLE( CTFCommandButton, CTFEditableButton );

public:
	CTFCommandButton(vgui::Panel *parent, const char *panelName);	 

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void OnReleasedSelected();
	virtual void OnReleasedUnselected();
	virtual void PaintBackground();
public:
	char	szCommand[128];
	char	szUnselectCommand[128];
	char	szConvref[128];
	char	szTargetVal[128];
};

#endif // OF_LOADOUT_H
