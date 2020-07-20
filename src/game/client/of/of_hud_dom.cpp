//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "iclientmode.h"
#include "ihudlcd.h"
#include "of_hud_dom.h"
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"

using namespace vgui;

DECLARE_HUDELEMENT( CTFHudDOM );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFHudDOM::CTFHudDOM( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudDOM" ) 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD );

	hudlcd->SetGlobalStat( "(score)", "0" );

	m_pRedScore = new CTFScoreProgressRed( this, "RedScore" );
	m_pBluScore = new CTFScoreProgressBlu( this, "BluScore" );
	
	m_nScore	= 0;
	m_flNextThink = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudDOM::Reset()
{
	m_flNextThink = gpGlobals->curtime + 0.05f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudDOM::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudDOM.res" );

	m_pScore = dynamic_cast<CExLabel *>( FindChildByName( "Score" ) );
	m_pScoreShadow = dynamic_cast<CExLabel *>( FindChildByName( "ScoreShadow" ) );

	m_nScore	= -1;
	m_flNextThink = 0.0f;

	UpdateDOMLabel( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHudDOM::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
	{
		return false;
	}
	
	if ( TFGameRules() && ( TFGameRules()->IsDOMGamemode() || TFGameRules()->IsESCGamemode() ) )
		return CHudElement::ShouldDraw();
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudDOM::UpdateDOMLabel( bool bScore )
{
	if ( m_pScore && m_pScoreShadow )
	{
		if ( m_pScore->IsVisible() != bScore )
		{
			m_pScore->SetVisible( bScore );
			m_pScoreShadow->SetVisible( bScore );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Update the team frag meter in DOM
//-----------------------------------------------------------------------------
void CTFHudDOM::OnThink()
{
	// Get the player and active weapon.
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>( g_PR );
	
	if ( m_flNextThink < gpGlobals->curtime )
	{
		if ( !pPlayer )
		{
			hudlcd->SetGlobalStat( "(score)", "n/a" );

			// turn off our ammo counts
			UpdateDOMLabel( false );

			m_nScore = 0;
		}
		else
		{
			// Get the ammo in our clip.
			int iIndex = GetLocalPlayerIndex();
			int nScore = tf_PR->GetPlayerScore( iIndex );
			
			hudlcd->SetGlobalStat( "(score)", VarArgs( "%d", nScore ) );

			m_nScore = nScore;
			
			UpdateDOMLabel( true );
			wchar_t string1[1024];

			m_pRedScore->SetProgress( (float)(TFGameRules()->m_nDomScore_red) / (float)(TFGameRules()->m_nDomScore_limit) );
			m_pBluScore->SetProgress( (float)(TFGameRules()->m_nDomScore_blue) / (float)(TFGameRules()->m_nDomScore_limit) );

			SetDialogVariable( "RedScore", TFGameRules()->m_nDomScore_red );
			SetDialogVariable( "BluScore", TFGameRules()->m_nDomScore_blue );

			SetDialogVariable( "FragLimit", TFGameRules()->m_nDomScore_limit );
			g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find( "#TF_ScoreBoard_Score" ), 1, 1 );

			SetDialogVariable( "scorelabel", string1 );
		}

		m_flNextThink = gpGlobals->curtime + 0.1f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFScoreProgressBlu::Paint()
{
	BaseClass::Paint();

	int x, y, w, h;
	GetBounds( x, y, w, h );

	Vertex_t vert[4];	
	float uv1 = 1.0f;
	float uv2 = 0.0f;
	int xpos = 0;
	int ypos = 0;

	float flProgressX = w * ( m_flProgress );

	// blend in the red "damage" part
	surface()->DrawSetTexture( m_iMaterialIndex );
	
	Vector2D uv11( uv1, uv2 );						//topleft
	Vector2D uv21( uv2, uv2 - m_flProgress );		//topright
	Vector2D uv22( uv2, uv2 - m_flProgress );		//bottomright
	Vector2D uv12( uv1, uv2 );						//bottomleft

	vert[0].Init( Vector2D( xpos, ypos ), uv11 );
	vert[1].Init( Vector2D( flProgressX, ypos ), uv21 );
	vert[2].Init( Vector2D( flProgressX, ypos + h ), uv22 );				
	vert[3].Init( Vector2D( xpos, ypos + h ), uv12 );

	surface()->DrawSetColor( GetFgColor() );
	
	surface()->DrawTexturedPolygon( 4, vert );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFScoreProgressRed::Paint()
{
	BaseClass::Paint();

	int x, y, w, h;
	GetBounds( x, y, w, h );

	Vertex_t vert[4];	
	float uv1 = 1.0f;
	float uv2 = 0.0f;
	int xpos = 0;
	int ypos = 0;

	float flProgressX = w * ( m_flProgress );

	// blend in the red "damage" part
	surface()->DrawSetTexture( m_iMaterialIndex );
	
	Vector2D uv11( uv1, uv2 - m_flProgress);		//topleft
	Vector2D uv21( uv2, uv2 );						//topright
	Vector2D uv22( uv2, uv2 );						//bottomright
	Vector2D uv12( uv1, uv2 - m_flProgress );		//bottomleft

	vert[0].Init( Vector2D( xpos + w - flProgressX, ypos ), uv11 );
	vert[1].Init( Vector2D( xpos + w, ypos ), uv21 );
	vert[2].Init( Vector2D( xpos + w, ypos + h ), uv22 );				
	vert[3].Init( Vector2D( xpos + w - flProgressX, ypos + h ), uv12 );

	surface()->DrawSetColor( GetFgColor() );
	
	surface()->DrawTexturedPolygon( 4, vert );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFScoreProgressBlu::CTFScoreProgressBlu(Panel *parent, const char *panelName) : CTFImagePanel(parent, panelName)
{
	m_flProgress = 1.0f;
	
	m_iMaterialIndex = surface()->DrawGetTextureId( "hud/objectives_tdm_left" );
	if ( m_iMaterialIndex == -1 ) // we didn't find it, so create a new one
	{
		m_iMaterialIndex = surface()->CreateNewTextureID();	
	}

	surface()->DrawSetTextureFile( m_iMaterialIndex, "hud/objectives_tdm_left", true, false );	
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFScoreProgressRed::CTFScoreProgressRed(Panel *parent, const char *panelName) : CTFImagePanel(parent, panelName)
{
	m_flProgress = 1.0f;

	m_iMaterialIndex = surface()->DrawGetTextureId("hud/objectives_tdm_right");
	if (m_iMaterialIndex == -1) // we didn't find it, so create a new one
	{
		m_iMaterialIndex = surface()->CreateNewTextureID();
	}

	surface()->DrawSetTextureFile(m_iMaterialIndex, "hud/objectives_tdm_right", true, false);
}
