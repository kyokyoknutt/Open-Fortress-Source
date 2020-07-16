//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. ========//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase_grenadeproj.h"

#ifdef GAME_DLL
	#include "tf_gamerules.h"
	#include "soundent.h"
	#include "func_nogrenades.h"
	#include "Sprite.h"
	#include "tf_fx.h"
	#include "tf_projectile_bomblet.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_gravity;

//=============================================================================
//
// TF Grenade projectile tables.
//

// Server specific.
#ifdef GAME_DLL
BEGIN_DATADESC( CTFWeaponBaseGrenadeProj )
DEFINE_THINKFUNC( DetonateThink ),
END_DATADESC()

ConVar tf_grenade_show_radius( "tf_grenade_show_radius", "0", FCVAR_CHEAT , "Render radius of grenades" );
ConVar tf_grenade_show_radius_time( "tf_grenade_show_radius_time", "5.0", FCVAR_CHEAT , "Time to show grenade radius" );
extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBaseGrenadeProj, DT_TFWeaponBaseGrenadeProj )

LINK_ENTITY_TO_CLASS( tf_weaponbase_grenade_proj, CTFWeaponBaseGrenadeProj );
PRECACHE_REGISTER( tf_weaponbase_grenade_proj );

BEGIN_NETWORK_TABLE( CTFWeaponBaseGrenadeProj, DT_TFWeaponBaseGrenadeProj )
#ifdef CLIENT_DLL
	RecvPropVector( RECVINFO( m_vInitialVelocity ) ),
	RecvPropInt( RECVINFO( m_bCritical ) ),
	RecvPropTime( RECVINFO( m_flSpawnTime ) ),
	
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),

