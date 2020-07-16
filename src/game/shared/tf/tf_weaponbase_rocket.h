//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: TF Base Rockets.
//
//=============================================================================//
#ifndef TF_WEAPONBASE_ROCKET_H
#define TF_WEAPONBASE_ROCKET_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_weaponbase.h"

#ifdef CLIENT_DLL
	#include "c_baseanimating.h"

	#define CTFBaseRocket C_TFBaseRocket
#endif

//#define TF_ROCKET_RADIUS	(110.0f * 1.1f)	//radius * TF scale up factor

//=============================================================================
//
// TF Base Rocket.
//
class CTFBaseRocket : public CBaseAnimating
{

//=============================================================================
//
// Shared (client/server).
//
public:

	DECLARE_CLASS( CTFBaseRocket, CBaseAnimating );
	DECLARE_NETWORKCLASS();

	CTFBaseRocket();
	~CTFBaseRocket();

	void	Precache( void );
	void	Spawn( void );

	virtual void	SetLauncher( CBaseEntity *pLauncher ) { m_hOriginalLauncher = pLauncher; }
	CBaseEntity		*GetOriginalLauncher( void ) { return m_hOriginalLauncher; }	
	
	virtual void	UpdateOnRemove( void );

protected:

	// Networked.
	CNetworkVector( m_vInitialVelocity );
	
public:

	CNetworkHandle( CBaseEntity, m_hOriginalLauncher );
	
	float	m_flCreationTime;

//=============================================================================
//
// Client specific.
//
#ifdef CLIENT_DLL

public:

	virtual int		DrawModel( int flags );
	virtual void	PostDataUpdate( DataUpdateType_t type );

private:

	float	 m_flSpawnTime;

//=============================================================================
//
// Server specific.
//
#else

public:

	DECLARE_DATADESC();

	static CTFBaseRocket *Create( CTFWeaponBase *pWeapon, const char *szClassname, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );	

	virtual void	RocketTouch( CBaseEntity *pOther );
	virtual void	Explode( trace_t *pTrace, CBaseEntity *pOther );
	virtual void 	Detonate( void );
	virtual void 	ExplodeManualy( trace_t *pTrace, int bitsDamageType, int bitsCustomDamageType );

	virtual float	GetDamage() { return m_flDamage; }
	virtual int		GetDamageType() { return g_aWeaponDamageTypes[ GetWeaponID() ]; }
	virtual int		GetCustomDamageType();
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }
	virtual void	SetDamageRadius(float flDamageRadius) { m_flDamageRadius = flDamageRadius; }

	virtual float	GetRadius() { return m_flDamageRadius; }	
	void			DrawRadius( float flRadius );

	unsigned int	PhysicsSolidMaskForEntity( void ) const;

	void			SetupInitialTransmittedGrenadeVelocity( const Vector &velocity )	{ m_vInitialVelocity = velocity; }

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_ROCKETLAUNCHER; }


	virtual CBaseEntity		*GetEnemy( void )			{ return m_hEnemy; }

	void			SetHomingTarget( CBaseEntity *pHomingTarget );
	void			SetHoming( bool bHoming );
	
	
	int 	m_hWeaponID;
	
	CNetworkVar( int,	m_bCritical );

	virtual bool	IsDeflectable( void ) { return true; }
	virtual void	Deflected( CBaseEntity *pDeflectedBy, Vector &vecDir );

protected:

	void			FlyThink( void );

protected:

	// Not networked.
	float					m_flDamage;
	float					m_flDamageRadius;

	float					m_flCollideWithTeammatesTime;
	bool					m_bCollideWithTeammates;


	CHandle<CBaseEntity>	m_hEnemy;
	
	CHandle<CBaseEntity>	m_hHomingTarget;
	
	bool	m_bHoming;

#endif
};

#endif // TF_WEAPONBASE_ROCKET_H