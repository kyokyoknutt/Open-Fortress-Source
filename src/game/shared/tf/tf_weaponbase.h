//========= Copyright ? 1996-2004, Valve LLC, All rights reserved. ============
//
//	Weapons.
//
//	CTFWeaponBase
//	|
//	|--> CTFWeaponBaseMelee
//	|		|
//	|		|--> CTFWeaponCrowbar
//	|		|--> CTFWeaponKnife
//	|		|--> CTFWeaponMedikit
//	|		|--> CTFWeaponWrench
//	|
//	|--> CTFWeaponBaseGrenade
//	|		|
//	|		|--> CTFWeapon
//	|		|--> CTFWeapon
//	|
//	|--> CTFWeaponBaseGun
//
//=============================================================================
#ifndef TF_WEAPONBASE_H
#define TF_WEAPONBASE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_playeranimstate.h"
#include "tf_weapon_parse.h"
#include "npcevent.h"

// Client specific.
#if defined( CLIENT_DLL )
#define CTFWeaponBase C_TFWeaponBase
#define CTFWeaponBaseGrenadeProj C_TFWeaponBaseGrenadeProj
#include "tf_fx_muzzleflash.h"
#include "baseparticleentity.h"
#endif

#define MAX_TRACER_NAME		128

class CTFPlayer;
class CBaseObject;
class CTFWeaponBaseGrenadeProj;

// Given an ammo type (like from a weapon's GetPrimaryAmmoType()), this compares it
// against the ammo name you specify.
// TFTODO: this should use indexing instead of searching and strcmp()'ing all the time.
bool IsAmmoType( int iAmmoType, const char *pAmmoName );
void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, CBaseEntity *pEntity );

// Reloading singly.
enum
{
	TF_RELOAD_START = 0,
	TF_RELOADING,
	TF_RELOADING_CONTINUE,
	TF_RELOAD_FINISH
};

// structure to encapsulate state of head bob
struct BobState_t
{
	BobState_t() 
	{ 
		m_flBobTime = 0; 
		m_flLastBobTime = 0;
		m_flLastSpeed = 0;
		m_flVerticalBob = 0;
		m_flLateralBob = 0;
	}

	float m_flBobTime;
	float m_flLastBobTime;
	float m_flLastSpeed;
	float m_flVerticalBob;
	float m_flLateralBob;
};

#ifdef CLIENT_DLL
float CalcViewModelBobHelper( CBasePlayer *player, BobState_t *pBobState );
void AddViewModelBobHelper( Vector &origin, QAngle &angles, BobState_t *pBobState );
#endif

// Interface for weapons that have a charge time
class ITFChargeUpWeapon 
{
public:
	virtual float GetChargeBeginTime( void ) = 0;

	virtual float GetChargeMaxTime( void ) = 0;
};

//=============================================================================
//
// Base TF Weapon Class
//
class CTFWeaponBase : public CBaseCombatWeapon
{
	DECLARE_CLASS( CTFWeaponBase, CBaseCombatWeapon );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
#if !defined ( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	// Setup.
	CTFWeaponBase();
	// Deconstructor
	~CTFWeaponBase();

	virtual void Spawn();
	virtual void Equip( CBaseCombatCharacter *pOwner );
	virtual void Precache();
	virtual bool IsPredicted() const			{ return true; }
	virtual void FallInit( void );

	int		GetTeamNum(){ return m_iTeamNum; };
	void	SetTeamNum( int iTeam ){ m_iTeamNum = iTeam; };
	
	// Weapon Data.
	CTFWeaponInfo const	&GetTFWpnData() const;
	virtual int GetWeaponID( void ) const;
	bool IsWeapon( int iWeapon ) const;
	virtual int	GetDamageType() const { return g_aWeaponDamageTypes[ GetWeaponID() ]; }
	float GetDamageRadius( void ) const;
	
	virtual int GetCustomDamageType() const { return TF_DMG_CUSTOM_NONE; }

