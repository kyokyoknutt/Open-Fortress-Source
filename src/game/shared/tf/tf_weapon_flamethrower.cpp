//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Flame Thrower
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_flamethrower.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL 
	#include "c_tf_player.h"
	#include "soundenvelope.h"
    #include "dlight.h"
    #include "iefx.h"
#else
	#include "tf_gamerules.h"
	#include "tf_gamestats.h"
	#include "ilagcompensationmanager.h"
	#include "collisionutils.h"
	#include "tf_team.h"
	#include "tf_obj.h"
	#include "ai_basenpc.h"
	#include "tf_bot_manager.h"

	ConVar	tf_debug_flamethrower("tf_debug_flamethrower", "0", FCVAR_CHEAT, "Visualize the flamethrower damage." );
	ConVar  tf_flamethrower_velocity( "tf_flamethrower_velocity", "2300.0", FCVAR_CHEAT, "Initial velocity of flame damage entities." );
	ConVar	tf_flamethrower_drag("tf_flamethrower_drag", "0.89", FCVAR_CHEAT, "Air drag of flame damage entities." );
	ConVar	tf_flamethrower_float("tf_flamethrower_float", "50.0", FCVAR_CHEAT, "Upward float velocity of flame damage entities." );
	ConVar  tf_flamethrower_flametime("tf_flamethrower_flametime", "0.5", FCVAR_CHEAT, "Time to live of flame damage entities." );
	ConVar  tf_flamethrower_vecrand("tf_flamethrower_vecrand", "0.05", FCVAR_CHEAT, "Random vector added to initial velocity of flame damage entities." );
	ConVar  tf_flamethrower_boxsize("tf_flamethrower_boxsize", "8.0", FCVAR_CHEAT, "Size of flame damage entities." );
	ConVar  tf_flamethrower_maxdamagedist("tf_flamethrower_maxdamagedist", "350.0", FCVAR_CHEAT, "Maximum damage distance for flamethrower." );
	ConVar  tf_flamethrower_shortrangedamagemultiplier("tf_flamethrower_shortrangedamagemultiplier", "1.2", FCVAR_CHEAT, "Damage multiplier for close-in flamethrower damage." );
	ConVar  tf_flamethrower_velocityfadestart("tf_flamethrower_velocityfadestart", ".3", FCVAR_CHEAT, "Time at which attacker's velocity contribution starts to fade." );
	ConVar  tf_flamethrower_velocityfadeend("tf_flamethrower_velocityfadeend", ".5", FCVAR_CHEAT, "Time at which attacker's velocity contribution finishes fading." );
	//ConVar  tf_flame_force( "tf_flame_force", "30" );
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// position of end of muzzle relative to shoot position
#define TF_FLAMETHROWER_MUZZLEPOS_FORWARD		70.0f
#define TF_FLAMETHROWER_MUZZLEPOS_RIGHT			12.0f
#define TF_FLAMETHROWER_MUZZLEPOS_UP			-12.0f

#define TF_FLAMETHROWER_AMMO_PER_SECOND_PRIMARY_ATTACK		14.0f
#define TF_FLAMETHROWER_AMMO_PER_SECONDARY_ATTACK	20

IMPLEMENT_NETWORKCLASS_ALIASED( TFFlameThrower, DT_WeaponFlameThrower )

BEGIN_NETWORK_TABLE( CTFFlameThrower, DT_WeaponFlameThrower )
	#if defined( CLIENT_DLL )
		RecvPropInt( RECVINFO( m_iWeaponState ) ),
		RecvPropInt( RECVINFO( m_bCritFire ) ),

	#else
		SendPropInt( SENDINFO( m_iWeaponState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
		SendPropInt( SENDINFO( m_bCritFire ) ),
	#endif
END_NETWORK_TABLE()

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CTFFlameThrower )
	DEFINE_PRED_FIELD( m_iWeaponState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bCritFire, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_flamethrower, CTFFlameThrower );
//PRECACHE_WEAPON_REGISTER( tf_weapon_flamethrower );

BEGIN_DATADESC( CTFFlameThrower )
END_DATADESC()

IMPLEMENT_NETWORKCLASS_ALIASED( TFCFlameThrower, DT_TFCFlameThrower )

BEGIN_NETWORK_TABLE( CTFCFlameThrower, DT_TFCFlameThrower )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tfc_weapon_flamethrower, CTFCFlameThrower );
//PRECACHE_WEAPON_REGISTER( tfc_weapon_flamethrower );

BEGIN_DATADESC( CTFCFlameThrower )
END_DATADESC()

#ifdef CLIENT_DLL
extern ConVar of_muzzlelight;
#endif

extern ConVar of_infiniteammo;

void CTFFlameThrower::Precache( void )
{
	BaseClass::Precache();

	PrecacheParticleSystem( "pyro_blast" );
	PrecacheParticleSystem( "deflect_fx" );

	PrecacheScriptSound( "TFPlayer.AirBlastImpact" );
	PrecacheScriptSound( "TFPlayer.FlameOut" );
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttack" );
	PrecacheScriptSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
	PrecacheScriptSound( "Weapon_FlameThrower.FireHit" );
}

