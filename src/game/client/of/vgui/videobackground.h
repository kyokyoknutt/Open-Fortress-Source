//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_VIDEOBACKGROUND_H
#define OF_VIDEOBACKGROUND_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui/tf_vgui_video.h"
#include <vgui_controls/EditablePanel.h>

namespace vgui {

class CVideoBackground : public Panel
{
	DECLARE_CLASS_SIMPLE( CVideoBackground, Panel );

public:
	CVideoBackground(Panel *parent, const char *panelName);

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	MESSAGE_FUNC( VideoReplay, "IntroFinished" );

private:
	void GetRandomVideo( char *pszBuf, int iBufLength, bool bWidescreen );

private:
	CTFVideoPanel		*m_pVideo;
	char				m_szVideoFile[ MAX_PATH ];
};
}

#endif // OF_CVideoBackground_H
