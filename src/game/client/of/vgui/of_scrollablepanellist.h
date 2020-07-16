//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_SCRLPNL_H
#define OF_SCRLPNL_H
#ifdef _WIN32
#pragma once
#endif

#include "of_editablebutton.h"

struct PanelListItem_t
{
	vgui::EditablePanel *pPanel;
	int	def_xpos;
	int def_ypos;
};

class CTFScrollablePanelList : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFScrollablePanelList, vgui::EditablePanel );
public:
	CTFScrollablePanelList( vgui::Panel *parent, const char *panelName );
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void PerformLayout();
	
	void AddItem( CTFEditableButton *pPanel );
	void ClearItemList( void );
	
	MESSAGE_FUNC( OnScrollBarSliderMoved, "ScrollBarSliderMoved" );
	MESSAGE_FUNC_PTR( OnSelectionChanged, "SetSelected", panel );
	virtual void OnMouseWheeled(int delta);	// respond to mouse wheel events	

public:
	CUtlVector<PanelListItem_t> m_hItems;
	vgui::ScrollBar *pScrollBar;
	
	CTFEditableButton *pSelectedItem;
	
	char szCategoryName[32];
	
	int iCollumnSpacing;
	int iRowSpacing;
	
	int iElementWidth;
	int iElementHeight;
	
	int iElementsPerRow;
	int iElementsPerScroll;
	
	int	iLastX;
	int	iLastY;
	
	PanelListItem_t t_PanelTemplate;
};

#endif // OF_LOADOUT_H
