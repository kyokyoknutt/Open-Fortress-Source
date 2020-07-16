//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//
#include "cbase.h"
#include "VLoadingProgress.h"
#include "vgui_controls/ProgressBar.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "fmtstr.h"
#include "filesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GAMEIU_MULTI_LOADSCREENS

using namespace vgui;
using namespace BaseModUI;

static bool IsAvatarFemale( int iAvatar )
{
	return ( iAvatar == 1 ); // Zoey or Producer
}

//=============================================================================
LoadingProgress::LoadingProgress(Panel *parent, const char *panelName, LoadingWindowType eLoadingType ):
	BaseClass( parent, panelName, false, false, false ),
	m_pDefaultPosterDataKV( NULL ),
	m_LoadingWindowType( eLoadingType )
{
	if ( IsPC() && eLoadingType == LWT_LOADINGPLAQUE )
	{
		MakePopup( false );
	}

	SetDeleteSelfOnClose( true );
	SetProportional( true );

	m_flPeakProgress = 0.0f;
	m_pProTotalProgress = NULL;
	m_pWorkingAnim = NULL;
	m_LoadingType = LT_UNDEFINED;

	m_pBGImage = NULL;
	m_pPoster = NULL;

	m_pCancelButton = new vgui::Button(this, "CancelButton", "");

	m_pFooter = NULL;

	// purposely not pre-caching the poster images
	// as they do not appear in-game, and are 1MB each, we will demand load them and ALWAYS discard them
	m_pMissionInfo = NULL;
	m_pChapterInfo = NULL;
	m_botFlags = 0xFF;

	m_textureID_LoadingBar = -1;
	m_textureID_LoadingBarBG = -1;
	m_textureID_DefaultPosterImage = -1;

	m_bDrawBackground = false;
	m_bDrawPoster = false;
	m_bDrawProgress = false;
	m_bDrawSpinner = false;
	m_bFullscreenPoster = true;

	m_flLastEngineTime = 0;

	// marked to indicate the controls exist
	m_bValid = false;

	MEM_ALLOC_CREDIT();
	m_pDefaultPosterDataKV = new KeyValues( "DefaultPosterData" );
	if ( !m_pDefaultPosterDataKV->LoadFromFile( g_pFullFileSystem, "resource/UI/BaseModUI/LoadingPosterDefault.res", "MOD" ) )
	{
		DevWarning( "Failed to load default poster information!\n" );
		m_pDefaultPosterDataKV->deleteThis();
		m_pDefaultPosterDataKV = NULL;
	}

	m_pTipPanel = new CLoadingTipPanel( this );

	ListenForGameEvent("server_spawn");

}

//=============================================================================
LoadingProgress::~LoadingProgress()
{
	if ( m_pDefaultPosterDataKV )
		m_pDefaultPosterDataKV->deleteThis();
	m_pDefaultPosterDataKV = NULL;
}

extern const char *GetMapDisplayName(const char *mapName);
extern const char *GetMapType(const char *mapName);

void LoadingProgress::FireGameEvent(IGameEvent * event)
{
	const char *pEventName = event->GetName();

	// when we are changing levels and 
	if (0 == Q_strcmp(pEventName, "server_spawn"))
	{
		const char *pMapName = event->GetString("mapname");
		if (pMapName)
		{
			// set the map name in the UI
			wchar_t wzMapName[255] = L"";
			g_pVGuiLocalize->ConvertANSIToUnicode(GetMapDisplayName(pMapName), wzMapName, sizeof(wzMapName));

			SetDialogVariable("maplabel", wzMapName);
			SetDialogVariable("maptype", g_pVGuiLocalize->Find(GetMapType(pMapName)));

			vgui::Panel *pPanelBG = dynamic_cast<Panel *>(FindChildByName("MapLabelBG"));
			if (pPanelBG && !pPanelBG->IsVisible())
			{
				pPanelBG->SetVisible(true);
			}

			vgui::Label *pLabel = dynamic_cast<Label *>(FindChildByName("OnYourWayLabel"));
			if (pLabel && !pLabel->IsVisible())
			{
				pLabel->SetVisible(true);
			}

			pLabel = dynamic_cast<Label *>(FindChildByName("MapLabel"));
			if (pLabel && !pLabel->IsVisible())
			{
				pLabel->SetVisible(true);
			}

			pLabel = dynamic_cast<Label *>(FindChildByName("MapType"));
			if (pLabel && !pLabel->IsVisible())
			{
				pLabel->SetVisible(true);
			}
		}
	}
}

