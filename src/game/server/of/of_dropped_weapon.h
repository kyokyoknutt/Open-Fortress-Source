//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef TF_DROPPED_WEAPON_H
#define TF_DROPPED_WEAPON_H
#ifdef _WIN32
#pragma once
#endif

#include "items.h"

class CTFDroppedWeapon : public CItem
{
public:
	DECLARE_CLASS( CTFDroppedWeapon, CItem );
	DECLARE_SERVERCLASS();

	CTFDroppedWeapon();

	virtual void Spawn();
	virtual void Precache();	

	void EXPORT FlyThink( void );
	void EXPORT PackTouch( CBaseEntity *pOther );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	static CTFDroppedWeapon *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName, int iWeaponID, const char *pszClassname );

	float GetCreationTime( void ) { return m_flCreationTime; }
	void  SetInitialVelocity( Vector &vecVelocity );
	
	int GetTeamNum(){ return m_iTeamNum; }
	void SetTeamNum( int iTeam ){ m_iTeamNum = iTeam; }

public:
	CNetworkVar( int, m_iReserveAmmo );
	CNetworkVar( int, m_iClip );
	CNetworkVar( bool, m_bFlamethrower );
	
	const char *pszWeaponName;
	CTFWeaponInfo *pWeaponInfo;
	int WeaponID;
private:
	float m_flCreationTime;

	bool m_bAllowOwnerPickup;
	CNetworkVector( m_vecInitialVelocity );
	
	int m_iTeamNum;

private:
	CTFDroppedWeapon( const CTFDroppedWeapon & );

	DECLARE_DATADESC();
};

#endif //TF_DROPPED_WEAPON_H