// ------------------------------------------------------------------------------------------------ //
// CTFFlameThrower implementation.
// ------------------------------------------------------------------------------------------------ //
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlameThrower::CTFFlameThrower()
{
	WeaponReset();

#if defined( CLIENT_DLL )
	m_pFiringStartSound = NULL;
	m_pFiringLoop = NULL;
	m_bFiringLoopCritical = false;
	m_pPilotLightSound = NULL;
	m_pFireParticle = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFFlameThrower::~CTFFlameThrower()
{
	DestroySounds();
}

void CTFFlameThrower::DestroySounds( void )
{
#if defined( CLIENT_DLL )
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
	if ( m_pFiringStartSound )
	{
		controller.SoundDestroy( m_pFiringStartSound );
		m_pFiringStartSound = NULL;
	}
	if ( m_pFiringLoop )
	{
		controller.SoundDestroy( m_pFiringLoop );
		m_pFiringLoop = NULL;
	}
	if ( m_pPilotLightSound )
	{
		controller.SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}
#endif

}
void CTFFlameThrower::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponState = FT_STATE_IDLE;
	m_bCritFire = false;
	m_flStartFiringTime = 0;
	m_flAmmoUseRemainder = 0;

	DestroySounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::Spawn( void )
{
	m_iAltFireHint = HINT_ALTFIRE_FLAMETHROWER;
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_iWeaponState = FT_STATE_IDLE;
	m_bCritFire = false;

#if defined ( CLIENT_DLL )
	StopFlame();
	StopPilotLight();
#endif

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::ItemPostFrame()
{
	if ( m_bLowered )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	int iAmmo = ReserveAmmo();
	
	// TODO: add a delay here
	if ( pOwner->IsAlive() && ( pOwner->m_nButtons & IN_ATTACK2 ) && iAmmo >= TF_FLAMETHROWER_AMMO_PER_SECONDARY_ATTACK && CanPerformSecondaryAttack() )
	{
		SecondaryAttack();
	}
	else if ( pOwner->IsAlive() && ( pOwner->m_nButtons & IN_ATTACK ) && ((!( pOwner->m_nButtons & IN_ATTACK2 ) && CanPerformSecondaryAttack()) || iAmmo < TF_FLAMETHROWER_AMMO_PER_SECONDARY_ATTACK) && iAmmo > 0 )
	{
		PrimaryAttack();
	}
	else if ( m_iWeaponState > FT_STATE_IDLE )
	{
		SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
		pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_POST );
		m_iWeaponState = FT_STATE_IDLE;
		m_bCritFire = false;
	}

	BaseClass::ItemPostFrame();
}

class CTraceFilterIgnoreObjects : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnoreObjects, CTraceFilterSimple );

	CTraceFilterIgnoreObjects( const IHandleEntity *passentity, int collisionGroup )
		: CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		// check if the entity is a building or NPC
		if (pEntity && (pEntity->IsBaseObject() || pEntity->IsNPC()))
			return false;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::PrimaryAttack()
{
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	if ( !CanPerformSecondaryAttack() && ReserveAmmo() >= TF_FLAMETHROWER_AMMO_PER_SECONDARY_ATTACK )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
	{
#if defined ( CLIENT_DLL )
		StopFlame();
#endif
		m_iWeaponState = FT_STATE_IDLE;
		return;
	}

	CalcIsAttackCritical();

	// Because the muzzle is so long, it can stick through a wall if the player is right up against it.
	// Make sure the weapon can't fire in this condition by tracing a line between the eye point and the end of the muzzle.
	trace_t trace;	
	Vector vecEye = pOwner->EyePosition();
	Vector vecMuzzlePos = GetVisualMuzzlePos();
	CTraceFilterIgnoreObjects traceFilter( this, COLLISION_GROUP_NONE );
	UTIL_TraceLine( vecEye, vecMuzzlePos, MASK_SOLID, &traceFilter, &trace );
	if ( trace.fraction < 1.0 && ( !trace.m_pEnt || trace.m_pEnt->m_takedamage == DAMAGE_NO ) )
	{
		// there is something between the eye and the end of the muzzle, most likely a wall, don't fire, and stop firing if we already are
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
#if defined ( CLIENT_DLL )
			StopFlame();
#endif
			m_iWeaponState = FT_STATE_IDLE;
		}
		return;
	}

	switch ( m_iWeaponState )
	{
	case FT_STATE_IDLE:
		{
			// Just started, play PRE and start looping view model anim

			pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRE );

			SendWeaponAnim( ACT_VM_PRIMARYATTACK );

			m_flStartFiringTime = gpGlobals->curtime;

			m_iWeaponState = FT_STATE_STARTFIRING;
		}
		break;
	case FT_STATE_STARTFIRING:
		{
			// if some time has elapsed, start playing the looping third person anim
			if ( gpGlobals->curtime > m_flStartFiringTime )
			{
				m_iWeaponState = FT_STATE_FIRING;
				m_flNextPrimaryAttackAnim = gpGlobals->curtime;
			}
		}
		break;
	case FT_STATE_FIRING:
		{
			if ( gpGlobals->curtime >= m_flNextPrimaryAttackAnim )
			{
				pOwner->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
				m_flNextPrimaryAttackAnim = gpGlobals->curtime + 1.4;		// fewer than 45 frames!
			}
		}
		break;

	default:
		break;
	}

#ifdef CLIENT_DLL
	// Restart our particle effect if we've transitioned across water boundaries
	if ( m_iParticleWaterLevel != -1 && pOwner->GetWaterLevel() != m_iParticleWaterLevel )
	{
		if ( m_iParticleWaterLevel == WL_Eyes || pOwner->GetWaterLevel() == WL_Eyes )
		{
			RestartParticleEffect();
		}
	}
