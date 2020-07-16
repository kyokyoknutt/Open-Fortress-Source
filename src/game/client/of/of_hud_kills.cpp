//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Kill counter for DM mode
//
//=============================================================================//

#include "cbase.h"
#include <vgui/ILocalize.h>
#include "iclientmode.h"
#include "tf_controls.h"
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"
#include "of_hud_kills.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar fraglimit;

ConVar of_disablekillcount( "of_disablekillcount", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Disable the HUD kill counter." );

DECLARE_HUDELEMENT( CTFHudKills );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudKills::CTFHudKills( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudKills" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH );
	
	bBottomVisible = true;

	m_nKills	= 0;
	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudKills::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudKills::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudKills.res" );

	m_pKills = dynamic_cast<CExLabel *>( FindChildByName( "Kills" ) );
	m_pKillsShadow = dynamic_cast<CExLabel *>( FindChildByName( "KillsShadow" ) );

	m_pAvatar = dynamic_cast<CAvatarImagePanel *>( FindChildByName("AvatarImage") );
	m_pLeadAvatar = dynamic_cast<CAvatarImagePanel *>( FindChildByName("TopAvatarImage") );	
	
	m_nKills	= -1;
	m_flNextThink = 0.0f;

	UpdateKillLabel( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudKills::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer || of_disablekillcount.GetBool() )
	{
		return false;
	}
	if (TFGameRules() &&
		!TFGameRules()->IsTeamplay() &&
		!TFGameRules()->IsArenaGamemode() &&
		!TFGameRules()->IsCoopEnabled() &&
		( ( TFGameRules()->IsDMGamemode() && !TFGameRules()->DontCountKills() ) 
		|| TFGameRules()->IsGGGamemode()) )
		return CHudElement::ShouldDraw();
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudKills::UpdateKillLabel( bool bKills )
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
// Purpose: Get the local player's kills and display them
//-----------------------------------------------------------------------------
void CTFHudKills::OnThink()
{
	// Get the player
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );
	if ( m_flNextThink < gpGlobals->curtime )
	{
		if ( !pPlayer )
		{
			// turn off our kill counts
			UpdateKillLabel( false );

			m_nKills = 0;
		}
		else
		{
			wchar_t string1[1024];
			wchar_t wzMaxLevel[128];
			wchar_t wzFragLimit[128];
			
			if ( TFGameRules() && TFGameRules()->IsGGGamemode() )
			{
				SetDialogVariable( "FragLimit", TFGameRules()->m_iMaxLevel  );
				_snwprintf( wzMaxLevel, ARRAYSIZE( wzMaxLevel ), L"%i", TFGameRules()->m_iMaxLevel );
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find( "#TF_ScoreBoard_LevelLimit" ), 1, wzMaxLevel );
			}
			else
			{
				SetDialogVariable( "FragLimit", fraglimit.GetInt()  );
				_snwprintf( wzFragLimit, ARRAYSIZE( wzFragLimit ), L"%i", fraglimit.GetInt() );
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Fraglimit" ), 1, wzFragLimit );
			}
			SetDialogVariable( "PlayingToLabel", string1 );
			
			bool bIsGG = TFGameRules() && TFGameRules()->IsGGGamemode();
			int iIndex = GetLocalPlayerIndex();
			int nKills = !bIsGG ? tf_PR->GetPlayerScore( iIndex ) : tf_PR->GetGGLevel( iIndex );
		
			int iTopKills = 0;
			int iTopIndex = 0;
			for ( int i = 1; i <= MAX_PLAYERS; i++ )
			{
				if ( g_PR->IsConnected( i ) )
				{
					int nTmpKills = !bIsGG ? tf_PR->GetPlayerScore( i ) : tf_PR->GetGGLevel( i );
					if ( i != iIndex && nTmpKills >= iTopKills )
					{
						iTopKills = nTmpKills;
						iTopIndex = i;
					}
				}
			}
			
			bool bLocalTop = iTopKills <= nKills;
			
			m_nKills = nKills;
			
			if( m_pAvatar )
			{
				m_pAvatar->SetPlayer( bLocalTop ? UTIL_PlayerByIndex(iTopIndex) : pPlayer );
				m_pAvatar->SetShouldDrawFriendIcon( false );
			}
			if( m_pLeadAvatar )
			{
				m_pLeadAvatar->SetPlayer( bLocalTop ? pPlayer : UTIL_PlayerByIndex(iTopIndex) );
				m_pLeadAvatar->SetShouldDrawFriendIcon( false );
			}
			
			char szTmp[64];
			char const *szPlacement = bLocalTop ? "top" : "";
			
			Q_snprintf( szTmp, sizeof( szTmp ), "%splayername", szPlacement );
			
			SetDialogVariable( szTmp, g_PR->GetPlayerName( pPlayer->entindex() ) );
			
			UpdateKillLabel( true );
			Q_snprintf( szTmp, sizeof( szTmp ), "%skills", szPlacement );
			SetDialogVariable( szTmp, m_nKills );
			
			if( iTopIndex != 0 )
			{
				ShowBottom( true );

				szPlacement = bLocalTop ? "" : "top";
				Q_snprintf( szTmp, sizeof( szTmp ), "%splayername", szPlacement );
				SetDialogVariable( szTmp, g_PR->GetPlayerName( iTopIndex ) );
				
				Q_snprintf( szTmp, sizeof( szTmp ), "%sKills", szPlacement );
				SetDialogVariable( szTmp, iTopKills );
			}
			else
				ShowBottom( false );
		}
		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}

void CTFHudKills::ShowBottom( bool bShow )
{
	if( bBottomVisible == bShow )
		return;

	ImagePanel *m_pBGImage = dynamic_cast<ImagePanel *>( FindChildByName("MainBG") );
	if( m_pBGImage )
	{
		int w,h;
		m_pBGImage->GetSize( w, h );
		int iAdj = bShow ? -50 : 50;
		m_pBGImage->SetSize( w, h - iAdj );
	}
	int x, y;
	GetPos( x, y );
	int iPosAdj = bShow ? 50 : -50;
	SetPos( x, y - iPosAdj );
	
	CExLabel *m_pPlayerPlayingToLabel = dynamic_cast<CExLabel *>( FindChildByName( "PlayingToLabel" ) );
	if( m_pPlayerPlayingToLabel )
	{
		int labelX, labelY;
		m_pPlayerPlayingToLabel->GetPos( labelX, labelY );
		int iAdj = bShow ? 50 : -50;	
		m_pPlayerPlayingToLabel->SetPos( labelX, labelY + iAdj );
	}
	
	CExLabel *m_pPlayerNameLabel = dynamic_cast<CExLabel *>( FindChildByName( "PlayerNameLabel" ) );
	CExLabel *m_pTmpKills = dynamic_cast<CExLabel *>( FindChildByName( "KillsLabel" ) );
	ImagePanel *m_pShadedBox = dynamic_cast<ImagePanel *>( FindChildByName("ShadedBarP2") );	
	
	bBottomVisible = bShow;
	
	if( m_pAvatar )
		m_pAvatar->SetVisible( bShow );
	if( m_pPlayerNameLabel )
		m_pPlayerNameLabel->SetVisible( bShow );
	if( m_pTmpKills )
		m_pTmpKills->SetVisible( bShow );
	if( m_pShadedBox )
		m_pShadedBox->SetVisible( bShow );
}