//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_TDM_H
#define TF_HUD_TDM_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "of_imageprogressbar.h"
#include "tf_controls.h"

#define TF_MAX_FILENAME_LENGTH	128

//-----------------------------------------------------------------------------
// Purpose:  Displays weapon ammo data
//-----------------------------------------------------------------------------
class CTFHudTDM : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudTDM, vgui::EditablePanel );

public:

	CTFHudTDM( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	virtual bool ShouldDraw( void );

protected:

	virtual void OnThink();

private:
	
	void UpdateKillLabel( bool bKills );

private:

	float							m_flNextThink;

	CHandle<C_BaseCombatWeapon>		m_hCurrentActiveWeapon;
	int								m_nKills;

	CTFImageProgressBar				*m_pProgressRed;
	CTFImageProgressBar				*m_pProgressBlu;
	CExLabel						*m_pKills;
	CExLabel						*m_pKillsShadow;
};



#endif	// TF_HUD_TDM_H