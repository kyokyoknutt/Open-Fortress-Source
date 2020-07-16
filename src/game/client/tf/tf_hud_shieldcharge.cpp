//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "tf_weaponbase_melee.h"
#include "iclientmode.h"
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudShieldCharge : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudShieldCharge, EditablePanel );

public:
	CHudShieldCharge( const char *pElementName );

	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );
	virtual void	OnTick( void );

private:
	vgui::ContinuousProgressBar *m_pChargeMeter;
};

DECLARE_HUDELEMENT( CHudShieldCharge );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudShieldCharge::CHudShieldCharge( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudShieldChargeMeter" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pChargeMeter = new ContinuousProgressBar( this, "ItemEffectMeter" );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudShieldCharge::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/HudShieldChargeMeter.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudShieldCharge::ShouldDraw( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
	{
		return false;
	}

	if ( !pPlayer->IsAlive() )
	{
		return false;
	}
	
	CTFWeaponBaseMelee *pWeapon = dynamic_cast<CTFWeaponBaseMelee*>( pPlayer->GetActiveWeapon() );
	
	if ( !pWeapon )
		return false;
	
	if ( !pWeapon->m_pWeaponInfo->m_bCanShieldCharge )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudShieldCharge::OnTick( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return;

	CTFWeaponBaseMelee *pWeapon = dynamic_cast<CTFWeaponBaseMelee*>( pPlayer->GetActiveWeapon() );
	
	if ( !pWeapon )
		return;
	
	if ( !pWeapon->m_pWeaponInfo->m_bCanShieldCharge )
		return;	
	
	if ( m_pChargeMeter )
	{
		m_pChargeMeter->SetProgress( pWeapon->GetShieldChargeMeter() );
		SetDialogVariable( "progresscount", pWeapon->GetShieldChargeMeter() * 100.0f );
	}
}