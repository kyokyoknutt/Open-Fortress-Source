//========= Copyright � 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef _IBASEPANEL_H__
#define _IBASEPANEL_H__

#include "vgui_controls/Panel.h"
#include "vgui_controls/Frame.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/PHandle.h"
#include "vgui_controls/MenuItem.h"
#include "vgui_controls/MessageDialog.h"
#include "tier1/utllinkedlist.h"

#include <GameUI/gameui_shared.h>


//=============================================================================
//
//=============================================================================
class IBasePanel
{
public:
	virtual ~IBasePanel() {}

	virtual vgui::Panel& GetVguiPanel() = 0;

public:
	// notifications
	virtual void OnLevelLoadingStarted( char const *levelName, bool bShowProgressDialog ) = 0;
	virtual void OnLevelLoadingFinished( KeyValues *kvEvent ) = 0;
	virtual bool UpdateProgressBar(float progress, const char *statusText) = 0;

	// update the taskbar a frame
	virtual void RunFrame() = 0;

	// handles gameUI being shown
	virtual void OnGameUIActivated() = 0;
	virtual void OnGameUIHidden() = 0;
};

#endif
