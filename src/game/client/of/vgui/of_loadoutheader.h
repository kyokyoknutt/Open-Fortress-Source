//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_LOADOUTHEADER_H
#define OF_LOADOUTHEADER_H
#ifdef _WIN32
#pragma once
#endif

#include "of_headeritem.h"
#include "of_scrollableitemlist.h"

struct HeaderListItem_t
{
	HeaderListItem_t()
	{
		pHeaderItem = NULL;
	}
	CTFHeaderItem *pHeaderItem;
	int	def_xpos;
	int def_ypos;
};

class CTFLoadoutHeader : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFLoadoutHeader, vgui::EditablePanel );
public:
	CTFLoadoutHeader( vgui::Panel *parent, const char *panelName );
	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void PerformLayout();
	
	void AddCategory( const char *szCategory );
	void ClearCategoryList( void );
	
	MESSAGE_FUNC( OnScrollBarSliderMoved, "ScrollBarSliderMoved" );
	virtual void OnMouseWheeled(int delta);	// respond to mouse wheel events	

public:
	CUtlVector<HeaderListItem_t> m_hCategories;
	vgui::ScrollBar *pScrollBar;
	
	CTFHeaderItem *pSelectedHeader;

	int iLastXPos;
	
	ItemTemplate_t t_ItemTemplate;
};



#endif // OF_LOADOUT_H
