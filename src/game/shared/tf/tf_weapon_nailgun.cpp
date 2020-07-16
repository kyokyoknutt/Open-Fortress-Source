//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_nailgun.h"

//=============================================================================
//
// Weapon SMG tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFNailgun, DT_WeaponNailgun )

BEGIN_NETWORK_TABLE( CTFNailgun, DT_WeaponNailgun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFNailgun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_nailgun, CTFNailgun );
//PRECACHE_WEAPON_REGISTER( tf_weapon_nailgun);

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFNailgun )
END_DATADESC()
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFCNailgun, DT_TFCNailgun )

BEGIN_NETWORK_TABLE( CTFCNailgun, DT_TFCNailgun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCNailgun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_nailgun, CTFCNailgun );
//PRECACHE_WEAPON_REGISTER( tfc_weapon_nailgun);

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFCNailgun )
END_DATADESC()
#endif


IMPLEMENT_NETWORKCLASS_ALIASED( TFCNailgunSuper, DT_TFCNailgunSuper )

BEGIN_NETWORK_TABLE( CTFCNailgunSuper, DT_TFCNailgunSuper )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCNailgunSuper )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_nailgun_super, CTFCNailgunSuper );
//PRECACHE_WEAPON_REGISTER( tfc_weapon_nailgun_super );

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFCNailgunSuper )
END_DATADESC()
#endif

//=============================================================================
//
// Weapon Nailgun functions.
//