//=============================================================================
void LoadingProgress::OnThink()
{
	UpdateWorkingAnim();
}

//=============================================================================
void LoadingProgress::OnCommand(const char *command)
{
	if ( !stricmp(command, "Cancel") )
	{
		// disconnect from the server
		engine->ClientCmd_Unrestricted("disconnect\n");

		// close
		Close();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

//=============================================================================
void LoadingProgress::ApplySchemeSettings( IScheme *pScheme )
{
	// will cause the controls to be instanced
	BaseClass::ApplySchemeSettings( pScheme );

	SetPaintBackgroundEnabled( true );
	
	// now have controls, can now do further initing
	m_bValid = true;

	// find or create pattern
	// purposely not freeing these, not worth the i/o hitch for something so small
	const char *pImageName = "vgui/loadingbar";
	m_textureID_LoadingBar = vgui::surface()->DrawGetTextureId( pImageName );
	if ( m_textureID_LoadingBar == -1 )
	{
		m_textureID_LoadingBar = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_textureID_LoadingBar, pImageName, true, false );	
	}

	// find or create pattern
	// purposely not freeing these, not worth the i/o hitch for something so small
	pImageName = "vgui/loadingbar_bg";
	m_textureID_LoadingBarBG = vgui::surface()->DrawGetTextureId( pImageName );
	if ( m_textureID_LoadingBarBG == -1 )
	{
		m_textureID_LoadingBarBG = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_textureID_LoadingBarBG, pImageName, true, false );	
	}

	// need to get the default image loaded now
	// find or create pattern
	// Purposely not freeing these, need this image to be resident always. We flip to
	// this image on a sign out during loading and cannot bring it into	existence then.
#if defined ( SUPPORT_DEFAULT_LOADING_POSTER )
	if ( m_pDefaultPosterDataKV )
	{
		static ConVarRef mat_xbox_iswidescreen( "mat_xbox_iswidescreen" );
		bool bIsWidescreen = mat_xbox_iswidescreen.GetBool();
		bool bFullscreenPoster = m_pDefaultPosterDataKV->GetBool( "fullscreen", false );
		const char *pszPosterImage = ( bFullscreenPoster && bIsWidescreen ) ? m_pDefaultPosterDataKV->GetString( "posterImage_widescreen" ) : m_pDefaultPosterDataKV->GetString( "posterImage" );

		// have to do this to mimic what the bowels of the scheme manager does with bitmaps
		bool bPrependVguiFix = V_strnicmp( pszPosterImage, "vgui", 4 ) != 0;
		CFmtStr sPosterImageFmt( "%s%s", ( bPrependVguiFix ? "vgui/" : "" ), pszPosterImage );
		pszPosterImage = sPosterImageFmt;

		m_textureID_DefaultPosterImage = vgui::surface()->DrawGetTextureId( pszPosterImage );
		if ( m_textureID_DefaultPosterImage == -1 )
		{
			m_textureID_DefaultPosterImage = vgui::surface()->CreateNewTextureID();
			vgui::surface()->DrawSetTextureFile( m_textureID_DefaultPosterImage, pszPosterImage, true, false );	
		}
	}
#endif

	if (m_LblTitle)
	{
		m_LblTitle->SetVisible(false);
	}

	SetupControlStates();
}

void LoadingProgress::Close()
{
	// hint to force any of the movie posters out of memory (purposely ignoring specific map search)
	// if the image is unreferenced (as it should be), it will evict, if already evicted or empty, then harmless
	if ( m_pPoster )
	{
		m_pPoster->EvictImage();
	}

	if ( m_pBGImage && ( m_LoadingType == LT_POSTER ) )
	{
		m_pBGImage->EvictImage();
	}
		
	BaseClass::Close();
}

