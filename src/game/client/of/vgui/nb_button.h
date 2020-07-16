#ifndef _INCLUDED_NB_BUTTON_H
#define _INCLUDED_NB_BUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Button.h>

// == MANAGED_CLASS_DECLARATIONS_START: Do not edit by hand ==
// == MANAGED_CLASS_DECLARATIONS_END ==

class CNB_Button : public vgui::Button
{
	DECLARE_CLASS_SIMPLE( CNB_Button, vgui::Button );
public:
	CNB_Button(Panel *parent, const char *panelName, const char *text, Panel *pActionSignalTarget=NULL, const char *pCmd=NULL);
	CNB_Button(Panel *parent, const char *panelName, const wchar_t *text, Panel *pActionSignalTarget=NULL, const char *pCmd=NULL);
	virtual ~CNB_Button();
	
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnThink();
	virtual void OnCommand( const char *command );
	virtual void Paint();
	virtual void PaintBackground();
	virtual void OnCursorEntered();
};

#endif // _INCLUDED_NB_BUTTON_H