#endif

#ifdef CLIENT_DLL
	// Handle the flamethrower light
	if (of_muzzlelight.GetBool())
	{
		dlight_t *dl = effects->CL_AllocDlight(LIGHT_INDEX_TE_DYNAMIC + index);
		dl->origin = vecMuzzlePos;
		dl->die = gpGlobals->curtime + 0.01f;
		dl->flags = DLIGHT_NO_MODEL_ILLUMINATION;
		if (m_bCritFire)
		{
			dl->color.r = 255;
			dl->color.g = 110;
			dl->color.b = 10;
			dl->radius = 400.f;
			dl->decay = 512.0f;
			dl->style = 1;
		}
		else
		{
			dl->color.r = 255;
			dl->color.g = 100;
			dl->color.b = 10;
			dl->radius = 340.f;
			dl->decay = 512.0f;
			dl->style = 1;
		}
	}
#endif

#if !defined (CLIENT_DLL)
	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pOwner->NoteWeaponFired();

	pOwner->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pOwner, m_bCritFire );

	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );
#endif

	float flFiringInterval = GetFireRate();

	// Don't attack if we're underwater
	if ( pOwner->GetWaterLevel() != WL_Eyes )
	{
		// Find eligible entities in a cone in front of us.
		Vector vOrigin = pOwner->Weapon_ShootPosition();
		Vector vForward, vRight, vUp;
		QAngle vAngles = pOwner->EyeAngles() + pOwner->GetPunchAngle();
		AngleVectors( vAngles, &vForward, &vRight, &vUp );

		#define NUM_TEST_VECTORS	30

#ifdef CLIENT_DLL
		bool bWasCritical = m_bCritFire;
#endif

		// Burn & Ignite 'em
		int iDmgType = g_aWeaponDamageTypes[ GetWeaponID() ];
		int iCustomDmgType = GetCustomDamageType();
		m_bCritFire = IsCurrentAttackACrit();
		if ( m_bCritFire )
		{
			iDmgType |= DMG_CRITICAL;
		}
		if ( m_bCritFire >= 2 )
		{
			iCustomDmgType = TF_DMG_CUSTOM_CRIT_POWERUP;
		}		

#ifdef CLIENT_DLL
		if ( bWasCritical != ( m_bCritFire > 0 ) )
		{
			RestartParticleEffect();
		}
#endif


#ifdef GAME_DLL
		// create the flame entity
		int iDamagePerSec = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;

		if ( !TFGameRules()->IsMutator( NO_MUTATOR ) && TFGameRules()->GetMutator() <= INSTAGIB_NO_MELEE ) 
			iDamagePerSec = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nInstagibDamage;

		if ( TFGameRules()->IsInfGamemode() )
			iDamagePerSec = ( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage ) / 2;

		float flDamage = (float)iDamagePerSec * flFiringInterval;

		CTFFlameEntity::Create( GetFlameOriginPos(), pOwner->EyeAngles(), this, iDmgType, flDamage, iCustomDmgType );
#endif
	}

#ifdef GAME_DLL
	// Figure how much ammo we're using per shot and add it to our remainder to subtract.  (We may be using less than 1.0 ammo units
	// per frame, depending on how constants are tuned, so keep an accumulator so we can expend fractional amounts of ammo per shot.)
	// Note we do this only on server and network it to client.  If we predict it on client, it can get slightly out of sync w/server
	// and cause ammo pickup indicators to appear
	m_flAmmoUseRemainder += TF_FLAMETHROWER_AMMO_PER_SECOND_PRIMARY_ATTACK * flFiringInterval;
	// take the integer portion of the ammo use accumulator and subtract it from player's ammo count; any fractional amount of ammo use
	// remains and will get used in the next shot
	int iAmmoToSubtract = (int) m_flAmmoUseRemainder;
	if( of_infiniteammo.GetBool() )
		iAmmoToSubtract = 0.0f;
	if ( iAmmoToSubtract > 0 )
	{
		m_iReserveAmmo -= iAmmoToSubtract;
		m_flAmmoUseRemainder -= iAmmoToSubtract;
		// round to 2 digits of precision
		m_flAmmoUseRemainder = (float) ( (int) (m_flAmmoUseRemainder * 100) ) / 100.0f;
	}
#endif

	m_flNextPrimaryAttack = gpGlobals->curtime + flFiringInterval;
	m_flTimeWeaponIdle = gpGlobals->curtime + flFiringInterval;

