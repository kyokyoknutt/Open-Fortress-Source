//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudSwapWeapons : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudSwapWeapons, EditablePanel );

public:
	CHudSwapWeapons( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	FireGameEvent( IGameEvent * event );

private:
	ImagePanel *m_pSwapWepImg;
	ImagePanel *m_pCurrentWepImg;
	float flHideTick;
	int m_iCurrentWeaponID;
	int m_iSwapWeaponID;
};

DECLARE_HUDELEMENT( CHudSwapWeapons );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudSwapWeapons::CHudSwapWeapons( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudSwapWeapons" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	flHideTick = 0.0f;
	m_iCurrentWeaponID = 0;
	m_iSwapWeaponID = 0;
	
	m_pSwapWepImg = new ImagePanel( this, "WeaponToSwapTo" );
	m_pCurrentWepImg = new ImagePanel( this, "CurrentWeapon" );
	
	ListenForGameEvent( "player_swap_weapons" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSwapWeapons::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...

	LoadControlSettings( "resource/UI/HudSwapWeapons.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudSwapWeapons::ShouldDraw( void )
{
	if ( flHideTick <= gpGlobals->curtime )
	{
		flHideTick = 0.0f;
		return false;
	}
	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSwapWeapons::FireGameEvent( IGameEvent * event )
{
	const char *eventname = event->GetName();

	if ( Q_strcmp( "player_swap_weapons", eventname ) == 0 )
	{
		// load control settings...
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pPlayer )
			return;
		
		if ( event->GetInt("playerid") != pPlayer->entindex() )
			return;
		const char *pszWeaponName = WeaponIdToClassname( event->GetInt("current_wep") );
		WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pszWeaponName );
		CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
		if( pWeaponInfo && pWeaponInfo->iconActive )
		{
			char temp[128];
			Q_snprintf( temp, sizeof(temp), "../%s",pWeaponInfo->iconActive->szTextureFile );
			m_pCurrentWepImg->SetImage( temp );
		}
		pszWeaponName = WeaponIdToClassname( event->GetInt("swap_wep") );
		hWpnInfo = LookupWeaponInfoSlot( pszWeaponName );
		pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
		if( pWeaponInfo && pWeaponInfo->iconActive )
		{
			char temp[128];
			Q_snprintf( temp, sizeof(temp), "../%s",pWeaponInfo->iconActive->szTextureFile );
			m_pSwapWepImg->SetImage( temp );
		}
		
		const char *key = engine->Key_LookupBinding( "+use" );
		if ( !key )
		{
			key = "< not bound >";
		}
		SetDialogVariable( "key", key );
		
		flHideTick = gpGlobals->curtime + 0.1f;
	}
}