//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_SELECTIONPANEL_H
#define OF_SELECTIONPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "of_editablebutton.h"
#include "tf_imagepanel.h"

class CTFSelectionPanel;

class CTFSelectionManager : public vgui::Panel
{
private:
	DECLARE_CLASS_SIMPLE(CTFSelectionManager, vgui::Panel);

public:
	CTFSelectionManager(vgui::Panel *parent, const char *panelName);

	virtual void ApplySettings(KeyValues *inResourceData);
	MESSAGE_FUNC_PTR(OnPanelSelected, "OnPanelSelected", panel);
public:
	vgui::Panel *pSelectedItem;
	bool bHasSelectedItem;

	CUtlVector<CTFSelectionPanel*> m_hPanels;
};


class CTFSelectionPanel : public CTFEditableButton
{
private:
	DECLARE_CLASS_SIMPLE( CTFSelectionPanel, CTFEditableButton );

public:
	CTFSelectionPanel(vgui::Panel *parent, const char *panelName);	 

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void OnReleasedSelected();
	virtual void OnReleasedUnselected();
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void SetSelected( bool bSelected );
	void	SetConnectedPanel( const char *szConnectedPanel );
public:
	vgui::Panel		*pConnectedPanel;
	CTFSelectionManager *pManger;

	int iBaseX;
	int iBaseY;
	
	int iXAdj;
	int iYAdj;
	vgui::HFont 	m_hTextFont;
	CTFImagePanel	*pImage;
};

#endif // OF_LOADOUT_H