	int			GetGGLevel( void ){ return m_iGGLevel; }
	void		SetGGLevel( int level ){ m_iGGLevel = level; }
	
	bool		NeverStrip( void ){ return m_bNeverStrip; }
	
	// View model.
	virtual const char *GetViewModel( int iViewModel = 0 ) const;

#ifdef CLIENT_DLL
	C_BaseAnimating *GetOwnModel( void );
#endif

	virtual void Drop( const Vector &vecVelocity );
	virtual void Detach();
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool CanHolster( void ) const;
	virtual bool Deploy( void );
	virtual bool ReloadOrSwitchWeapons( void );
	virtual int	 GetSlot( void );
	virtual int	 GetPosition( void );
	
	virtual int  GetSlotOverride( void ) { return m_iSlotOverride; };
	virtual int  GetPositionOverride( void ){ return m_iPositionOverride; };
	
	virtual void  SetSlotOverride( int iSlot ) { m_iSlotOverride = iSlot; };
	virtual void  SetPositionOverride( int iPos ){ m_iPositionOverride = iPos; };
	
	virtual int	 GetDamage( void ) const;
	virtual bool CanSecondaryAttack( void ) const;
	virtual bool CanDropManualy( void ) const;
	virtual bool DontAutoEquip( void ) const;
	virtual bool LoadsManualy( void ) const;
	virtual int  GetDefaultClip1( void ) const;
	
	virtual void PlayWeaponShootSound( void );
	virtual bool PrimaryAttackSwapsActivities(void) { return GetTFWpnData().m_bSwapFireAnims; }
	
	// Attacks.
	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual bool FiresInBursts();
	void CalcIsAttackCritical( void );
	virtual bool CalcIsAttackCriticalHelper();
	int IsCurrentAttackACrit();
	virtual float GetFireRate( void );

	// Reloads.
	virtual bool Reload( void );
	virtual void AbortReload( void );
	virtual bool DefaultReload( int iClipSize1, int iClipSize2, int iActivity );
	void SendReloadEvents();

	virtual bool CanDrop( void ) { return false; }
	virtual bool CanSoftZoom( void ) { return GetTFWpnData().m_bCanSoftZoom; }

	// Sound.
	bool PlayEmptySound();

	// Activities.
	virtual void ItemPreFrame( void );
	virtual void ItemBusyFrame( void );
	virtual void ItemPostFrame( void );
	virtual void SoftZoomCheck( void );
	
	// Reloading
	virtual	void			CheckReload( void );	
	
	void			BurstFire( void );
	void			BeginBurstFire( void );

	bool InBurst()
	{
		return m_iShotsDue > 0;
	}
	virtual bool InBarrage();
	
	virtual float	GetBurstTotalTime( void ) { return GetFireRate() * GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nBurstSize; }	
		

	virtual void SetWeaponVisible( bool visible );

	virtual acttable_t *ActivityList( int &iActivityCount );
	static acttable_t m_acttablePrimary[];
	static acttable_t m_acttableSecondary[];
	static acttable_t m_acttableMelee[];
	static acttable_t m_acttableBuilding[];
	static acttable_t m_acttablePDA[];

#ifdef GAME_DLL
	virtual void	AddAssociatedObject( CBaseObject *pObject ) { }
	virtual void	RemoveAssociatedObject( CBaseObject *pObject ) { }
#endif

	// Utility.
	CBasePlayer *GetPlayerOwner() const;
	CTFPlayer *GetTFPlayerOwner() const;

#ifdef CLIENT_DLL
	C_BaseEntity *GetWeaponForEffect();
#endif

	bool CanAttack( void );

	// Raising & Lowering for grenade throws
	bool			WeaponShouldBeLowered( void );
	virtual bool	Ready( void );
	virtual bool	Lower( void );

	virtual void	WeaponIdle( void );

	virtual void	WeaponReset( void );

