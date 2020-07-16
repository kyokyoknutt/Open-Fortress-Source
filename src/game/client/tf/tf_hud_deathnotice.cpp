//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Draws CSPort's death notices
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include <vgui/ILocalize.h>
#include "tf_gamerules.h"
#include "clientmode_tf.h"
#include "c_tf_playerresource.h"
#include "tf_hud_freezepanel.h"
#include "engine/IEngineSound.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Player entries in a death notice
struct DeathNoticePlayer
{
	DeathNoticePlayer()
	{
		szName[0] = 0;
		iTeam = TEAM_UNASSIGNED;

		iPlayerID = 0;
		iColor = Color(255, 255, 255, 255);
	}
	char		szName[MAX_PLAYER_NAME_LENGTH * 2];	// big enough for player name and additional information
	int			iTeam;								// team #	

	int			iPlayerID;
	Color		iColor;
};

// Contents of each entry in our list of death notices
struct DeathNoticeItem
{
	DeathNoticeItem()
	{
		szIcon[0] = 0;
		wzInfoText[0] = 0;
		iconDeath = NULL;
		bCrit = false;
		bSelfInflicted = false;
		flCreationTime = 0;
		bLocalPlayerInvolved = false;
	}

	float GetExpiryTime();

	DeathNoticePlayer	Killer;
	DeathNoticePlayer   Victim;
	DeathNoticePlayer   Assister;
	char		szIcon[32];		// name of icon to display
	wchar_t		wzInfoText[32];	// any additional text to display next to icon
	CHudTexture *iconDeath;
	CHudTexture *iconCrit;

	bool		bSelfInflicted;
	bool		bCrit;
	float		flCreationTime;
	bool		bLocalPlayerInvolved;
};

// Must match resource/tf_objects.txt!!!
const char *szLocalizedObjectNames[OBJ_LAST] =
{
	"#TF_Object_Dispenser",
	"#TF_Object_Tele",
	"#TF_Object_Sentry",
	"#TF_object_sapper"
};

#define NUM_CORNER_COORD 10
#define NUM_BACKGROUND_COORD NUM_CORNER_COORD*4