#if !defined (CLIENT_DLL)
	lagcompensation->FinishLagCompensation( pOwner );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::SecondaryAttack()
{
	m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;

	// Are we capable of firing again?
	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
		return;

	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( !CanPerformSecondaryAttack() )
	{
		/*
#if defined ( CLIENT_DLL )
		StopFlame();
#endif
		*/
		m_iWeaponState = FT_STATE_IDLE;
		return;
	}

	if( !of_infiniteammo.GetBool() )
		m_iReserveAmmo = m_iReserveAmmo - TF_FLAMETHROWER_AMMO_PER_SECONDARY_ATTACK;

	float flFiringInterval = GetFireRate();
	m_flNextSecondaryAttack = gpGlobals->curtime + flFiringInterval;
	m_flNextPrimaryAttack = gpGlobals->curtime + flFiringInterval;

#if defined ( CLIENT_DLL )
	StopFlame();

	C_BaseEntity *pModel = GetWeaponForEffect();

	if ( pModel )
		pModel->ParticleProp()->Create( "pyro_blast", PATTACH_POINT_FOLLOW, "muzzle" );
#endif

	SendWeaponAnim( ACT_VM_SECONDARYATTACK );
	WeaponSound( WPN_DOUBLE );

#if !defined ( CLIENT_DLL )
	// Let the player remember the usercmd he fired a weapon on. Assists in making decisions about lag compensation.
	pOwner->NoteWeaponFired();

	pOwner->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pOwner, m_bCritFire );

	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pOwner, pOwner->GetCurrentCommand() );

	// ---------------------------------------------------------------------------------------------------
	// Special thanks to sigsegv for documentation
	// Source: https://www.youtube.com/watch?v=W1g2x4b_Byg and https://www.youtube.com/watch?v=PZ-d4oUSVzE
	// ---------------------------------------------------------------------------------------------------

	// Where are we aiming?
	Vector vForward;
	QAngle vAngles = pOwner->EyeAngles();
	AngleVectors( vAngles, &vForward);

	// "256x256x128 HU box"
	Vector vAirBlastBox = Vector( 128, 128, 64 );	

	// TODO: this isn't an accurate distance
	// offset the box origin from our shoot position
	float flDist = 128.0f;

	// Used as the centre of the box trace
	Vector vOrigin = pOwner->Weapon_ShootPosition() + vForward * flDist;

	//CBaseEntity *pList[ 32 ];
	CBaseEntity *pList[ 64 ];

	int count = UTIL_EntitiesInBox( pList, 64, vOrigin - vAirBlastBox, vOrigin + vAirBlastBox, 0 );

	for ( int i = 0; i < count; i++ )
	{
		CBaseEntity *pEntity = pList[ i ];

		if ( !pEntity )
			continue;

		if ( !pEntity->IsAlive() )
			continue;

		if ( pEntity->GetTeamNumber() < TF_TEAM_RED )
			continue;

		if ( pEntity == pOwner )
			continue;

		if ( !pEntity->IsDeflectable() )
			continue;

		trace_t trWorld;

		// now, let's see if the flame visual could have actually hit this player.  Trace backward from the
		// point of impact to where the flame was fired, see if we hit anything.  Since the point of impact was
		// determined using the flame's bounding box and we're just doing a ray test here, we extend the
		// start point out by the radius of the box.
		// UTIL_TraceLine( GetAbsOrigin() + vDir * WorldAlignMaxs().x, m_vecInitialPos, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trWorld );		
		UTIL_TraceLine( pOwner->Weapon_ShootPosition(), pEntity->WorldSpaceCenter(), MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trWorld );

		// can't see it!
		if ( trWorld.fraction != 1.0f )
			continue;

		if ( pEntity->IsCombatCharacter() )
		{
			// Convert angles to vector
			Vector vForwardDir;
			AngleVectors( vAngles, &vForwardDir );

			CBaseCombatCharacter *pCharacter = dynamic_cast<CBaseCombatCharacter *>( pEntity );

			if ( pCharacter )
				AirBlastCharacter( pCharacter, vForwardDir );
		}
		else
		{
			// TODO: vphysics specific tracing!

			Vector vecPos = pEntity->GetAbsOrigin();
			Vector vecAirBlast;

			// TODO: handle trails here i guess?
			GetProjectileAirblastSetup( GetTFPlayerOwner(), vecPos, &vecAirBlast, false );

			AirBlastProjectile( pEntity, vecAirBlast );
		}
	}

	lagcompensation->FinishLagCompensation( pOwner );
