//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/ScrollBarSlider.h>
#include "of_scrollableitemlist.h"

using namespace vgui;


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFScrollableItemList::CTFScrollableItemList( Panel *parent, const char *panelName ) : EditablePanel( parent, panelName )
{
	pScrollBar = new ScrollBar( this, "ScrollBar", true );
	pScrollBar->AddActionSignalTarget( this );
	
	pSelectedItem = NULL;
	iLastX = 0;
	iLastY = 0;
}

void CTFScrollableItemList::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	iCollumnSpacing = inResourceData->GetInt( "CollumnSpacing" );
	iRowSpacing = inResourceData->GetInt( "RowSpacing" );

	KeyValues *kvItemNew = inResourceData->FindKey("ItemTemplate");

	if( kvItemNew )
	{
		Q_strncpy(t_ItemTemplate.wide, kvItemNew->GetString("wide"), sizeof(t_ItemTemplate.wide));
		Q_strncpy(t_ItemTemplate.tall, kvItemNew->GetString("tall"), sizeof(t_ItemTemplate.tall));
		Q_strncpy(t_ItemTemplate.border_idle, kvItemNew->GetString("border_idle"), sizeof(t_ItemTemplate.border_idle));
		Q_strncpy(t_ItemTemplate.border_hover, kvItemNew->GetString("border_hover"), sizeof(t_ItemTemplate.border_hover));		
		Q_strncpy(t_ItemTemplate.border_pressed, kvItemNew->GetString("border_pressed"), sizeof(t_ItemTemplate.border_pressed));
		Q_strncpy(t_ItemTemplate.border_selected, kvItemNew->GetString("border_selected"), sizeof(t_ItemTemplate.border_selected));			

		KeyValues *kvItemButton = kvItemNew->FindKey( "Button" );
		if( kvItemButton )
		{
			Q_strncpy(t_ItemTemplate.button_wide, kvItemButton->GetString("wide"), sizeof(t_ItemTemplate.button_wide));
			Q_strncpy(t_ItemTemplate.button_tall, kvItemButton->GetString("tall"), sizeof(t_ItemTemplate.button_tall));
			Q_strncpy(t_ItemTemplate.button_xpos, kvItemButton->GetString("xpos"), sizeof(t_ItemTemplate.button_xpos));		
			Q_strncpy(t_ItemTemplate.button_ypos, kvItemButton->GetString("ypos"), sizeof(t_ItemTemplate.button_ypos));
			Q_strncpy(t_ItemTemplate.button_zpos, kvItemButton->GetString("zpos"), sizeof(t_ItemTemplate.button_zpos));			
		}

		KeyValues *kvItemImage = kvItemNew->FindKey( "ItemImage" );
		if( kvItemImage )
		{
			Q_strncpy(t_ItemTemplate.extra_wide, kvItemImage->GetString("wide"), sizeof(t_ItemTemplate.extra_wide));
			Q_strncpy(t_ItemTemplate.extra_tall, kvItemImage->GetString("tall"), sizeof(t_ItemTemplate.extra_tall));
			Q_strncpy(t_ItemTemplate.extra_xpos, kvItemImage->GetString("xpos"), sizeof(t_ItemTemplate.extra_xpos));		
			Q_strncpy(t_ItemTemplate.extra_ypos, kvItemImage->GetString("ypos"), sizeof(t_ItemTemplate.extra_ypos));
			Q_strncpy(t_ItemTemplate.extra_zpos, kvItemImage->GetString("zpos"), sizeof(t_ItemTemplate.extra_zpos));	
		}		

		iElementWidth = kvItemNew->GetInt( "wide" );
		iElementHeight = kvItemNew->GetInt( "tall" );
		iElementsPerRow = ( inResourceData->GetInt("wide") - 30 ) / ( iElementWidth + iRowSpacing );
		iElementsPerScroll = iElementsPerRow * (( inResourceData->GetInt("tall") ) / ( iElementHeight  + iCollumnSpacing ));
	}
}

void CTFScrollableItemList::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( pScrollBar->GetSlider() )
	{
		pScrollBar->GetSlider()->SetFgColor( pScrollBar->GetFgColor() );
	}
	if ( pScrollBar->GetButton(0) )
	{
		pScrollBar->GetButton(0)->SetFgColor( pScrollBar->GetFgColor() );
	}
	if ( pScrollBar->GetButton(1) )
	{
		pScrollBar->GetButton(1)->SetFgColor( pScrollBar->GetFgColor() );
	}
}

