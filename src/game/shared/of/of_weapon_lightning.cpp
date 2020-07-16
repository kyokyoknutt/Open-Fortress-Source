//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#include "cbase.h"
#include "of_weapon_lightning.h"
#include "tf_weaponbase.h"
#include "tf_weaponbase_gun.h"
#include "tf_weapon_smg.h"
#include "tf_fx_shared.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "tf_gamerules.h"

#if defined( CLIENT_DLL )

	#include "c_tf_player.h"
	#include "vstdlib/random.h"
	#include "engine/IEngineSound.h"
	#include "soundenvelope.h"
    #include "dlight.h"
    #include "iefx.h"

#else

	#include "explode.h"
	#include "tf_player.h"
	#include "tf_gamestats.h"
	#include "ilagcompensationmanager.h"
	#include "collisionutils.h"
	#include "tf_team.h"
	#include "tf_obj.h"
	#include "util.h"
	#include "ai_basenpc.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================
//
// SMG
//
//=============================================================================
IMPLEMENT_NETWORKCLASS_ALIASED( TFLightningGun, DT_WeaponLightningGun )

BEGIN_NETWORK_TABLE( CTFLightningGun, DT_WeaponLightningGun )
	#if defined( CLIENT_DLL )
		RecvPropInt( RECVINFO( m_iWeaponState ) ),
		RecvPropInt( RECVINFO( m_bCritFire ) )
	#else
		SendPropInt( SENDINFO( m_iWeaponState ), 4, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
		SendPropInt( SENDINFO( m_bCritFire ) )
	#endif
END_NETWORK_TABLE()

#if defined( CLIENT_DLL )
BEGIN_PREDICTION_DATA( CTFLightningGun )
	DEFINE_PRED_FIELD( m_iWeaponState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bCritFire, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_lightning_gun, CTFLightningGun );
//PRECACHE_WEAPON_REGISTER( tf_weapon_lightning_gun );

BEGIN_DATADESC( CTFLightningGun )
END_DATADESC()

#ifdef CLIENT_DLL
extern ConVar of_muzzlelight;
#endif

CTFLightningGun::CTFLightningGun()
{
	WeaponReset();

#if defined( CLIENT_DLL )
	m_pFiringStartSound = NULL;
	m_pFiringLoop = NULL;
	m_bFiringLoopCritical = false;
	m_pLightningParticle = NULL;
#endif
}

CTFLightningGun::~CTFLightningGun()
{
	DestroySounds();
#if defined ( CLIENT_DLL )
	StopLightning();
#endif	
}

void CTFLightningGun::DestroySounds( void )
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
#endif
}

void CTFLightningGun::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_iWeaponState = FT_STATE_IDLE;
	m_bCritFire = false;
	m_flStartFiringTime = 0;

	DestroySounds();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFLightningGun::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_iWeaponState = FT_STATE_IDLE;
	m_bCritFire = false;

#if defined ( CLIENT_DLL )
	StopLightning();