	// Muzzleflashes
	virtual const char *GetMuzzleFlashEffectName_3rd( void ) { return NULL; }
	virtual const char *GetMuzzleFlashEffectName_1st( void ) { return NULL; }
	virtual const char *GetMuzzleFlashModel( void );
	virtual float	GetMuzzleFlashModelLifetime( void );
	virtual const char *GetMuzzleFlashParticleEffect( void );
	
	virtual float GetWindupTime( void );

	virtual const char	*GetTracerType( void );

	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	virtual bool CanFireCriticalShot( bool bIsHeadshot = false ){ return true; }

// Server specific.
#if !defined( CLIENT_DLL )

	// Spawning.
	virtual void CheckRespawn();
	virtual CBaseEntity* Respawn();
	void Materialize();
	void AttemptToMaterialize();

	// Death.
	void Die( void );
	void SetDieThink( bool bDie );

	// Ammo.
	virtual const Vector& GetBulletSpread();

// Client specific.
#else

	virtual void	ProcessMuzzleFlashEvent( void );
	virtual int		InternalDrawModel( int flags );

	virtual bool	ShouldPredict();
	virtual void	OnDataChanged( DataUpdateType_t type );
	virtual void	OnPreDataChanged( DataUpdateType_t updateType );
	virtual int		CalcOverrideModelIndex( void );
	virtual int		GetWorldModelIndex( void );
	virtual bool	ShouldDrawCrosshair( void );
	virtual void	Redraw( void );

	virtual void	AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual	float	CalcViewmodelBob( void );
	virtual ShadowType_t	ShadowCastType( void );
	virtual int		GetSkin();
	BobState_t		*GetBobState();

	bool OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options );

	// Model muzzleflashes
	CHandle<C_MuzzleFlashModel>		m_hMuzzleFlashModel[2];

#endif
public:
	CNetworkVar( bool,  m_bWindingUp );
	CNetworkVar( float, m_flWindTick );
	CNetworkVar( bool,	m_bSwapFire );
	
	CNetworkVar( int,	m_iSlotOverride );
	CNetworkVar( int,	m_iPositionOverride );
protected:
#ifdef CLIENT_DLL
	virtual void CreateMuzzleFlashEffects( C_BaseEntity *pAttachEnt, int nIndex );
#endif // CLIENT_DLL

	// Reloads.
	void UpdateReloadTimers( bool bStart );
	void SetReloadTimer( float flReloadTime );
	bool ReloadSingly( void );
	void ReloadSinglyPostFrame( void );

protected:

	int				m_iWeaponMode;
	CNetworkVar( int,	m_iReloadMode );
	CTFWeaponInfo	*m_pWeaponInfo;
	bool			m_bInAttack;
	bool			m_bInAttack2;
	bool			m_bCurrentAttackIsCrit;
	bool			m_bNeverStrip;

	CNetworkVar(	bool,	m_bLowered );

	int				m_iAltFireHint;

	int				m_iReloadStartClipAmount;

	float			m_flCritTime;
	float			m_flLastCritCheckTime;
	int				m_iLastCritCheckFrame;
	int				m_iCurrentSeed;
	int				m_iGGLevel;

	char			m_szTracerName[MAX_TRACER_NAME];

	CNetworkVar( bool, m_bResetParity );

#ifdef CLIENT_DLL
	bool m_bOldResetParity;
#endif
protected:
	CNetworkVar( bool, m_bReloadedThroughAnimEvent );

	CNetworkVar( float, m_flNextShotTime );

	CNetworkVar( int, m_iShotsDue );
	CNetworkVar( bool, m_bInBarrage );
	CNetworkVar( int, m_iDamageIncrease );
	CNetworkVar( float, m_flBlastRadiusIncrease );
	CNetworkVar( int, m_iTeamNum );
private:
	CTFWeaponBase( const CTFWeaponBase & );
};

#ifdef GAME_DLL
typedef CHandle<CTFWeaponBase> WeaponHandle;
#endif

#define WEAPON_RANDOM_RANGE 10000

#endif // TF_WEAPONBASE_H
