//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/QueryBox.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include <game/client/iviewport.h>
#include "tf_tips.h"
#include "renderparm.h"
#include "animation.h"
#include "tf_controls.h"
#include "cvartogglecheckbutton.h"
#include "datacache/imdlcache.h"

#include "of_loadoutheader.h"

#include "engine/IEngineSound.h"
#include "basemodelpanel.h"
#include "tf_gamerules.h"
#include "of_shared_schemas.h"
#include <convar.h>
#include <vgui_controls/ScrollBarSlider.h>
#include <vgui_controls/Slider.h>
#include "fmtstr.h"

#include "tier0/dbg.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFLoadoutHeader::CTFLoadoutHeader( Panel *parent, const char *panelName ) : EditablePanel( parent, panelName )
{
	pScrollBar = new ScrollBar( this, "ScrollBar", false );
	pScrollBar->AddActionSignalTarget( this );
	
	iLastXPos = 0;
}

void CTFLoadoutHeader::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings(inResourceData);
	
	iLastXPos = 0;
}

void CTFLoadoutHeader::PerformLayout()
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
void CTFLoadoutHeader::OnScrollBarSliderMoved()
{
	int nScrollAmount = pScrollBar->GetValue();
	
	for( int i = 0; i < m_hCategories.Count(); i++ )
	{
		int x, y;
		m_hCategories[i].pHeaderItem->GetPos( x, y );
		m_hCategories[i].pHeaderItem->SetPos( m_hCategories[i].def_xpos - nScrollAmount, y );
	}
}

//-----------------------------------------------------------------------------
// respond to mouse wheel events
//-----------------------------------------------------------------------------
void CTFLoadoutHeader::OnMouseWheeled(int delta)
{
	int val = pScrollBar->GetValue();
	val -= (delta * 50);
	pScrollBar->SetValue( val );
}

void CTFLoadoutHeader::AddCategory( const char *szCategory )
{
	HeaderListItem_t pNewItem;

	pNewItem.pHeaderItem = new CTFHeaderItem(this, szCategory);
	
	KeyValues *kvHeaderTemplate = new KeyValues("Template");
	
	kvHeaderTemplate->SetString("panel", VarArgs("%sList",szCategory));
	kvHeaderTemplate->SetString("wide", "10");
	kvHeaderTemplate->SetString("tall", "57");
	kvHeaderTemplate->SetString("ypos", "20");
	kvHeaderTemplate->SetString("border_idle", "LoadoutThickBorder");
	kvHeaderTemplate->SetString("border_hover", "LoadoutThickBorder");
	kvHeaderTemplate->SetString("border_pressed", "LoadoutThickBorder");
	kvHeaderTemplate->SetString("border_selected", "LoadoutThickBorder");
	kvHeaderTemplate->SetString("proportionalToParent", "1" );
	
	KeyValues *kvButton = new KeyValues("Button");
	
	kvButton->SetString("wide", "10");
	kvButton->SetString("tall", "100");
	kvButton->SetString("zpos", "10");
	kvButton->SetString("ypos", "c-50");
	kvButton->SetString( "proportionalToParent", "1" );
	
	KeyValues *kvLabel = new KeyValues("Label");
	
	kvLabel->SetString("wide", "10");
	kvLabel->SetString("tall", "50");
	kvLabel->SetString("ypos", "c-32");
	kvLabel->SetString("zpos", "1");
	
	kvLabel->SetString("font", "HudFontMediumBold" );
	kvLabel->SetString("fgcolor", "TanLight" );
	kvLabel->SetString("textAlignment", "right" );
	kvLabel->SetString("proportionalToParent", "1" );
	
	wchar_t wszLocalized[32];
	g_pVGuiLocalize->ConstructString( wszLocalized, sizeof( wszLocalized ), g_pVGuiLocalize->Find( VarArgs("#OF_Loadout_Region_%s", szCategory) ), 0 );
	
	kvLabel->SetWString("labelText", wszLocalized );
	
	kvHeaderTemplate->AddSubKey(kvButton);
	kvHeaderTemplate->AddSubKey(kvLabel);
	
	pNewItem.pHeaderItem->ApplySettings(kvHeaderTemplate);

	int x,y;
	pNewItem.pHeaderItem->GetPos( x, y );
	pNewItem.pHeaderItem->SetPos(iLastXPos , y);

	pNewItem.def_xpos = iLastXPos;
	pNewItem.def_ypos = y;
	
	m_hCategories.AddToTail(pNewItem);
	
	int w, h;
	pNewItem.pHeaderItem->GetSize( w, h );
	iLastXPos += w;
	
	GetSize( w, h );
	pScrollBar->SetRange( 0, iLastXPos );
	pScrollBar->SetRangeWindow( w );
}

void CTFLoadoutHeader::ClearCategoryList()
{
	for( int i = 0; i < m_hCategories.Count(); i++ )
	{
		m_hCategories[i].pHeaderItem->DeletePanel();
	}
	m_hCategories.Purge();
}
