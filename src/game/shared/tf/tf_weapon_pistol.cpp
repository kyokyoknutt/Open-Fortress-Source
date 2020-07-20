//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_pistol.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
#endif

//=============================================================================
//
// Weapon Pistol tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol, DT_WeaponPistol )

BEGIN_NETWORK_TABLE_NOBASE( CTFPistol, DT_PistolLocalData )
#ifdef GAME_DLL
	SendPropTime( SENDINFO( m_flSoonestPrimaryAttack ) ),
#else
	RecvPropTime( RECVINFO( m_flSoonestPrimaryAttack ) ),
#endif
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CTFPistol, DT_WeaponPistol )
#ifdef GAME_DLL
	SendPropDataTable( "PistolLocalData", 0, &REFERENCE_SEND_TABLE( DT_PistolLocalData ), SendProxy_SendLocalWeaponDataTable ),
#else
	RecvPropDataTable( "PistolLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_PistolLocalData ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_flSoonestPrimaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pistol, CTFPistol );
//PRECACHE_WEAPON_REGISTER( tf_weapon_pistol );

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFPistol )
END_DATADESC()
#endif

//============================

IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol_Scout, DT_WeaponPistol_Scout )

BEGIN_NETWORK_TABLE( CTFPistol_Scout, DT_WeaponPistol_Scout )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol_Scout )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pistol_scout, CTFPistol_Scout );
//PRECACHE_WEAPON_REGISTER( tf_weapon_pistol_scout );


//
IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol_Mercenary, DT_WeaponPistol_Mercenary )

BEGIN_NETWORK_TABLE( CTFPistol_Mercenary, DT_WeaponPistol_Mercenary )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol_Mercenary )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pistol_mercenary, CTFPistol_Mercenary );
//PRECACHE_WEAPON_REGISTER( tf_weapon_pistol_mercenary );
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFPistol_Akimbo, DT_WeaponPistol_Akimbo )

BEGIN_NETWORK_TABLE( CTFPistol_Akimbo, DT_WeaponPistol_Akimbo )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFPistol_Akimbo )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_pistol_akimbo, CTFPistol_Akimbo );
//PRECACHE_WEAPON_REGISTER( tf_weapon_pistol_akimbo );
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFCRailPistol, DT_TFCRailPistol )

BEGIN_NETWORK_TABLE( CTFCRailPistol, DT_TFCRailPistol )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCRailPistol)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_railpistol, CTFCRailPistol);
//PRECACHE_WEAPON_REGISTER( tfc_weapon_railpistol );

