//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_SCRLITEMLIST_H
#define OF_SCRLITEMLIST_H
#ifdef _WIN32
#pragma once
#endif

#include "of_itemselection.h"

struct ItemListItem_t
{
	CTFItemSelection *pItemPanel;
	int	def_xpos;
	int def_ypos;
};

class CTFScrollableItemList : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE(CTFScrollableItemList, vgui::EditablePanel);
public:
	CTFScrollableItemList(vgui::Panel *parent, const char *panelName);
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void PerformLayout();

	void AddItem(int iID, bool bSelected);
	void ClearItemList(void);

	virtual void OnCommand(const char * command)
	{
		BaseClass::OnCommand(command);
		PostActionSignal(new KeyValues("Command", "command", command));
	}
	MESSAGE_FUNC(OnScrollBarSliderMoved, "ScrollBarSliderMoved");
	virtual void OnMouseWheeled(int delta);	// respond to mouse wheel events	

public:
	CUtlVector<ItemListItem_t> m_hItems;
	vgui::ScrollBar *pScrollBar;

	CTFItemSelection *pSelectedItem;

	char szCategoryName[32];

	int iCollumnSpacing;
	int iRowSpacing;

	int iElementWidth;
	int iElementHeight;

	int iElementsPerRow;
	int iElementsPerScroll;

	int iLastX;
	int iLastY;

	ItemTemplate_t t_ItemTemplate;
};


#endif // OF_LOADOUT_H
