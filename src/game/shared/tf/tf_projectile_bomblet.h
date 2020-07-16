//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Bomblets Spawn By the BFG and Dynamite
//
//=============================================================================//
#ifndef TF_PROJECTILE_BOMBLET_H
#define TF_PROJECTILE_BOMBLET_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weapon_grenade_pipebomb.h"

#ifdef CLIENT_DLL
	#define CTFGrenadeMirvBomb C_TFGrenadeMirvBomb
#endif

class CTFGrenadeMirvBomb : public CTFGrenadePipebombProjectile
{
public:

	DECLARE_CLASS( CTFGrenadeMirvBomb, CTFGrenadePipebombProjectile );
	DECLARE_NETWORKCLASS();

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_GRENADE_MIRVBOMB; }

	virtual void	Spawn();
	virtual void	Precache();
#ifdef GAME_DLL
	// Creation.
	static CTFGrenadeMirvBomb *Create( const Vector &position, const QAngle &angles, const Vector &velocity, 
		                               const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, CTFWeaponInfo *pWeaponInfo, int teamnumber );
	virtual void	BounceSound( void );
	virtual bool	ExplodeOnImpact( void );
	KeyValues 		*pStatValues;
	bool			m_bExplodeOnImpact;
#else

	virtual void			OnDataChanged( DataUpdateType_t type );
	virtual void	CreateLightEffects(void);
	virtual const char *GetTrailParticleName( void );	
#endif
#ifdef CLIENT_DLL
	char	m_szBombletTrailParticle[MAX_WEAPON_STRING];
#else
	CNetworkVar(string_t,m_szBombletTrailParticle);
#endif
};

#endif //TF_PROJECTILE_BOMBLET_H