//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Music Player, used for Handling Music in Maps
//
//=============================================================================//

#include "cbase.h"
#include "c_of_music_player.h"
#include "c_tf_player.h"
#include "tf_gamerules.h"
#include "of_shared_schemas.h"
#include "fmod_manager.h"

// Imported from sounds	
#include "stringregistry.h"
#include <ctype.h>
#include "igamesystem.h"
#include "KeyValues.h"
#include "filesystem.h"

// Hud stuff
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include "tf_controls.h"
#include "tf_imagepanel.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "tier0/memdbgon.h"

extern System *pSystem;

// Inputs.
LINK_ENTITY_TO_CLASS( of_music_player, C_TFMusicPlayer );

IMPLEMENT_CLIENTCLASS_DT( C_TFMusicPlayer, DT_MusicPlayer, CTFMusicPlayer )
	RecvPropString( RECVINFO( szLoopingSong ) ),
	RecvPropBool( RECVINFO( m_bShouldBePlaying ) ),
	RecvPropBool( RECVINFO( m_bHardTransition ) ),
	RecvPropFloat( RECVINFO( m_flDelay ) ),
	RecvPropFloat( RECVINFO( m_flVolume ) ),
END_RECV_TABLE()

C_TFMusicPlayer::C_TFMusicPlayer()
{
	bIsPlaying = false;
	bInLoop = false;
	bParsed = false;
	m_flDelay = 0;
	m_iPhase = 0;
	m_Songdata.SetSize( 3 );
	if( pSystem )
		pSystem->createChannelGroup("Parent", &pChannel);
}

C_TFMusicPlayer::~C_TFMusicPlayer()
{
	bIsPlaying = false;
	bInLoop = false;
	bParsed = false;
	m_iPhase = 0;
	m_Songdata.Purge();
	FMODManager()->StopAmbientSound( pChannel, false );
}

void C_TFMusicPlayer::Spawn( void )
{	
	BaseClass::Spawn();
	ClientThink();
}

void C_TFMusicPlayer::ClientThink( void )
{
// Called every frame when the client is in-game

	if ( !bParsed )
	{
		SetNextClientThink( gpGlobals->curtime + 0.1f );
		return;
	}
	
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
	{
		SetNextClientThink( gpGlobals->curtime + 0.1f );
		return;
	}
	if ( pLocalPlayer->m_Shared.InState( TF_STATE_WELCOME ) )
	{
		SetNextClientThink( gpGlobals->curtime + 0.1f );
		return;
	}

	if ( bIsPlaying != m_bShouldBePlaying )
	{
		if ( !m_bShouldBePlaying )
		{
			bIsPlaying = false;
			m_iPhase = 0;
			FMODManager()->StopAmbientSound( pChannel, false );
			if( m_bHardTransition )
			{
				FMODManager()->PlayMusicEnd( pChannel, m_Songdata[2].path );
			}
		}
		else
		{
			FMODManager()->StopAmbientSound( pChannel, false );
			bIsPlaying = true;
			bInLoop = false;
			if ( m_Songdata[0].path[0] != 0 )
			{
				FMODManager()->PlayLoopingMusic( pChannel, m_Songdata[1].path, m_Songdata[0].path, m_flDelay );
			}
			else
			{
				FMODManager()->PlayLoopingMusic( pChannel,m_Songdata[1].path, NULL , m_flDelay );
			}
		}
	}

	HandleVolume();
	
	SetNextClientThink( gpGlobals->curtime + 0.1f );
}

static const ConVar *snd_musicvolume = NULL;
static const ConVar *snd_mute_losefocus = NULL;

extern ConVar of_winscreenratio;

void C_TFMusicPlayer::HandleVolume( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	snd_musicvolume = g_pCVar->FindVar("snd_musicvolume");
	snd_mute_losefocus = g_pCVar->FindVar("snd_mute_losefocus");

	float flVolumeMod = 1.0f;
	
	if (pLocalPlayer && !pLocalPlayer->IsAlive())
		flVolumeMod *= 0.4f;

	if (!engine->IsActiveApp() && snd_mute_losefocus->GetBool())
		flVolumeMod = 0.0f;
	
	if( TeamplayRoundBasedRules()->RoundHasBeenWon() )
		flVolumeMod *= ( TeamplayRoundBasedRules()->m_flStartedWinState + TeamplayRoundBasedRules()->GetBonusRoundTime() - gpGlobals->curtime ) / ( TeamplayRoundBasedRules()->GetBonusRoundTime() * of_winscreenratio.GetFloat() );
		
	if( pChannel )
		pChannel->setVolume(m_flVolume * snd_musicvolume->GetFloat() * flVolumeMod);
}

