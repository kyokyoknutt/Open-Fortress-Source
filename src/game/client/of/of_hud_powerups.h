//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_POWERUPS_H
#define TF_HUD_POWERUPS_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_avatarimage.h"
#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include "vgui_controls/CircularProgressBar.h"

using namespace vgui;

#define TF_MAX_FILENAME_LENGTH	128

//-----------------------------------------------------------------------------
// Purpose:  Displays weapon ammo data
//-----------------------------------------------------------------------------
class CTFHudPowerups : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudPowerups, EditablePanel );

public:

	CTFHudPowerups( const char *pElementName );

	virtual void ApplySchemeSettings( IScheme *pScheme );

	virtual bool ShouldDraw( void );
	
	virtual void FireGameEvent( IGameEvent * event );

	struct powerup_timer_t
	{
		int m_iCondID;
		float m_flStartTime;
		CircularProgressBar *m_pBar;
	};	
	CUtlVector<powerup_timer_t>	m_iConditions;
	
protected:

	virtual void OnThink();
	SectionedListPanel	*m_pPowerupList;
};

extern KeyValues *kvPowerupTimer;

#endif	// TF_HUD_POWERUPS_H