//-----------------------------------------------------------------------------
// Called when the scroll bar moves
//-----------------------------------------------------------------------------
void CTFScrollableItemList::OnScrollBarSliderMoved()
{
	int nScrollAmount = pScrollBar->GetValue();
	
	for( int i = 0; i < m_hItems.Count(); i++ )
	{
		m_hItems[i].pItemPanel->SetPos( m_hItems[i].def_xpos, m_hItems[i].def_ypos - nScrollAmount );
	}
}

//-----------------------------------------------------------------------------
// respond to mouse wheel events
//-----------------------------------------------------------------------------
void CTFScrollableItemList::OnMouseWheeled(int delta)
{
	int val = pScrollBar->GetValue();
	val -= (delta * 50);
	pScrollBar->SetValue( val );
}

void CTFScrollableItemList::AddItem( int iID, bool bSelected )
{
	ItemListItem_t pNewItem;

	pNewItem.pItemPanel = new CTFItemSelection(this, VarArgs("%d", iID), iID);
	pNewItem.pItemPanel->AddActionSignalTarget(this);
	
	KeyValues *kvItemTemplate = new KeyValues("Template");
	
	kvItemTemplate->SetString("wide", t_ItemTemplate.wide);
	kvItemTemplate->SetString("tall", t_ItemTemplate.tall);
	kvItemTemplate->SetString("border_idle", t_ItemTemplate.border_idle);
	kvItemTemplate->SetString("border_hover", t_ItemTemplate.border_hover);
	kvItemTemplate->SetString("border_pressed", t_ItemTemplate.border_pressed);
	kvItemTemplate->SetString("border_selected", t_ItemTemplate.border_selected);
	
	KeyValues *kvButton = new KeyValues("Button");
	
	kvButton->SetString("wide", t_ItemTemplate.button_wide);
	kvButton->SetString("tall", t_ItemTemplate.button_tall);
	kvButton->SetString("xpos", t_ItemTemplate.button_xpos);
	kvButton->SetString("ypos", t_ItemTemplate.button_ypos);
	kvButton->SetString("zpos", t_ItemTemplate.button_zpos);
	kvButton->SetString( "proportionalToParent", "1" );
	
	KeyValues *kvItemImage = new KeyValues("ItemImage");
	
	kvItemImage->SetString("wide", t_ItemTemplate.extra_wide);
	kvItemImage->SetString("tall", t_ItemTemplate.extra_tall);
	kvItemImage->SetString("xpos", t_ItemTemplate.extra_xpos);
	kvItemImage->SetString("ypos", t_ItemTemplate.extra_ypos);
	kvItemImage->SetString("zpos", t_ItemTemplate.extra_zpos);
	
	kvItemImage->SetString("visible", "1" );
	kvItemImage->SetString("scaleImage", "1" );
	kvItemImage->SetString("proportionalToParent", "1" );
	
	kvItemTemplate->AddSubKey(kvButton);
	kvItemTemplate->AddSubKey(kvItemImage);
	
	pNewItem.pItemPanel->ApplySettings(kvItemTemplate);

	if( !iLastX )
	{
		iLastX = iCollumnSpacing;
	}
	if( !iLastY )
	{
		iLastY = iRowSpacing;
	}
	
	int x = iLastX;
	int y = iLastY;

	pNewItem.pItemPanel->SetPos(x,y);
	pNewItem.pItemPanel->SetItemID(iID);

	pNewItem.def_xpos = x;
	pNewItem.def_ypos = y;
	
	m_hItems.AddToTail(pNewItem);

	int w, h;
	int gw, gh;
	pNewItem.pItemPanel->GetSize( w, h );
	GetSize( gw, gh );
	
	iLastX = iLastX + w + iCollumnSpacing;
	
	if( iLastX + w + iCollumnSpacing > gw )
	{
		iLastX = 0;
		iLastY = iLastY + h + iRowSpacing;
	}

	int iWide, iTall;
	pNewItem.pItemPanel->GetSize( iWide, iTall );
	
	pScrollBar->SetRange( 0, y + iTall + iCollumnSpacing );
	pScrollBar->SetRangeWindow( gh );

	pNewItem.pItemPanel->SetSelected( bSelected );
}

void CTFScrollableItemList::ClearItemList()
{
	for( int i = 0; i < m_hItems.Count(); i++ )
	{
		m_hItems[i].pItemPanel->DeletePanel();
	}
	m_hItems.Purge();

	iLastX = 0;
	iLastY = 0;
}
