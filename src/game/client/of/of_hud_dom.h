//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_DOM_H
#define TF_HUD_DOM_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_controls.h"

#define TF_MAX_FILENAME_LENGTH	128

//-----------------------------------------------------------------------------
// Purpose:  Clips the frag image to the appropriate percentage
//-----------------------------------------------------------------------------
class CTFScoreProgressBlu : public CTFImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFScoreProgressBlu, CTFImagePanel );

	CTFScoreProgressBlu( vgui::Panel *parent, const char *name );
	virtual void Paint();
	void SetProgress( float flProgress ){ m_flProgress = ( flProgress <= 1.0 ) ? flProgress : 1.0f; }

	float	m_flProgress; // percentage from 0.0 -> 1.0
	int		m_iMaterialIndex;
	int		m_iDeadMaterialIndex;
};

//-----------------------------------------------------------------------------
// Purpose:  Clips the frag meter to the appropriate percentage
//-----------------------------------------------------------------------------
class CTFScoreProgressRed : public CTFImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFScoreProgressRed, CTFImagePanel );

	CTFScoreProgressRed( vgui::Panel *parent, const char *name );
	virtual void Paint();
	void SetProgress(float flProgress){ m_flProgress = (flProgress <= 1.0) ? flProgress : 1.0f; }

	float	m_flProgress; // percentage from 0.0 -> 1.0
	int		m_iMaterialIndex;
	int		m_iDeadMaterialIndex;
};

//-----------------------------------------------------------------------------
// Purpose:  Displays weapon ammo data
//-----------------------------------------------------------------------------
class CTFHudDOM : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudDOM, vgui::EditablePanel );

public:

	CTFHudDOM( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	virtual bool ShouldDraw( void );

protected:

	virtual void OnThink();

private:
	
	void UpdateDOMLabel( bool bScore );

private:

	float							m_flNextThink;

	int								m_nScore;

	CExLabel						*m_pScore;
	CExLabel						*m_pScoreShadow;
	CTFScoreProgressRed 			*m_pRedScore;
	CTFScoreProgressBlu				*m_pBluScore;
};



#endif	// TF_HUD_DOM_H