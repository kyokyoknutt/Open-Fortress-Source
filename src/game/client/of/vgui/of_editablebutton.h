//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_EDITABLE_BTN_H
#define OF_EDITABLE_BTN_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>

enum
{
	BORDER_IDLE = 0,
	BORDER_HOVEROVER,
	BORDER_PRESSED,
	BORDER_SELECTED,
};

class CTFEditableButtonFunc : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFEditableButtonFunc, vgui::EditablePanel );

public:
	CTFEditableButtonFunc(vgui::Panel *parent, const char *panelName);	 

	virtual void OnCursorExited();
	virtual void OnCursorEntered();
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);
};

class CTFEditableButton : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFEditableButton, vgui::EditablePanel );

public:
	CTFEditableButton(vgui::Panel *parent, const char *panelName);	 

	virtual void ApplySettings(KeyValues *inResourceData);

	virtual void OnCursorExited();
	virtual void OnCursorEntered();
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void OnReleasedSelected();
	virtual void OnReleasedUnselected();
	void AddOnPressSound( char *szPressedSound );
	
	void	SetBorderType( int iBorder );
	virtual void SetSelected( bool bSelected );
public:
	char	szBorderIdle[128];
	char	szBorderHover[128];
	char	szBorderPressed[128];
	char	szBorderSelected[128];
	
	int		iCurrentBorder;
	int		m_iSoundChance;
	
	bool	m_bSelected;
	
	CUtlVector<char*> m_hPressedSounds;
	
	CTFEditableButtonFunc	*pButton;
};


#endif // OF_EDITABLE_BTN_H
