//===== Copyright © 1996-2008, Valve Corporation, All rights reserved. ======//
//
// Purpose: Tip display during level loads.
//
//===========================================================================//
#include "cbase.h"
#include "loadingtippanel.h"
#include "vgui/ISurface.h"
#include "tf_tips.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar ui_loading_tip_refresh( "ui_loading_tip_refresh", "5", FCVAR_CHEAT );
ConVar ui_loading_tip_f1( "ui_loading_tip_f1", "0.05", FCVAR_CHEAT );
ConVar ui_loading_tip_f2( "ui_loading_tip_f2", "0.40", FCVAR_CHEAT );

//--------------------------------------------------------------------------------------------------------
CLoadingTipPanel::CLoadingTipPanel( Panel *pParent ) : EditablePanel( pParent, "loadingtippanel" )
{
	m_flLastTipTime = 0.f;
	m_pTipIcon = NULL;

	m_smearColor = Color( 0, 0, 0, 255 );

	SetupTips();
}

//--------------------------------------------------------------------------------------------------------
CLoadingTipPanel::~CLoadingTipPanel()
{
}

//--------------------------------------------------------------------------------------------------------
void CLoadingTipPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_smearColor = pScheme->GetColor( "Frame.SmearColor", Color( 0, 0, 0, 225 ) );

	ReloadScheme();
}

//--------------------------------------------------------------------------------------------------------
void CLoadingTipPanel::ReloadScheme( void )
{
	LoadControlSettings( "resource/ui/loadingtippanel.res" );

	NextTip();
}

//--------------------------------------------------------------------------------------------------------
void CLoadingTipPanel::SetupTips( void )
{

	KeyValues *pKV = new KeyValues( "Tips" );
	KeyValues::AutoDelete autodelete( pKV );
	
	NextTip();

}

//--------------------------------------------------------------------------------------------------------
void CLoadingTipPanel::NextTip( void )
{
	if ( !IsEnabled() )
		return;

	/*
	if ( !m_flLastTipTime )
	{
		// Initialize timer on first render
		m_flLastTipTime = Plat_FloatTime();
		return;
	}

	if ( Plat_FloatTime() - m_flLastTipTime < ui_loading_tip_refresh.GetFloat() )
		return;

	m_flLastTipTime = Plat_FloatTime();
	*/

	SetDialogVariable( "TipText", g_TFTips.GetRandomTip() );
	// Set our control visible
	SetVisible( true );
}


#define TOP_BORDER_HEIGHT		21
#define BOTTOM_BORDER_HEIGHT	21
int CLoadingTipPanel::DrawSmearBackgroundFade( int x0, int y0, int x1, int y1 )
{
	int wide = x1 - x0;
	int tall = y1 - y0;

	int topTall = scheme()->GetProportionalScaledValue( TOP_BORDER_HEIGHT );
	int bottomTall = scheme()->GetProportionalScaledValue( BOTTOM_BORDER_HEIGHT );

	float f1 = ui_loading_tip_f1.GetFloat();
	float f2 = ui_loading_tip_f2.GetFloat();

	topTall  = 1.00f * topTall;
	bottomTall = 1.00f * bottomTall;

	int middleTall = tall - ( topTall + bottomTall );
	if ( middleTall < 0 )
	{
		middleTall = 0;
	}

	surface()->DrawSetColor( m_smearColor );

	y0 += topTall;

	if ( middleTall )
	{
		// middle
		surface()->DrawFilledRectFade( x0, y0, x0 + f1*wide, y0 + middleTall, 0, 255, true );
		surface()->DrawFilledRectFade( x0 + f1*wide, y0, x0 + f2*wide, y0 + middleTall, 255, 255, true );
		surface()->DrawFilledRectFade( x0 + f2*wide, y0, x0 + wide, y0 + middleTall, 255, 0, true );
		y0 += middleTall;
	}

	return topTall + middleTall + bottomTall;
}

//--------------------------------------------------------------------------------------------------------
void CLoadingTipPanel::PaintBackground( void )
{
	BaseClass::PaintBackground();

	DrawSmearBackgroundFade( 
		0, 
		-scheme()->GetProportionalScaledValue( 20 ), 
		GetWide(), 
		GetTall() ); 

}