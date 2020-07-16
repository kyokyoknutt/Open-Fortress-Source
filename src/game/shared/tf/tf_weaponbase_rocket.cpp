//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase_rocket.h"
#include "tf_weapon_rocketlauncher.h"
#include "tf_gamerules.h"

#ifdef GAME_DLL
	#include "soundent.h"
	#include "tf_fx.h"
	#include "tf_projectile_bomblet.h"

	extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
	extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
#endif

//=============================================================================
//
// TF Base Rocket tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFBaseRocket, DT_TFBaseRocket )

BEGIN_NETWORK_TABLE( CTFBaseRocket, DT_TFBaseRocket )
#ifdef CLIENT_DLL
	RecvPropVector( RECVINFO( m_vInitialVelocity ) ),
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropQAngles( RECVINFO_NAME( m_angNetworkAngles, m_angRotation ) ),
#else
	SendPropVector( SENDINFO( m_vInitialVelocity ), 12 /*nbits*/, 0 /*flags*/, -3000 /*low value*/, 3000 /*high value*/	),
	SendPropExclude( "DT_BaseEntity", "m_vecOrigin" ),
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD_MP_INTEGRAL|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropQAngles	(SENDINFO(m_angRotation), 6, SPROP_CHANGES_OFTEN, SendProxy_Angles ),
#endif
END_NETWORK_TABLE()

// Server specific.
#ifdef GAME_DLL
	BEGIN_DATADESC( CTFBaseRocket )
	DEFINE_ENTITYFUNC( RocketTouch ),
	DEFINE_THINKFUNC( FlyThink ),
	END_DATADESC()
#endif

ConVar tf_rocket_show_radius( "tf_rocket_show_radius", "0", FCVAR_REPLICATED | FCVAR_CHEAT , "Render rocket radius." );