#endif

	return BaseClass::Holster( pSwitchingTo );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLightningGun::ItemPostFrame()
{
	if ( m_bLowered )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	int iAmmo = ReserveAmmo();

	if ( pOwner->IsAlive() && ( pOwner->m_nButtons & IN_ATTACK ) && iAmmo > 0 )
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
void CTFLightningGun::PrimaryAttack()
{
	// Are we capable of firing again?
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	// Get the player owning the weapon.
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	if ( !CanAttack() )
	{
#if defined ( CLIENT_DLL )
		StopLightning();
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
			StopLightning();
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

			m_flStartFiringTime = gpGlobals->curtime;	// 5 frames at 30 fps

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
	// Handle the flamethrower light
	if (of_muzzlelight.GetBool())
	{
		dlight_t *dl = effects->CL_AllocDlight(LIGHT_INDEX_TE_DYNAMIC + index);
		dl->origin = vecMuzzlePos;
		dl->die = gpGlobals->curtime + 0.01f;
		dl->flags = DLIGHT_NO_MODEL_ILLUMINATION;
		if (m_bCritFire)
		{
			dl->color.r = 91;
			dl->color.g = 87;
			dl->color.b = 255;
			dl->radius = 400.f;
			dl->decay = 512.0f;
			dl->style = 1;
		}
		else
		{
			dl->color.r = 68;
			dl->color.g = 209;
			dl->color.b = 252;
			dl->radius = 340.f;
			dl->decay = 512.0f;
			dl->style = 1;
		}
	}
#endif

#if !defined (CLIENT_DLL)
	pOwner->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pOwner, m_bCritFire );
#endif

#ifdef CLIENT_DLL
		bool bWasCritical = m_bCritFire;
#endif

	// Burn & Ignite 'em
	int iDmgType = g_aWeaponDamageTypes[ GetWeaponID() ];
	int iCustomDmgType = BaseClass::GetCustomDamageType();
	m_bCritFire = IsCurrentAttackACrit();
	if ( m_bCritFire == 1 )
	{
		iDmgType |= DMG_CRITICAL;
	}
	else if ( m_bCritFire >= 2 )
	{
		iCustomDmgType = TF_DMG_CUSTOM_CRIT_POWERUP;
	}

	

#ifdef CLIENT_DLL
	if ( bWasCritical != ( m_bCritFire > 0 ) )
	{
		RestartParticleEffect();
	}
#endif

	float flFiringInterval = GetFireRate();

	FireProjectile( pOwner );
	
	m_flNextPrimaryAttack = gpGlobals->curtime + flFiringInterval;
	m_flTimeWeaponIdle = gpGlobals->curtime + flFiringInterval;

}

//-----------------------------------------------------------------------------
// Purpose: Accessor for damage, so sniper etc can modify damage
//-----------------------------------------------------------------------------
float CTFLightningGun::GetProjectileDamage( void )
{
	// create the flame entity
	int iDamagePerSec = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage;
	float flFiringInterval = GetFireRate();
	if ( TFGameRules()->IsMutator( INSTAGIB ) || TFGameRules()->IsMutator( INSTAGIB_NO_MELEE ) )
		iDamagePerSec = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nInstagibDamage;
	float flDamage = (float)iDamagePerSec * flFiringInterval;
	
	return flDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFLightningGun::Lower( void )
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
Vector CTFLightningGun::GetVisualMuzzlePos()
{
	return GetMuzzlePosHelper( true );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the position at which to spawn flame damage entities
//-----------------------------------------------------------------------------
Vector CTFLightningGun::GetFlameOriginPos()
{
	return GetMuzzlePosHelper( false );
}
// position of end of muzzle relative to shoot position
#define TF_FLAMETHROWER_MUZZLEPOS_FORWARD		70.0f
#define TF_FLAMETHROWER_MUZZLEPOS_RIGHT			12.0f
#define TF_FLAMETHROWER_MUZZLEPOS_UP			-12.0f
//-----------------------------------------------------------------------------
// Purpose: Returns the position of the tip of the muzzle
//-----------------------------------------------------------------------------
Vector CTFLightningGun::GetMuzzlePosHelper( bool bVisualPos )
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

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLightningGun::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged(updateType);
	
	if ( IsCarrierAlive() && ( WeaponState() == WEAPON_IS_ACTIVE ) && ( ReserveAmmo() > 0 ) )
	{
		if ( m_pLightningParticle )
			SetParticleEnd();
		if ( m_iWeaponState > FT_STATE_IDLE )
		{
			StartLightning();
		}
		else
		{
			StopLightning();
		}	
	}
	else 
	{
		StopLightning();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if the particle effect should be on or off
//-----------------------------------------------------------------------------
void CTFLightningGun::RestartParticleEffect( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	// Start the appropriate particle effect
	const char *pszParticleEffect;	
	
	if ( m_bCritFire )
	{
		pszParticleEffect = "LG_bolt_crit";
	}
	else 
	{
		pszParticleEffect = "LG_bolt";
	}		
		
	// Start the effect on the viewmodel if our owner is the local player
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer && pLocalPlayer == GetOwner() )
	{	
		if ( pLocalPlayer->GetViewModel() )
		{
			if ( m_pLightningParticle )
			{
				pLocalPlayer->GetViewModel()->ParticleProp()->StopEmission( m_pLightningParticle );
				m_pLightningParticle = NULL;
			}

			m_pLightningParticle = pLocalPlayer->GetViewModel()->ParticleProp()->Create( pszParticleEffect, PATTACH_POINT_FOLLOW, "muzzle" );

		}
	}
	else
	{		
		if ( m_pLightningParticle )
		{
			ParticleProp()->StopEmission( m_pLightningParticle );
			m_pLightningParticle = NULL;
		}

		m_pLightningParticle = ParticleProp()->Create( pszParticleEffect, PATTACH_POINT_FOLLOW, "muzzle" );
	}
	
	SetParticleEnd();
	m_bLightningEffects = true;
}

#ifdef CLIENT_DLL

class CTraceFilterIgnoreTeammates : public CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterIgnoreTeammates, CTraceFilterSimple );

	CTraceFilterIgnoreTeammates( const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam, CBaseEntity *pOwner )
		: CTraceFilterSimple( passentity, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam ), m_hOwner( pOwner )
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( ( pEntity->IsPlayer() && pEntity->GetTeamNumber() == m_iIgnoreTeam ) || pEntity == m_hOwner )
		{
			return false;
		}

		return true;
	}
	CBaseEntity *m_hOwner;
	int m_iIgnoreTeam;
};

