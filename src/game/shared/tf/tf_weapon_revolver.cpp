//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_revolver.h"

#ifdef CLIENT_DLL 
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Revolver tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFRevolver, DT_WeaponRevolver )

BEGIN_NETWORK_TABLE( CTFRevolver, DT_WeaponRevolver )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRevolver )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_revolver, CTFRevolver );
//PRECACHE_WEAPON_REGISTER( tf_weapon_revolver );

IMPLEMENT_NETWORKCLASS_ALIASED( TFRevolver_Mercenary, DT_WeaponRevolver_Mercenary )

BEGIN_NETWORK_TABLE( CTFRevolver_Mercenary, DT_WeaponRevolver_Mercenary )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFRevolver_Mercenary )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_revolver_mercenary, CTFRevolver_Mercenary );
//PRECACHE_WEAPON_REGISTER( tf_weapon_revolver_mercenary );

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFRevolver )
END_DATADESC()
#endif

CTFRevolver::CTFRevolver()
{
}

acttable_t m_acttableRevolver[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_REVOLVER_MERCENARY, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_REVOLVER_MERCENARY, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_REVOLVER_MERCENARY, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_REVOLVER_MERCENARY, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_REVOLVER_MERCENARY, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_REVOLVER_MERCENARY, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_REVOLVER_MERCENARY, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_REVOLVER_MERCENARY, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_REVOLVER_MERCENARY, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_REVOLVER_MERCENARY, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_REVOLVER_MERCENARY, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_REVOLVER_MERCENARY, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_REVOLVER_MERCENARY, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_REVOLVER_MERCENARY, false },

	{ ACT_MP_RELOAD_STAND, ACT_MERC_RELOAD_STAND_REVOLVER_MERCENARY, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MERC_RELOAD_CROUCH_REVOLVER_MERCENARY, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MERC_RELOAD_SWIM_REVOLVER_MERCENARY, false },
};

// So soldier holds it somewhat correctly
acttable_t m_acttableRevolver2[] =
{
	{ ACT_MP_STAND_IDLE,					ACT_MP_STAND_SECONDARY2,			false },
	{ ACT_MP_CROUCH_IDLE,					ACT_MP_CROUCH_SECONDARY2,			false },
	{ ACT_MP_RUN,							ACT_MP_RUN_SECONDARY2,				false },
	{ ACT_MP_WALK,							ACT_MP_WALK_SECONDARY2,			false },
	{ ACT_MP_AIRWALK,						ACT_MP_AIRWALK_SECONDARY2,			false },
	{ ACT_MP_CROUCHWALK,					ACT_MP_CROUCHWALK_SECONDARY2,		false },
	{ ACT_MP_JUMP,							ACT_MP_JUMP_SECONDARY2,			false },
	{ ACT_MP_JUMP_START,					ACT_MP_JUMP_START_SECONDARY2,		false },
	{ ACT_MP_JUMP_FLOAT,					ACT_MP_JUMP_FLOAT_SECONDARY2,		false },
	{ ACT_MP_JUMP_LAND,						ACT_MP_JUMP_LAND_SECONDARY2,		false },
	{ ACT_MP_SWIM,							ACT_MP_SWIM_SECONDARY2,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_SECONDARY2,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_SECONDARY2,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_SECONDARY2,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_SECONDARY2,		false },

	{ ACT_MP_RELOAD_STAND,					ACT_MP_RELOAD_STAND_SECONDARY2_LOOP,	false },
	{ ACT_MP_RELOAD_CROUCH,					ACT_MP_RELOAD_CROUCH_SECONDARY2_LOOP,	false },
	{ ACT_MP_RELOAD_SWIM,					ACT_MP_RELOAD_SWIM_SECONDARY2_LOOP,		false },
	{ ACT_MP_RELOAD_AIRWALK,				ACT_MP_RELOAD_AIRWALK_SECONDARY2_LOOP,		false },
};

//Act table remapping for Merc
acttable_t *CTFRevolver::ActivityList(int &iActivityCount)
{
	if ( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY )
	{
		iActivityCount = ARRAYSIZE( m_acttableRevolver );
		return m_acttableRevolver;
	}
	else if ( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_SOLDIER )
	{
		iActivityCount = ARRAYSIZE( m_acttableRevolver2 );
		return m_acttableRevolver2;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}

//Act table remapping for Merc
acttable_t *CTFRevolver_Mercenary::ActivityList(int &iActivityCount)
{
	if ( !GetTFPlayerOwner() )
		return BaseClass::ActivityList(iActivityCount);

	if ( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY )
	{
		iActivityCount = ARRAYSIZE( m_acttableRevolver );
		return m_acttableRevolver;
	}
	else if ( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_SOLDIER )
	{
		iActivityCount = ARRAYSIZE( m_acttableRevolver2 );
		return m_acttableRevolver2;
	}
	return BaseClass::ActivityList(iActivityCount);
}