//=============================================================================
// this is where the spinner gets updated.
void LoadingProgress::UpdateWorkingAnim()
{
	if ( m_pWorkingAnim && ( m_bDrawSpinner || m_LoadingType == LT_TRANSITION ) )
	{
		// clock the anim at 10hz
		float time = Plat_FloatTime();
		if ( ( m_flLastEngineTime + 0.1f ) < time )
		{
			m_flLastEngineTime = time;
			m_pWorkingAnim->SetFrame( m_pWorkingAnim->GetFrame() + 1 );
		}
	}
}

//=============================================================================
void LoadingProgress::SetProgress( float progress )
{
	if ( m_pProTotalProgress && m_bDrawProgress )
	{
		if ( progress > m_flPeakProgress )
		{
			m_flPeakProgress = progress;
		}
		m_pProTotalProgress->SetProgress( m_flPeakProgress );
	}

	UpdateWorkingAnim();
}

void LoadingProgress::SetStatusText(const char * statusText)
{
	if (statusText)
	{
		vgui::Label *pLoadingText = dynamic_cast<vgui::Label*>(FindChildByName("LoadingText"));
		pLoadingText->SetText(statusText);
	}
}

//=============================================================================
float LoadingProgress::GetProgress()
{
	float retVal = -1.0f;

	if ( m_pProTotalProgress )
	{
		retVal = m_pProTotalProgress->GetProgress();
	}

	return retVal;
}

void LoadingProgress::PaintBackground()
{
	int screenWide, screenTall;
	surface()->GetScreenSize( screenWide, screenTall );

	if ( m_bDrawBackground && m_pBGImage )
	{
		int x, y, wide, tall;
		m_pBGImage->GetBounds( x, y, wide, tall );
		surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );
		surface()->DrawSetTexture( m_pBGImage->GetImage()->GetID() );
		surface()->DrawTexturedRect( x, y, x+wide, y+tall );
	}

	if ( m_bDrawPoster && m_pPoster && m_pPoster->GetImage() )
	{
		if ( m_bFullscreenPoster )
		{
			surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );
			surface()->DrawSetTexture( m_pPoster->GetImage()->GetID() );
			surface()->DrawTexturedRect( 0, 0, screenWide, screenTall );
		}
		else
		{
			// fill black
			surface()->DrawSetColor( Color( 0, 0, 0, 255 ) );
			surface()->DrawFilledRect( 0, 0, screenWide, screenTall );

			// overlay poster
			int x, y, wide, tall;
			m_pPoster->GetBounds( x, y, wide, tall );
			surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );
			surface()->DrawSetTexture( m_pPoster->GetImage()->GetID() );
			surface()->DrawTexturedRect( x, y, x+wide, y+tall );
		}		
	}

	if ( m_bDrawPoster && m_pFooter )
	{
		int screenWidth, screenHeight;
		CBaseModPanel::GetSingleton().GetSize( screenWidth, screenHeight );

		int x, y, wide, tall;
		m_pFooter->GetBounds( x, y, wide, tall );
		surface()->DrawSetColor( m_pFooter->GetBgColor() );
		surface()->DrawFilledRect( 0, y, x+screenWidth, y+tall );
	}

	// this is where the spinner draws
	bool bRenderSpinner = ( m_bDrawSpinner && m_pWorkingAnim );
	Panel *pWaitscreen = CBaseModPanel::GetSingleton().GetWindow( WT_GENERICWAITSCREEN );
	if ( pWaitscreen && pWaitscreen->IsVisible() && ( m_LoadingWindowType == LWT_BKGNDSCREEN ) )
		bRenderSpinner = false;	// Don't render spinner if the progress screen is displaying progress
	if ( bRenderSpinner  )
	{
		int x, y, wide, tall;

		wide = tall = scheme()->GetProportionalScaledValue( 45 );
		x = scheme()->GetProportionalScaledValue( 45 ) - wide/2;
		y = screenTall - scheme()->GetProportionalScaledValue( 32 ) - tall/2;

		m_pWorkingAnim->GetImage()->SetFrame( m_pWorkingAnim->GetFrame() );

		surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );
		surface()->DrawSetTexture( m_pWorkingAnim->GetImage()->GetID() );
		surface()->DrawTexturedRect( x, y, x+wide, y+tall );
	}

	if ( m_bDrawProgress && m_pProTotalProgress )
	{
		int x, y, wide, tall;
		m_pProTotalProgress->GetBounds( x, y, wide, tall );

		int iScreenWide, iScreenTall;
		surface()->GetScreenSize( iScreenWide, iScreenTall );

		float f = m_pProTotalProgress->GetProgress();
		f = clamp( f, 0, 1.0f );

		// Textured bar
		surface()->DrawSetColor( Color( 255, 255, 255, 255 ) );

		// Texture BG
		surface()->DrawSetTexture( m_textureID_LoadingBarBG );
		surface()->DrawTexturedRect( x, y, x + wide, y + tall );

		surface()->DrawSetTexture( m_textureID_LoadingBar );

		// YWB 10/13/2009:  If we don't crop the texture coordinate down then we will see a jitter of the texture as the texture coordinate and the rounded width 
		//  alias

		int nIntegerWide = f * wide;		
		//float flUsedFrac = (float)nIntegerWide / (float)wide;
		
		/*
		DrawTexturedRectParms_t params;
		params.x0 = x;
		params.y0 = y;
		params.x1 = x + nIntegerWide;
		params.y1 = y + tall;
		params.s0 = 0;
		params.s1 = flUsedFrac;
		surface()->DrawTexturedRectEx( &params );
		*/
		surface()->DrawTexturedRect(x, y, x + nIntegerWide, y + tall);
	}

	if ( m_pCancelButton )
		m_pCancelButton->SetVisible(true);

	// Need to call this periodically to collect sign in and sign out notifications,
	// do NOT dispatch events here in the middle of loading and rendering!
	if ( ThreadInMainThread() )
	{
		XBX_ProcessEvents();
	}
}

