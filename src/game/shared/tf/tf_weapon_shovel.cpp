//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_shovel.h"

//=============================================================================
//
// Weapon Shovel tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFShovel, DT_TFWeaponShovel )

BEGIN_NETWORK_TABLE( CTFShovel, DT_TFWeaponShovel )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFShovel )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_shovel, CTFShovel );
//PRECACHE_WEAPON_REGISTER( tf_weapon_shovel );

//=============================================================================
//
// Weapon Shovel functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFShovel::CTFShovel()
{
}
