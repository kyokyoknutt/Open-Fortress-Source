//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_smg.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
#endif

//=============================================================================
//
// SMG
//
//=============================================================================
IMPLEMENT_NETWORKCLASS_ALIASED( TFSMG, DT_WeaponSMG )

BEGIN_NETWORK_TABLE( CTFSMG, DT_WeaponSMG )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSMG )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_smg, CTFSMG );
//PRECACHE_WEAPON_REGISTER( tf_weapon_smg );

//=============================================================================
//
// Merc SMG
//
//=============================================================================
IMPLEMENT_NETWORKCLASS_ALIASED( TFSMG_Mercenary, DT_WeaponSMG_Mercenary )

BEGIN_NETWORK_TABLE( CTFSMG_Mercenary, DT_WeaponSMG_Mercenary )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFSMG_Mercenary )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_smg_mercenary, CTFSMG_Mercenary );
//PRECACHE_WEAPON_REGISTER( tf_weapon_smg_mercenary );

//=============================================================================
//
// Tommy Gun
//
//=============================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( TFTommyGun, DT_WeaponTommyGun )

BEGIN_NETWORK_TABLE( CTFTommyGun, DT_WeaponTommyGun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFTommyGun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_tommygun, CTFTommyGun );
//PRECACHE_WEAPON_REGISTER( tf_weapon_tommygun );


//=============================================================================
//
// AR
//
//=============================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( TFAssaultRifle, DT_WeaponAssaultRifle )

BEGIN_NETWORK_TABLE( CTFAssaultRifle, DT_WeaponAssaultRifle )
END_NETWORK_TABLE()


BEGIN_PREDICTION_DATA( CTFAssaultRifle )
END_PREDICTION_DATA()


LINK_ENTITY_TO_CLASS( tf_weapon_assaultrifle, CTFAssaultRifle );
//PRECACHE_WEAPON_REGISTER( tf_weapon_assaultrifle );

//=============================================================================
//
// TFC Sniper Assault Rifle
//
//=============================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( TFCAssaultRifle, DT_TFCAssaultRifle)

BEGIN_NETWORK_TABLE( CTFCAssaultRifle, DT_TFCAssaultRifle)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCAssaultRifle)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_assault_rifle, CTFCAssaultRifle);
//PRECACHE_WEAPON_REGISTER( tfc_weapon_assault_rifle );

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFSMG )
END_DATADESC()
#endif


acttable_t CTFSMG_Mercenary::m_acttableSMG_Mercenary[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_SMG_MERCENARY, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_SMG_MERCENARY, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_SMG_MERCENARY, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_SMG_MERCENARY, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_SMG_MERCENARY, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_SMG_MERCENARY, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_SMG_MERCENARY, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_SMG_MERCENARY, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_SMG_MERCENARY, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_SMG_MERCENARY, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_SMG_MERCENARY, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_SMG_MERCENARY, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_SMG_MERCENARY, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_SMG_MERCENARY, false },

	{ ACT_MP_RELOAD_STAND, ACT_MERC_RELOAD_STAND_SMG_MERCENARY, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MERC_RELOAD_CROUCH_SMG_MERCENARY, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MERC_RELOAD_SWIM_SMG_MERCENARY, false },
};

//Act table remapping for Merc
acttable_t *CTFSMG_Mercenary::ActivityList(int &iActivityCount)
{
	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableSMG_Mercenary);
		return m_acttableSMG_Mercenary;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}
