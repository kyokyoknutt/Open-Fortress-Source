//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include <vgui/ILocalize.h>
#include "iclientmode.h"
#include "c_team.h"
#include "ihudlcd.h"
#include "of_hud_tdm.h"
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"

using namespace vgui;

extern ConVar fraglimit;
extern ConVar of_arena;
extern ConVar of_disablekillcount;

DECLARE_HUDELEMENT( CTFHudTDM );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudTDM::CTFHudTDM( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTDM" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH );

	hudlcd->SetGlobalStat( "(kills)", "0" );

	m_nKills	= 0;
	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTDM::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTDM::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudTDM.res" );

	m_pKills = dynamic_cast<CExLabel *>( FindChildByName( "Kills" ) );
	m_pKillsShadow = dynamic_cast<CExLabel *>( FindChildByName( "KillsShadow" ) );
	m_pProgressRed = dynamic_cast<CTFImageProgressBar *>( FindChildByName( "RedProgress" ) );
	m_pProgressBlu = dynamic_cast<CTFImageProgressBar *>( FindChildByName( "BluProgress" ) );

//	m_pProgressRed->SetParent(this);
//	m_pProgressBlu->SetParent(this);

	m_nKills	= -1;
	m_flNextThink = 0.0f;

	UpdateKillLabel( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudTDM::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || of_disablekillcount.GetBool() )
	{
		return false;
	}
	
	if (TFGameRules() &&
		TFGameRules()->IsTDMGamemode() &&
		!TFGameRules()->DontCountKills() )
		return CHudElement::ShouldDraw();
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudTDM::UpdateKillLabel( bool bKills )
{
	if ( m_pKills && m_pKillsShadow )
	{
		if ( m_pKills->IsVisible() != bKills )
		{
			m_pKills->SetVisible( bKills );
			m_pKillsShadow->SetVisible( bKills );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update the team frag meter in TDM
//-----------------------------------------------------------------------------
void CTFHudTDM::OnThink()
{
	// Get the player and active weapon.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );
	
	if ( m_flNextThink < gpGlobals->curtime )
	{
		if ( !pPlayer )
		{
			hudlcd->SetGlobalStat( "(kills)", "n/a" );

			// turn off our ammo counts
			UpdateKillLabel( false );

			m_nKills = 0;
		}
		else
		{
			// Get the ammo in our clip.
			int iIndex = GetLocalPlayerIndex();
			int nKills = tf_PR->GetPlayerScore( iIndex );
			int nGGLevel = tf_PR->GetGGLevel( iIndex );
			
			hudlcd->SetGlobalStat( "(kills)", VarArgs( "%d", nKills ) );
			if ( TFGameRules() && TFGameRules()->IsGGGamemode() )
				m_nKills = nGGLevel;
			else
				m_nKills = nKills;
			
			UpdateKillLabel( true );
			wchar_t string1[1024];

			C_Team *pRedTeam = GetGlobalTeam( TF_TEAM_RED );
			C_Team *pBluTeam = GetGlobalTeam( TF_TEAM_BLUE );

			if( m_pProgressRed )
			{
				m_pProgressRed->SetProgress( (float)(pRedTeam->Get_Score()) / (float)(fraglimit.GetInt()) );
				m_pProgressRed->Update();
			}
			if( m_pProgressBlu )
			{
				m_pProgressBlu->SetProgress( (float)(pBluTeam->Get_Score()) / (float)(fraglimit.GetInt()) );
				m_pProgressBlu->Update();
			}

			SetDialogVariable( "RedKills", pRedTeam->Get_Score() );
			SetDialogVariable( "BluKills", pBluTeam->Get_Score() );

			if ( TFGameRules() && TFGameRules()->IsGGGamemode() )
			{
				SetDialogVariable( "FragLimit", TFGameRules()->m_iMaxLevel  );
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find( "#TF_ScoreBoard_GGLevel" ), 1, 1 );
			}
			else
			{
				SetDialogVariable( "FragLimit", fraglimit.GetInt()  );
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Kills" ), 1, 1 );
			}
			SetDialogVariable( "killslabel", string1 );
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}