//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: An entity that shoots TF projectiles
//
// $NoKeywords: $
//=============================================================================//

// https://raw.githubusercontent.com/powerlord/tf2-data/master/datamaps.txt
/*
CBaseEntity - tf_point_weapon_mimic
- m_nWeaponType (Offset 856) (Save|Key)(4 Bytes) - WeaponType
- m_pzsFireSound (Offset 864) (Save|Key)(4 Bytes) - FireSound
- m_pzsFireParticles (Offset 868) (Save|Key)(4 Bytes) - ParticleEffect
- m_pzsModelOverride (Offset 872) (Save|Key)(4 Bytes) - ModelOverride
- m_flModelScale (Offset 876) (Save|Key)(4 Bytes) - ModelScale
- m_flSpeedMin (Offset 880) (Save|Key)(4 Bytes) - SpeedMin
- m_flSpeedMax (Offset 884) (Save|Key)(4 Bytes) - SpeedMax
- m_flDamage (Offset 888) (Save|Key)(4 Bytes) - Damage
- m_flSplashRadius (Offset 892) (Save|Key)(4 Bytes) - SplashRadius
- m_flSpreadAngle (Offset 896) (Save|Key)(4 Bytes) - SpreadAngle
- m_bCrits (Offset 900) (Save|Key)(1 Bytes) - Crits
- InputFireOnce (Offset 0) (Input)(0 Bytes) - FireOnce
- InputFireMultiple (Offset 0) (Input)(0 Bytes) - FireMultiple
- DetonateStickies (Offset 0) (Input)(0 Bytes) - DetonateStickies
*/