void C_TFMusicPlayer::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
	if ( updateType == DATA_UPDATE_CREATED )
	{
		if ( bParsed )
			return;
		
		char szScriptName[MAX_PATH];
		Q_strncpy( szScriptName, szLoopingSong, sizeof( szScriptName ) );
		
		KeyValues *pSound = GetSoundscript( szScriptName );
		
		if ( pSound )
		{
			Q_strncpy( m_Songdata[1].name, pSound->GetString( "Name", "Unknown" ) , sizeof( m_Songdata[1].name ) );
			Q_strncpy( m_Songdata[1].artist, pSound->GetString( "Artist", "Unknown" ) , sizeof( m_Songdata[1].artist ) );
			Q_strncpy( m_Songdata[1].path, pSound->GetString( "wave" ) , sizeof( m_Songdata[1].path ) );
			m_Songdata[1].volume = pSound->GetFloat( "volume", 0.9f );
			const char *pszSrc = NULL;
			if ( !Q_strncmp( m_Songdata[1].path, "#", 1 ) )
			{
				pszSrc = m_Songdata[1].path + 1;
				Q_strncpy( m_Songdata[1].path, pszSrc, sizeof(m_Songdata[1].path) );
			}
			DevMsg("Intro is %s\nOutro is %s\n", pSound->GetString( "intro" ), pSound->GetString( "outro" ));
			KeyValues *pSoundIntro = GetSoundscript( pSound->GetString( "intro" ) );
			if( pSoundIntro )
			{
				Q_strncpy( m_Songdata[0].path, pSoundIntro->GetString( "wave" ) , sizeof( m_Songdata[0].path ) );
				if ( !Q_strncmp( m_Songdata[0].path, "#", 1 ) )
				{
					pszSrc = m_Songdata[0].path + 1;
					Q_strncpy( m_Songdata[0].path, pszSrc, sizeof(m_Songdata[0].path) );
				}
				DevMsg("Intro wav is %s\n", m_Songdata[0].path);
			}
			KeyValues *pSoundOutro = GetSoundscript( pSound->GetString( "outro" ) );
			if( pSoundOutro )
			{
				Q_strncpy( m_Songdata[2].path, pSoundOutro->GetString( "wave" ) , sizeof( m_Songdata[2].path ) );
				if ( !Q_strncmp( m_Songdata[2].path, "#", 1 ) )
				{
					pszSrc = m_Songdata[2].path + 1;
					Q_strncpy( m_Songdata[2].path, pszSrc, sizeof(m_Songdata[2].path) );
				}
				DevMsg("Outro wav is %s\n", m_Songdata[2].path);
			}
		}

		bParsed = true;			
	}
}

static ConCommand fm_parse_soundmanifest( "fm_parse_soundmanifest", ParseSoundManifest, "Parses the global Sounscript File.", FCVAR_NONE );
static ConCommand fm_check_globalsoundmanifest( "fm_check_globalsoundmanifest", CheckGlobalSounManifest, "Prints out all Manifest files.", FCVAR_NONE );
static ConCommand fm_parse_level_sounds( "fm_parse_level_sounds", ParseLevelSoundManifest, "Parses the level_souns file for the current map.", FCVAR_NONE );

// Inputs.
LINK_ENTITY_TO_CLASS( dm_music_manager, C_TFDMMusicManager );

IMPLEMENT_CLIENTCLASS_DT( C_TFDMMusicManager, DT_DMMusicManager, CTFDMMusicManager )
	RecvPropString( RECVINFO( szWaitingForPlayerMusic ) ),
	RecvPropString( RECVINFO( szRoundMusic ) ),
	RecvPropString( RECVINFO( szWaitingMusicPlayer ) ),
	RecvPropString( RECVINFO( szRoundMusicPlayer ) ),	
	RecvPropInt( RECVINFO( m_iIndex ) ),
	RecvPropEHandle( RECVINFO( pWaitingMusicPlayer ) ),
	RecvPropEHandle( RECVINFO( pRoundMusicPlayer ) ),
END_RECV_TABLE()

C_TFDMMusicManager *gDMMusicManager;
C_TFDMMusicManager* DMMusicManager()
{
	return gDMMusicManager;
}

C_TFDMMusicManager::C_TFDMMusicManager()
{
	gDMMusicManager = this;
}

C_TFDMMusicManager::~C_TFDMMusicManager()
{
	gDMMusicManager = NULL;
}

using namespace vgui;

