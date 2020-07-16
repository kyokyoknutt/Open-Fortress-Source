//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "c_tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_TFAmmoPack : public C_BaseAnimating, public ITargetIDProvidesHint
{
	DECLARE_CLASS( C_TFAmmoPack, C_BaseAnimating );

public:

	DECLARE_CLIENTCLASS();

	virtual void	OnDataChanged( DataUpdateType_t updateType );
	virtual bool	Interpolate( float currentTime );

	// ITargetIDProvidesHint
public:
	virtual void	DisplayHintTo( C_BasePlayer *pPlayer );

private:

	Vector		m_vecInitialVelocity;

};

// Network table.
IMPLEMENT_CLIENTCLASS_DT( C_TFAmmoPack, DT_AmmoPack, CTFAmmoPack )
	RecvPropVector( RECVINFO( m_vecInitialVelocity ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_TFAmmoPack::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{ 
		float flChangeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );
		Vector vecCurOrigin = GetLocalOrigin();

		// Now stick our initial velocity into the interpolation history 
		CInterpolatedVar< Vector > &interpolator = GetOriginInterpolator();
		interpolator.ClearHistory();
		interpolator.AddToHead( flChangeTime - 0.15f, &vecCurOrigin, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : currentTime - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFAmmoPack::Interpolate( float currentTime )
{
	return BaseClass::Interpolate( currentTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void C_TFAmmoPack::DisplayHintTo( C_BasePlayer *pPlayer )
{
	C_TFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if ( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		pTFPlayer->HintMessage( HINT_ENGINEER_PICKUP_METAL );
	}
	else
	{
		pTFPlayer->HintMessage( HINT_PICKUP_AMMO );
	}
}