void CTFLightningGun::SetParticleEnd()
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;
	
	Vector vecForward, vecRight, vecUp;
	AngleVectors( pOwner->EyeAngles(), &vecForward, &vecRight, &vecUp );

	Vector vecShootPos = pOwner->Weapon_ShootPosition();

	// Estimate end point
	Vector endPos = vecShootPos + vecForward * 720;	

	int team = pOwner->GetTeamNumber();
	if ( team == TF_TEAM_MERCENARY )
		team = 0;		
	
	trace_t tr;
	
	CTraceFilterIgnoreTeammates filter( pOwner, COLLISION_GROUP_NONE, team, pOwner );

	CTraceFilterIgnoreTeammates *pTmpFilter = &filter;
	if ( !pTmpFilter )
		return;

	UTIL_TraceLine( vecShootPos, endPos, MASK_SOLID, &filter, &tr );

	trace_t *pTmpTr = &tr;
	if ( !pTmpTr )
		return;

	if ( m_pLightningParticle && m_pLightningParticle->m_pDef && m_pLightningParticle->m_pDef->ReadsControlPoint( 1 ) )
	{
		m_pLightningParticle->SetControlPoint( 1, tr.endpos );
	}
}

#endif
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFLightningGun::StartLightning()
{
	CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();

	// normally, crossfade between start sound & firing loop in 3.5 sec
	float flCrossfadeTime = 3.5;

	if ( m_pFiringLoop && ( ( m_bCritFire > 0 ) != m_bFiringLoopCritical ) )
	{
		// If we're firing and changing between critical & noncritical, just need to change the firing loop.
		// Set crossfade time to zero so we skip the start sound and go to the loop immediately.

		flCrossfadeTime = 0;
		StopLightning( true );
	}

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
void CTFLightningGun::StopLightning( bool bAbrupt /* = false */ )
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
	
	if ( m_bLightningEffects )
	{
		// Stop the effect on the viewmodel if our owner is the local player
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pLocalPlayer && pLocalPlayer == GetOwner() && m_pLightningParticle )
		{
			if ( pLocalPlayer->GetViewModel() )
			{
				pLocalPlayer->GetViewModel()->ParticleProp()->StopEmission(m_pLightningParticle);
				m_pLightningParticle = NULL;
			}
		}
		else if ( m_pLightningParticle )
		{
			ParticleProp()->StopEmission( m_pLightningParticle );
			m_pLightningParticle = NULL;
		}
	}

	m_bLightningEffects = false;
}

#endif

acttable_t CTFLightningGun::m_acttableLightningGun[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_LIGHTNING_GUN, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_LIGHTNING_GUN, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_LIGHTNING_GUN, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_LIGHTNING_GUN, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_LIGHTNING_GUN, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_LIGHTNING_GUN, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_LIGHTNING_GUN, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_LIGHTNING_GUN, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_LIGHTNING_GUN, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_LIGHTNING_GUN, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_LIGHTNING_GUN, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_LIGHTNING_GUN, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_LIGHTNING_GUN, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_LIGHTNING_GUN, false },
};

//Act table remapping for Merc
acttable_t *CTFLightningGun::ActivityList(int &iActivityCount)
{
	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableLightningGun);
		return m_acttableLightningGun;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}