class CTFHudDeathNotice : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CTFHudDeathNotice, vgui::Panel );
public:
	CTFHudDeathNotice( const char *pElementName );

	void VidInit( void );
	virtual void Init( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual bool IsVisible( void );
	virtual bool ShouldDraw( void );
	virtual void Paint( void );

	void RetireExpiredDeathNotices( void );

	virtual void FireGameEvent( IGameEvent *event );

	//virtual Color GetInfoTextColor( int iDeathNoticeMsg ){ return Color( 255, 255, 255, 255 ); }
	virtual Color GetInfoTextColor( int iDeathNoticeMsg, bool bLocalPlayerInvolved ){ return bLocalPlayerInvolved ? Color( 0, 0, 0, 255 ) : Color( 255, 255, 255, 255 ); }
	void PlayRivalrySounds( int iKillerIndex, int iVictimIndex, int iType );

protected:
	virtual void OnGameEvent( IGameEvent *event, int iDeathNoticeMsg );
	virtual Color GetTeamColor( int iTeamNumber );
	void DrawText( int x, int y, vgui::HFont hFont, Color clr, const wchar_t *szText );
	int AddDeathNoticeItem();
	void GetBackgroundPolygonVerts( int x0, int y0, int x1, int y1, int iVerts, vgui::Vertex_t vert[] );
	void CalcRoundedCorners();
	CHudTexture *GetIcon( const char *szIcon, bool bInvert );

	void GetLocalizedControlPointName( IGameEvent *event, char *namebuf, int namelen );

private:
	void AddAdditionalMsg( int iKillerID, int iVictimID, const char *pMsgKey );

	CHudTexture		*m_iconDomination;

	CPanelAnimationVar( Color, m_clrBlueText, "TeamBlue", "153 204 255 255" );
	CPanelAnimationVar( Color, m_clrRedText, "TeamRed", "255 64 64 255" );
	CPanelAnimationVar( Color, m_clrMercenaryText, "TeamMercenary", "128 0 128 255");

	CPanelAnimationVarAliasType( float, m_flLineHeight, "LineHeight", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flLineSpacing, "LineSpacing", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flCornerRadius, "CornerRadius", "3", "proportional_float" );
	CPanelAnimationVar( float, m_flMaxDeathNotices, "MaxDeathNotices", "4" );
	CPanelAnimationVar( bool, m_bRightJustify, "RightJustify", "1" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
	CPanelAnimationVar( Color, m_clrIcon, "IconColor", "255 80 0 255" );
	CPanelAnimationVar( Color, m_clrBaseBGColor, "BaseBackgroundColor", "46 43 42 220" );
	CPanelAnimationVar( Color, m_clrLocalBGColor, "LocalBackgroundColor", "245 229 196 200" );

	CUtlVector<DeathNoticeItem> m_DeathNotices;

	Vector2D	m_CornerCoord[NUM_CORNER_COORD];
};


using namespace vgui;

static ConVar hud_deathnotice_time( "hud_deathnotice_time", "6", 0 );

ConVar of_dm_soundcues( "of_dm_soundcues", "1", FCVAR_ARCHIVE, "Whether or not Dominations, Nemesiss and Revenges use the DM specific ones in Deathmatch" );

ConVar of_killfeed_color_threshold( "of_killfeed_color_threshold", "1.5", FCVAR_ARCHIVE, "Minimum contrast ratio before applying an effect to increase visibility" );
ConVar of_killfeed_color_contrast( "of_killfeed_color_contrast", "1", FCVAR_ARCHIVE, "0: Off, 1: If a color does not contrast well with the killfeed background, modify its color to increase its visibility" );

DECLARE_HUDELEMENT( CTFHudDeathNotice );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudDeathNotice::CTFHudDeathNotice( const char *pElementName ) :
CHudElement( pElementName ), BaseClass( NULL, "HudDeathNotice" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::Init( void )
{
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "object_destroyed" );
	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_capture_blocked" );
	ListenForGameEvent( "teamplay_flag_event" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::VidInit( void )
{
	m_DeathNotices.RemoveAll();
}

void CTFHudDeathNotice::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	SetPaintBackgroundEnabled( false );

	CalcRoundedCorners();

	m_iconDomination = gHUD.GetIcon( "leaderboard_dominated" );
}

bool CTFHudDeathNotice::IsVisible( void )
{
	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
// Purpose: Draw if we've got at least one death notice in the queue
//-----------------------------------------------------------------------------
bool CTFHudDeathNotice::ShouldDraw( void )
{
	return ( CHudElement::ShouldDraw() && ( m_DeathNotices.Count() ) );
}

double Luminance(Color col) 
{
    double r, g, b;
    r = col.r(); g = col.g(); b = col.b();
    double Rg = r <= 10 ? r / 3294 : pow(r / 269 + 0.0513, 2.4);
    double Gg = g <= 10 ? g / 3294 : pow(g / 269 + 0.0513, 2.4);
    double Bg = b <= 10 ? b / 3294 : pow(b / 269 + 0.0513, 2.4);
    return 0.2126 * Rg + 0.7152 * Gg + 0.0722 * Bg;
}

float LuminanceContrast(double a, double b) 
{
    if (a > b) {
        return (a + 0.05) / (b + 0.05);
    }
    else {
        return (b + 0.05) / (a + 0.05);
    }
}

// THIS DOES NOT WORK, DO NOT USE THIS
Color FindAcceptableContrast(Color fg, Color bg, float minimum) 
{
	float bg_l = Luminance(bg);
	float fg_l = Luminance(bg);
	float contrast;

	bool dark = 0;
	if(fg_l >= bg_l) 
	{
		contrast = fg_l / bg_l;
		dark = 1;
	} 
	else 
	{
		contrast = bg_l / fg_l;
	}

	Vector fg_HSV;
	RGBtoHSV( Vector(fg.r(), fg.g(), fg.b()), fg_HSV);
	Vector new_col;

	while( contrast < minimum ) 
	{
		if(dark) 
		{
			if( new_col.x > 254.0f && new_col.y > 254.0f && new_col.z > 254.0f ) 
			{
				if( fg_HSV.z < 255.0f ) 
				{
					fg_HSV.z += 0.1f;
				}	
			}
			else
			{
				if( fg_HSV.z > 0.0f ) 
				{
					fg_HSV.z -= 0.1f;
				}		
			}
		}
		else
		{
			if( new_col.x < 1.0f && new_col.y < 1.0f && new_col.z < 1.0f ) 
			{
				if( fg_HSV.z > 0.0f ) 
				{
					fg_HSV.z -= 0.1f;
				}		
			}
			else
			{
				if( fg_HSV.z < 255.0f ) 
				{
					fg_HSV.z += 0.1f;
				}	
			}

		}
		
		// printf("fg_HSV: %f %f %f\n", fg_HSV.x, fg_HSV.y, fg_HSV.z);

		HSVtoRGB(fg_HSV, new_col);
		fg_l = Luminance( Color( new_col.x, new_col.y, new_col.z, 255 ) );
		contrast = LuminanceContrast(fg_l, bg_l);
	}

	// Msg("Old color: (%i, %i, %i) New color: (%i, %i, %i)\n", fg.r(), fg.g(), fg.b(), (int)ceil( new_col.x ), (int)ceil( new_col.y ), (int)ceil( new_col.z) );
	return Color( new_col.x, new_col.y, new_col.z, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::Paint()
{
	// Retire any death notices that have expired
	RetireExpiredDeathNotices();

	CBaseViewport *pViewport = dynamic_cast<CBaseViewport *>( GetClientModeNormal()->GetViewport() );
	int yStart = pViewport->GetDeathMessageStartHeight();

	surface()->DrawSetTextFont( m_hTextFont );

	int xMargin = XRES( 10 );
	int xSpacing = UTIL_ComputeStringWidth( m_hTextFont, L" " );

	int iCount = m_DeathNotices.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		DeathNoticeItem &msg = m_DeathNotices[i];

		CHudTexture *icon = msg.iconDeath;

		wchar_t victim[256] = L"";
		wchar_t killer[256] = L"";
		wchar_t assister[256] = L"";

		// TEMP - print the death icon name if we don't have a material for it

		g_pVGuiLocalize->ConvertANSIToUnicode( msg.Victim.szName, victim, sizeof( victim ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( msg.Killer.szName, killer, sizeof( killer ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( msg.Assister.szName, assister, sizeof( assister ) );

		int iVictimTextWide = UTIL_ComputeStringWidth( m_hTextFont, victim ) + xSpacing;
		int iAssisterTextWide = assister[0] ? UTIL_ComputeStringWidth( m_hTextFont, assister ) + xSpacing : 0;
		int iDeathInfoTextWide = msg.wzInfoText[0] ? UTIL_ComputeStringWidth( m_hTextFont, msg.wzInfoText ) + xSpacing : 0;
		int iKillerTextWide = killer[0] ? UTIL_ComputeStringWidth( m_hTextFont, killer ) + xSpacing : 0;
		int iLineTall = m_flLineHeight;
		int iTextTall = surface()->GetFontTall( m_hTextFont );
		int iconWide = 0, iconTall = 0, iDeathInfoOffset = 0, iVictimTextOffset = 0, iconActualWide = 0;

		// Get the local position for this notice
		if ( icon )
		{
			iconActualWide = icon->EffectiveWidth( 1.0f );
			iconWide = iconActualWide + xSpacing;
			iconTall = icon->EffectiveHeight( 1.0f );

			int iconTallDesired = iLineTall - YRES( 2 );
			Assert( 0 != iconTallDesired );
			float flScale = (float)iconTallDesired / (float)iconTall;

			iconActualWide *= flScale;
			iconTall *= flScale;
			iconWide *= flScale;
		}
		int iTotalWide = iKillerTextWide + iAssisterTextWide + iconWide + iVictimTextWide + iDeathInfoTextWide + ( xMargin * 2 );
		int y = yStart + ( ( iLineTall + m_flLineSpacing ) * i );
		int yText = y + ( ( iLineTall - iTextTall ) / 2 );
		int yIcon = y + ( ( iLineTall - iconTall ) / 2 );

		int x = 0;
		if ( m_bRightJustify )
		{
			x = GetWide() - iTotalWide;
		}

		// draw a background panel for the message
		Vertex_t vert[NUM_BACKGROUND_COORD];
		GetBackgroundPolygonVerts( x, y + 1, x + iTotalWide, y + iLineTall - 1, ARRAYSIZE( vert ), vert );
		surface()->DrawSetTexture( -1 );
		surface()->DrawSetColor( msg.bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor );
		surface()->DrawTexturedPolygon( ARRAYSIZE( vert ), vert );

		x += xMargin;

		//C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);

		if ( killer[0] )
		{
			Color clr = TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() ? msg.Killer.iColor : GetTeamColor( msg.Killer.iTeam );

			DrawText( x, yText, m_hTextFont, clr, killer );

			x += iKillerTextWide;
		}

		if ( assister[0] )
		{
			// Draw a + between the names
			Color clr = TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() ? msg.Assister.iColor : GetTeamColor( msg.Assister.iTeam );

			DrawText(x, yText, m_hTextFont, GetInfoTextColor( i, msg.bLocalPlayerInvolved ), L" + ");
			x += 24;

			// Draw assister's name
			DrawText( x, yText, m_hTextFont, clr, assister );
			x += iAssisterTextWide;
		}

		// Draw glow behind weapon icon to show it was a crit death
		if ( msg.bCrit && msg.iconCrit )
		{
			msg.iconCrit->DrawSelf( x, yIcon, iconActualWide, iconTall, m_clrIcon );
		}

		// Draw death icon
		if ( icon )
		{
			icon->DrawSelf( x, yIcon, iconActualWide, iconTall, m_clrIcon );
			x += iconWide;
		}

		// Draw additional info text next to death icon 
		if ( msg.wzInfoText[0] )
		{
			if ( msg.bSelfInflicted )
			{
				iDeathInfoOffset += iVictimTextWide;
				iVictimTextOffset -= iDeathInfoTextWide;
			}

			DrawText( x + iDeathInfoOffset, yText, m_hTextFont, GetInfoTextColor( i, msg.bLocalPlayerInvolved ), msg.wzInfoText );
			x += iDeathInfoTextWide;
		}

		// Draw victims name
		Color clr = TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() ? msg.Victim.iColor : GetTeamColor( msg.Victim.iTeam );

		DrawText( x + iVictimTextOffset, yText, m_hTextFont, clr, victim );
		x += iVictimTextWide;
	}
}

//-----------------------------------------------------------------------------
// Purpose: This message handler may be better off elsewhere
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::RetireExpiredDeathNotices()
{
	// Remove any expired death notices.  Loop backwards because we might remove one
	int iCount = m_DeathNotices.Count();
	for ( int i = iCount - 1; i >= 0; i-- )
	{
		if ( gpGlobals->curtime > m_DeathNotices[i].GetExpiryTime() )
		{
			m_DeathNotices.Remove( i );
		}
	}

	// Do we have too many death messages in the queue?
	if ( m_DeathNotices.Count() > 0 &&
		m_DeathNotices.Count() > (int)m_flMaxDeathNotices )
	{
		// First, remove any notices not involving the local player, since they are lower priority.		
		iCount = m_DeathNotices.Count();
		int iNeedToRemove = iCount - (int)m_flMaxDeathNotices;
		// loop condition is iCount-1 because we won't remove the most recent death notice, otherwise
		// new non-local-player-involved messages would not appear if the queue was full of messages involving the local player
		for ( int i = 0; i < iCount - 1 && iNeedToRemove > 0; i++ )
		{
			if ( !m_DeathNotices[i].bLocalPlayerInvolved )
			{
				m_DeathNotices.Remove( i );
				iCount--;
				iNeedToRemove--;
			}
		}

		// Now that we've culled any non-local-player-involved messages up to the amount we needed to remove, see
		// if we've removed enough
		iCount = m_DeathNotices.Count();
		iNeedToRemove = iCount - (int)m_flMaxDeathNotices;
		if ( iNeedToRemove > 0 )
		{
			// if we still have too many messages, then just remove however many we need, oldest first
			for ( int i = 0; i < iNeedToRemove; i++ )
			{
				m_DeathNotices.Remove( 0 );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Server's told us that someone's died
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::FireGameEvent( IGameEvent *event )
{
	if ( !g_PR )
	{
		return;
	}

	if ( hud_deathnotice_time.GetFloat() == 0 )
	{
		return;
	}

	const char *pszEventName = event->GetName();

	// Add a new death message.  Note we always look it up by index rather than create a reference or pointer to it;
	// additional messages may get added during this function that cause the underlying array to get realloced, so don't
	// ever keep a pointer to memory here.
	int iMsg = AddDeathNoticeItem();
	int iLocalPlayerIndex = GetLocalPlayerIndex();

	if ( FStrEq( pszEventName, "player_death" ) || FStrEq( pszEventName, "object_destroyed" ) )
	{
		int victim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		int killer = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		const char *killedwith = event->GetString( "weapon" );

		// Get the names of the players
		const char *killer_name = g_PR->GetPlayerName( killer );
		const char *victim_name = g_PR->GetPlayerName( victim );

		if ( !killer_name )
		{
			killer_name = "";
		}

		if ( !victim_name )
		{
			victim_name = "";
		}

		// Make a new death notice
		bool bLocalPlayerInvolved = false;
		if ( iLocalPlayerIndex == killer || iLocalPlayerIndex == victim )
		{
			bLocalPlayerInvolved = true;
		}
			
		m_DeathNotices[iMsg].bLocalPlayerInvolved = bLocalPlayerInvolved;
		m_DeathNotices[iMsg].Killer.iTeam = g_PR->GetTeam( killer );
		m_DeathNotices[iMsg].Victim.iTeam = g_PR->GetTeam( victim );

		// grab id of killer or victim for player coloring in DM
		m_DeathNotices[iMsg].Killer.iPlayerID = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		m_DeathNotices[iMsg].Victim.iPlayerID = engine->GetPlayerForUserID( event->GetInt( "userid" ) );

		Q_strncpy( m_DeathNotices[iMsg].Killer.szName, killer_name, ARRAYSIZE( m_DeathNotices[iMsg].Killer.szName ) );
		Q_strncpy( m_DeathNotices[iMsg].Victim.szName, victim_name, ARRAYSIZE( m_DeathNotices[iMsg].Victim.szName ) );
		if ( killedwith && *killedwith )
		{
			Q_snprintf( m_DeathNotices[iMsg].szIcon, sizeof( m_DeathNotices[iMsg].szIcon ), "d_%s", killedwith );
		}
		
		if ( !killer || killer == victim )
		{
			m_DeathNotices[iMsg].bSelfInflicted = true;
			m_DeathNotices[iMsg].Killer.szName[0] = 0;

			if ( event->GetInt( "damagebits" ) & DMG_FALL )
			{
				// special case text for falling death
				V_wcsncpy( m_DeathNotices[iMsg].wzInfoText, g_pVGuiLocalize->Find( "#DeathMsg_Fall" ), sizeof( m_DeathNotices[iMsg].wzInfoText ) );
			}
			else if ( ( event->GetInt( "damagebits" ) & DMG_VEHICLE ) || ( 0 == Q_stricmp( m_DeathNotices[iMsg].szIcon, "d_tracktrain" ) ) )
			{
				// special case icon for hit-by-vehicle death
				Q_strncpy( m_DeathNotices[iMsg].szIcon, "d_vehicle", ARRAYSIZE( m_DeathNotices[iMsg].szIcon ) );
			}
			else if ( ( event->GetInt( "damagebits" ) & DMG_NERVEGAS ) )
			{
				// saw blade!
				Q_strncpy( m_DeathNotices[iMsg].szIcon, "d_saw_kill", ARRAYSIZE( m_DeathNotices[iMsg].szIcon ) );
			}
		}

		if ( event->GetInt( "damagebits" ) & DMG_CRITICAL )
		{
			// special case text for crits
			m_DeathNotices[iMsg].bCrit = true;
			m_DeathNotices[iMsg].iconCrit = GetIcon( "d_crit", bLocalPlayerInvolved );
		}	

		char sDeathMsg[512];

		// Record the death notice in the console
		if ( m_DeathNotices[iMsg].bSelfInflicted )
		{
			if ( !strcmp( m_DeathNotices[iMsg].szIcon, "d_worldspawn" ) )
			{
				Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s died.", m_DeathNotices[iMsg].Victim.szName );
			}
			else	// d_world
			{
				Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s suicided.", m_DeathNotices[iMsg].Victim.szName );
			}
		}
		else
		{
			Q_snprintf( sDeathMsg, sizeof( sDeathMsg ), "%s killed %s", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName );

			if ( m_DeathNotices[iMsg].szIcon[0] && ( m_DeathNotices[iMsg].szIcon[0] > 13 ) )
			{
				Q_strncat( sDeathMsg, VarArgs( " with %s.", &m_DeathNotices[iMsg].szIcon[2] ), sizeof( sDeathMsg ), COPY_ALL_CHARACTERS );
			}
		}

		ConColorMsg( Color( 135, 206, 235, 255 ), "%s\n", sDeathMsg );

	}
	else if ( FStrEq( "teamplay_point_captured", pszEventName ) )
	{
		GetLocalizedControlPointName( event, m_DeathNotices[iMsg].Victim.szName, ARRAYSIZE( m_DeathNotices[iMsg].Victim.szName ) );

		// Array of capper indices
		const char *cappers = event->GetString( "cappers" );

		char szCappers[256];
		szCappers[0] = '\0';

		int len = Q_strlen( cappers );
		for ( int i = 0; i<len; i++ )
		{
			int iPlayerIndex = (int)cappers[i];

			Assert( iPlayerIndex > 0 && iPlayerIndex <= gpGlobals->maxClients );

			const char *pPlayerName = g_PR->GetPlayerName( iPlayerIndex );

			if ( i == 0 )
			{
				// use first player as the team
				m_DeathNotices[iMsg].Killer.iTeam = g_PR->GetTeam( iPlayerIndex );
				m_DeathNotices[iMsg].Victim.iTeam = TEAM_UNASSIGNED;
			}
			else
			{
				Q_strncat( szCappers, ", ", sizeof( szCappers ), 2 );
			}

			Q_strncat( szCappers, pPlayerName, sizeof( szCappers ), COPY_ALL_CHARACTERS );
			if ( iLocalPlayerIndex == iPlayerIndex )
				m_DeathNotices[iMsg].bLocalPlayerInvolved = true;
		}

		Q_strncpy( m_DeathNotices[iMsg].Killer.szName, szCappers, sizeof( m_DeathNotices[iMsg].Killer.szName ) );
		V_wcsncpy( m_DeathNotices[iMsg].wzInfoText, g_pVGuiLocalize->Find( "#Msg_Captured" ), sizeof( m_DeathNotices[iMsg].wzInfoText ) );

		// print a log message
		ConColorMsg( Color( 135, 206, 235, 255 ), "%s captured %s for team #%d\n", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName, m_DeathNotices[iMsg].Killer.iTeam );
	}
	else if ( FStrEq( "teamplay_capture_blocked", pszEventName ) )
	{
		GetLocalizedControlPointName( event, m_DeathNotices[iMsg].Victim.szName, ARRAYSIZE( m_DeathNotices[iMsg].Victim.szName ) );
		V_wcsncpy( m_DeathNotices[iMsg].wzInfoText, g_pVGuiLocalize->Find( "#Msg_Defended" ), sizeof( m_DeathNotices[iMsg].wzInfoText ) );

		int iPlayerIndex = event->GetInt( "blocker" );
		const char *blocker_name = g_PR->GetPlayerName( iPlayerIndex );
		Q_strncpy( m_DeathNotices[iMsg].Killer.szName, blocker_name, ARRAYSIZE( m_DeathNotices[iMsg].Killer.szName ) );
		m_DeathNotices[iMsg].Killer.iTeam = g_PR->GetTeam( iPlayerIndex );
		if ( iLocalPlayerIndex == iPlayerIndex )
			m_DeathNotices[iMsg].bLocalPlayerInvolved = true;

		// print a log message
		ConColorMsg( Color( 135, 206, 235, 255 ), "%s defended %s for team #%d\n", m_DeathNotices[iMsg].Killer.szName, m_DeathNotices[iMsg].Victim.szName, m_DeathNotices[iMsg].Killer.iTeam );
	}
	else if ( FStrEq( "teamplay_flag_event", pszEventName ) )
	{
		const char *pszMsgKey = NULL;
		int iEventType = event->GetInt( "eventtype" );
		switch ( iEventType )
		{
		case TF_FLAGEVENT_PICKUP:
			pszMsgKey = "#Msg_PickedUpFlag";
			break;
		case TF_FLAGEVENT_CAPTURE:
			pszMsgKey = "#Msg_CapturedFlag";
			break;
		case TF_FLAGEVENT_DEFEND:
			pszMsgKey = "#Msg_DefendedFlag";
			break;

			// Add this when we can get localization for it
			//case TF_FLAGEVENT_DROPPED: 
			//	pszMsgKey = "#Msg_DroppedFlag"; 
			//	break;

		default:
			// unsupported, don't put anything up			
			m_DeathNotices.Remove( iMsg );
			return;
		}

		wchar_t *pwzEventText = g_pVGuiLocalize->Find( pszMsgKey );
		Assert( pwzEventText );
		if ( pwzEventText )
		{
			V_wcsncpy( m_DeathNotices[iMsg].wzInfoText, pwzEventText, sizeof( m_DeathNotices[iMsg].wzInfoText ) );
		}
		else
		{
			V_memset( m_DeathNotices[iMsg].wzInfoText, 0, sizeof( m_DeathNotices[iMsg].wzInfoText ) );
		}

		int iPlayerIndex = event->GetInt( "player" );
		const char *szPlayerName = g_PR->GetPlayerName( iPlayerIndex );
		Q_strncpy( m_DeathNotices[iMsg].Killer.szName, szPlayerName, ARRAYSIZE( m_DeathNotices[iMsg].Killer.szName ) );
		m_DeathNotices[iMsg].Killer.iTeam = g_PR->GetTeam( iPlayerIndex );
		if ( iLocalPlayerIndex == iPlayerIndex )
			m_DeathNotices[iMsg].bLocalPlayerInvolved = true;
	}

	OnGameEvent( event, iMsg );

	if ( !m_DeathNotices[iMsg].iconDeath && m_DeathNotices[iMsg].szIcon )
	{
		// Try and find the death identifier in the icon list
		// On consoles, we flip usage of the inverted icon to make it more visible
		bool bInverted = m_DeathNotices[iMsg].bLocalPlayerInvolved;
		if ( IsConsole() )
		{
			bInverted = !bInverted;
		}
		m_DeathNotices[iMsg].iconDeath = GetIcon( m_DeathNotices[iMsg].szIcon, bInverted );
		if ( !m_DeathNotices[iMsg].iconDeath )
		{
			// Can't find it, so use the default skull & crossbones icon
			m_DeathNotices[iMsg].iconDeath = GetIcon( "d_skull_tf", m_DeathNotices[iMsg].bLocalPlayerInvolved );
		}
	}
}

void CTFHudDeathNotice::PlayRivalrySounds( int iKillerIndex, int iVictimIndex, int iType )
{
	int iLocalPlayerIndex = GetLocalPlayerIndex();

	//We're not involved in this kill
	if ( iKillerIndex != iLocalPlayerIndex && iVictimIndex != iLocalPlayerIndex )
		return;

	const char *pszSoundName = NULL;

	if ( iType == TF_DEATH_DOMINATION )
	{
		if ( iKillerIndex == iLocalPlayerIndex )
		{
			pszSoundName = "Game.Domination";
		}
		else if ( iVictimIndex == iLocalPlayerIndex )
		{
			pszSoundName = "Game.Nemesis";
		}
		if ( TeamplayRoundBasedRules() && TFGameRules() && TFGameRules()->IsDMGamemode() && !TFGameRules()->DontCountKills() )
		{
			if( of_dm_soundcues.GetBool() )
				pszSoundName = iKillerIndex == iLocalPlayerIndex ? "DM.Domination" : "DM.Nemesis";
			TeamplayRoundBasedRules()->BroadcastSoundFFA( iKillerIndex, "Dominating" );
			TeamplayRoundBasedRules()->BroadcastSoundFFA( iVictimIndex, "Dominating" );
		}
	}
	else if ( iType == TF_DEATH_REVENGE )
	{
		pszSoundName = "Game.Revenge";
		if ( TeamplayRoundBasedRules() && TFGameRules() && TFGameRules()->IsDMGamemode() && !TFGameRules()->DontCountKills() )
		{
			if( of_dm_soundcues.GetBool() )
				pszSoundName = "DM.Revenge";
			TeamplayRoundBasedRules()->BroadcastSoundFFA( iKillerIndex, "Revenge" );
			TeamplayRoundBasedRules()->BroadcastSoundFFA( iVictimIndex, "Revenge" );
		}
	}

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound( filter, SOUND_FROM_LOCAL_PLAYER, pszSoundName );
}

//-----------------------------------------------------------------------------
// Purpose: Called when a game event happens and a death notice is about to be 
//			displayed.  This method can examine the event and death notice and
//			make game-specific tweaks to it before it is displayed
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::OnGameEvent( IGameEvent *event, int iDeathNoticeMsg )
{
	const char *pszEventName = event->GetName();
	DeathNoticeItem &msg = m_DeathNotices[iDeathNoticeMsg];
	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);

	if ( FStrEq( pszEventName, "player_death" ) || FStrEq( pszEventName, "object_destroyed" ) )
	{
		bool bIsObjectDestroyed = FStrEq( pszEventName, "object_destroyed" );
		int iCustomDamage = event->GetInt( "customkill" );
		int iLocalPlayerIndex = GetLocalPlayerIndex();

		// if there was an assister, put both the killer's and assister's names in the death message
		int iAssisterID = engine->GetPlayerForUserID( event->GetInt( "assister" ) );

		m_DeathNotices[iDeathNoticeMsg].Assister.iPlayerID = iAssisterID;

		const char *assister_name = ( iAssisterID > 0 ? g_PR->GetPlayerName( iAssisterID ) : NULL );
		if ( assister_name )
		{
			// compose the string
			m_DeathNotices[ iDeathNoticeMsg ].Assister.iTeam = ( iAssisterID > 0 ) ? g_PR->GetTeam( iAssisterID ) : 0;

			char szKillerBuf[ MAX_PLAYER_NAME_LENGTH ] ;
			Q_snprintf( szKillerBuf, ARRAYSIZE( szKillerBuf ), "%s", assister_name );
			Q_strncpy( m_DeathNotices[ iDeathNoticeMsg ].Assister.szName, szKillerBuf, ARRAYSIZE( m_DeathNotices[ iDeathNoticeMsg ].Assister.szName ) );

			if ( iLocalPlayerIndex == iAssisterID )
			{
				m_DeathNotices[ iDeathNoticeMsg ].bLocalPlayerInvolved = true;
			}
		}

		if (!bIsObjectDestroyed)
		{
			// if this death involved a player dominating another player or getting revenge on another player, add an additional message
			// mentioning that
			int iKillerID = engine->GetPlayerForUserID(event->GetInt("attacker"));
			int iVictimID = engine->GetPlayerForUserID(event->GetInt("userid"));

			double backGroundColorLuminance = Luminance(msg.bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor);

			// msg.Killer.iColor = FindAcceptableContrast(tf_PR->GetPlayerColor(iKillerID), !msg.bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor, of_killfeed_color_threshold.GetFloat());
			// msg.Assister.iColor = FindAcceptableContrast(tf_PR->GetPlayerColor(iAssisterID), !msg.bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor, of_killfeed_color_threshold.GetFloat());
			// msg.Victim.iColor = FindAcceptableContrast(tf_PR->GetPlayerColor(iVictimID), !msg.bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor, of_killfeed_color_threshold.GetFloat());
			if(LuminanceContrast(Luminance(tf_PR->GetPlayerColor(iKillerID)), backGroundColorLuminance) > of_killfeed_color_threshold.GetFloat())
				msg.Killer.iColor = tf_PR->GetPlayerColor(iKillerID);
			else
				msg.Killer.iColor = !msg.bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor;

			if(LuminanceContrast(Luminance(tf_PR->GetPlayerColor(iAssisterID)), backGroundColorLuminance) > of_killfeed_color_threshold.GetFloat())
				msg.Assister.iColor = tf_PR->GetPlayerColor(iAssisterID);
			else
				msg.Assister.iColor = !msg.bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor;

			if(LuminanceContrast(Luminance(tf_PR->GetPlayerColor(iVictimID)), backGroundColorLuminance) > of_killfeed_color_threshold.GetFloat())
				msg.Victim.iColor = tf_PR->GetPlayerColor(iVictimID);
			else
				msg.Victim.iColor = !msg.bLocalPlayerInvolved ? m_clrLocalBGColor : m_clrBaseBGColor;


			if (event->GetInt("dominated") > 0)
			{
				AddAdditionalMsg(iKillerID, iVictimID, "#Msg_Dominating");
				PlayRivalrySounds(iKillerID, iVictimID, TF_DEATH_DOMINATION);
			}
			if (event->GetInt("assister_dominated") > 0 && (iAssisterID > 0))
			{
				AddAdditionalMsg(iAssisterID, iVictimID, "#Msg_Dominating");
				PlayRivalrySounds(iAssisterID, iVictimID, TF_DEATH_DOMINATION);
			}
			if (event->GetInt("revenge") > 0)
			{
				AddAdditionalMsg(iKillerID, iVictimID, "#Msg_Revenge");
				PlayRivalrySounds(iKillerID, iVictimID, TF_DEATH_REVENGE);
			}
			if (event->GetInt("assister_revenge") > 0 && (iAssisterID > 0))
			{
				AddAdditionalMsg(iAssisterID, iVictimID, "#Msg_Revenge");
				PlayRivalrySounds(iAssisterID, iVictimID, TF_DEATH_REVENGE);
			}
		}

		if ( bIsObjectDestroyed )
		{
			// if this is an object destroyed message, set the victim name to "<object type> (<owner>)"
			int iObjectType = event->GetInt( "objecttype" );
			int bWasBuilding = event->GetBool( "was_building" );
			if ( iObjectType >= 0 && iObjectType < OBJ_LAST )
			{
				// get the localized name for the object
				char szLocalizedObjectName[MAX_PLAYER_NAME_LENGTH];
				szLocalizedObjectName[ 0 ] = 0;
				const wchar_t *wszLocalizedObjectName = g_pVGuiLocalize->Find( szLocalizedObjectNames[iObjectType] );
				if ( wszLocalizedObjectName )
				{
					g_pVGuiLocalize->ConvertUnicodeToANSI( wszLocalizedObjectName, szLocalizedObjectName, ARRAYSIZE( szLocalizedObjectName ) );
				}
				else
				{
					Warning( "Couldn't find localized object name for '%s'\n", szLocalizedObjectNames[iObjectType] );
					Q_strncpy( szLocalizedObjectName, szLocalizedObjectNames[iObjectType], sizeof( szLocalizedObjectName ) );
				}

				// compose the string
				if ( msg.Victim.szName[0] )
				{
					char szVictimBuf[MAX_PLAYER_NAME_LENGTH*2];
					Q_snprintf( szVictimBuf, ARRAYSIZE(szVictimBuf), "%s (%s)", szLocalizedObjectName, msg.Victim.szName );
					Q_strncpy( msg.Victim.szName, szVictimBuf, ARRAYSIZE( msg.Victim.szName ) );
				}
				else
				{
					Q_strncpy( msg.Victim.szName, szLocalizedObjectName, ARRAYSIZE( msg.Victim.szName ) );
				}

				if ( bWasBuilding )
				{
					Q_strncpy( msg.szIcon, "d_building_carried_destroyed", ARRAYSIZE( msg.szIcon ) );
					msg.wzInfoText[0] = 0;
				}
			}
			else
			{
				Assert( false ); // invalid object type
			}
		}

		const wchar_t *pMsg = NULL;
		switch ( iCustomDamage )
		{
		case TF_DMG_CUSTOM_BACKSTAB:
			Q_strncpy( msg.szIcon, "d_backstab", ARRAYSIZE( msg.szIcon ) );
			break;
		case TF_DMG_CUSTOM_HEADSHOT:
			Q_strncpy( msg.szIcon, "d_headshot", ARRAYSIZE( msg.szIcon ) );
			break;
		case TF_DMG_CUSTOM_BURNING:
			// special-case if custom kill is burning; if the attacker is dead we can't get weapon information, so force flamethrower as weapon
			Q_strncpy( msg.szIcon, "d_flamethrower", ARRAYSIZE( msg.szIcon ) );
			msg.wzInfoText[0] = 0;
			break;
		case TF_DMG_CUSTOM_SUICIDE:
			{
				// display a different message if this was suicide, or assisted suicide (suicide w/recent damage, kill awarded to damager)
				bool bAssistedSuicide = event->GetInt( "userid" ) != event->GetInt( "attacker" );
				pMsg = g_pVGuiLocalize->Find( bAssistedSuicide ? "#DeathMsg_AssistedSuicide" : "#DeathMsg_Suicide" );
				if ( pMsg )
				{
					V_wcsncpy( msg.wzInfoText, pMsg, sizeof( msg.wzInfoText ) );
				}			
				break;
			}
		case TF_DMG_CUSTOM_TAUNTATK_HADOUKEN:
			Q_strncpy( msg.szIcon, "d_taunt_pyro", ARRAYSIZE( msg.szIcon ) );
			msg.wzInfoText[0] = 0;
			break;
		case TF_DMG_CUSTOM_TAUNTATK_HIGH_NOON:
			Q_strncpy( msg.szIcon, "d_taunt_heavy", ARRAYSIZE( msg.szIcon ) );
			msg.wzInfoText[0] = 0;
			break;
		case TF_DMG_CUSTOM_TAUNTATK_FENCING:
			Q_strncpy( msg.szIcon, "d_taunt_spy", ARRAYSIZE( msg.szIcon ) );
			msg.wzInfoText[0] = 0;
			break;
		case TF_DMG_CUSTOM_CROC:
			Q_strncpy( msg.szIcon, "d_crocodile", ARRAYSIZE( msg.szIcon ) );
			msg.wzInfoText[0] = 0;
			break;
		default:
			break;
		}
	} 
	else if ( FStrEq( "teamplay_point_captured", pszEventName ) || FStrEq( "teamplay_capture_blocked", pszEventName ) || 
		FStrEq( "teamplay_flag_event", pszEventName ) )
	{
		bool bDefense = ( FStrEq( "teamplay_capture_blocked", pszEventName ) || ( FStrEq( "teamplay_flag_event", pszEventName ) &&
			TF_FLAGEVENT_DEFEND == event->GetInt( "eventtype" ) ) );

		const char *szCaptureIcons[] = { "d_redcapture", "d_bluecapture" };
		const char *szDefenseIcons[] = { "d_reddefend", "d_bluedefend" };
		
		int iTeam = msg.Killer.iTeam;
		Assert( iTeam >= FIRST_GAME_TEAM );
		Assert( iTeam < FIRST_GAME_TEAM + TF_TEAM_COUNT );
		if ( iTeam < FIRST_GAME_TEAM || iTeam >= FIRST_GAME_TEAM + TF_TEAM_COUNT )
			return;

		int iIndex = msg.Killer.iTeam - FIRST_GAME_TEAM;
		Assert( iIndex < ARRAYSIZE( szCaptureIcons ) );

		Q_strncpy( msg.szIcon, bDefense ? szDefenseIcons[iIndex] : szCaptureIcons[iIndex], ARRAYSIZE( msg.szIcon ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the localized name of the control point sent in the event
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::GetLocalizedControlPointName( IGameEvent *event, char *namebuf, int namelen )
{
	// Cap point name ( MATTTODO: can't we find this from the point index ? )
	const char *pName = event->GetString( "cpname", "Unnamed Control Point" );
	const wchar_t *pLocalizedName = g_pVGuiLocalize->Find( pName );

	if ( pLocalizedName )
	{
		g_pVGuiLocalize->ConvertUnicodeToANSI( pLocalizedName, namebuf, namelen );
	}
	else
	{
		Q_strncpy( namebuf, pName, namelen );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new death notice to the queue
//-----------------------------------------------------------------------------
int CTFHudDeathNotice::AddDeathNoticeItem()
{
	int iMsg = m_DeathNotices.AddToTail();
	DeathNoticeItem &msg = m_DeathNotices[iMsg];
	msg.flCreationTime = gpGlobals->curtime;
	return iMsg;
}

//-----------------------------------------------------------------------------
// Purpose: draw text helper
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::DrawText( int x, int y, HFont hFont, Color clr, const wchar_t *szText )
{
	surface()->DrawSetTextPos( x, y );
	surface()->DrawSetTextColor( clr );
	surface()->DrawSetTextFont( hFont );	//reset the font, draw icon can change it
	surface()->DrawUnicodeString( szText, vgui::FONT_DRAW_NONADDITIVE );
}

//-----------------------------------------------------------------------------
// Purpose: Creates a rounded-corner polygon that fits in the specified bounds
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::GetBackgroundPolygonVerts( int x0, int y0, int x1, int y1, int iVerts, vgui::Vertex_t vert[] )
{
	Assert( iVerts == NUM_BACKGROUND_COORD );
	// use the offsets we generated for one corner and apply those to the passed-in dimensions to create verts for the poly
	for ( int i = 0; i < NUM_CORNER_COORD; i++ )
	{
		int j = ( NUM_CORNER_COORD - 1 ) - i;
		// upper left corner
		vert[i].Init( Vector2D( x0 + m_CornerCoord[i].x, y0 + m_CornerCoord[i].y ) );
		// upper right corner
		vert[i + NUM_CORNER_COORD].Init( Vector2D( x1 - m_CornerCoord[j].x, y0 + m_CornerCoord[j].y ) );
		// lower right corner
		vert[i + ( NUM_CORNER_COORD * 2 )].Init( Vector2D( x1 - m_CornerCoord[i].x, y1 - m_CornerCoord[i].y ) );
		// lower left corner
		vert[i + ( NUM_CORNER_COORD * 3 )].Init( Vector2D( x0 + m_CornerCoord[j].x, y1 - m_CornerCoord[j].y ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets specified icon
//-----------------------------------------------------------------------------
CHudTexture *CTFHudDeathNotice::GetIcon( const char *szIcon, bool bInvert )
{
	// get the inverted version if specified
	if ( bInvert && 0 == V_strncmp( "d_", szIcon, 2 ) )
	{
		// change prefix from d_ to dneg_
		char szIconTmp[255] = "dneg_";
		V_strcat( szIconTmp, szIcon + 2, ARRAYSIZE( szIconTmp ) );
		CHudTexture *pIcon = gHUD.GetIcon( szIconTmp );
		// return inverted version if found
		if ( pIcon )
			return pIcon;
		// if we didn't find the inverted version, keep going and try the normal version
	}
	return gHUD.GetIcon( szIcon );
}

//-----------------------------------------------------------------------------
// Purpose: Creates the offsets for rounded corners based on current screen res
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::CalcRoundedCorners()
{
	// generate the offset geometry for upper left corner
	int iMax = ARRAYSIZE( m_CornerCoord );
	for ( int i = 0; i < iMax; i++ )
	{
		m_CornerCoord[i].x = m_flCornerRadius * ( 1 - cos( ( (float)i / (float)( iMax - 1 ) ) * ( M_PI / 2 ) ) );
		m_CornerCoord[i].y = m_flCornerRadius * ( 1 - sin( ( (float)i / (float)( iMax - 1 ) ) * ( M_PI / 2 ) ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds an additional death message
//-----------------------------------------------------------------------------
void CTFHudDeathNotice::AddAdditionalMsg( int iKillerID, int iVictimID, const char *pMsgKey )
{
	DeathNoticeItem &msg2 = m_DeathNotices[AddDeathNoticeItem()];
	Q_strncpy( msg2.Killer.szName, g_PR->GetPlayerName( iKillerID ), ARRAYSIZE( msg2.Killer.szName ) );
	Q_strncpy( msg2.Victim.szName, g_PR->GetPlayerName( iVictimID ), ARRAYSIZE( msg2.Victim.szName ) );

	msg2.Killer.iTeam = g_PR->GetTeam( iKillerID );
	msg2.Victim.iTeam = g_PR->GetTeam( iVictimID );

	msg2.Killer.iPlayerID = iKillerID;
	msg2.Victim.iPlayerID = iVictimID;

	const wchar_t *wzMsg = g_pVGuiLocalize->Find( pMsgKey );
	if ( wzMsg )
	{
		V_wcsncpy( msg2.wzInfoText, wzMsg, sizeof( msg2.wzInfoText ) );
	}
	msg2.iconDeath = m_iconDomination;
	int iLocalPlayerIndex = GetLocalPlayerIndex();
	if ( iLocalPlayerIndex == iVictimID || iLocalPlayerIndex == iKillerID )
	{
		msg2.bLocalPlayerInvolved = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the color to draw text in for this team.  
//-----------------------------------------------------------------------------
Color CTFHudDeathNotice::GetTeamColor( int iTeamNumber )
{
	switch ( iTeamNumber )
	{
	case TF_TEAM_BLUE:
		return m_clrBlueText;
		break;
	case TF_TEAM_RED:
		return m_clrRedText;
		break;
	case TF_TEAM_MERCENARY:
		return m_clrMercenaryText;
		break;
	case TEAM_UNASSIGNED:
		return Color( 0, 0, 0, 255 );
		break;
	default:
		AssertOnce( false );	// invalid team
		return Color( 0, 0, 0, 255 );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the expiry time for this death notice item
//-----------------------------------------------------------------------------
float DeathNoticeItem::GetExpiryTime()
{
	float flDuration = hud_deathnotice_time.GetFloat();
	if ( bLocalPlayerInvolved )
	{
		// if the local player is involved, make the message last longer
		flDuration *= 2;
	}
	return flCreationTime + flDuration;
}
