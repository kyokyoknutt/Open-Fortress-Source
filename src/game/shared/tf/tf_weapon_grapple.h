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
	//virtual void    SecondaryAttack( void );
	bool            CanHolster( void );
	virtual bool    Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	void            Drop( const Vector &vecVelocity );
	virtual bool    Reload( void );
	virtual void    ItemPostFrame( void );

	void			RemoveHook(void);
	void            NotifyHookDied(void);
	void			NotifyHookAttached(void);

	bool            HasAnyAmmo( void );

	void   			DrawBeam(const Vector &endPos, const float width = 2.f);
	void			DoImpactEffect( trace_t &tr, int nDamageType );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

private:

#ifdef GAME_DLL
	CHandle<CBeam>		pBeam;
	CHandle<CSprite>	m_pLightGlow;
	CNetworkHandle(CBaseEntity, m_hHook);	//server hook
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

	bool CreateVPhysics( void );
	unsigned int PhysicsSolidMaskForEntity() const;
	CWeaponGrapple *GetOwner(void) { return m_hOwner; }
	Class_T Classify( void ) { return CLASS_NONE; }
 
protected:

    DECLARE_DATADESC();
 
private:

	void HookTouch( CBaseEntity *pOther );
	void FlyThink( void );
  
    CHandle<CWeaponGrapple>     m_hOwner;
	CHandle<CTFPlayer>          m_hPlayer;
};
#endif

#endif // WEAPON_GRAPPLE_H