#else
	SendPropVector( SENDINFO( m_vInitialVelocity ), 20 /*nbits*/, 0 /*flags*/, -3000 /*low value*/, 3000 /*high value*/	),
	SendPropInt( SENDINFO( m_bCritical ) ),
	SendPropTime( SENDINFO( m_flSpawnTime ) ),

	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),

	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_INTEGRAL|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropQAngles	(SENDINFO(m_angRotation), 6, SPROP_CHANGES_OFTEN, SendProxy_Angles ),
	
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj::CTFWeaponBaseGrenadeProj()
{
#ifdef GAME_DLL
	m_bUseImpactNormal = false;
	m_vecImpactNormal.Init();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj::~CTFWeaponBaseGrenadeProj()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFWeaponBaseGrenadeProj::GetDamageType() 
{ 
	int iDmgType = g_aWeaponDamageTypes[ GetWeaponID() ];
	if ( m_bCritical )
	{
		iDmgType |= DMG_CRITICAL;
	}

	return iDmgType;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFWeaponBaseGrenadeProj::GetCustomDamageType() 
{ 
	if ( m_bCritical >= 2)
	{
		return TF_DMG_CUSTOM_CRIT_POWERUP;
	}
	else
	{
		return TF_DMG_CUSTOM_NONE;
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Precache( void )
{
	BaseClass::Precache();

#ifdef GAME_DLL
	PrecacheModel( NOGRENADE_SPRITE );
	PrecacheParticleSystem( "critical_grenade_blue" );
	PrecacheParticleSystem( "critical_grenade_red" );
	PrecacheParticleSystem( "critical_grenade_mercenary" );
#endif
}

//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Spawn()
{
	BaseClass::Spawn();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		// Now stick our initial velocity into the interpolation history 
		CInterpolatedVar< Vector > &interpolator = GetOriginInterpolator();

		interpolator.ClearHistory();
		float changeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );

		// Add a sample 1 second back.
		Vector vCurOrigin = GetLocalOrigin() - m_vInitialVelocity;
		interpolator.AddToHead( changeTime - 1.0, &vCurOrigin, false );

		// Add the current sample.
		vCurOrigin = GetLocalOrigin();
		interpolator.AddToHead( changeTime, &vCurOrigin, false );
	}
}

//=============================================================================
//
// Server specific functions.
//
#else

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFWeaponBaseGrenadeProj::Create( const char *szName, const Vector &position, const QAngle &angles, 
													   const Vector &velocity, const AngularImpulse &angVelocity, 
													   CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, int iFlags, CTFWeaponBase *pWeapon )
{
	CTFWeaponBaseGrenadeProj *pGrenade = static_cast<CTFWeaponBaseGrenadeProj*>( CBaseEntity::CreateNoSpawn( szName, position, angles, pOwner ) );
	if ( pGrenade )
	{
		pGrenade->InitGrenade( velocity, angVelocity, pOwner, weaponInfo, pWeapon);
		pGrenade->Spawn();
	}

	return pGrenade;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::InitGrenade( const Vector &velocity, const AngularImpulse &angVelocity, 
									CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo, CTFWeaponBase *pWeapon )
{
	// We can't use OwnerEntity for grenades, because then the owner can't shoot them with his hitscan weapons (due to collide rules)
	// Thrower is used to store the person who threw the grenade, for damage purposes.
	SetOwnerEntity( NULL );

	if ( pOwner )
	{
		SetThrower( pOwner ); 
	}

	SetupInitialTransmittedGrenadeVelocity( velocity );

	SetGravity( 0.4f/*BaseClass::GetGrenadeGravity()*/ );
	SetFriction( 0.2f/*BaseClass::GetGrenadeFriction()*/ );
	SetElasticity( 0.45f/*BaseClass::GetGrenadeElasticity()*/ );
	
	if ( TFGameRules()->IsMutator( NO_MUTATOR ) || TFGameRules()->GetMutator() > INSTAGIB_NO_MELEE )  SetDamage( weaponInfo.GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_nDamage );
	else SetDamage( weaponInfo.GetWeaponData( TF_WEAPON_PRIMARY_MODE ).m_nInstagibDamage );
	
	SetDamageRadius( pWeapon ? pWeapon->GetDamageRadius() : weaponInfo.m_flDamageRadius );

	if ( pOwner )
		ChangeTeam( pOwner->GetTeamNumber() );
	else
		ChangeTeam( TF_TEAM_MERCENARY );

/*
	if ( pTFWeapon->GetTFWpnData().m_nProjectileModel[0] != 0 )
	{
		PrecacheModel(pTFWeapon->GetTFWpnData().m_nProjectileModel);
		SetModel( pTFWeapon->GetTFWpnData().m_nProjectileModel );	
	}
*/
	
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Spawn( void )
{
	// Base class spawn.
	BaseClass::Spawn();

	// So it will collide with physics props!
	SetSolidFlags( FSOLID_NOT_STANDABLE );
	SetSolid( SOLID_BBOX );	

	AddEffects( EF_NOSHADOW );

	// Set the grenade size here.
	UTIL_SetSize( this, Vector( -2.0f, -2.0f, -2.0f ), Vector( 2.0f, 2.0f, 2.0f ) );

	// Set the movement type.
	SetCollisionGroup( TF_COLLISIONGROUP_GRENADES );

	// Don't collide with players on the owner's team for the first bit of our life
	m_flCollideWithTeammatesTime = gpGlobals->curtime + 0.25;
	m_bCollideWithTeammates = false;
	VPhysicsInitNormal( SOLID_BBOX, 0, false );

	m_takedamage = DAMAGE_EVENTS_ONLY;

	// Set the team.
	if ( GetThrower() )
		ChangeTeam( GetThrower()->GetTeamNumber() );
	else
		ChangeTeam( TF_TEAM_MERCENARY );

	// Set skin based on team ( red = 1, blue = 2 )
	if ( GetTeamNumber() == TF_TEAM_RED ) m_nSkin = 0;
	else if ( GetTeamNumber() == TF_TEAM_BLUE ) m_nSkin = 1;
	else m_nSkin = 2;
	
	m_flSpawnTime = gpGlobals->curtime;

	// Setup the think and touch functions (see CBaseEntity).
	SetThink( &CTFWeaponBaseGrenadeProj::DetonateThink );
	SetNextThink( gpGlobals->curtime + 0.2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Explode( trace_t *pTrace, int bitsDamageType, int bitsCustomDamageType )
{
	SetModelName( NULL_STRING );//invisible
	AddSolidFlags( FSOLID_NOT_SOLID );

	m_takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}

	CSoundEnt::InsertSound ( SOUND_COMBAT, GetAbsOrigin(), BASEGRENADE_EXPLOSION_VOLUME, 3.0 );
	
	// Explosion effect on client
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	
	CTFWeaponBase *pTmpLauncher = dynamic_cast<CTFWeaponBase*>( GetOriginalLauncher() );
	
	if ( UseImpactNormal() )
	{
		if ( pTrace->m_pEnt && pTrace->m_pEnt->IsPlayer() )
		{
			TE_TFExplosion( filter, 0.0f, vecOrigin, GetImpactNormal(), pTrace->m_pEnt->entindex(), GetWeaponID(), pTmpLauncher );
		}
		else
		{
			TE_TFExplosion( filter, 0.0f, vecOrigin, GetImpactNormal(), -1, GetWeaponID(), pTmpLauncher );
		}
	}
	else
	{
		if ( pTrace->m_pEnt && pTrace->m_pEnt->IsPlayer() )
		{
			TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, pTrace->m_pEnt->entindex(), GetWeaponID(), pTmpLauncher );
		}
		else
		{
			TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, -1, GetWeaponID(), pTmpLauncher );
		}
	}

	// Use the thrower's position as the reported position
	Vector vecReported = GetThrower() ? GetThrower()->GetAbsOrigin() : vec3_origin;

	CTakeDamageInfo info( this, GetThrower(), GetOriginalLauncher() , GetBlastForce(), GetAbsOrigin(), m_flDamage, bitsDamageType, 0, &vecReported );
	info.SetWeapon( GetOriginalLauncher() );
	
	info.SetDamageCustom( bitsCustomDamageType );
	float flRadius = GetDamageRadius();

	if ( tf_grenade_show_radius.GetBool() )
	{
		DrawRadius( flRadius );
	}

	RadiusDamage( info, vecOrigin, flRadius, CLASS_NONE, NULL );

	// Don't decal players with scorch.
	if ( pTrace->m_pEnt && !pTrace->m_pEnt->IsPlayer() )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

	// Get the Weapon info
	
	CTFWeaponBase *pWeapon = GetWeaponID() != TF_WEAPON_GRENADE_MIRVBOMB && pTmpLauncher ? (CTFWeaponBase * )CreateEntityByName( WeaponIdToAlias( pTmpLauncher->GetWeaponID() ) ) : NULL;
	if ( pWeapon )
	{
		WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pWeapon->GetClassname() );
		Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
		CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );

#ifdef GAME_DLL
		// Create the bomblets.
		if ( pWeapon && pWeaponInfo && pWeaponInfo->m_bDropBomblets && GetWeaponID() != TF_WEAPON_GRENADE_MIRVBOMB )
		{
			for ( int iBomb = 0; iBomb < pWeaponInfo->m_iBombletAmount; ++iBomb )
			{
				Vector vecSrc = pTrace->endpos + Vector( 0, 0, 1.0f ); 
				Vector vecVelocity( random->RandomFloat( -75.0f, 75.0f ) * 3.0f,
								random->RandomFloat( -75.0f, 75.0f ) * 3.0f,
								random->RandomFloat( 30.0f, 70.0f ) * 5.0f );
				Vector vecZero( 0,0,0 );
				CTFPlayer *pPlayer = ToTFPlayer( GetThrower() );

				CTFGrenadeMirvBomb *pBomb = CTFGrenadeMirvBomb::Create( vecSrc, GetAbsAngles(), vecVelocity, vecZero, pPlayer, pWeaponInfo, GetTeamNumber() );
				pBomb->SetDamage( pWeaponInfo->m_flBombletDamage );
				pBomb->SetDetonateTimerLength( pWeaponInfo->m_flBombletTimer + random->RandomFloat( 0.0f, 1.0f ) );
				pBomb->SetDamageRadius( pWeaponInfo->m_flBombletDamageRadius );
				pBomb->SetCritical( m_bCritical );
			}		
		}
#endif
		UTIL_Remove( pWeapon );
	}
	SetThink( &CBaseGrenade::SUB_Remove );
	SetTouch( NULL );

	AddEffects( EF_NODRAW );
	SetAbsVelocity( vec3_origin );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBaseGrenadeProj::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo info2 = info;

	// Reduce explosion damage so that we don't get knocked too far
	if ( info.GetDamageType() & DMG_BLAST )
	{
		info2.ScaleDamageForce( 0.05 );
	}

	// We need to skip back to the base entity take damage, because
	// CBaseCombatCharacter doesn't, which prevents us from reacting
	// to physics impact damage.
	return CBaseEntity::OnTakeDamage( info2 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::DetonateThink( void )
{
	if ( !IsInWorld() )
	{
		Remove( );
		return;
	}

	if ( gpGlobals->curtime > m_flCollideWithTeammatesTime && m_bCollideWithTeammates == false )
	{
		m_bCollideWithTeammates = true;
	}

	if ( gpGlobals->curtime > m_flDetonateTime )
	{
		Detonate();
		return;
	}


	SetNextThink( gpGlobals->curtime + 0.2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::Detonate( void )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	Explode(&tr, GetDamageType(), GetCustomDamageType());

	if ( GetShakeAmplitude() )
	{
		UTIL_ScreenShake( GetAbsOrigin(), GetShakeAmplitude(), 150.0, 1.0, GetShakeRadius(), SHAKE_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the time at which the grenade will explode.
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::SetDetonateTimerLength( float timer )
{
	m_flDetonateTime = gpGlobals->curtime + timer;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::ResolveFlyCollisionCustom( trace_t &trace, Vector &vecVelocity )
{
	//Assume all surfaces have the same elasticity
	float flSurfaceElasticity = 1.0;

	//Don't bounce off of players with perfect elasticity
	if( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
	{
		flSurfaceElasticity = 0.3;
	}

	// if its breakable glass and we kill it, don't bounce.
	// give some damage to the glass, and if it breaks, pass 
	// through it.
	bool breakthrough = false;

	if( trace.m_pEnt && FClassnameIs( trace.m_pEnt, "func_breakable" ) )
	{
		breakthrough = true;
	}

	if( trace.m_pEnt && FClassnameIs( trace.m_pEnt, "func_breakable_surf" ) )
	{
		breakthrough = true;
	}

	if (breakthrough)
	{
		CTakeDamageInfo info( this, this, 10, DMG_CLUB );
		trace.m_pEnt->DispatchTraceAttack( info, GetAbsVelocity(), &trace );

		ApplyMultiDamage();

		if( trace.m_pEnt->m_iHealth <= 0 )
		{
			// slow our flight a little bit
			Vector vel = GetAbsVelocity();

			vel *= 0.4;

			SetAbsVelocity( vel );
			return;
		}
	}

	float flTotalElasticity = GetElasticity() * flSurfaceElasticity;
	flTotalElasticity = clamp( flTotalElasticity, 0.0f, 0.9f );

	// NOTE: A backoff of 2.0f is a reflection
	Vector vecAbsVelocity;
	PhysicsClipVelocity( GetAbsVelocity(), trace.plane.normal, vecAbsVelocity, 2.0f );
	vecAbsVelocity *= flTotalElasticity;

	// Get the total velocity (player + conveyors, etc.)
	VectorAdd( vecAbsVelocity, GetBaseVelocity(), vecVelocity );
	float flSpeedSqr = DotProduct( vecVelocity, vecVelocity );

	// Stop if on ground.
	if ( trace.plane.normal.z > 0.7f )			// Floor
	{
		// Verify that we have an entity.
		CBaseEntity *pEntity = trace.m_pEnt;
		Assert( pEntity );

		SetAbsVelocity( vecAbsVelocity );

		if ( flSpeedSqr < ( 30 * 30 ) )
		{
			if ( pEntity->IsStandable() )
			{
				SetGroundEntity( pEntity );
			}

			// Reset velocities.
			SetAbsVelocity( vec3_origin );
			SetLocalAngularVelocity( vec3_angle );

			//align to the ground so we're not standing on end
			QAngle angle;
			VectorAngles( trace.plane.normal, angle );

			// rotate randomly in yaw
			angle[1] = random->RandomFloat( 0, 360 );

			// TFTODO: rotate around trace.plane.normal

			SetAbsAngles( angle );			
		}
		else
		{
			Vector vecDelta = GetBaseVelocity() - vecAbsVelocity;	
			Vector vecBaseDir = GetBaseVelocity();
			VectorNormalize( vecBaseDir );
			float flScale = vecDelta.Dot( vecBaseDir );

			VectorScale( vecAbsVelocity, ( 1.0f - trace.fraction ) * gpGlobals->frametime, vecVelocity ); 
			VectorMA( vecVelocity, ( 1.0f - trace.fraction ) * gpGlobals->frametime, GetBaseVelocity() * flScale, vecVelocity );
			PhysicsPushEntity( vecVelocity, &trace );
		}
	}
	else
	{
		// If we get *too* slow, we'll stick without ever coming to rest because
		// we'll get pushed down by gravity faster than we can escape from the wall.
		if ( flSpeedSqr < ( 30 * 30 ) )
		{
			// Reset velocities.
			SetAbsVelocity( vec3_origin );
			SetLocalAngularVelocity( vec3_angle );
		}
		else
		{
			SetAbsVelocity( vecAbsVelocity );
		}
	}

	BounceSound();

#if 0
	// tell the bots a grenade has bounced
	CCSPlayer *player = ToCSPlayer(GetThrower());
	if ( player )
	{
		KeyValues *event = new KeyValues( "grenade_bounce" );
		event->SetInt( "userid", player->GetUserID() );
		gameeventmanager->FireEventServerOnly( event );
	}
#endif
}

bool CTFWeaponBaseGrenadeProj::ShouldNotDetonate( void )
{
	return InNoGrenadeZone( this );
}

void CTFWeaponBaseGrenadeProj::RemoveGrenade( bool bBlinkOut )
{
	// Kill it
	SetThink( &BaseClass::SUB_Remove );
	SetNextThink( gpGlobals->curtime );
	SetTouch( NULL );
	AddEffects( EF_NODRAW );

	if ( bBlinkOut )
	{
		// Sprite flash
		CSprite *pGlowSprite = CSprite::SpriteCreate( NOGRENADE_SPRITE, GetAbsOrigin(), false );
		if ( pGlowSprite )
		{
			pGlowSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxFadeFast );
			pGlowSprite->SetThink( &CSprite::SUB_Remove );
			pGlowSprite->SetNextThink( gpGlobals->curtime + 1.0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: This will hit only things that are in newCollisionGroup, but NOT in collisionGroupAlreadyChecked
//			Always ignores other grenade projectiles.
//-----------------------------------------------------------------------------
class CTraceFilterCollisionGrenades : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterCollisionGrenades );

	CTraceFilterCollisionGrenades( const IHandleEntity *passentity, const IHandleEntity *passentity2 )
		: m_pPassEnt(passentity), m_pPassEnt2(passentity2)
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
		if ( pEntity )
		{
			if ( pEntity == m_pPassEnt2 )
				return false;
			if ( pEntity->GetCollisionGroup() == TF_COLLISIONGROUP_GRENADES )
				return false;
			if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_ROCKETS )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
				return false;
			if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_NONE )
				return false;

			return true;
		}

		return true;
	}

protected:
	const IHandleEntity *m_pPassEnt;
	const IHandleEntity *m_pPassEnt2;
};


const float GRENADE_COEFFICIENT_OF_RESTITUTION = 0.2f;

//-----------------------------------------------------------------------------
// Purpose: Grenades aren't solid to players, so players don't get stuck on
//			them when they're lying on the ground. We still want thrown grenades
//			to bounce of players though, so manually trace ahead and see if we'd
//			hit something that we'd like the grenade to "collide" with.
//-----------------------------------------------------------------------------
void CTFWeaponBaseGrenadeProj::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );

	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity( &vel, &angVel );

	Vector start = GetAbsOrigin();

	// find all entities that my collision group wouldn't hit, but COLLISION_GROUP_NONE would and bounce off of them as a ray cast
	CTraceFilterCollisionGrenades filter( this, GetThrower() );
	trace_t tr;

	UTIL_TraceLine( start, start + vel * gpGlobals->frametime, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filter, &tr );

	bool bHitEnemy = false;

	if ( ( tr.m_pEnt && tr.m_pEnt->GetTeamNumber() != GetTeamNumber() ) || ( tr.m_pEnt && tr.m_pEnt->GetTeamNumber() == TF_TEAM_MERCENARY ) )
	{
		bHitEnemy = true;
	}

	if ( tr.startsolid )
	{
		if ( (m_bInSolid == false && m_bCollideWithTeammates == true) || ( m_bInSolid == false  && bHitEnemy == true ) )
		{
			// UNDONE: Do a better contact solution that uses relative velocity?
			vel *= -GRENADE_COEFFICIENT_OF_RESTITUTION; // bounce backwards
			pPhysics->SetVelocity( &vel, NULL );
		}
		m_bInSolid = true;
		return;
	}

	m_bInSolid = false;

	if ( tr.DidHit() )
	{
		Touch( tr.m_pEnt );
		
		if ( m_bCollideWithTeammates == true || bHitEnemy == true )
		{
			// reflect velocity around normal
			vel = -2.0f * tr.plane.normal * DotProduct(vel,tr.plane.normal) + vel;

			// absorb 80% in impact
			vel *= GetElasticity();

			if ( bHitEnemy == true )
			{
				vel *= 0.5f;
			}

			angVel *= -0.5f;
			pPhysics->SetVelocity( &vel, &angVel );
		}
	}
}


void CTFWeaponBaseGrenadeProj::DrawRadius( float flRadius )
{
	Vector pos = GetAbsOrigin();
	int r = 255;
	int g = 0, b = 0;
	float flLifetime = tf_grenade_show_radius_time.GetFloat();
	bool bDepthTest = true;

	Vector edge, lastEdge;
	NDebugOverlay::Line( pos, pos + Vector( 0, 0, 50 ), r, g, b, !bDepthTest, flLifetime );

	lastEdge = Vector( flRadius + pos.x, pos.y, pos.z );
	float angle;
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = pos.x;
		edge.y = flRadius * cos( angle ) + pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = flRadius * sin( angle ) + pos.y;
		edge.z = pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}
}

#endif

#ifdef GAME_DLL
void CTFWeaponBaseGrenadeProj::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	Vector vec = vecDir;

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if ( pPhysicsObject )
	{
		Vector vecZero;
		Vector vecVelocity;

		pPhysicsObject->GetVelocity( &vecZero, NULL );

		float flVelocity = 0.0f;
		flVelocity = vecZero.Length();

		vecVelocity = vec;
		vecVelocity *= flVelocity;

		AngularImpulse angImpulse( ( 600, random->RandomInt( -1200, 1200 ), 0 ) );

		// pPhysicsObject->AddVelocity( &vecVelocity, &angImpulse );
		pPhysicsObject->SetVelocityInstantaneous( &vecVelocity, &angImpulse );
	}
	
	SetOwnerEntity( pDeflectedBy );

	CBaseCombatCharacter *pBCC = dynamic_cast< CBaseCombatCharacter * >( pDeflectedBy );
	if ( pBCC )
		SetThrower( pBCC );

	ChangeTeam( pDeflectedBy->GetTeamNumber() );
	m_nSkin = pDeflectedBy->GetTeamNumber() - 2;
}
#endif