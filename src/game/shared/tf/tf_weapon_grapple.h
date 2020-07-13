//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the grapple hook weapon.
//			
//			Primary attack: fires a beam that hooks on a surface.
//			Secondary attack: switches between pull and rapple modes
//
//
//=============================================================================//

#ifndef WEAPON_GRAPPLE_H
#define WEAPON_GRAPPLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "Sprite.h"
#include "rope_shared.h"
#include "beam_shared.h"

#ifdef CLIENT_DLL
	#define CWeaponGrapple C_WeaponGrapple
	#define CTFEternalShotgun C_TFEternalShotgun
#else
	#include "props.h"
	#include "te_effect_dispatch.h"
#endif

//-----------------------------------------------------------------------------
// CWeaponGrapple
//-----------------------------------------------------------------------------

class CWeaponGrapple : public CTFWeaponBaseGun       
{                                                            
	DECLARE_CLASS( CWeaponGrapple, CTFWeaponBaseGun ); 

public:

	CWeaponGrapple( void );

	virtual void    Precache( void );
	virtual void    PrimaryAttack( void );
	bool            CanHolster( void );
	virtual bool    Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	void            Drop( const Vector &vecVelocity );
	virtual void    ItemPostFrame( void );

	void			RemoveHook(void);
	void			DoImpactEffect(trace_t &tr, int nDamageType);

#ifdef GAME_DLL
	void			NotifyHookAttached(void);
	void   			DrawBeam(const Vector &endPos, const float width = 2.f);
#endif

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

private:

	friend class CTFEternalShotgun;

	void InitiateHook(CTFPlayer * pPlayer, CBaseEntity *hook);

#ifdef GAME_DLL
	CHandle<CBeam>		pBeam;
	CHandle<CSprite>	m_pLightGlow;
	CNetworkHandle(CBaseEntity, m_hHook);		//server hook
#else
	EHANDLE			m_hHook;				//client hook relay
#endif

	CWeaponGrapple(const CWeaponGrapple &);
	CNetworkVar(int, m_iAttached);
	CNetworkVar(int, m_nBulletType);
};

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Grapple Hook
//-----------------------------------------------------------------------------
class CGrappleHook : public CBaseCombatCharacter
{
    DECLARE_CLASS( CGrappleHook, CBaseCombatCharacter );

public:

	CGrappleHook(void) {}
    ~CGrappleHook(void);
    void Spawn( void );
    void Precache( void );
	static CGrappleHook *HookCreate( const Vector &vecOrigin, const QAngle &angAngles, CBaseEntity *pentOwner = NULL );
	bool HookLOS();

	virtual bool CreateVPhysics( void );
	virtual unsigned int PhysicsSolidMaskForEntity() const;
	CWeaponGrapple *GetOwner(void) { return m_hOwner; }
	virtual Class_T Classify( void ) { return CLASS_NONE; }
 
protected:

    DECLARE_DATADESC();
 
private:

	friend class MeatHook;

	void HookTouch( CBaseEntity *pOther );
	virtual void FlyThink( void );
  
	CWeaponGrapple		*m_hOwner;
	CTFPlayer			*m_hPlayer;
};
#endif

#endif // WEAPON_GRAPPLE_H