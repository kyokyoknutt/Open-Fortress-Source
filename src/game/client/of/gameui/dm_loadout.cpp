//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "of_commandbutton.h"
#include "dm_loadout.h"
#include "of_shared_schemas.h"
#include <convar.h>
#include "vgui_controls/AnimationController.h"
#include "filesystem.h"

#include "tier0/dbg.h"

using namespace BaseModUI;
using namespace vgui;

extern ConVar of_tennisball;
extern ConVar of_respawn_particle;
extern ConVar of_announcer_override;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
DMLoadout::DMLoadout(Panel *parent, const char *panelName) : BaseClass(parent, panelName) 
{
	///
	SetProportional(true);	
	///
	m_pCloseButton = new Button( this, "CloseButton", "" );	
	pCosmeticPanel = new EditablePanel( this, "CosmeticPanel" );
	pArsenalPanel = new EditablePanel( this, "ArsenalPanel" );
	pVisualPanel = new EditablePanel( this, "VisualPanel" );
	pParticleList = new CTFScrollablePanelList( pVisualPanel, "ParticleList" );
	pAnnouncerList = new CTFScrollablePanelList( pVisualPanel, "AnnouncerList" );
	
	pPrimaryToggle = new CTFSelectionPanel( pArsenalPanel, "PrimaryToggle" );
	pSecondaryToggle = new CTFSelectionPanel( pArsenalPanel, "SecondaryToggle" );
	pMeleeToggle = new CTFSelectionPanel( pArsenalPanel, "MeleeToggle" );
	
	for( int i = 0; i < 3; i++ )
	{
		pWeaponList[i] = new CTFScrollablePanelList( pArsenalPanel, VarArgs("WeaponList%d", i) );
	}

	m_pItemHeader = new CTFLoadoutHeader( pCosmeticPanel, "ItemHeader" );
	
	m_bControlsLoaded = false;
	m_bInteractive = false;
	m_pSelectedOptions = NULL;
	m_bParsedParticles = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void DMLoadout::ApplySettings( KeyValues *inResourceData )
{
	InitLoadoutHandle();
	BaseClass::ApplySettings( inResourceData );

	GetAnimationController()->SetScriptFile( GetVPanel(), "scripts/HudAnimations_tf.txt" );
	
	KeyValues *inNewResourceData = new KeyValues("ResourceData");	
	if( !inNewResourceData->LoadFromFile( filesystem, m_ResourceName) )
		return;	
		
	KeyValues *inCosmeticPanel = inNewResourceData->FindKey("CosmeticPanel");
	if( !inCosmeticPanel )
		return;
	
	if( !GetItemsGame() )
		return;

	KeyValues *pCosmetics = GetItemsGame()->FindKey("Cosmetics");
	if( !pCosmetics )
		return;
	
	m_pItemHeader->ApplySettings(inCosmeticPanel->FindKey("ItemHeader"));
	
	m_pItemHeader->ClearCategoryList();
	for( int i = 0; i < m_pItemCategories.Count(); i++ )
	{
		m_pItemCategories[i]->ClearItemList();
		m_pItemCategories[i]->DeletePanel();
	}
	m_pItemCategories.Purge();
	
	m_pItemHeader->pSelectedHeader = NULL;

	CUtlVector<const char*> hCategories;
	KeyValues *pLoop = pCosmetics->GetFirstSubKey();
	for( pLoop; pLoop != NULL; pLoop = pLoop->GetNextKey() ) // Loop through all the keyvalues
	{
		bool bExists = false;
		int iExistingLocation = 0;
		for( int i = 0; i < hCategories.Count(); i++ )
		{
			if( !Q_stricmp( hCategories[i], pLoop->GetString("region") ) )
			{
				bExists = true;
				iExistingLocation = i;
				break;
			}
		}
		
		if( bExists )
		{
			int iID = atoi(pLoop->GetName());
			bool bSelected = false;

			if (GetLoadout())
			{
				KeyValues *kvCosmetics = GetLoadout()->FindKey("Cosmetics");
				if (kvCosmetics)
				{
					KeyValues *kvMerc = kvCosmetics->FindKey("mercenary");
					if (kvMerc)
					{
						if (iID == kvMerc->GetInt(m_pItemCategories[iExistingLocation]->szCategoryName))
						{
							bSelected = true;
						}
					}
				}
			}

			m_pItemCategories[iExistingLocation]->AddItem(iID, bSelected);
		}
		else
		{
			hCategories.AddToTail(pLoop->GetString("region"));
			CTFScrollableItemList *pNew = new CTFScrollableItemList( m_pItemHeader->GetParent(), VarArgs("%sList",pLoop->GetString("region")) );
			pNew->ApplySettings(inCosmeticPanel->FindKey("ListTemplate"));
			pNew->AddActionSignalTarget(this);
			Q_strncpy(pNew->szCategoryName, pLoop->GetString("region"), sizeof(pNew->szCategoryName));
			m_pItemCategories.AddToTail(pNew);
			m_pItemCategories[m_pItemCategories.Count()-1]->AddItem(atoi(pLoop->GetName()), false);
			m_pItemHeader->AddCategory(pLoop->GetString("region"));
			if( m_pItemCategories.Count() > 0 )
			{
				if( m_pItemCategories.Count() == 1 )
				{	
					m_pItemHeader->m_hCategories[0].pHeaderItem->SetSelected(true);
					m_pItemHeader->m_hCategories[0].pHeaderItem->OnReleasedUnselected();
				}
				else
				{
					if( m_pItemHeader->m_hCategories[m_pItemCategories.Count() - 1].pHeaderItem )
						m_pItemHeader->m_hCategories[m_pItemCategories.Count() - 1].pHeaderItem->OnReleasedSelected();
				}
			}
		}
	}
	
	KeyValues *inVisualPanel = inNewResourceData->FindKey("VisualPanel");
	if( !inVisualPanel )
		return;
	
	if( !m_bParsedParticles )
	{
		m_bParsedParticles = true;
		KeyValues *inParticleList = inVisualPanel->FindKey("ParticleList");
		if( !inParticleList )
			return;	

		if( pParticleList )
		{
			pParticleList->ClearItemList();
			
			pParticleList->ApplySettings( inParticleList );
			
			KeyValues *kvTemp = new KeyValues("Resource");
			
			kvTemp->SetString( "fieldName", "ItemTemplate" );
			kvTemp->SetString( "wide", "50" );
			kvTemp->SetString( "tall", "50" );
			kvTemp->SetString( "autoResize", "0" );
			kvTemp->SetString( "pinCorner", "2" );
			kvTemp->SetString( "visible", "1" );
			kvTemp->SetString( "enabled", "1" );
			kvTemp->SetString( "tabPosition", "0" );
			kvTemp->SetString( "proportionalToParent", "1" );
			kvTemp->SetString( "border_idle", "ItemOutlineIdle" );
			kvTemp->SetString( "border_hover", "ItemOutlineHoverover" );
			kvTemp->SetString( "border_pressed", "ItemOutlineIdle" );
			kvTemp->SetString( "border_selected", "ItemOutlineSelected"	);
			kvTemp->SetString( "command", "of_respawn_particle 0" );
			kvTemp->SetString( "sound_chances", "2" );
			kvTemp->SetString( "pressed_sound", "Player.Spawn" );
			
			KeyValues *kvButtTemp = new KeyValues("Button");
			kvButtTemp->SetString( "wide", "50" );
			kvButtTemp->SetString( "tall", "50" );
			kvButtTemp->SetString( "xpos", "c-25" );
			kvButtTemp->SetString( "ypos", "c-25" );
			kvButtTemp->SetString( "zpos", "10" );
			kvButtTemp->SetString( "proportionalToParent", "1" );
			
			KeyValues *kvModelTemp = new KeyValues("ParticleModel");
			kvModelTemp->SetString( "wide", "50" );
			kvModelTemp->SetString( "tall", "50" );
			kvModelTemp->SetString( "xpos", "c-25" );
			kvModelTemp->SetString( "ypos", "c-25" );
			kvModelTemp->SetString( "zpos", "6" );
			kvModelTemp->SetString( "fov", "25" );
			kvModelTemp->SetString( "render_texture", "0" );
			kvModelTemp->SetString( "allow_rot", "1" );
			kvModelTemp->SetString( "use_particle", "1" );
			kvModelTemp->SetString( "particle_loop", "1" );
			kvModelTemp->SetString( "proportionalToParent", "1" );
			
			KeyValues *kvModelModelTemp = new KeyValues("model");
			kvModelModelTemp->SetString( "modelname", "models/empty.mdl" );
			kvModelModelTemp->SetString( "force_pos", "1" );
			kvModelModelTemp->SetString( "skin"	,"4" );
			kvModelModelTemp->SetString( "origin_z"	,"-40" );
			kvModelModelTemp->SetString( "origin_x", "450");
			
			KeyValues *kvModelAnimTemp = new KeyValues("animation");
			kvModelAnimTemp->SetString( "name", "PRIMARY" );
			kvModelAnimTemp->SetString( "activity", "ACT_MERC_LOADOUT" );
			kvModelAnimTemp->SetString( "default", "1" );
			
			kvModelModelTemp->AddSubKey( kvModelAnimTemp );
			kvModelTemp->AddSubKey( kvModelModelTemp );
			
			kvTemp->AddSubKey( kvButtTemp );
			
			for( int i = 1; i <= 36; i++ )
			{
				CTFCommandButton *pTemp = new CTFCommandButton( pParticleList, "Temp" );
				kvTemp->SetString( "command", VarArgs( "of_respawn_particle %d", i ) );
				kvTemp->SetString( "convref", "of_respawn_particle" );
				kvTemp->SetString( "targetval", VarArgs( "%d", i ) );
				pTemp->ApplySettings( kvTemp );
				
				pParticleList->AddItem( pTemp );

				if( of_respawn_particle.GetInt() == i )
				{
					pTemp->SetSelected(true);
					pParticleList->pSelectedItem = pTemp;
				}
				
				KeyValues *pParticle = GetRespawnParticle( i );
				if( pParticle )
				{	
					kvModelTemp->SetString("particle_loop_time", pParticle->GetString("loop_time", "1.2") );
					kvModelTemp->SetString("particle_z_offset", pParticle->GetString("particle_z_offset", "0") );
					CTFModelPanel *pTempMDL = new CTFModelPanel( pTemp, "ParticleModel" );
					pTempMDL->ApplySettings( kvModelTemp );
					
					char pEffectName[32];
					pEffectName[0] = '\0';
					if ( i < 10 )
						Q_snprintf( pEffectName, sizeof( pEffectName ), "dm_respawn_0%d", i );
					else
						Q_snprintf( pEffectName, sizeof( pEffectName ), "dm_respawn_%d", i );
					if ( pEffectName[0] != '\0' )
						Q_strncpy( pTempMDL->szLoopingParticle, pEffectName, sizeof(pTempMDL->szLoopingParticle) );
					pTempMDL->Update();
				}
				else
				{
					kvModelTemp->deleteThis();
				}
			}
		}
	}
	modelinfo->FindOrLoadModel( "models/player/mercenary.mdl" );
	
	KeyValues *inAnnouncerList = inVisualPanel->FindKey("AnnouncerList");
	if( !inAnnouncerList )
		return;	

	if( pAnnouncerList )
	{
		pAnnouncerList->ClearItemList();
		
		pAnnouncerList->ApplySettings( inAnnouncerList );
		
		KeyValues *kvTemp = new KeyValues("Resource");
		
		kvTemp->SetString( "fieldName", "ItemTemplate" );
		kvTemp->SetString( "wide", "50" );
		kvTemp->SetString( "tall", "50" );
		kvTemp->SetString( "autoResize", "0" );
		kvTemp->SetString( "pinCorner", "2" );
		kvTemp->SetString( "visible", "1" );
		kvTemp->SetString( "enabled", "1" );
		kvTemp->SetString( "tabPosition", "0" );
		kvTemp->SetString( "proportionalToParent", "1" );
		kvTemp->SetString( "border_idle", "ItemOutlineIdle" );
		kvTemp->SetString( "border_hover", "ItemOutlineHoverover" );
		kvTemp->SetString( "border_pressed", "ItemOutlineIdle" );
		kvTemp->SetString( "border_selected", "ItemOutlineSelected"	);
		kvTemp->SetString( "command", "of_announcer_override \"\"" );
		
		KeyValues *kvButtTemp = new KeyValues("Button");
		kvButtTemp->SetString( "wide", "50" );
		kvButtTemp->SetString( "tall", "50" );
		kvButtTemp->SetString( "xpos", "c-25" );
		kvButtTemp->SetString( "ypos", "c-25" );
		kvButtTemp->SetString( "zpos", "10" );
		kvButtTemp->SetString( "proportionalToParent", "1" );
		
		KeyValues *kvImageTemp = new KeyValues("AnnouncerImage");
		kvImageTemp->SetString( "wide", "50" );
		kvImageTemp->SetString( "tall", "50" );
		kvImageTemp->SetString( "xpos", "c-25" );
		kvImageTemp->SetString( "ypos", "c-25" );
		kvImageTemp->SetString( "zpos", "6" );
		kvImageTemp->SetString( "scaleImage", "1" );
		kvImageTemp->SetString( "proportionalToParent", "1" );
		
		kvTemp->AddSubKey( kvButtTemp );
		
		KeyValues* pSupport = new KeyValues( "AnnouncerSupport" );
		pSupport->LoadFromFile( filesystem, "scripts/announcer_support.txt" );
		
		for( KeyValues *pSub = pSupport->GetFirstSubKey(); pSub != NULL ; pSub = pSub->GetNextKey() )
		{
			CTFCommandButton *pTemp = new CTFCommandButton( pAnnouncerList, "Temp" );
			kvTemp->SetString( "command", VarArgs( "of_announcer_override %s", pSub->GetName() ) );
			kvTemp->SetString( "convref", "of_announcer_override" );
			kvTemp->SetString( "targetval", VarArgs( "%s", pSub->GetName() ) );
			pTemp->ApplySettings( kvTemp );
			pTemp->AddOnPressSound( VarArgs( "%s.Deathmatch", pSub->GetName() ) );
			pTemp->AddOnPressSound( VarArgs( "%s.Impressive", pSub->GetName() ) );
			pTemp->AddOnPressSound( VarArgs( "%s.Excellent", pSub->GetName() ) );
			pTemp->AddOnPressSound( VarArgs( "%s.Dominating", pSub->GetName() ) );
			pTemp->AddOnPressSound( VarArgs( "%s.DMRoundStart", pSub->GetName() ) );
			pTemp->AddOnPressSound( VarArgs( "%s.DMRoundPrepare", pSub->GetName() ) );
			
			kvImageTemp->SetString( "image", VarArgs( "../backpack/announcers/%s", pSub->GetName() ) );
			CTFImagePanel *pTempImage = new CTFImagePanel( pTemp, "AnnouncerImage" );
			pTempImage->ApplySettings( kvImageTemp );
			
			pAnnouncerList->AddItem( pTemp );

			if( !Q_stricmp( of_announcer_override.GetString(), pSub->GetName() ) )
			{
				pTemp->SetSelected(true);
				pAnnouncerList->pSelectedItem = pTemp;
			}
		}
	}
	
	KeyValues *inArsenalPanel = inNewResourceData->FindKey("ArsenalPanel");
	if( !inArsenalPanel )
		return;	
	
	KeyValues *inWeaponList = inArsenalPanel->FindKey("WeaponList");
	if( !inWeaponList )
		return;	

	for( int i = 0; i < 3; i++ )
	{
		if( pWeaponList[i] )
		{
			pWeaponList[i]->ClearItemList();
			
			inWeaponList->SetString("fieldName",VarArgs("WeaponList%d", i));
			
			inWeaponList->SetInt( "visible", !i );
			
			pWeaponList[i]->ApplySettings( inWeaponList );
			
			KeyValues *kvTemp = new KeyValues("Resource");
			
			kvTemp->SetString( "fieldName", "ItemTemplate" );
			kvTemp->SetString( "wide", "82" );
			kvTemp->SetString( "tall", "60" );
			kvTemp->SetString( "autoResize", "0" );
			kvTemp->SetString( "pinCorner", "2" );
			kvTemp->SetString( "visible", "1" );
			kvTemp->SetString( "enabled", "1" );
			kvTemp->SetString( "tabPosition", "0" );
			kvTemp->SetString( "proportionalToParent", "1" );
			kvTemp->SetString( "border_idle", "ItemOutlineIdle" );
			kvTemp->SetString( "border_hover", "ItemOutlineHoverover" );
			kvTemp->SetString( "border_pressed", "ItemOutlineIdle" );
			kvTemp->SetString( "border_selected", "ItemOutlineSelected"	);
			kvTemp->SetString( "command", "loadout_equip weapons mercenary \"\"" );

			KeyValues *kvButtTemp = new KeyValues("Button");
			kvButtTemp->SetString( "wide", "50" );
			kvButtTemp->SetString( "tall", "50" );
			kvButtTemp->SetString( "xpos", "c-25" );
			kvButtTemp->SetString( "ypos", "c-25" );
			kvButtTemp->SetString( "zpos", "10" );
			kvButtTemp->SetString( "proportionalToParent", "1" );
			
			KeyValues *kvImageTemp = new KeyValues("WeaponImage");
			kvImageTemp->SetString( "wide", "100" );
			kvImageTemp->SetString( "tall", "50" );
			kvImageTemp->SetString( "xpos", "c-41" );
			kvImageTemp->SetString( "ypos", "c-25" );
			kvImageTemp->SetString( "zpos", "6" );
			kvImageTemp->SetString( "scaleImage", "1" );
			kvImageTemp->SetString( "proportionalToParent", "1" );
			
			kvTemp->AddSubKey( kvButtTemp );
			
			for( int y = 0; y < GetItemSchema()->GetWeaponCount(); y++ )
			{
				KeyValues *pSlot = GetItemSchema()->GetWeapon(y)->FindKey("slot");
				if( pSlot )
				{
					if( pSlot->GetInt( "mercenary", 0 ) != i + 1 )
						continue;
				}
				else if( i == 2 )
					continue;
				CTFCommandButton *pTemp = new CTFCommandButton( pWeaponList[i], "Temp" );
				kvTemp->SetString( "fieldName", GetItemSchema()->GetWeapon(y)->GetName() );

				kvTemp->SetString( "command", VarArgs( "loadout_equip weapons mercenary %s %d", GetItemSchema()->GetWeapon(y)->GetName(), i + 1 ) );
				pTemp->ApplySettings( kvTemp );
				
				KeyValues *pWeapons = GetLoadout()->FindKey("Weapons");
				if( pWeapons )
				{
					KeyValues *pMercenary = pWeapons->FindKey("mercenary");
					if( pMercenary )
					{
						if( !Q_stricmp(pMercenary->GetString(VarArgs("%d", i+1 ) ), GetItemSchema()->GetWeapon(y)->GetName()) )
						{
							pTemp->SetSelected(true);
							
							switch( i )
							{
								case 0:
								pPrimaryToggle->pImage->SetImage( GetItemSchema()->GetWeapon(y)->GetString("backpack_icon", "..\backpack\blocked") );
								break;
								case 1:
								pSecondaryToggle->pImage->SetImage( GetItemSchema()->GetWeapon(y)->GetString("backpack_icon", "..\backpack\blocked") );
								break;
								case 2:
								pMeleeToggle->pImage->SetImage( GetItemSchema()->GetWeapon(y)->GetString("backpack_icon", "..\backpack\blocked") );
								break;								
							}
						}
					}
				}
				kvImageTemp->SetString( "image", GetItemSchema()->GetWeapon(y)->GetString( "backpack_icon", "..\backpack\blocked" ) );
				CTFImagePanel *pTempImage = new CTFImagePanel( pTemp, "WeaponImage" );
				pTempImage->ApplySettings( kvImageTemp );
				
				pWeaponList[i]->AddItem( pTemp );
			}
		}
	}
}

//=============================================================================
void DMLoadout::OnCommand(const char *command)
{
	if (Q_stricmp("Back", command) == 0)
	{
		// OnApplyChanges();
		OnKeyCodePressed(KEY_XBUTTON_B);
	}
	else if (Q_stricmp("Cancel", command) == 0)
	{
		OnKeyCodePressed(KEY_XBUTTON_B);
	}
	else if (Q_strncmp(command, "loadout_equip", strlen("loadout_equip")) == 0)
	{
		GetClassModel()->SetLoadoutCosmetics();
	}
	else if (Q_strncmp(command, "loadout_unequip", strlen("loadout_unequip")) == 0)
	{
		GetClassModel()->SetLoadoutCosmetics();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void DMLoadout::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pClassModel = dynamic_cast<vgui::DMModelPanel*>(FindChildByName("classmodelpanel"));
	m_bControlsLoaded = true;
	
	// required for new style
	SetPaintBackgroundEnabled(true);
	// SetupAsDialogStyle();
	
	if( !pVisualPanel )
		return;
}

void DMLoadout::PerformLayout()
{	
	BaseClass::PerformLayout();

	GetAnimationController()->StartAnimationSequence( this, "LoadoutPopup" );
	
//	GetAnimationController()->StartAnimationSequence("LoadoutPopup");
}

#define QUICK_CVAR(x) ConVar x(#x, "0", FCVAR_NONE);
QUICK_CVAR(of_bodygroup)
QUICK_CVAR(of_bodygroup_value)

void DMLoadout::PaintBackground()
{
	BaseClass::PaintBackground();

}

void DMLoadout::SelectWeapon( int iSlot, const char *szWeapon, bool bChangeSelection )
{
	vgui::Panel *pPanel = NULL;
	switch( iSlot )
	{
		case 1:
		pPanel = GetArsenalPanel()->FindChildByName("PrimaryToggle");
		break;
		case 2:
		pPanel = GetArsenalPanel()->FindChildByName("SecondaryToggle");
		break;
		case 3:
		pPanel = GetArsenalPanel()->FindChildByName("MeleeToggle");
		break;
	}
	
	if( pPanel )
	{
		CTFImagePanel *pImage = dynamic_cast<CTFImagePanel*>( pPanel->FindChildByName("Image"));
		if( pImage )
		{
			KeyValues *pWeapon = GetWeaponFromSchema(szWeapon);
			if( pWeapon )
				pImage->SetImage( pWeapon->GetString("backpack_icon", "..\backpack\blocked") );
		}
	}
	
	if( bChangeSelection )
	{
		CTFCommandButton *pWeapon = dynamic_cast<CTFCommandButton*>(pWeaponList[iSlot-1]->FindChildByName(szWeapon));
		if( pWeapon )
		{
			pWeapon->SetSelected(true);
		}
	}
}