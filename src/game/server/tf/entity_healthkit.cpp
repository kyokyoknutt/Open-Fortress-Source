//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF HealthKit.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
//#include "tf_shareddefs.h"
#include "tf_team.h"
#include "entity_healthkit.h"

ConVar  of_item_debug("of_item_debug", "0", FCVAR_CHEAT, "Visualize Item bounding boxes." );

//=============================================================================
//
// CTF HealthKit defines.
//

#define TF_HEALTHKIT_PICKUP_SOUND	"HealthKit.Touch"

LINK_ENTITY_TO_CLASS( item_healthkit_full, CHealthKit );
LINK_ENTITY_TO_CLASS( item_healthkit_small, CHealthKitSmall );
LINK_ENTITY_TO_CLASS( item_healthkit_medium, CHealthKitMedium );

BEGIN_DATADESC( CHealthKit )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

BEGIN_DATADESC( CHealthKitSmall )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

BEGIN_DATADESC( CHealthKitMedium )

// Inputs.
DEFINE_KEYFIELD( m_iszModel, FIELD_STRING, "model" ),
DEFINE_KEYFIELD( m_iszModelOLD, FIELD_STRING, "powerup_model" ),
DEFINE_KEYFIELD( m_iszPickupSound, FIELD_STRING, "pickup_sound" ),

END_DATADESC()

IMPLEMENT_AUTO_LIST( IHealthKitAutoList );

//=============================================================================
//
// CTF HealthKit functions.
//

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Spawn( void )
{
	Precache();

	if ( m_iszModel == MAKE_STRING( "" ) )
	{
		if ( m_iszModelOLD != MAKE_STRING( "" ) )
			SetModel( STRING(m_iszModelOLD) );
		else
			SetModel( GetPowerupModel() );
	}
	else
	{
		SetModel(STRING(m_iszModel));
	}

	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: Precache function for the healthkit
//-----------------------------------------------------------------------------
void CHealthKit::Precache( void )
{
	if ( m_iszModel == MAKE_STRING( "" ) )
	{
		if ( m_iszModelOLD != MAKE_STRING( "" ) )
			PrecacheModel( STRING(m_iszModelOLD) );
		else
			PrecacheModel( GetPowerupModel() );
	}
	else
	{
		PrecacheModel(STRING(m_iszModel));
	}

	PrecacheScriptSound( TF_HEALTHKIT_PICKUP_SOUND );
}

//-----------------------------------------------------------------------------
// Purpose: MyTouch function for the healthkit
//-----------------------------------------------------------------------------
bool CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	bool bSuccess = false;
	
	// Render debug visualization
	if ( of_item_debug.GetBool() )
		NDebugOverlay::EntityBounds(this, 0, 100, 255, 0 ,0) ;

	if (!ValidTouch(pPlayer))
		return bSuccess;

	if ( pPlayer->TakeHealth( ceil(pPlayer->GetMaxHealth() * PackRatios[GetPowerupSize()]), DMG_GENERIC ) )
	{
		CSingleUserRecipientFilter user( pPlayer );
		user.MakeReliable();

		UserMessageBegin( user, "ItemPickup" );
			WRITE_STRING( GetClassname() );
		MessageEnd();

		EmitSound( user, entindex(), TF_HEALTHKIT_PICKUP_SOUND );

		bSuccess = true;

		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

		Assert( pTFPlayer );

		// Healthkits cures burning...
		if (pTFPlayer->m_Shared.InCond(TF_COND_BURNING))
			pTFPlayer->m_Shared.RemoveCond( TF_COND_BURNING );
		//...and poison
		if (pTFPlayer->m_Shared.InCond(TF_COND_POISON))
			pTFPlayer->m_Shared.RemoveCond(TF_COND_POISON);
		AddEffects( EF_NODRAW );
	}

	return bSuccess;
}