//=============================================================================
//
// Weapon Pistol functions.
//

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPistol::CTFPistol( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CTFPistol::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;

	BaseClass::ItemPostFrame();

	if ( m_bInReload )
		return;

	//Allow a refire as fast as the player can click
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPistol::PrimaryAttack( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime + PISTOL_FASTEST_REFIRE_TIME;
#if 0
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if( pOwner )
	{
		// Each time the player fires the pistol, reset the view punch. This prevents
		// the aim from 'drifting off' when the player fires very quickly. This may
		// not be the ideal way to achieve this, but it's cheap and it works, which is
		// great for a feature we're evaluating. (sjb)
		//pOwner->ViewPunchReset();
	}
#endif

	if ( !CanAttack() )
		return;

	BaseClass::PrimaryAttack();
}

//Act tables for Merc
acttable_t m_acttablePistol[] =
{
	{ ACT_MP_STAND_IDLE,					ACT_MERC_STAND_PISTOL_MERCENARY,			false },
	{ ACT_MP_CROUCH_IDLE,					ACT_MERC_CROUCH_PISTOL_MERCENARY,			false },
	{ ACT_MP_RUN,							ACT_MERC_RUN_PISTOL_MERCENARY,				false },
	{ ACT_MP_WALK,							ACT_MERC_WALK_PISTOL_MERCENARY,			false },
	{ ACT_MP_AIRWALK,						ACT_MERC_AIRWALK_PISTOL_MERCENARY,			false },
	{ ACT_MP_CROUCHWALK,					ACT_MERC_CROUCHWALK_PISTOL_MERCENARY,		false },
	{ ACT_MP_JUMP,							ACT_MERC_JUMP_PISTOL_MERCENARY,			false },
	{ ACT_MP_JUMP_START,					ACT_MERC_JUMP_START_PISTOL_MERCENARY,		false },
	{ ACT_MP_JUMP_FLOAT,					ACT_MERC_JUMP_FLOAT_PISTOL_MERCENARY,		false },
	{ ACT_MP_JUMP_LAND,						ACT_MERC_JUMP_LAND_PISTOL_MERCENARY,		false },
	{ ACT_MP_SWIM,							ACT_MERC_SWIM_PISTOL_MERCENARY,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MERC_ATTACK_STAND_PISTOL_MERCENARY,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MERC_ATTACK_CROUCH_PISTOL_MERCENARY,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MERC_ATTACK_SWIM_PISTOL_MERCENARY,		false },

	{ ACT_MP_RELOAD_STAND,					ACT_MERC_RELOAD_STAND_PISTOL_MERCENARY,	false },
	{ ACT_MP_RELOAD_CROUCH,					ACT_MERC_RELOAD_CROUCH_PISTOL_MERCENARY,	false },
	{ ACT_MP_RELOAD_SWIM,					ACT_MERC_RELOAD_SWIM_PISTOL_MERCENARY,		false },
};

acttable_t CTFPistol_Akimbo::m_acttablePistolAkimbo[] =
{
	{ ACT_MP_STAND_IDLE,					ACT_MERC_STAND_PISTOL_AKIMBO,			false },
	{ ACT_MP_CROUCH_IDLE,					ACT_MERC_CROUCH_PISTOL_AKIMBO,			false },
	{ ACT_MP_RUN,							ACT_MERC_RUN_PISTOL_AKIMBO,				false },
	{ ACT_MP_WALK,							ACT_MERC_WALK_PISTOL_AKIMBO,			false },
	{ ACT_MP_AIRWALK,						ACT_MERC_AIRWALK_PISTOL_AKIMBO,			false },
	{ ACT_MP_CROUCHWALK,					ACT_MERC_CROUCHWALK_PISTOL_AKIMBO,		false },
	{ ACT_MP_JUMP,							ACT_MERC_JUMP_PISTOL_AKIMBO,			false },
	{ ACT_MP_JUMP_START,					ACT_MERC_JUMP_START_PISTOL_AKIMBO,		false },
	{ ACT_MP_JUMP_FLOAT,					ACT_MERC_JUMP_FLOAT_PISTOL_AKIMBO,		false },
	{ ACT_MP_JUMP_LAND,						ACT_MERC_JUMP_LAND_PISTOL_AKIMBO,		false },
	{ ACT_MP_SWIM,							ACT_MERC_SWIM_PISTOL_AKIMBO,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MERC_ATTACK_STAND_PISTOL_AKIMBO,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MERC_ATTACK_CROUCH_PISTOL_AKIMBO,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MERC_ATTACK_SWIM_PISTOL_AKIMBO,		false },

	{ ACT_MP_RELOAD_STAND,					ACT_MERC_RELOAD_STAND_PISTOL_AKIMBO,	false },
	{ ACT_MP_RELOAD_CROUCH,					ACT_MERC_RELOAD_CROUCH_PISTOL_AKIMBO,	false },
	{ ACT_MP_RELOAD_SWIM,					ACT_MERC_RELOAD_SWIM_PISTOL_AKIMBO,		false },
};

// So soldier holds it somewhat correctly
acttable_t m_acttableSecondary2[] =
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
acttable_t *CTFPistol::ActivityList( int &iActivityCount )
{
	if ( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY )
	{
		iActivityCount = ARRAYSIZE( m_acttablePistol );
		return m_acttablePistol;
	}
	else if ( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_SOLDIER )
	{
		iActivityCount = ARRAYSIZE( m_acttableSecondary2 );
		return m_acttableSecondary2;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}

acttable_t *CTFPistol_Mercenary::ActivityList( int &iActivityCount )
{
	if ( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY )
	{
		iActivityCount = ARRAYSIZE( m_acttablePistol );
		return m_acttablePistol;
	}
	else if ( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_SOLDIER )
	{
		iActivityCount = ARRAYSIZE( m_acttableSecondary2 );
		return m_acttableSecondary2;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}

acttable_t *CTFPistol_Akimbo::ActivityList( int &iActivityCount )
{
	if ( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY )
	{
		iActivityCount = ARRAYSIZE( m_acttablePistolAkimbo );
		return m_acttablePistolAkimbo;
	}
	else if ( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_SOLDIER )
	{
		iActivityCount = ARRAYSIZE( m_acttableSecondary2 );
		return m_acttableSecondary2;
	}
	return BaseClass::ActivityList(iActivityCount);
}
