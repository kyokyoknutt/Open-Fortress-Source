//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_IMAGEPROGRESSBAR_H
#define TF_IMAGEPROGRESSBAR_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>

class CTFImageProgressBar : public CHudElement, public vgui::EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFImageProgressBar, vgui::EditablePanel );

	CTFImageProgressBar(vgui::Panel *parent, const char *name);

	virtual void ApplySettings( KeyValues *inResourceData );
	void		 SetProgress( float flProgress )
							{ m_flProggress = flProgress; }
	void		 Update( void );
	vgui::ImagePanel *m_pBarImage;
	
	int	m_iOriginalWidth;
	int	m_iOriginalHeight;
	int m_iOriginalXPos;
	int m_iOriginalYPos;
	
	int m_iOriginalImgXPos;
	int m_iOriginalImgYPos;
	
	int m_iAlignment;
	
	float m_flProggress;
};


#endif // TF_IMAGEPROGRESSBAR_H
