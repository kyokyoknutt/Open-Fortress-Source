//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_LIGHTNING_H
#define TF_WEAPON_LIGHTNING_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "particlemgr.h"
	#include "particle_util.h"
	#include "particles_simple.h"
#else
	#include "baseentity.h"
#endif

// Client specific.
#ifdef CLIENT_DLL
#define CTFLightningGun C_TFLightningGun
#endif

enum LightningGunState_t
{
	// Firing states.
	FT_STATE_IDLE = 0,
	FT_STATE_STARTFIRING,
	FT_STATE_FIRING
};

//=============================================================================
//
// TF Weapon Sub-machine gun.
//
class CTFLightningGun : public CTFWeaponBaseGun
{
public:
	DECLARE_CLASS(CTFLightningGun, CTFWeaponBaseGun);
	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID(void) const { return TF_WEAPON_LIGHTNING_GUN; }
	
	virtual acttable_t *ActivityList(int &iActivityCount);
	static acttable_t m_acttableLightningGun[];

	CTFLightningGun();
	~CTFLightningGun();	

	
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	ItemPostFrame( void );
	virtual void	PrimaryAttack();
//	virtual void	SecondaryAttack();
	virtual bool	Lower( void );
	virtual void	WeaponReset( void );

	virtual void	DestroySounds( void );

	Vector GetVisualMuzzlePos();
	Vector GetFlameOriginPos();
	
#if defined( CLIENT_DLL )
//	virtual bool	Deploy( void );

	virtual void	OnDataChanged(DataUpdateType_t updateType);
//	virtual void	UpdateOnRemove( void );
//	virtual void	SetDormant( bool bDormant );

	//	Start/stop flame sound and particle effects
	void			StartLightning();
	void			StopLightning( bool bAbrupt = false );

	void			RestartParticleEffect();
	virtual void	SetParticleEnd();

	// constant pilot light sound
//	void 			StartPilotLight();
//	void 			StopPilotLight();
#endif	
private:

	virtual float GetProjectileDamage( void );

private:
	Vector GetMuzzlePosHelper( bool bVisualPos );
	CNetworkVar( int, m_iWeaponState );
	CNetworkVar( int, m_bCritFire );

	float m_flStartFiringTime;
	float m_flNextPrimaryAttackAnim;

	int			m_iParticleWaterLevel;
	float		m_flAmmoUseRemainder;
#if defined( CLIENT_DLL )
	CSoundPatch	*m_pFiringStartSound;
	CSoundPatch	*m_pFiringLoop;
	bool		m_bFiringLoopCritical;
	bool		m_bLightningEffects;

	CNewParticleEffect	*m_pLightningParticle;
#endif

	CTFLightningGun( const CTFLightningGun & );
};


#endif // TF_WEAPON_LIGHTNING_H