//=============================================================================
//
// Shared (client/server) functions.
//

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CTFBaseRocket::CTFBaseRocket()
{
	m_vInitialVelocity.Init();

// Client specific.
#ifdef CLIENT_DLL

	m_flSpawnTime = 0.0f;
		
// Server specific.
#else

	m_flDamage = 0.0f;
	m_flDamageRadius = (110.0f * 1.1f);
	m_bHoming = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------
CTFBaseRocket::~CTFBaseRocket()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::Spawn( void )
{
	// Precache.
	Precache();

// Client specific.
#ifdef CLIENT_DLL

	m_flSpawnTime = gpGlobals->curtime;
	BaseClass::Spawn();

// Server specific.
#else

	//Derived classes must have set model, Things Like the BFG dont, and similar example may occur later
//	Assert( GetModel() );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );
	AddEffects( EF_NOSHADOW );

	SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );

	UTIL_SetSize( this, -Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	// Setup attributes.
	m_takedamage = DAMAGE_NO;
	SetGravity( 0.0f );

	// Setup the touch and think functions.
	SetTouch( &CTFBaseRocket::RocketTouch );
	SetThink( &CTFBaseRocket::FlyThink );
	SetNextThink( gpGlobals->curtime );

	// Don't collide with players on the owner's team for the first bit of our life
	m_flCollideWithTeammatesTime = gpGlobals->curtime + 0.25;
	m_bCollideWithTeammates = false;

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::UpdateOnRemove( void )
{
	// Tell our launcher that we were removed
	CTFSuperRocketLauncher *pLauncher = dynamic_cast<CTFSuperRocketLauncher*>( GetOriginalLauncher() );

	if ( pLauncher )
	{
		pLauncher->DeathNotice( this );
	}

	BaseClass::UpdateOnRemove();
}



//=============================================================================
//
// Client specific functions.
//
#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::PostDataUpdate( DataUpdateType_t type )
{
	// Pass through to the base class.
	BaseClass::PostDataUpdate( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		// Now stick our initial velocity and angles into the interpolation history.
		CInterpolatedVar<Vector> &interpolator = GetOriginInterpolator();
		interpolator.ClearHistory();

		CInterpolatedVar<QAngle> &rotInterpolator = GetRotationInterpolator();
		rotInterpolator.ClearHistory();

		float flChangeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );

		// Add a sample 1 second back.
		Vector vCurOrigin = GetLocalOrigin() - m_vInitialVelocity;
		interpolator.AddToHead( flChangeTime - 1.0f, &vCurOrigin, false );

		QAngle vCurAngles = GetLocalAngles();
		rotInterpolator.AddToHead( flChangeTime - 1.0f, &vCurAngles, false );

		// Add the current sample.
		vCurOrigin = GetLocalOrigin();
		interpolator.AddToHead( flChangeTime, &vCurOrigin, false );

		rotInterpolator.AddToHead( flChangeTime - 1.0, &vCurAngles, false );
		
		CTFSuperRocketLauncher *pLauncher = dynamic_cast<CTFSuperRocketLauncher*>( m_hOriginalLauncher.Get() );

		if ( pLauncher )
		{
			pLauncher->AddRocket( this );
		}		
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFBaseRocket::DrawModel( int flags )
{
	// During the first 0.2 seconds of our life, don't draw ourselves.
	if ( gpGlobals->curtime - m_flSpawnTime < 0.2f )
		return 0;

	return BaseClass::DrawModel( flags );
}

//=============================================================================
//
// Server specific functions.
//
#else

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFBaseRocket *CTFBaseRocket::Create( CTFWeaponBase *pWeapon, const char *pszClassname, const Vector &vecOrigin, 
									  const QAngle &vecAngles, CBaseEntity *pOwner )
{
	CTFBaseRocket *pRocket = static_cast<CTFBaseRocket*>( CBaseEntity::CreateNoSpawn( pszClassname, vecOrigin, vecAngles, pOwner ) );
	if ( !pRocket )
		return NULL;
	
	// Initialize the owner.
	pRocket->SetOwnerEntity( pOwner );
	if ( pWeapon )
	{
		pRocket->m_hWeaponID = pWeapon->GetWeaponID();
		pRocket->SetLauncher(pWeapon);
		if ( pWeapon->GetDamageRadius() >= 0 )
			pRocket->SetDamageRadius( pWeapon->GetDamageRadius() );
	}
	// Spawn.
	pRocket->Spawn();
	
	if( pWeapon && pWeapon->GetTFWpnData().m_nProjectileModel[0] != 0 )
	{
		const char *s_PipebombModel = pWeapon->GetTFWpnData().m_nProjectileModel;	
		if ( s_PipebombModel )
		{
			PrecacheModel( s_PipebombModel );
			pRocket->SetModel( s_PipebombModel );
		}	
		UTIL_SetSize( pRocket, -Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	}

	// Setup the initial velocity.
	Vector vecForward, vecRight, vecUp;
	AngleVectors( vecAngles, &vecForward, &vecRight, &vecUp );

	Vector vecVelocity = vecForward * 1100.0f;
	pRocket->SetAbsVelocity( vecVelocity );	
	pRocket->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

	// Setup the initial angles.
	QAngle angles;
	VectorAngles( vecVelocity, angles );
	pRocket->SetAbsAngles( angles );
	
	// Set team.
	if ( pOwner )
		pRocket->ChangeTeam( pOwner->GetTeamNumber() );
	else
		pRocket->ChangeTeam( TF_TEAM_MERCENARY );

	pRocket->m_flCreationTime = gpGlobals->curtime;
	
	return pRocket;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::RocketTouch( CBaseEntity *pOther )
{
	// Verify a correct "other."
	if( pOther )
	{
		Assert( pOther );
		if ( pOther->IsSolidFlagSet( FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS ) )
			return;
	}
	// Handle hitting skybox (disappear).
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if( pTrace->surface.flags & SURF_SKY )
	{
		UTIL_Remove( this );
		return;
	}

	trace_t trace;
	memcpy( &trace, pTrace, sizeof( trace_t ) );
	Explode( &trace, pOther );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFBaseRocket::PhysicsSolidMaskForEntity( void ) const
{ 
	int teamContents = 0;

	if ( m_bCollideWithTeammates == false )
	{
		// Only collide with the other team
		teamContents = ( GetTeamNumber() == TF_TEAM_RED ) ? CONTENTS_BLUETEAM : CONTENTS_REDTEAM;
	}
	else
	{
		// Collide with both teams
		teamContents = CONTENTS_REDTEAM | CONTENTS_BLUETEAM;
	}

	return BaseClass::PhysicsSolidMaskForEntity() | teamContents;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFBaseRocket::GetCustomDamageType() 
{ 
	return TF_DMG_CUSTOM_NONE;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::Detonate( void )
{
	trace_t		tr;
	Vector		vecSpot;// trace starts here!

	SetThink( NULL );

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & tr);

	ExplodeManualy(&tr, GetDamageType(), GetCustomDamageType());

}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::ExplodeManualy( trace_t *pTrace, int bitsDamageType, int bitsCustomDamageType )
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

	if ( pTrace->m_pEnt && pTrace->m_pEnt->IsPlayer() )
	{
		TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, pTrace->m_pEnt->entindex(), GetWeaponID() , GetOriginalLauncher() );
	}
	else
	{
		TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, -1, GetWeaponID(), GetOriginalLauncher() );
	}


	// Use the thrower's position as the reported position
	Vector vecReported = GetOwnerEntity() ? GetOwnerEntity()->GetAbsOrigin() : vec3_origin;

	CTakeDamageInfo info( this, GetOwnerEntity(), vec3_origin, vecOrigin, GetDamage(), bitsDamageType, 0, &vecReported );

	CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase*>( GetOriginalLauncher() );
	info.SetWeapon( pTFWeapon );
	
	float flRadius = GetRadius();

	if ( tf_rocket_show_radius.GetBool() )
	{
		DrawRadius( flRadius );
	}

	RadiusDamage( info, vecOrigin, flRadius, CLASS_NONE, NULL );

	// Don't decal players with scorch.
	if ( pTrace->m_pEnt && !pTrace->m_pEnt->IsPlayer() )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

	UTIL_Remove( this );

	AddEffects( EF_NODRAW );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBaseRocket::Explode( trace_t *pTrace, CBaseEntity *pOther )
{
	// Save this entity as enemy, they will take 100% damage.
	if ( !pOther )
		pOther = pTrace->m_pEnt;
	m_hEnemy = pOther;

	// Invisible.
	SetModelName( NULL_STRING );
	AddSolidFlags( FSOLID_NOT_SOLID );
	m_takedamage = DAMAGE_NO;

	// Pull out a bit.
	if ( pTrace->fraction != 1.0 )
	{
		SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
	}
	// SetAbsOrigin( pTrace->endpos - GetAbsVelocity() );

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter( vecOrigin );
	int EntIndex = 0;
	if ( pOther )
		EntIndex = pOther->entindex();
	TE_TFExplosion( filter, 0.0f, vecOrigin, pTrace->plane.normal, EntIndex, GetWeaponID(), GetOriginalLauncher() );
	CSoundEnt::InsertSound ( SOUND_COMBAT, vecOrigin, 1024, 3.0 );

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>( pAttacker );
	if ( pScorerInterface )
	{
		pAttacker = pScorerInterface->GetScorer();
	}

	CTakeDamageInfo info( this, pAttacker, vec3_origin, vecOrigin, GetDamage(), GetDamageType() );
	CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase*>( GetOriginalLauncher() );
	info.SetWeapon( pTFWeapon );
	info.SetDamageCustom( GetCustomDamageType() );
	float flRadius = GetRadius();

	RadiusDamage( info, vecOrigin, flRadius, CLASS_NONE, NULL );

	// Debug!
	if ( tf_rocket_show_radius.GetBool() )
	{
		DrawRadius( flRadius );
	}

	// Don't decal players with scorch.
	if ( !pOther->IsPlayer() )
	{
		UTIL_DecalTrace( pTrace, "Scorch" );
	}

// Get the Weapon info
	CTFWeaponBase *pWeapon = (CTFWeaponBase * )CreateEntityByName( WeaponIdToAlias( m_hWeaponID ) );
	if ( pWeapon )
	{
		WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pWeapon->GetClassname() );
		Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
		CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
		Assert( pWeaponInfo && "Failed to get CTFWeaponInfo in weapon spawn" );
		
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
				CTFPlayer *pPlayer = ToTFPlayer( GetOwnerEntity() );	
		
				CTFGrenadeMirvBomb *pBomb = CTFGrenadeMirvBomb::Create( vecSrc, GetAbsAngles(), vecVelocity, vecZero, pPlayer, pWeaponInfo, GetTeamNumber() );
				PrecacheModel ( pWeaponInfo->m_szBombletModel );
				pBomb->SetModel( pWeaponInfo->m_szBombletModel );
				pBomb->SetDamage( pWeaponInfo->m_flBombletDamage );
				pBomb->SetDetonateTimerLength( pWeaponInfo->m_flBombletTimer + random->RandomFloat( 0.0f, 1.0f ) );
				pBomb->SetDamageRadius( pWeaponInfo->m_flBombletDamageRadius );
				pBomb->SetCritical( m_bCritical );
				pBomb->SetLauncher(GetOriginalLauncher());
			}		
		}
#endif
		UTIL_Remove( pWeapon );
	}	
	
	// Remove the rocket.
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFBaseRocket::DrawRadius( float flRadius )
{
	Vector pos = GetAbsOrigin();
	int r = 255;
	int g = 0, b = 0;
	float flLifetime = 10.0f;
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

void CTFBaseRocket::FlyThink( void )
{
	if ( gpGlobals->curtime > m_flCollideWithTeammatesTime && m_bCollideWithTeammates == false )
	{
		m_bCollideWithTeammates = true;
	}

	if( m_bHoming && m_hHomingTarget )
	{
		Vector vecTarget = m_hHomingTarget->GetAbsOrigin();
		Vector vecDir = vecTarget - GetAbsOrigin();
		VectorNormalize( vecDir );
		float flSpeed = GetAbsVelocity().Length();
		QAngle angForward;
		VectorAngles( vecDir, angForward );
		SetAbsAngles( angForward );
		SetAbsVelocity( vecDir * flSpeed );
	}
	
	SetNextThink( gpGlobals->curtime + 0.1 );
}

void CTFBaseRocket::SetHomingTarget( CBaseEntity *pHomingTarget )
{
	m_bHoming = true;
	m_hHomingTarget = pHomingTarget;
}

void CTFBaseRocket::SetHoming( bool bHoming )
{
	m_bHoming = bHoming;
	if( !m_bHoming )
		m_hHomingTarget = NULL;
}

#endif

#ifdef GAME_DLL
void CTFBaseRocket::Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir )
{
	m_bHoming = false;
	Vector vec = vecDir;

	// conserver speed when changing trajectory
	float flLength = GetAbsVelocity().Length();

	QAngle angles;
	VectorAngles( vec, angles );

	SetAbsAngles( angles );

	SetAbsVelocity( vec * flLength );
	
	SetOwnerEntity( pDeflectedBy );
	ChangeTeam( pDeflectedBy->GetTeamNumber() );
}
#endif