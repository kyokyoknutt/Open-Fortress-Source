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

#include "of_itemselection.h"
#include "of_scrollableitemlist.h"
#include "gameui/BaseModPanel.h"
#include "gameui/dm_loadout.h"

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

Panel *CTFItemSelection_Factory()
{
	return new CTFItemSelection(NULL, NULL, 0);
}

DECLARE_BUILD_FACTORY_CUSTOM( CTFItemSelection, CTFItemSelection_Factory );


CTFItemSelection::CTFItemSelection(Panel *parent, const char *panelName, const int iID) : CTFEditableButton(parent, panelName)
{
	pItemImage = new CTFImagePanel(this, "ItemImage");
	
	SetItemID( iID );
	
	for( int i = 1; i <= 5; i++ )
		AddOnPressSound( VarArgs( "Mercenary.PositiveVocalization0%d", i ) );
}

void CTFItemSelection::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );

	Q_strncpy(szCommand, inResourceData->GetString("command"), sizeof(szCommand));
	
	KeyValues *inItemImage = inResourceData->FindKey("ItemImage");
	if( inItemImage )
		pItemImage->ApplySettings(inItemImage);

	if( inResourceData->GetInt("ItemID") )
		SetItemID( inResourceData->GetInt("ItemID") );
	
	m_iSoundChance = 4;
}

void CTFItemSelection::SetItemID( int iID )
{
	iItemID = iID;
	Q_strncpy(szCommand, VarArgs("loadout_equip cosmetics mercenary %d", iItemID), sizeof(szCommand));
	
	KeyValues *pCosmetic = GetCosmetic(iItemID);
	if( !pCosmetic )
		return;

	pItemImage->SetImage( pCosmetic->GetString("backpack_icon", "../backpack/blocked") );
	pItemImage->InvalidateLayout( false, true );
	bParsedBPImage = true;
}

void CTFItemSelection::Paint()
{
	BaseClass::Paint();
	if( iItemID && !bParsedBPImage )
	{
		SetItemID( iItemID );
	}
}

void CTFItemSelection::OnReleasedSelected()
{
	BaseClass::OnReleasedSelected();

	PostActionSignal(new KeyValues("Command", "command", szCommand));
	engine->ExecuteClientCmd( szCommand );
}

void CTFItemSelection::OnReleasedUnselected()
{
	BaseClass::OnReleasedUnselected();

	PostActionSignal(new KeyValues("Command", "command", szCommand));
	engine->ExecuteClientCmd( szCommand );
}

void CTFItemSelection::SetSelected( bool bSelected )
{
	BaseClass::SetSelected( bSelected );

	if( !bSelected )
		Q_strncpy(szCommand, VarArgs("loadout_equip cosmetics mercenary %d", iItemID), sizeof(szCommand));
	else
		Q_strncpy(szCommand, VarArgs("loadout_unequip cosmetics mercenary %d", iItemID), sizeof(szCommand));

	CTFScrollableItemList *pItemList = dynamic_cast<CTFScrollableItemList*>( GetParent() );

	if( pItemList )
	{
		if( bSelected )
		{
			if( pItemList->pSelectedItem )
			{
				pItemList->pSelectedItem->SetSelected( false );
			}
			pItemList->pSelectedItem = this;
		}
		else if( pItemList->pSelectedItem == this )
		{
			pItemList->pSelectedItem = NULL;
		}
	}
}
