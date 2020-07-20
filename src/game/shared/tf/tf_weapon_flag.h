//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_FLAG_H
#define TF_WEAPON_FLAG_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

#ifdef CLIENT_DLL
#define CTFFlag C_TFFlag
#endif

//=============================================================================
//
// Bottle class.
//
class CTFFlag : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFFlag, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_DATADESC();

	CTFFlag();
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_C4; }
	virtual bool		Deploy( void );
	virtual bool 		Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void 		PrimaryAttack( void );
	virtual void 		ItemPostFrame( void );
	//
	void				ResetBomb();
	void				PlantBomb();
	void				Explode();
	
#ifdef GAME_DLL
	void				PlantedThink();
	void				SetExplodeTick( float flExplodeTick ){ m_flExplodeTick = flExplodeTick; }
	virtual void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void 				DrawRadius( float flRadius );
#endif

	virtual bool		CanDrop( void ) { return true; }
	virtual bool		CanDropManualy( void ){ return true; }
	virtual void 		Equip( CBaseCombatCharacter *pOwner );
public:
	CNetworkVar( bool,	m_bInPlantZone );
	CNetworkVar( bool,	m_bOldInPlantZone );
	CNetworkVar( bool,	m_bPlanting );
	CNetworkVar( bool, m_bPlanted );

	CNetworkVar( float, m_flPlantStart );
	
	CNetworkVar( float, m_flExplodeTick );
	
private:

	CTFFlag( const CTFFlag & ) {}
};

#endif // TF_WEAPON_FLAG_H