//=============================================================================
// Must be called first. Establishes the loading style
//=============================================================================
void LoadingProgress::SetLoadingType( LoadingProgress::LoadingType loadingType )
{
	m_LoadingType = loadingType;

	// the first time initing occurs during ApplySchemeSettings() or if the panel is deleted
	// if the panel is re-used, this is for the second time the panel gets used
	SetupControlStates();
}

//=============================================================================
LoadingProgress::LoadingType LoadingProgress::GetLoadingType()
{
	return m_LoadingType;
}

//=============================================================================
void LoadingProgress::SetupControlStates()
{
	m_flPeakProgress = 0.0f;

	if ( !m_bValid )
	{
		// haven't been functionally initialized yet
		// can't set or query control states until they get established
		return;
	}

	m_bDrawBackground = false;
	m_bDrawPoster = false;
	m_bDrawSpinner = false;
	m_bDrawProgress = false;

	switch( m_LoadingType )
	{
	case LT_MAINMENU:
		m_bDrawBackground = true;
		m_bDrawProgress = true;
		m_bDrawSpinner = true;
		break;
	case LT_TRANSITION:
		// the transition screen shows stats and owns all the drawing
		// spinner needs to be drawn as child of the window that is shown while loading
		break;
	case LT_POSTER:
		m_bDrawBackground = false;
		m_bDrawPoster = true;
		m_bDrawProgress = true;
		m_bDrawSpinner = true;
		break;
	default:
		break;
	}

	m_pFooter = dynamic_cast< vgui::EditablePanel* >( FindChildByName( "CampaignFooter" ) );

	m_pCancelButton  = dynamic_cast< vgui::Button* >( FindChildByName( "CancelButton" ) );

	m_pBGImage = dynamic_cast< vgui::ImagePanel* >( FindChildByName( "BGImage" ) );
	if ( m_pBGImage )
	{
		// set the correct background image
		if ( m_LoadingType == LT_POSTER )
		{
			//m_pBGImage->SetImage( "../vgui/LoadingScreen_background" );
		}
		else
		{
			int screenWide, screenTall;
			surface()->GetScreenSize( screenWide, screenTall );
			char filename[MAX_PATH];
			V_snprintf( filename, sizeof( filename ), "console/background01" ); // TODO: engine->GetStartupImage( filename, sizeof( filename ), screenWide, screenTall );
			m_pBGImage->SetImage( CFmtStr( "../%s", filename ) );
		}

		// we will custom draw
		m_pBGImage->SetVisible( false );
	}

	m_pPoster = dynamic_cast< vgui::ImagePanel* >( FindChildByName( "Poster" ) );
	if ( m_pPoster )
	{
		// we will custom draw
		m_pPoster->SetVisible( false );
	}
	
	m_pProTotalProgress = dynamic_cast< vgui::ProgressBar* >( FindChildByName( "ProTotalProgress" ) );
	if ( m_pProTotalProgress )
	{
		// we will custom draw
		m_pProTotalProgress->SetVisible( false );
	}
/*
	m_pWorkingAnim = dynamic_cast< vgui::ImagePanel* >( FindChildByName( "WorkingAnim" ) );
	if ( m_pWorkingAnim )
	{
		// we will custom draw
		m_pWorkingAnim->SetVisible( false );
	}
*/
	vgui::Label *pLoadingLabel = dynamic_cast< vgui::Label *>( FindChildByName( "LoadingText" ) );
	if ( pLoadingLabel )
	{
		pLoadingLabel->SetVisible( m_bDrawProgress );
	}

	if ( m_LoadingType == LT_POSTER )
	{
		SetupPoster();
	}

	// Hold on to start frame slightly
	m_flLastEngineTime = Plat_FloatTime() + 0.2f;
}

