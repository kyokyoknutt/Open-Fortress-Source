//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "of_music_player.h"
#include "tf_player.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "engine/IEngineSound.h"
#include "entitylist.h"
#include "KeyValues.h"
#include "filesystem.h"

#include "tier0/memdbgon.h"

BEGIN_DATADESC(CTFMusicPlayer)
// Inputs.
DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
DEFINE_KEYFIELD( szLoopingSong , FIELD_SOUNDNAME, "LoopingSongName"), // Darude Sandstorm
DEFINE_KEYFIELD( m_flVolume, FIELD_FLOAT, "Volume" ),
DEFINE_KEYFIELD( m_iIndex, FIELD_INTEGER, "Index" ),
// Inputs.
DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),
DEFINE_INPUTFUNC( FIELD_FLOAT, "SetVolume", InputSetVolume ),
DEFINE_INPUTFUNC( FIELD_FLOAT, "AddVolume", InputAddVolume ),
DEFINE_THINKFUNC( MusicThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CTFMusicPlayer, DT_MusicPlayer )
	SendPropStringT( SENDINFO( szLoopingSong ) ),
	SendPropBool( SENDINFO( m_bShouldBePlaying ) ),
	SendPropBool( SENDINFO( m_bHardTransition ) ),
	SendPropFloat( SENDINFO( m_flDelay ) ),
	SendPropFloat( SENDINFO( m_flVolume ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( of_music_player, CTFMusicPlayer );

CTFDMMusicManager *gDMMusicManager;
CTFDMMusicManager* DMMusicManager()
{
	return gDMMusicManager;
}

CTFMusicPlayer::CTFMusicPlayer()
{
	m_flDelay = 0.0f;
	SetThink( &CTFMusicPlayer::MusicThink );
	SetNextThink( gpGlobals->curtime );
	m_flVolume = 1.0f;
	m_bHardTransition = false;
}

void CTFMusicPlayer::MusicThink( void )
{
	m_bShouldBePlaying = ShouldPlay();
	SetNextThink( gpGlobals->curtime + 0.1 );
}

bool CTFMusicPlayer::ShouldPlay( void )
{
	if ( IsDisabled() )
		return false;	
	
	if( TFGameRules() && TFGameRules()->IsInfGamemode() && !m_bInfection )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicPlayer::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicPlayer::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMusicPlayer::IsDisabled( void )
{
	return m_bDisabled;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicPlayer::SetDisabled( bool bDisabled )
{
	m_bDisabled = bDisabled;
//	if( !m_bDisabled )
//		m_bHardTransition = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicPlayer::InputToggle( inputdata_t &inputdata )
{
	m_bDisabled = !m_bDisabled;
//	if( !m_bDisabled )
//		m_bHardTransition = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicPlayer::InputSetVolume( inputdata_t &inputdata )
{
	m_flVolume = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMusicPlayer::InputAddVolume( inputdata_t &inputdata )
{
	m_flVolume += inputdata.value.Float();
	
	if( m_flVolume > 1.0f )
		m_flVolume = 1.0f;
	else if( m_flVolume < 0.0f )
		m_flVolume = 0.0f;
}

void CTFMusicPlayer::EndTransition( void )
{
//	m_bHardTransition = true;
}

BEGIN_DATADESC(CTFDMMusicManager)
// Keyfields.
DEFINE_KEYFIELD( szWaitingForPlayerMusic , FIELD_SOUNDNAME, "WaitingForPlayerMusic"),
DEFINE_KEYFIELD( szRoundMusic , FIELD_SOUNDNAME, "RoundMusic"),
// Optional
DEFINE_KEYFIELD( szWaitingMusicPlayer , FIELD_SOUNDNAME, "WaitingMusicPlayer"),
DEFINE_KEYFIELD( szRoundMusicPlayer , FIELD_SOUNDNAME, "RoundMusicPlayer"),
// Inputs.
DEFINE_THINKFUNC( DMMusicThink ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CTFDMMusicManager, DT_DMMusicManager )
	SendPropStringT( SENDINFO( szWaitingForPlayerMusic ) ),
	SendPropStringT( SENDINFO( szRoundMusic ) ),
	SendPropStringT( SENDINFO( szWaitingMusicPlayer ) ),
	SendPropStringT( SENDINFO( szRoundMusicPlayer ) ),	
	SendPropInt( SENDINFO( m_iIndex ) ),
	
	SendPropEHandle( SENDINFO( pWaitingMusicPlayer ) ),
	SendPropEHandle( SENDINFO( pRoundMusicPlayer ) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( dm_music_manager, CTFDMMusicManager );

CTFDMMusicManager::CTFDMMusicManager()
{
	szWaitingForPlayerMusic = MAKE_STRING("");
	szRoundMusic = MAKE_STRING("");
	SetThink( &CTFDMMusicManager::DMMusicThink );
	SetNextThink( gpGlobals->curtime );
	m_bDisableThink = false;
	gDMMusicManager = this;
}

CTFDMMusicManager::~CTFDMMusicManager()
{
	UTIL_Remove( pWaitingMusicPlayer );
	UTIL_Remove( pRoundMusicPlayer );
	gDMMusicManager = NULL;
}

extern KeyValues *GetSoundscript( const char *szSoundScript );

void CTFDMMusicManager::Spawn( void )
{
	BaseClass::Spawn();
	
	if (szRoundMusicPlayer.Get() != MAKE_STRING("") && gEntList.FindEntityByName(NULL, szRoundMusicPlayer))
	{
		pRoundMusicPlayer = dynamic_cast<CTFMusicPlayer *>(gEntList.FindEntityByName( NULL, szRoundMusicPlayer ));
		if( szRoundMusic.Get() != MAKE_STRING("") )
			pRoundMusicPlayer->szLoopingSong = szRoundMusic;
	}
	else if (szRoundMusic.Get() != MAKE_STRING(""))
	{
		pRoundMusicPlayer = (CTFMusicPlayer *)CBaseEntity::CreateNoSpawn( "of_music_player", GetAbsOrigin() , vec3_angle );
		pRoundMusicPlayer->szLoopingSong = szRoundMusic;
		pRoundMusicPlayer->Spawn();
	}
	if (szWaitingMusicPlayer.Get() != MAKE_STRING(""))
	{
		pWaitingMusicPlayer = dynamic_cast<CTFMusicPlayer *>(gEntList.FindEntityByName( NULL, szWaitingMusicPlayer ));
		if (szWaitingForPlayerMusic.Get() != MAKE_STRING(""))
			pWaitingMusicPlayer->szLoopingSong = szWaitingForPlayerMusic;
	}
	else if (szWaitingForPlayerMusic.Get() != MAKE_STRING(""))
	{
		pWaitingMusicPlayer = (CTFMusicPlayer *)CBaseEntity::CreateNoSpawn( "of_music_player", GetAbsOrigin() , vec3_angle );
		pWaitingMusicPlayer->szLoopingSong = szWaitingForPlayerMusic;
		pWaitingMusicPlayer->Spawn();
	}
	
	KeyValues *kvRoundMusic = GetSoundscript( STRING( szRoundMusic.GetForModify() ) );
	if( kvRoundMusic )
	{
		PrecacheScriptSound( kvRoundMusic->GetString("win") );
		PrecacheScriptSound( kvRoundMusic->GetString("loose") );
	}
}

void CTFDMMusicManager::DMMusicThink( void )
{
	if( !m_bDisableThink )
	{
		if ( TeamplayRoundBasedRules() && TeamplayRoundBasedRules()->IsInWaitingForPlayers() )
		{
			if( pWaitingMusicPlayer )
				pWaitingMusicPlayer->SetDisabled( false );
			if( pRoundMusicPlayer )
				pRoundMusicPlayer->SetDisabled( true );
			
			IGameEvent *event = gameeventmanager->CreateEvent( "music_round_start" );
			if ( event )
				gameeventmanager->FireEvent( event );
		}
		else
		{
			if( pWaitingMusicPlayer )
				pWaitingMusicPlayer->SetDisabled( true );
			if( pRoundMusicPlayer )
				pRoundMusicPlayer->SetDisabled( false );
		}
	}
	SetNextThink( gpGlobals->curtime + 0.1 );
}