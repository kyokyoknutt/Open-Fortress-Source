//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include "videobackground.h"

#include "ienginevgui.h"
#include <vgui/ISurface.h>
#include "filesystem.h"

#include "tier0/dbg.h"

using namespace vgui;

extern IFileSystem *filesystem;

#define DEFAULT_RATIO_WIDE 1920.0 / 1080.0
#define DEFAULT_RATIO 1024.0 / 768.0

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CVideoBackground::CVideoBackground(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	m_pVideo = new CTFVideoPanel( this, "VideoPanel" );
}

void CVideoBackground::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings(pScheme);

	if ( m_pVideo )
	{
		int width, height;
		surface()->GetScreenSize( width, height );

		float fRatio = ( float )width / ( float )height;
		bool bWidescreen = ( fRatio < 1.5 ? false : true );

		GetRandomVideo( m_szVideoFile, sizeof( m_szVideoFile ), bWidescreen );

		float iRatio = ( bWidescreen ? DEFAULT_RATIO_WIDE : DEFAULT_RATIO );
		int iWide = ( float )height * iRatio + 4;
		m_pVideo->SetBounds( -1, -1, iWide, iWide );

		VideoReplay();
	}
}

void CVideoBackground::VideoReplay()
{
	if ( !m_pVideo )
		return;

	if ( IsVisible() && m_szVideoFile[ 0 ] != '\0' )
	{
		m_pVideo->Activate();
		m_pVideo->BeginPlaybackNoAudio( m_szVideoFile );
	}
	else
	{
		m_pVideo->Shutdown();
	}
}

void CVideoBackground::GetRandomVideo( char *pszBuf, int iBufLength, bool bWidescreen )
{
	char szPath[ MAX_PATH ];

	const char *pszFormat = bWidescreen ? "media/bg_%02d_widescreen.bik" : "media/bg_%02d.bik";

	// See how many videos there are.
	int iLastVideo = 0;
	do
	{
		V_snprintf( szPath, sizeof( szPath ), pszFormat, ++iLastVideo );
	} while ( filesystem->FileExists( szPath ) );

	if ( iLastVideo == 1 )
	{
		// No video files present, bail.
		Assert( false );
		pszBuf[ 0 ] = '\0';
		return;
	}

	V_snprintf( pszBuf, iBufLength, pszFormat, RandomInt( 1, iLastVideo - 1 ) );
}