DECLARE_HUDELEMENT( CTFHudNowPlaying );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudNowPlaying::CTFHudNowPlaying( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudNowPlaying" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH );
	
	ListenForGameEvent("teamplay_round_start");
	
	m_pNameContainer = new EditablePanel( this, "MusicNameContainer" );
	m_pNameBG = new CTFImagePanel( m_pNameContainer, "MusicNameBG" );
	m_pNameLabel = new CExLabel( m_pNameContainer, "MusicNameLabel", "" );
	
	m_pArtistContainer = new EditablePanel( this, "ArtistNameContainer" );
	m_pArtistBG = new CTFImagePanel( m_pArtistContainer, "ArtistNameBG" );
	m_pArtistLabel = new CExLabel( m_pArtistContainer, "ArtistNameLabel", "" );

	flDrawTime = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudNowPlaying::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudNowPlaying.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudNowPlaying::ShouldDraw( void )
{
	if( flDrawTime - 9.9 > gpGlobals->curtime )
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "NowPlaying");
	
	if( TeamplayRoundBasedRules() && !TeamplayRoundBasedRules()->IsInWaitingForPlayers()
		&& flDrawTime > gpGlobals->curtime && TeamplayRoundBasedRules()->InRoundRestart() )
		return CHudElement::ShouldDraw();
	else
		return false;
}

void CTFHudNowPlaying::OnThink()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudNowPlaying::FireGameEvent(IGameEvent * event)
{
	snd_musicvolume = g_pCVar->FindVar("snd_musicvolume");
	
	if( snd_musicvolume->GetFloat() <= 0.0f )
		return;

	if( !DMMusicManager() )
		return;

	KeyValues *pSoundscript = GetSoundscript( DMMusicManager()->szRoundMusic );
	if( !pSoundscript )
		return;

	const char *szSongName = pSoundscript->GetString("Name", "");

	bool bLongName = false;
	
	if( szSongName[0] == '\0' )
	{
		szSongName = pSoundscript->GetName();
		bLongName = true;
	}

	static char temp[128];
	V_strncpy( temp, szSongName, sizeof( temp ) );	
	
	if( bLongName )
	{
		bool bFound = false;
		// strip certain prefixes from the Name
		const char *prefix[] = { "DeathmatchMusic." };
		for ( int i = 0; i< ARRAYSIZE( prefix ); i++ )
		{
			// if prefix matches, advance the string pointer past the prefix
			int len = Q_strlen( prefix[i] );
			if ( strncmp( temp, prefix[i], len ) == 0 )
			{
				szSongName = temp;
				szSongName += len;
				bFound = true;
				break;
			}
		}

		if( !bFound || Q_strlen( szSongName ) > 19 )
			return;
	}

	m_pNameContainer->SetDialogVariable( "SongName", szSongName );

	int textLen = 0;
	
	wchar_t szUnicode[512];

	wchar_t wszSongName[128];
	g_pVGuiLocalize->ConvertANSIToUnicode( szSongName, wszSongName, sizeof( wszSongName ) );
	
	g_pVGuiLocalize->ConstructString( szUnicode, sizeof( szUnicode ), g_pVGuiLocalize->Find( "#OF_SongName" ), 1, wszSongName );

	int len = wcslen( szUnicode );
	for ( int i=0;i<len;i++ )
		textLen += surface()->GetCharacterWidth( m_pNameLabel->GetFont(), szUnicode[i] );

	int w,h;
	m_pNameLabel->GetSize( w,h );
	m_pNameLabel->SetSize( textLen, h );
	
	m_pNameLabel->SetText(szUnicode);

	textLen += XRES(24); // Padding

	m_pNameBG->GetSize( w,h );
	m_pNameBG->SetSize( textLen, h );

	m_pNameContainer->GetSize( w,h );
	m_pNameContainer->SetSize( textLen, h );
	
	const char *szArtistName = pSoundscript->GetString("Artist", "");

	if( szArtistName[0] == '\0' )
	{
		m_pArtistContainer->SetVisible( false );
	}
	else
	{
		m_pArtistContainer->SetVisible( true );
		m_pArtistContainer->SetDialogVariable( "ArtistName", szArtistName );
		
		DevMsg( "%s\n", szArtistName );
		
		textLen = 0;
		
		wchar_t wszArtistName[128];
		g_pVGuiLocalize->ConvertANSIToUnicode( szArtistName, wszArtistName, sizeof( wszArtistName ) );
		g_pVGuiLocalize->ConstructString( szUnicode, sizeof( szUnicode ), g_pVGuiLocalize->Find( "#OF_ArtistName" ), 1, wszArtistName );

		len = wcslen( szUnicode );
		for ( int i=0;i<len;i++ )
			textLen += surface()->GetCharacterWidth( m_pArtistLabel->GetFont(), szUnicode[i] );
		
		textLen += XRES(14); // Padding

		m_pArtistLabel->GetSize( w,h );

		m_pArtistLabel->SetText(szUnicode);

		m_pArtistBG->GetSize( w,h );
		m_pArtistBG->SetSize( textLen, h );

		m_pArtistContainer->GetSize( w,h );
		m_pArtistContainer->SetSize( textLen, h );
	}

	flDrawTime = gpGlobals->curtime + 10.0f;
}