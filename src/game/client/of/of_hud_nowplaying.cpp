//========= Open Fortress Micro License 2020, Open Fortress Team , All rights reserved. ============
//
// Purpose: Now Playing Hud element, displays which round music is playing on round start
//
//=============================================================================//

#include "cbase.h"
#include "tf_gamerules.h"
#include "of_hud_nowplaying.h"
#include "c_of_music_player.h"
#include "of_shared_schemas.h"

// Hud stuff
#include "iclientmode.h"
#include "vgui_controls/AnimationController.h"
#include "tf_controls.h"
#include "tf_imagepanel.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

#include "tier0/memdbgon.h"

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

static const ConVar *snd_musicvolume = NULL;

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