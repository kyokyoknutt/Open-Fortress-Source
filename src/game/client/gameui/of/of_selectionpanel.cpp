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

#include "engine/IEngineSound.h"
#include "basemodelpanel.h"
#include "tf_gamerules.h"
#include "of_shared_schemas.h"
#include <convar.h>
#include <vgui_controls/ScrollBarSlider.h>
#include <vgui_controls/Slider.h>
#include "fmtstr.h"

#include "of_selectionpanel.h"

#include "tier0/dbg.h"

using namespace vgui;


DECLARE_BUILD_FACTORY(CTFSelectionManager);

CTFSelectionManager::CTFSelectionManager(Panel *parent, const char *panelName) : Panel(parent, panelName)
{
	pSelectedItem = NULL;
	bHasSelectedItem = false;
}

void CTFSelectionManager::OnPanelSelected(Panel *panel)
{
	if (bHasSelectedItem)
	{
		CTFSelectionPanel *pTarget = (CTFSelectionPanel*)pSelectedItem;
		if (pTarget)
		{
			pTarget->SetSelected(false);
			pTarget->OnReleasedSelected();
		}
	}

	pSelectedItem = panel;
	bHasSelectedItem = true;
}

void CTFSelectionManager::ApplySettings(KeyValues *inResourceData)
{
	KeyValues *pPanels = inResourceData->FindKey("Panels");

	bool bFirst = true;

	if (pPanels)
	{
		FOR_EACH_VALUE(pPanels, kvValue)
		{
			CTFSelectionPanel *pPanel = (CTFSelectionPanel*)GetParent()->FindChildByName(kvValue->GetName());
			if (pPanel)
			{
				m_hPanels.AddToTail(pPanel);
				pPanel->AddActionSignalTarget(this);
				if (bFirst)
				{
					pPanel->SetSelected(true);
					pPanel->OnReleasedUnselected();
					bFirst = false;
				}
			}
		}
	}

	BaseClass::ApplySettings(inResourceData);
}

DECLARE_BUILD_FACTORY( CTFSelectionPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFSelectionPanel::CTFSelectionPanel(Panel *parent, const char *panelName) : CTFEditableButton(parent, panelName)
{
	pImage = new CTFImagePanel(this, "Image");
	iBaseX = -999;
	iBaseY = -999;
	
	pManger = NULL;
}

void CTFSelectionPanel::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );
	
	iXAdj = inResourceData->GetInt( "XAdjustment", 0 );
	iYAdj = inResourceData->GetInt( "YAdjustment", 0 );

	KeyValues *inImage = inResourceData->FindKey("Image");
	if( inImage )
	{
		pImage->ApplySettings(inImage);
	}

	iBaseX = -999;
    iBaseY = -999;

	SetConnectedPanel( inResourceData->GetString("panel") );
}

void CTFSelectionPanel::SetConnectedPanel( const char *szConnectedPanel )
{
	Panel *pParent = GetParent();
	if( !pParent )
		return;

	pConnectedPanel = pParent->FindChildByName( szConnectedPanel );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSelectionPanel::OnMouseReleased(MouseCode code)
{
	if( m_bSelected )
		return;

	BaseClass::OnMouseReleased(code);
}

void CTFSelectionPanel::OnReleasedSelected()
{
	BaseClass::OnReleasedSelected();

	if( pConnectedPanel )
		pConnectedPanel->SetVisible( false );
}

void CTFSelectionPanel::OnReleasedUnselected()
{
	BaseClass::OnReleasedUnselected();

	if( pConnectedPanel )
		pConnectedPanel->SetVisible( true );
}

void CTFSelectionPanel::SetSelected( bool bSelected )
{
	BaseClass::SetSelected( bSelected );
	
	int x, y;
	GetPos( x, y );	
	
	if( !bSelected )
	{
		if( iBaseX == -999 )
			iBaseX = x;
		
		if( iBaseY == -999 )
			iBaseY = y;
		SetPos( iBaseX , iBaseY );
	}
	else
	{
		if( iBaseX == -999 )
			iBaseX = x;
		
		if( iBaseY == -999 )
			iBaseY = y;	

		SetPos( iBaseX - iXAdj , iBaseY - iYAdj );
		
		PostActionSignal(new KeyValues("OnPanelSelected"));
	}
}