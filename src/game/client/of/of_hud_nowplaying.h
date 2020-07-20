#ifndef OF_NOW_PLAYING_H
#define OF_NOW_PLAYING_H
#ifdef _WIN32
#pragma once
#endif

// HUD
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "tf_controls.h"

class CTFImagePanel;
class CExLabel;

class CTFHudNowPlaying : public vgui::EditablePanel, public CHudElement
{
private:
	DECLARE_CLASS_SIMPLE(CTFHudNowPlaying, EditablePanel);

public:
	CTFHudNowPlaying(const char *pElementName);

	virtual void FireGameEvent(IGameEvent * event);
	virtual void OnThink();
	virtual bool ShouldDraw(void);
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual int GetRenderGroupPriority() { return 70; }

private:
	float flDrawTime;
	
	EditablePanel 	*m_pNameContainer;
	CTFImagePanel 	*m_pNameBG;
	CExLabel 		*m_pNameLabel;
	
	EditablePanel 	*m_pArtistContainer;
	CTFImagePanel 	*m_pArtistBG;
	CExLabel 		*m_pArtistLabel;
};

#endif //OF_NOW_PLAYING_H