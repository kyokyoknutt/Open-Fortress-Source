//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "of_scrollablepanellist.h"
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/ScrollBarSlider.h>
#include <vgui_controls/Button.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFScrollablePanelList::CTFScrollablePanelList( Panel *parent, const char *panelName ) : EditablePanel( parent, panelName )
{
	pScrollBar = new ScrollBar( this, "ScrollBar", true );
	pScrollBar->AddActionSignalTarget( this );
	
	pSelectedItem = NULL;
	
	iElementsPerRow = 1;
	iElementsPerScroll = 1;
	
	iLastX = 0;
	iLastY = 0;
}

void CTFScrollablePanelList::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);

	iCollumnSpacing = inResourceData->GetInt( "CollumnSpacing" );
	iRowSpacing = inResourceData->GetInt( "RowSpacing" );

	iElementWidth = inResourceData->GetInt( "element_width" );
	iElementHeight = inResourceData->GetInt( "element_height" );
	iElementsPerRow = ( inResourceData->GetInt("wide") - 30 ) / ( iElementWidth + iRowSpacing );
	iElementsPerScroll = iElementsPerRow * (( inResourceData->GetInt("tall") ) / ( iElementHeight  + iCollumnSpacing ));
}

void CTFScrollablePanelList::PerformLayout()
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
void CTFScrollablePanelList::OnScrollBarSliderMoved()
{
	int nScrollAmount = pScrollBar->GetValue();
	
	for( int i = 0; i < m_hItems.Count(); i++ )
	{
		m_hItems[i].pPanel->SetPos( m_hItems[i].def_xpos, m_hItems[i].def_ypos - nScrollAmount );
	}
}

//-----------------------------------------------------------------------------
// respond to mouse wheel events
//-----------------------------------------------------------------------------
void CTFScrollablePanelList::OnMouseWheeled(int delta)
{
	int val = pScrollBar->GetValue();
	val -= (delta * 50);
	pScrollBar->SetValue( val );
}

void CTFScrollablePanelList::OnSelectionChanged( Panel *panel )
{
	CTFEditableButton *pEditableButton = dynamic_cast<CTFEditableButton*>( panel );
	
	if( pEditableButton )
	{
		if( pEditableButton->m_bSelected )
		{
			if( pSelectedItem )
			{
				pSelectedItem->OnReleasedSelected();
				pSelectedItem->SetSelected( false );
			}
			pSelectedItem = pEditableButton;
		}
		else if( pSelectedItem == pEditableButton )
		{
				pSelectedItem = NULL;
		}
	}
}

void CTFScrollablePanelList::AddItem( CTFEditableButton *pPanel )
{
	PanelListItem_t pNewItem;

	pNewItem.pPanel = pPanel;
	
	int iWide, iTall;
	pPanel->GetSize( iWide, iTall );
	
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

	pNewItem.pPanel->SetPos(x,y);

	pNewItem.def_xpos = x;
	pNewItem.def_ypos = y;
	
	m_hItems.AddToTail(pNewItem);

	int w, h;
	int gw, gh;
	pNewItem.pPanel->GetSize( w, h );
	GetSize( gw, gh );
	
	iLastX = iLastX + w + iCollumnSpacing;
	
	if( iLastX + w + iCollumnSpacing > gw )
	{
		iLastX = 0;
		iLastY = iLastY + h + iRowSpacing;
	}

	pScrollBar->SetRange( 0, y + iTall + iCollumnSpacing );
	pScrollBar->SetRangeWindow( gh );
}

void CTFScrollablePanelList::ClearItemList()
{
	for( int i = 0; i < m_hItems.Count(); i++ )
	{
		m_hItems[i].pPanel->DeletePanel();
	}
	m_hItems.Purge();
	
	iLastX = 0;
	iLastY = 0;
}