#endif
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Airblast a player / npc
//
// Special thanks to sigsegv for the majority of this function
// Source: https://gist.github.com/sigsegv-mvm/269d1e0abacb29040b5c
// 
//-----------------------------------------------------------------------------
void CTFFlameThrower::AirBlastCharacter( CBaseCombatCharacter *pCharacter, const Vector &vec_in )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	CTFPlayer *pTFPlayer = ToTFPlayer( pCharacter );

	if ( ( pCharacter->InSameTeam( pOwner ) && pCharacter->GetTeamNumber() != TF_TEAM_MERCENARY ) )
	{
		if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_BURNING ) )
		{
			pTFPlayer->m_Shared.RemoveCond( TF_COND_BURNING );

			pTFPlayer->EmitSound( "TFPlayer.FlameOut" );
		}
	}
	else
	{
		Vector vec = vec_in;
	
		float impulse = 360;
		impulse = MIN( 1000, impulse );
	
		vec *= impulse;
	
		vec.z += 350.0f;

		if ( pTFPlayer )
		{
			pTFPlayer->AddDamagerToHistory( pOwner );
			pTFPlayer->EmitSound( "TFPlayer.AirBlastImpact" );
		}

		pCharacter->ApplyAirBlastImpulse( vec );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::AirBlastProjectile( CBaseEntity *pEntity, const Vector &vec_in )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( ( pEntity->InSameTeam( pOwner ) && pEntity->GetTeamNumber() != TF_TEAM_MERCENARY ) )
		return;

	Vector vec = vec_in;
	Vector vecAirBlast;

	GetProjectileAirblastSetup( pOwner, pEntity->GetAbsOrigin(), &vecAirBlast, false );

	pEntity->Deflected( pEntity, vec );

	pEntity->EmitSound( "Weapon_FlameThrower.AirBurstAttackDeflect" );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFFlameThrower::Lower( void )
{
	if ( BaseClass::Lower() )
	{
		// If we were firing, stop
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
			SendWeaponAnim( ACT_MP_ATTACK_STAND_POSTFIRE );
			m_iWeaponState = FT_STATE_IDLE;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position of the tip of the muzzle at it appears visually
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetVisualMuzzlePos()
{
	return GetMuzzlePosHelper( true );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position at which to spawn flame damage entities
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetFlameOriginPos()
{
	return GetMuzzlePosHelper( false );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position of the tip of the muzzle
//-----------------------------------------------------------------------------
Vector CTFFlameThrower::GetMuzzlePosHelper( bool bVisualPos )
{
	Vector vecMuzzlePos;
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( pOwner ) 
	{
		Vector vecForward, vecRight, vecUp;
		AngleVectors( pOwner->GetAbsAngles(), &vecForward, &vecRight, &vecUp );
		vecMuzzlePos = pOwner->Weapon_ShootPosition();
		vecMuzzlePos +=  vecRight * TF_FLAMETHROWER_MUZZLEPOS_RIGHT;
		// if asking for visual position of muzzle, include the forward component
		if ( bVisualPos )
		{
			vecMuzzlePos +=  vecForward * TF_FLAMETHROWER_MUZZLEPOS_FORWARD;
		}
	}
	return vecMuzzlePos;
}

#if defined( CLIENT_DLL )

bool CTFFlameThrower::Deploy( void )
{
	StartPilotLight();

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);

	if ( IsCarrierAlive() && ( WeaponState() == WEAPON_IS_ACTIVE ) && ( ReserveAmmo() > 0 ) )
	{
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
			StartFlame();
		}
		else
		{
			StartPilotLight();
		}		
	}
	else 
	{
		StopFlame();
		StopPilotLight();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::UpdateOnRemove( void )
{
	StopFlame();
	StopPilotLight();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::SetDormant( bool bDormant )
{
	// If I'm going from active to dormant and I'm carried by another player, stop our firing sound.
	if ( !IsCarriedByLocalPlayer() )
	{
		if ( !IsDormant() && bDormant )
		{
			StopFlame();
			StopPilotLight();
		}
	}

	// Deliberately skip base combat weapon to avoid being holstered
	C_BaseEntity::SetDormant( bDormant );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StartFlame()
{
	if ( !CanPerformSecondaryAttack() )
		return;

	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	// normally, crossfade between start sound & firing loop in 3.5 sec
	float flCrossfadeTime = 3.5;

	if ( m_pFiringLoop && ( ( m_bCritFire > 0 ) != m_bFiringLoopCritical ) )
	{
		// If we're firing and changing between critical & noncritical, just need to change the firing loop.
		// Set crossfade time to zero so we skip the start sound and go to the loop immediately.

		flCrossfadeTime = 0;
		StopFlame( true );
	}

	StopPilotLight();

	if ( !m_pFiringStartSound && !m_pFiringLoop )
	{
		RestartParticleEffect();
		CLocalPlayerFilter filter;

		// Play the fire start sound
		const char *shootsound = GetShootSound( SINGLE );
		if ( flCrossfadeTime > 0.0 )
		{
			// play the firing start sound and fade it out
			m_pFiringStartSound = controller.SoundCreate( filter, entindex(), shootsound );		
			controller.Play( m_pFiringStartSound, 1.0, 100 );
			controller.SoundChangeVolume( m_pFiringStartSound, 0.0, flCrossfadeTime );
		}

		// Start the fire sound loop and fade it in
		if ( m_bCritFire )
		{
			shootsound = GetShootSound( BURST );
		}
		else
		{
			shootsound = GetShootSound( SPECIAL1 );
		}
		m_pFiringLoop = controller.SoundCreate( filter, entindex(), shootsound );
		m_bFiringLoopCritical = m_bCritFire;

		// play the firing loop sound and fade it in
		if ( flCrossfadeTime > 0.0 )
		{
			controller.Play( m_pFiringLoop, 0.0, 100 );
			controller.SoundChangeVolume( m_pFiringLoop, 1.0, flCrossfadeTime );
		}
		else
		{
			controller.Play( m_pFiringLoop, 1.0, 100 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StopFlame( bool bAbrupt /* = false */ )
{
	if ( ( m_pFiringLoop || m_pFiringStartSound ) && !bAbrupt )
	{
		// play a quick wind-down poof when the flame stops
		CLocalPlayerFilter filter;
		const char *shootsound = GetShootSound( SPECIAL3 );
		EmitSound( filter, entindex(), shootsound );
	}

	if ( m_pFiringLoop )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pFiringLoop );
		m_pFiringLoop = NULL;
	}

	if ( m_pFiringStartSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pFiringStartSound );
		m_pFiringStartSound = NULL;
	}

	if ( m_bFlameEffects )
	{
		// Stop the effect on the viewmodel if our owner is the local player
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );

		if ( pOwner )
		{
			m_iParticleWaterLevel = pOwner->GetWaterLevel();

			// Stop the appropriate particle effect
			const char *pszTeamName;
			switch ( pOwner->GetTeamNumber() )
			{
				case TF_TEAM_RED:
					pszTeamName = "red";
					break;
				case TF_TEAM_BLUE:
					pszTeamName = "blue";
					break;
				default:
					pszTeamName = "dm";
					break;
			}
			if ( pLocalPlayer && pLocalPlayer == GetOwner() )
			{
				if ( pLocalPlayer->GetViewModel() )
				{
					pLocalPlayer->GetViewModel()->ParticleProp()->StopEmission( m_pFireParticle );
					m_pFireParticle = NULL;
				}
			}
			else
			{
				ParticleProp()->StopEmission( m_pFireParticle );
				m_pFireParticle = NULL;
			}
		}
	}

	m_bFlameEffects = false;
	m_iParticleWaterLevel = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StartPilotLight()
{
	if ( !m_pPilotLightSound )
	{
		StopFlame();

		// Create the looping pilot light sound
		const char *pilotlightsound = GetShootSound( SPECIAL2 );
		CLocalPlayerFilter filter;

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pPilotLightSound = controller.SoundCreate( filter, entindex(), pilotlightsound );

		controller.Play( m_pPilotLightSound, 1.0, 100 );
	}	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::StopPilotLight()
{
	if ( m_pPilotLightSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFFlameThrower::RestartParticleEffect( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	m_iParticleWaterLevel = pOwner->GetWaterLevel();

	// Start the appropriate particle effect
	const char *pszParticleEffect;
	if ( pOwner->GetWaterLevel() == WL_Eyes )
	{
		pszParticleEffect = "flamethrower_underwater";
	}
	else
	{
		if ( m_bCritFire )
		{
			switch ( pOwner->GetTeamNumber() )
			{
				case TF_TEAM_RED:
					pszParticleEffect = "flamethrower_crit_red";
					break;
				case TF_TEAM_BLUE:
					pszParticleEffect = "flamethrower_crit_blue";
					break;
				default:
					pszParticleEffect = "flamethrower_crit_dm";
					break;
			}
		}
		else 
		{
			switch ( pOwner->GetTeamNumber() )
			{
				case TF_TEAM_RED:
					pszParticleEffect = "flamethrower_red";
					break;
				case TF_TEAM_BLUE:
					pszParticleEffect = "flamethrower_blue";
					break;
				default:
					pszParticleEffect = "flamethrower_dm";
					break;
			}
		}		
	}

	// Start the effect on the viewmodel if our owner is the local player
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer && pLocalPlayer == GetOwner() )
	{
		if ( pLocalPlayer->GetViewModel() )
		{
			pLocalPlayer->GetViewModel()->ParticleProp()->StopEmission(m_pFireParticle);
			m_pFireParticle = pLocalPlayer->GetViewModel()->ParticleProp()->Create( pszParticleEffect, PATTACH_POINT_FOLLOW, "muzzle" ) ;
			pOwner->m_Shared.UpdateParticleColor ( m_pFireParticle );
			pLocalPlayer->GetViewModel()->ParticleProp()->AddControlPoint( m_pFireParticle, 2, pOwner, PATTACH_ABSORIGIN_FOLLOW );
		}
	}
	else
	{
		ParticleProp()->StopEmission(m_pFireParticle);
		m_pFireParticle = ParticleProp()->Create( pszParticleEffect, PATTACH_POINT_FOLLOW, "muzzle" );
		pOwner->m_Shared.UpdateParticleColor ( m_pFireParticle );
		ParticleProp()->AddControlPoint( m_pFireParticle, 2, pOwner, PATTACH_ABSORIGIN_FOLLOW );
	}
	m_bFlameEffects = true;
}

#endif

#ifdef GAME_DLL

LINK_ENTITY_TO_CLASS( tf_flame, CTFFlameEntity );

//-----------------------------------------------------------------------------
// Purpose: Spawns this entitye
//-----------------------------------------------------------------------------
void CTFFlameEntity::Spawn( void )
{
	BaseClass::Spawn();

	// don't collide with anything, we do our own collision detection in our think method
	SetSolid( SOLID_NONE );
	SetSolidFlags( FSOLID_NOT_SOLID );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	// move noclip: update position from velocity, that's it
	SetMoveType( MOVETYPE_NOCLIP, MOVECOLLIDE_DEFAULT );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );

	float iBoxSize = tf_flamethrower_boxsize.GetFloat();
	UTIL_SetSize( this, -Vector( iBoxSize, iBoxSize, iBoxSize ), Vector( iBoxSize, iBoxSize, iBoxSize ) );

	// Setup attributes.
	m_takedamage = DAMAGE_NO;
	m_vecInitialPos = GetAbsOrigin();
	m_vecPrevPos = m_vecInitialPos;
	m_flTimeRemove = gpGlobals->curtime + ( tf_flamethrower_flametime.GetFloat() * random->RandomFloat( 0.9, 1.1 ) );
	
	// Setup the think function.
	SetThink( &CTFFlameEntity::FlameThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: Creates an instance of this entity
//-----------------------------------------------------------------------------
CTFFlameEntity *CTFFlameEntity::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, int iDmgType, float flDmgAmount, int iCustomDmgType )
{
	CTFFlameEntity *pFlame = static_cast<CTFFlameEntity*>( CBaseEntity::Create( "tf_flame", vecOrigin, vecAngles, pOwner ) );
	if ( !pFlame )
		return NULL;

	// Initialize the owner.
	pFlame->SetOwnerEntity( pOwner );
	pFlame->m_hAttacker = pOwner->GetOwnerEntity();
	CBaseEntity *pAttacker = (CBaseEntity *) pFlame->m_hAttacker;
	if ( pAttacker )
	{
		pFlame->m_iAttackerTeam =  pAttacker->GetTeamNumber();
	}

	// Set team.
	pFlame->ChangeTeam( pOwner->GetTeamNumber()  );
	pFlame->m_iDmgType = iDmgType;
	pFlame->m_iCustomDmgType = iCustomDmgType;
	pFlame->m_flDmgAmount = flDmgAmount;

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	float velocity = tf_flamethrower_velocity.GetFloat();
	pFlame->m_vecBaseVelocity = vecForward * velocity;
	pFlame->m_vecBaseVelocity += RandomVector( -velocity * tf_flamethrower_vecrand.GetFloat(), velocity * tf_flamethrower_vecrand.GetFloat() );
	pFlame->m_vecAttackerVelocity = pOwner->GetOwnerEntity()->GetAbsVelocity();
	pFlame->SetAbsVelocity( pFlame->m_vecBaseVelocity );	
	// Setup the initial angles.
	pFlame->SetAbsAngles( vecAngles );

	return pFlame;
}

//-----------------------------------------------------------------------------
// Purpose: Think method
//-----------------------------------------------------------------------------
void CTFFlameEntity::FlameThink( void )
{
	// if we've expired, remove ourselves
	if ( gpGlobals->curtime >= m_flTimeRemove )
	{
		UTIL_Remove( this );
		return;
	}

	// Do collision detection.  We do custom collision detection because we can do it more cheaply than the
	// standard collision detection (don't need to check against world unless we might have hit an enemy) and
	// flame entity collision detection w/o this was a bottleneck on the X360 server
	if ( GetAbsOrigin() != m_vecPrevPos )
	{
		CTFPlayer *pAttacker = dynamic_cast<CTFPlayer *>( (CBaseEntity *) m_hAttacker );
		if ( !pAttacker )
			return;

		CTFTeam *pTeam = pAttacker->GetOpposingTFTeam();
		if ( !pTeam )
			return;
		
		bool bHitWorld = false;

		// check collision against all enemy players
		for ( int iPlayer= 0; iPlayer < pTeam->GetNumPlayers(); iPlayer++ )
		{
			CBasePlayer *pPlayer = pTeam->GetPlayer( iPlayer );
			// Is this player connected, alive, and an enemy?
			if ( pPlayer && pPlayer->IsConnected() && pPlayer->IsAlive() && pPlayer != pAttacker )
			{
				CheckCollision( pPlayer, &bHitWorld );
				if ( bHitWorld )
					return;
			}
		}

		// check collision against all enemy objects
		for ( int iObject = 0; iObject < pTeam->GetNumObjects(); iObject++ )
		{
			CBaseObject *pObject = pTeam->GetObject( iObject );
			if ( pObject )
			{
				CheckCollision( pObject, &bHitWorld );
				if ( bHitWorld )
					return;
			}
		}
		// check collision against npcs
		CAI_BaseNPC **ppAIs = g_AI_Manager.AccessAIs();
		for (int iNPC = 0; iNPC < g_AI_Manager.NumAIs(); iNPC++)
		{
			CAI_BaseNPC *pNPC = ppAIs[iNPC];
			// Is this npc alive?
			if (pNPC && pNPC->IsAlive())
			{
				CheckCollision(pNPC, &bHitWorld);
				if (bHitWorld)
					return;
			}
		}
		
		CUtlVector<INextBot *> bots;
		TheNextBots().CollectAllBots( &bots );
		for ( int i=0; i < bots.Count(); ++i )
		{
			CBaseCombatCharacter *pActor = bots[i]->GetEntity();
			if ( pActor && !pActor->IsPlayer() && pActor->IsAlive() )
			{
				CheckCollision( pActor, &bHitWorld );
				if ( bHitWorld )
					return;
			}
		}
	}

	// Calculate how long the flame has been alive for
	float flFlameElapsedTime = tf_flamethrower_flametime.GetFloat() - ( m_flTimeRemove - gpGlobals->curtime );
	// Calculate how much of the attacker's velocity to blend in to the flame's velocity.  The flame gets the attacker's velocity
	// added right when the flame is fired, but that velocity addition fades quickly to zero.
	float flAttackerVelocityBlend = RemapValClamped( flFlameElapsedTime, tf_flamethrower_velocityfadestart.GetFloat(), 
		tf_flamethrower_velocityfadeend.GetFloat(), 1.0, 0 );

	// Reduce our base velocity by the air drag constant
	m_vecBaseVelocity *= tf_flamethrower_drag.GetFloat();

	// Add our float upward velocity
	Vector vecVelocity = m_vecBaseVelocity + Vector( 0, 0, tf_flamethrower_float.GetFloat() ) + ( flAttackerVelocityBlend * m_vecAttackerVelocity );

	// Update our velocity
	SetAbsVelocity( vecVelocity );

	// Render debug visualization if convar on
	if ( tf_debug_flamethrower.GetBool() )
	{
		if ( m_hEntitiesBurnt.Count() > 0 )
		{
			int val = ( (int) ( gpGlobals->curtime * 10 ) ) % 255;
			NDebugOverlay::EntityBounds(this, val, 255, val, 0 ,0 );
		} 
		else 
		{
			NDebugOverlay::EntityBounds(this, 0, 100, 255, 0 ,0) ;
		}
	}

	SetNextThink( gpGlobals->curtime );

	m_vecPrevPos = GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: Checks collisions against other entities
//-----------------------------------------------------------------------------
void CTFFlameEntity::CheckCollision( CBaseEntity *pOther, bool *pbHitWorld )
{
	*pbHitWorld = false;

	// if we've already burnt this entity, don't do more damage, so skip even checking for collision with the entity
	int iIndex = m_hEntitiesBurnt.Find( pOther );
	if ( iIndex != m_hEntitiesBurnt.InvalidIndex() )
		return;
	
	// Do a bounding box check against the entity
	Vector vecMins, vecMaxs;
	pOther->GetCollideable()->WorldSpaceSurroundingBounds( &vecMins, &vecMaxs );
	CBaseTrace trace;
	Ray_t ray;
	float flFractionLeftSolid;				
	ray.Init( m_vecPrevPos, GetAbsOrigin(), WorldAlignMins(), WorldAlignMaxs() );
	if ( IntersectRayWithBox( ray, vecMins, vecMaxs, 0.0, &trace, &flFractionLeftSolid ) )
	{
		// if bounding box check passes, check player hitboxes
		trace_t trHitbox;
		trace_t trWorld;
		bool bTested = pOther->GetCollideable()->TestHitboxes( ray, MASK_SOLID | CONTENTS_HITBOX, trHitbox );
		if ( !bTested || !trHitbox.DidHit() )
			return;

		// now, let's see if the flame visual could have actually hit this player.  Trace backward from the
		// point of impact to where the flame was fired, see if we hit anything.  Since the point of impact was
		// determined using the flame's bounding box and we're just doing a ray test here, we extend the
		// start point out by the radius of the box.
		Vector vDir = ray.m_Delta;
		vDir.NormalizeInPlace();
		UTIL_TraceLine( GetAbsOrigin() + vDir * WorldAlignMaxs().x, m_vecInitialPos, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &trWorld );			

		if ( tf_debug_flamethrower.GetBool() )
		{
			NDebugOverlay::Line( trWorld.startpos, trWorld.endpos, 0, 255, 0, true, 3.0f );
		}
		
		if ( trWorld.fraction == 1.0 )
		{						
			// if there is nothing solid in the way, damage the entity
			OnCollide( pOther );
		}					
		else
		{
			// we hit the world, remove ourselves
			*pbHitWorld = true;
			UTIL_Remove( this );
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: Called when we've collided with another entity
//-----------------------------------------------------------------------------
void CTFFlameEntity::OnCollide( CBaseEntity *pOther )
{
	// remember that we've burnt this player
	m_hEntitiesBurnt.AddToTail( pOther );

	float flDistance = GetAbsOrigin().DistTo( m_vecInitialPos );
	float flMultiplier;
	if ( flDistance <= 125 )
	{
		// at very short range, apply short range damage multiplier
		flMultiplier = tf_flamethrower_shortrangedamagemultiplier.GetFloat();
	}
	else
	{
		// make damage ramp down from 100% to 25% from half the max dist to the max dist
		flMultiplier = RemapValClamped( flDistance, tf_flamethrower_maxdamagedist.GetFloat()/2, tf_flamethrower_maxdamagedist.GetFloat(), 1.0, 0.25 );
	}
	float flDamage = m_flDmgAmount * flMultiplier;
	flDamage = max( flDamage, 1.0 );
	if ( tf_debug_flamethrower.GetBool() )
	{
		Msg( "Flame touch dmg: %.1f\n", flDamage );
	}

	CBaseEntity *pAttacker = m_hAttacker;
	if ( !pAttacker )
		return;

	m_iCustomDmgType |= TF_DMG_CUSTOM_BURNING;

	CTakeDamageInfo info( GetOwnerEntity(), pAttacker, flDamage, m_iDmgType, m_iCustomDmgType );
	info.SetReportedPosition( pAttacker->GetAbsOrigin() );

	// We collided with pOther, so try to find a place on their surface to show blood
	trace_t pTrace;
	UTIL_TraceLine( WorldSpaceCenter(), pOther->WorldSpaceCenter(), MASK_SOLID|CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &pTrace );

	pOther->DispatchTraceAttack( info, GetAbsVelocity(), &pTrace );
	ApplyMultiDamage();

	CAI_BaseNPC *pNPC = pOther->MyNPCPointer();

	if ( pNPC )
		pNPC->Ignite( TF_BURNING_FLAME_LIFE / 5 ); // much shorter time as otherwise its too overpowered
}

#endif // GAME_DLL

acttable_t CTFFlameThrower::m_acttableFlameThrower[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_FLAMETHROWER, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_FLAMETHROWER, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_FLAMETHROWER, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_FLAMETHROWER, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_FLAMETHROWER, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_FLAMETHROWER, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_FLAMETHROWER, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_FLAMETHROWER, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_FLAMETHROWER, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_FLAMETHROWER, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_FLAMETHROWER, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_FLAMETHROWER, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_FLAMETHROWER, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_FLAMETHROWER, false },
};

//Act table remapping for Merc
acttable_t *CTFFlameThrower::ActivityList(int &iActivityCount)
{
	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableFlameThrower);
		return m_acttableFlameThrower;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}