#include "cbase.h"
#include "tf_projectile_rocket.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_fx.h"
#include "tf_point_weapon_mimic.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar tf_grenadelauncher_livetime;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CTFPointWeaponMimic )
	DEFINE_KEYFIELD( m_nWeaponType, FIELD_INTEGER, "WeaponType" ),
	DEFINE_KEYFIELD( m_pzsFireSound, FIELD_SOUNDNAME, "FireSound" ),
	DEFINE_KEYFIELD( m_pzsFireParticles, FIELD_STRING, "ParticleEffect" ),
	DEFINE_KEYFIELD( m_pzsModelOverride, FIELD_MODELNAME, "m_pzsModelOverride" ),
	DEFINE_KEYFIELD( m_flModelScale, FIELD_FLOAT, "ModelScale" ),
	DEFINE_KEYFIELD( m_flSpeedMin, FIELD_FLOAT, "SpeedMin" ),
	DEFINE_KEYFIELD( m_flSpeedMax, FIELD_FLOAT, "SpeedMax" ),
	DEFINE_KEYFIELD( m_flDamage, FIELD_FLOAT, "Damage" ),
	DEFINE_KEYFIELD( m_flSplashRadius, FIELD_FLOAT, "SplashRadius" ),
	DEFINE_KEYFIELD( m_flSpreadAngle, FIELD_FLOAT, "SpreadAngle" ),
	DEFINE_KEYFIELD( m_bCrits, FIELD_INTEGER, "Crits" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID,	"FireOnce", InputFireOnce ),
	DEFINE_INPUTFUNC( FIELD_INTEGER,	"FireMultiple", InputFireMultiple ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DetonateStickies", DetonateStickies ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_point_weapon_mimic, CTFPointWeaponMimic );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponInfo *GetTFWeaponInfo( int iWeapon )
{
	// Get the weapon information.
	const char *pszWeaponAlias = WeaponIdToAlias( iWeapon );

	if ( !pszWeaponAlias )
	{
		return NULL;
	}

	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pszWeaponAlias );
	if ( hWpnInfo == GetInvalidWeaponInfoHandle() )
	{
		return NULL;
	}

	CTFWeaponInfo *pWeaponInfo = static_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	return pWeaponInfo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPointWeaponMimic::CTFPointWeaponMimic()
{
	m_nWeaponType = 0;
	m_pzsFireParticles = NULL_STRING;
	m_pzsFireSound = NULL_STRING;
	m_pzsModelOverride = NULL_STRING;
	m_flModelScale = 1;
	m_flSpeedMin = 1000;
	m_flSpeedMax = 1000;
	m_flDamage = 75;
	m_flSplashRadius = 50;
	m_flSpreadAngle = 0;
	m_bCrits = 0;

	m_bHasOverridenModel = true;
}

CTFPointWeaponMimic::~CTFPointWeaponMimic()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPointWeaponMimic::Spawn()
{
	switch ( m_nWeaponType )
	{
	case 0:
		pRocketInfo = GetTFWeaponInfo( TF_WEAPON_ROCKETLAUNCHER );
		if ( !pRocketInfo )
		{
			Warning( "tf_point_weapon_mimic at %.0f %.0f %0.f missing Rocket weapon info, removed\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
			UTIL_Remove( this );
		}
		break;
	case 1:
		pGrenadeInfo = GetTFWeaponInfo( TF_WEAPON_PIPEBOMBLAUNCHER );
		if ( !pGrenadeInfo )
		{
			Warning( "tf_point_weapon_mimic at %.0f %.0f %0.f missing Grenade weapon info, removed\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
			UTIL_Remove( this );
		}
		break;
	case 2:		
		// 2 does not exist as we dont have arrows
		{
			Warning( "tf_point_weapon_mimic at %.0f %.0f %0.f is using Arrow weapon type. This is not supported yet, removed\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
			UTIL_Remove( this );
		}
		break;
	// 2 does not exist as we dont have arrows
	case 3:
		pStickyBombInfo = GetTFWeaponInfo( TF_WEAPON_GRENADELAUNCHER );
		if ( !pStickyBombInfo )
		{
			Warning( "tf_point_weapon_mimic at %.0f %.0f %0.f missing Stickybomb weapon info, removed\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
			UTIL_Remove( this );
		}
		break;
	default:
		{
			Warning( "tf_point_weapon_mimic at %.0f %.0f %0.f is using Unknown weapon type, removed\n", GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z );
			UTIL_Remove( this );
		}
		break;
	}

	char *szModel = (char *)STRING( m_pzsModelOverride );

	if ( !szModel || !*szModel )
	{
		m_bHasOverridenModel = false;
	}
	else
	{
		m_bHasOverridenModel = true;
		PrecacheModel( STRING( m_pzsModelOverride ) );
	}

	if ( m_pzsFireSound != NULL_STRING )
		PrecacheScriptSound( STRING( m_pzsFireSound ) );
	if ( m_pzsFireParticles != NULL_STRING )
		PrecacheParticleSystem( STRING( m_pzsFireParticles ) );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPointWeaponMimic::GetSpeed()
{
	return RandomFloat( m_flSpeedMin, m_flSpeedMax );
}

Vector CTFPointWeaponMimic::GetFiringOrigin()
{
	Vector vecForward, vecRight, vecUp;
	AngleVectors( GetAbsAngles(), &vecForward, &vecRight, &vecUp );

	Vector vecSrc = GetAbsOrigin();
	vecSrc +=  vecForward * 16.0f + vecRight + vecUp * -6.0f;
	
	Vector vecVelocity = ( vecForward * GetSpeed() ) + ( vecUp * 200.0f ) + ( random->RandomFloat( -10.0f, 10.0f ) * vecRight ) +		
		( random->RandomFloat( -10.0f, 10.0f ) * vecUp );

	return vecVelocity;
}

QAngle CTFPointWeaponMimic::GetFiringAngles()
{
	QAngle angForward = GetAbsAngles();

	angForward.x += RandomFloat( -m_flSpreadAngle, m_flSpreadAngle );
	angForward.y += RandomFloat( -m_flSpreadAngle, m_flSpreadAngle );

	return angForward;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPointWeaponMimic::Fire()
{
	switch ( m_nWeaponType )
	{
	case 0:
		this->FireRocket();
		break;
	case 1:
		this->FireGrenade();
		break;
	// 2 does not exist as we dont have arrows
	case 3:
		this->FireStickyGrenade();
		break;
	}

	if ( m_pzsFireSound != NULL_STRING )
		EmitSound( STRING( m_pzsFireSound ) );

	if ( m_pzsFireParticles != NULL_STRING )
	{
		CPVSFilter filter( GetAbsOrigin() );
		TE_TFParticleEffect( filter, 0.0f, STRING( m_pzsFireParticles ), GetAbsOrigin(), GetAbsAngles(), NULL, PATTACH_CUSTOMORIGIN );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPointWeaponMimic::FireRocket()
{
	CTFProjectile_Rocket *pProjectile = CTFProjectile_Rocket::Create( NULL, GetAbsOrigin(), GetFiringAngles(), this, this );

	if ( pProjectile )
	{
		if ( m_bHasOverridenModel )
		{
			pProjectile->SetModel( STRING( m_pzsModelOverride ) );
		}
		pProjectile->SetCritical( m_bCrits );
		pProjectile->SetDamage( m_flDamage );
		pProjectile->SetDamageRadius( m_flSplashRadius );
		pProjectile->SetModelScale( m_flModelScale );
	}
}

void CTFPointWeaponMimic::FireGrenade()
{
	CTFGrenadePipebombProjectile *pProjectile = CTFGrenadePipebombProjectile::Create( GetAbsOrigin(), GetFiringAngles(), GetFiringOrigin(), 
		AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ),
		NULL, *pGrenadeInfo, false, dynamic_cast<CTFWeaponBase*>(this));

	if ( pProjectile )
	{
		if ( m_bHasOverridenModel )
		{
			pProjectile->SetModel( STRING( m_pzsModelOverride ) );
		}
		pProjectile->SetCritical( m_bCrits );
		pProjectile->SetDamage( m_flDamage );
		pProjectile->SetDamageRadius( m_flSplashRadius );
		pProjectile->SetModelScale( m_flModelScale );
	}
}

void CTFPointWeaponMimic::FireStickyGrenade()
{
	CTFGrenadePipebombProjectile *pProjectile = CTFGrenadePipebombProjectile::Create( GetAbsOrigin(), GetFiringAngles(), GetFiringOrigin(), 
		AngularImpulse( 600, random->RandomInt( -1200, 1200 ), 0 ),
		NULL, *pStickyBombInfo, true, dynamic_cast<CTFWeaponBase*>(this) );

	if ( pProjectile )
	{
		m_StickyBombs.AddToTail( pProjectile );
		m_iStickyBombCount = m_StickyBombs.Count();

		if ( m_bHasOverridenModel )
		{
			pProjectile->SetModel( STRING( m_pzsModelOverride ) );
		}
		pProjectile->SetCritical( m_bCrits );
		pProjectile->SetDamage( m_flDamage );
		pProjectile->SetDamageRadius( m_flSplashRadius );
		pProjectile->SetModelScale( m_flModelScale );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPointWeaponMimic::DeathNotice( CBaseEntity *pVictim )
{
	if ( pVictim )
	{
		CTFGrenadePipebombProjectile *pSticky = (CTFGrenadePipebombProjectile*)pVictim;

		if ( pSticky )
		{
			m_StickyBombs.FindAndRemove( pSticky );

			m_iStickyBombCount = m_StickyBombs.Count();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPointWeaponMimic::InputFireOnce( inputdata_t &inputdata )
{
	Fire();
}

void CTFPointWeaponMimic::InputFireMultiple( inputdata_t &inputdata )
{
	int iCount = inputdata.value.Int();

	if ( iCount <= 0 )
		return;

	int i;

	for ( i = 0; i < iCount; i++ )
	{
		Fire();
	}
}

void CTFPointWeaponMimic::DetonateStickies( inputdata_t &inputdata )
{
	int count = m_StickyBombs.Count();

	for ( int i = 0; i < count; i++ )
	{
		CTFGrenadePipebombProjectile *pTemp = m_StickyBombs[i];

		if ( pTemp )
		{
			//This guy will die soon enough.
			if ( pTemp->IsEffectActive( EF_NODRAW ) )
				continue;

			if ( ( gpGlobals->curtime - pTemp->m_flCreationTime ) < tf_grenadelauncher_livetime.GetFloat() )
			{
				continue;
			}

			pTemp->SetTimer( gpGlobals->curtime - 10 );
		}
	}
}