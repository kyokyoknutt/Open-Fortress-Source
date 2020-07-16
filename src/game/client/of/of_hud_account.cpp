//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "tf_controls.h"
#include "ihudlcd.h"
#include "of_hud_account.h"
#include "tf_gamerules.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ofb_disablemoneycount( "ofb_disablemoneycount", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Disable moneycount showing in your HUD" );
ConVar ofb_forcemoneycount( "ofb_forcemoneycount", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Force the Money Count on" );

DECLARE_HUDELEMENT( CTFHudMoney );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudMoney::CTFHudMoney( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudMoney" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	hudlcd->SetGlobalStat( "(money)", "0" );

	m_nMoney	= 0;
	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMoney::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMoney::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudMoney.res" );

	m_pMoney = dynamic_cast<CExLabel *>( FindChildByName( "Money" ) );
	m_pMoneyShadow = dynamic_cast<CExLabel *>( FindChildByName( "MoneyShadow" ) );

	m_nMoney	= -1;
	m_flNextThink = 0.0f;

	UpdateMoneyLabel( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudMoney::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || ( TFGameRules() && !TFGameRules()->UsesMoney() && !ofb_forcemoneycount.GetBool() ) )
	{
		return false;
	}
	return CHudElement::ShouldDraw();;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudMoney::UpdateMoneyLabel( bool bMoney )
{
	if ( m_pMoney && m_pMoneyShadow )
	{
		if ( m_pMoney->IsVisible() != bMoney )
		{
			m_pMoney->SetVisible( bMoney );
			m_pMoneyShadow->SetVisible( bMoney );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Get ammo info from the weapon and update the displays.
//-----------------------------------------------------------------------------
void CTFHudMoney::OnThink()
{
	// Get the player and active weapon.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
//	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );
	
	if ( m_flNextThink < gpGlobals->curtime )
	{
		if ( !pPlayer )
		{
			hudlcd->SetGlobalStat( "(money)", "n/a" );

			// turn off our ammo counts
			UpdateMoneyLabel( false );

			m_nMoney = 0;
		}
		else
		{
			// Get the ammo in our clip.
			int nMoney = (int)pPlayer->GetAccount();
			
			hudlcd->SetGlobalStat( "(money)", VarArgs( "%d $", nMoney ) );
			m_nMoney = nMoney;
			
			UpdateMoneyLabel( true );
			SetDialogVariable( "Money", VarArgs( "$ %d",m_nMoney ) );
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}