void LoadingProgress::SetPosterData( KeyValues *pMissionInfo, KeyValues *pChapterInfo, const char **pPlayerNames, unsigned int botFlags, const char *pszGameMode, const char *pszMapName )
{
	m_botFlags = botFlags;
	m_pMissionInfo = pMissionInfo;
	m_pChapterInfo = pChapterInfo;
}

//=============================================================================
bool LoadingProgress::ShouldShowPosterForLevel( KeyValues *pMissionInfo, KeyValues *pChapterInfo )
{
	// Do not show loading poster for credits
	if ( pMissionInfo && !Q_stricmp( pMissionInfo->GetString( "name" ), "credits" ) )
		return false;

	if ( pMissionInfo )
	{
		char const *szPosterImage = pMissionInfo->GetString( "poster/posterImage", NULL );
		if ( szPosterImage && *szPosterImage )
			// If the campaign specifies a valid poster, then we'll use it
			return true;
	}

	// All other campaigns can fall back to a default poster
	return ( m_pDefaultPosterDataKV != NULL );
}

extern KeyValues* BackgroundSettings();

//=============================================================================
void LoadingProgress::SetupPoster( void )
{	
	vgui::ImagePanel *pPoster = dynamic_cast< vgui::ImagePanel* >( FindChildByName( "Poster" ) );
	if ( pPoster )
	{ 
#if !defined( _X360 )
		int screenWide, screenTall;
		surface()->GetScreenSize( screenWide, screenTall );
		float aspectRatio = (float)screenWide/(float)screenTall;
		bool bIsWidescreen = aspectRatio >= 1.5999f;
#else
		static ConVarRef mat_xbox_iswidescreen( "mat_xbox_iswidescreen" );
		bool bIsWidescreen = mat_xbox_iswidescreen.GetBool();
#endif

		const char *pszPosterImage = NULL;
		pszPosterImage = ( m_bFullscreenPoster && bIsWidescreen ) ? "../console/background01_widescreen" : "../console/background01";
		
		if( BackgroundSettings() )
		{
			KeyValues *inLoadingSettings = BackgroundSettings()->FindKey("loading");
			if( inLoadingSettings )
			{
				int nChosenLoadingImage = RandomInt( 1, inLoadingSettings->GetInt( "background_count", 1 ) );
				
				int i = 0;
				
				for( KeyValues *pSub = inLoadingSettings->GetFirstValue(); pSub; pSub = pSub->GetNextValue() )
				{
					if( !Q_strcmp(pSub->GetName(), "background_count") )
						continue;

					i++;
					if( nChosenLoadingImage == i )
					{
						pszPosterImage = ( m_bFullscreenPoster && bIsWidescreen ) ? VarArgs( "%s_widescreen", pSub->GetString() ) : pSub->GetString();
						break;
					}
				}
			}
		}
		// if the image was cached this will just hook it up, otherwise it will load it
		pPoster->SetImage( pszPosterImage );
	}

	bool bIsLocalized = false;

	char uilanguage[ 64 ];
	engine->GetUILanguage( uilanguage, sizeof( uilanguage ) );
	if ( Q_stricmp( uilanguage, "english" ) )
	{
		bIsLocalized = true;
	}


}