//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Kill counter for DM mode
//
//=============================================================================//

#include "cbase.h"
#include "iclientmode.h"
#include "filesystem.h"
#include "c_tf_player.h"
#include "of_hud_powerups.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CTFHudPowerups );

ConVar of_debug_powerup_progress( "of_debug_powerup_progress", "1", FCVAR_NONE );

KeyValues *kvPowerupTimer;

void ReparsePowerupTimer()
{
	kvPowerupTimer = new KeyValues("PowerupTimer");
	kvPowerupTimer->LoadFromFile( filesystem, "resource/UI/HudPowerupTimer.res", "MOD" );
	kvPowerupTimer = kvPowerupTimer->FindKey( "ProgressBar" );
}
static ConCommand hud_reloadpoweruptimer( "hud_reloadpoweruptimer", ReparsePowerupTimer, "Reparses the poweurp timer res file.", FCVAR_NONE );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudPowerups::CTFHudPowerups( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudPowerups" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	if( !kvPowerupTimer )
	{
		kvPowerupTimer = new KeyValues("PowerupTimer");
		kvPowerupTimer->LoadFromFile( filesystem, "resource/UI/HudPowerupTimer.res", "MOD" );
		kvPowerupTimer = kvPowerupTimer->FindKey( "ProgressBar" );
	}
	ListenForGameEvent( "add_powerup_timer" );
	SetHiddenBits( HIDEHUD_HEALTH );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPowerups::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudPowerups.res" );

}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudPowerups::FireGameEvent( IGameEvent * event )
{
	if( !event )
		return;
	const char *eventname = event->GetName();

	if ( Q_strcmp( "add_powerup_timer", eventname ) == 0 )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		if( !pLocalPlayer )
			return;

		if ( event->GetInt("player") != pLocalPlayer->entindex() )
			return;
		
		DevMsg("ProgressBar added for cond %d\n", event->GetInt( "cond", 0 ) );
		powerup_timer_t s_aPowerupTimer;
		s_aPowerupTimer.m_iCondID = event->GetInt( "cond", 0 );
		s_aPowerupTimer.m_flStartTime = gpGlobals->curtime;
		s_aPowerupTimer.m_pBar = new CircularProgressBar( this, "ProgressBar" );
		s_aPowerupTimer.m_pBar->ApplySettings( kvPowerupTimer );
		
		if( !strcmp( event->GetString( "icon" ), "" ) )
		{
			switch( s_aPowerupTimer.m_iCondID )
			{
				case TF_COND_SHIELD:
				case TF_COND_INVULNERABLE:
					s_aPowerupTimer.m_pBar->SetFgImage("../effects/powerup_resist_hud");
					break;
				case TF_COND_CRIT_POWERUP:
				case TF_COND_CRITBOOSTED:
					s_aPowerupTimer.m_pBar->SetFgImage("../effects/powerup_crit_hud");
					break;
				case TF_COND_HASTE:
					s_aPowerupTimer.m_pBar->SetFgImage("../effects/powerup_haste_hud");
					break;
				case TF_COND_BERSERK:
					s_aPowerupTimer.m_pBar->SetFgImage("../effects/powerup_knockout_hud");
					break;
				case TF_COND_INVIS_POWERUP:
				case TF_COND_STEALTHED:
					s_aPowerupTimer.m_pBar->SetFgImage("../effects/powerup_invis_hud");
					break;
				case TF_COND_JAUGGERNAUGHT: //change me
					s_aPowerupTimer.m_pBar->SetFgImage("../effects/powerup_knockout_hud");
					break;
			}
		}
		else
			s_aPowerupTimer.m_pBar->SetFgImage( event->GetString( "icon" ) );

		int x, y;
		s_aPowerupTimer.m_pBar->GetPos( x, y );
		s_aPowerupTimer.m_pBar->SetPos( x + ( m_iConditions.Count() * 50 ), y );
		
		m_iConditions.AddToTail( s_aPowerupTimer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudPowerups::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return false;
	return CHudElement::ShouldDraw();
}

void CTFHudPowerups::OnThink()
{
	// no thinks
	// brain empty
	
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;
	
	for( int i = 0; i < m_iConditions.Count(); i++ )
	{
		if( pPlayer->m_Shared.InCond( m_iConditions[i].m_iCondID ) )
		{
			float flRemoveTime = pPlayer->m_Shared.GetConditionDuration( m_iConditions[i].m_iCondID ) + gpGlobals->curtime;
			float flProgress = 1 - ( (flRemoveTime - gpGlobals->curtime) / (flRemoveTime - m_iConditions[i].m_flStartTime) );
			if( m_iConditions[i].m_pBar )
			{
				m_iConditions[i].m_pBar->SetProgress( flProgress );
			}
		}
		else
		{
			m_iConditions[i].m_pBar->SetVisible( false );
			m_iConditions[i].m_pBar->MarkForDeletion();
			m_iConditions.Remove( i );
		}
	}
}