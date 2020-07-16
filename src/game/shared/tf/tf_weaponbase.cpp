//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//	Weapons.
//
//=============================================================================
#include "cbase.h"
#include "tf_weaponbase.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "eventlist.h"

#ifdef CLIENT_DLL
	#include "tf_viewmodel.h"
	#include "clientmode_tf.h"
	#include "toolframework_client.h"
	// for spy material proxy
	#include "proxyentity.h"
	#include "materialsystem/imaterialvar.h"

	extern CTFWeaponInfo *GetTFWeaponInfo( int iWeapon );
#else
	#include "tf_weapon_builder.h"
#endif

#ifdef CLIENT_DLL
	extern ConVar of_muzzlelight;
	extern ConVar of_beta_muzzleflash;
	extern ConVar fov_softzoom;
	extern ConVar fov_desired;

	ConVar of_autoreload( "of_autoreload", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO, "Automatically reload when not firing." );
	ConVar of_autoswitchweapons("of_autoswitchweapons", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO , "Toggles autoswitching when picking up new weapons.");
#endif

// This was in the CLIENT_DLL section, but then I was unable to set it on a listen server so moved it out here to Shared space.
// To allow players to change their zoom speeds on most weapons (With callbacks & caching for performance)
// Currently DOES AFFECT the re-zooming time, but is does still use m_flNextZoomTime as a minimum (m_flNextZoomTime + zoom in/out time).
void QuickzoomConVarChanged(IConVar *var, const char *pOldValue, float flOldValue); // Callback - should be overriden down below
ConVar cl_quickzoom_in_time("cl_quickzoom_in_time", "0.25", FCVAR_ARCHIVE | FCVAR_USERINFO, "The time it takes to zoom in with 'soft zoom' (secondary attack on most weapons)", true, 0.0f, true, 3.0f, QuickzoomConVarChanged);
ConVar cl_quickzoom_out_time("cl_quickzoom_out_time", "0.1", FCVAR_ARCHIVE | FCVAR_USERINFO, "The time it takes to zoom out with 'soft zoom' (secondary attack on most weapons)", true, 0.0f, true, 3.0f, QuickzoomConVarChanged);
float fConVarQuickZoomInTime = cl_quickzoom_in_time.GetFloat();
float fConVarQuickZoomOutTime = cl_quickzoom_out_time.GetFloat();


ConVar tf_weapon_criticals( "tf_weapon_criticals", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles random crits." );
ConVar of_crit_multiplier( "of_crit_multiplier", "3", FCVAR_NOTIFY | FCVAR_REPLICATED, "How much the crit powerup increases your damage." );
ConVar of_infiniteammo( "of_infiniteammo", "0", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles infinite ammo." );
ConVar sv_reloadsync( "sv_reloadsync", "0", FCVAR_NOTIFY | FCVAR_REPLICATED | FCVAR_CHEAT , "Sync up weapon reloads." );
ConVar of_haste_reload_multiplier("of_haste_reload_multiplier", "0.65", FCVAR_NOTIFY | FCVAR_REPLICATED, "By how much the reload time should be multiplied when in Haste.");
ConVar of_haste_fire_multiplier("of_haste_fire_multiplier", "0.75", FCVAR_NOTIFY | FCVAR_REPLICATED, "By how much the fire rate should be multiplied when in Haste.");
ConVar of_haste_drawtime_multiplier("of_haste_drawtime_multiplier", "0.75", FCVAR_NOTIFY | FCVAR_REPLICATED, "By how much the draw speed should be multiplied when in Haste.");
extern ConVar tf_useparticletracers;
extern ConVar of_multiweapons;
//=============================================================================
//
// Global functions.
//

//-----------------------------------------------------------------------------
// Purpose: Callback for the soft-zoom speed convars. Caches them in variables for efficiency!
//-----------------------------------------------------------------------------

void QuickzoomConVarChanged(IConVar *var, const char *pOldValue, float flOldValue)
{
	fConVarQuickZoomInTime = cl_quickzoom_in_time.GetFloat();
	fConVarQuickZoomOutTime = cl_quickzoom_out_time.GetFloat();
	Log("Updated zoom values successfully!");
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool IsAmmoType( int iAmmoType, const char *pAmmoName )
{
	return GetAmmoDef()->Index( pAmmoName ) == iAmmoType;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void FindHullIntersection( const Vector &vecSrc, trace_t &tr, const Vector &mins, const Vector &maxs, CBaseEntity *pEntity )
{
	int	i, j, k;
	trace_t tmpTrace;
	Vector vecEnd;
	float distance = 1e6f;
	Vector minmaxs[2] = {mins, maxs};
	Vector vecHullEnd = tr.endpos;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tmpTrace );
	if ( tmpTrace.fraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID, pEntity, COLLISION_GROUP_NONE, &tmpTrace );
				if ( tmpTrace.fraction < 1.0 )
				{
					float thisDistance = (tmpTrace.endpos - vecSrc).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

//=============================================================================
//
// TFWeaponBase tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFWeaponBase, DT_TFWeaponBase )

BEGIN_NETWORK_TABLE( CTFWeaponBase, DT_TFWeaponBase )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bLowered ) ),
	RecvPropInt( RECVINFO( m_iReloadMode ) ),
	RecvPropBool( RECVINFO( m_bResetParity ) ), 
	RecvPropBool( RECVINFO( m_bReloadedThroughAnimEvent ) ),
	RecvPropBool( RECVINFO( m_bInBarrage ) ),
	RecvPropBool( RECVINFO( m_bWindingUp ) ),
	RecvPropBool( RECVINFO( m_bSwapFire ) ),
	RecvPropTime( RECVINFO( m_flWindTick ) ),
	RecvPropInt( RECVINFO( m_iDamageIncrease ) ),
	RecvPropInt( RECVINFO( m_iSlotOverride ) ),
	RecvPropInt( RECVINFO( m_iPositionOverride ) ),
	RecvPropInt( RECVINFO( m_iTeamNum ) ),
	RecvPropFloat( RECVINFO( m_flBlastRadiusIncrease ) ),
// Server specific.
#else
	SendPropBool( SENDINFO( m_bLowered ) ),
	SendPropBool( SENDINFO( m_bResetParity ) ),
	SendPropInt( SENDINFO( m_iReloadMode ), 4, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO( m_bReloadedThroughAnimEvent ) ),
	SendPropBool( SENDINFO( m_bInBarrage ) ),
	SendPropBool( SENDINFO( m_bWindingUp ) ),
	SendPropBool( SENDINFO( m_bSwapFire ) ),
	SendPropTime( SENDINFO( m_flWindTick ) ),
	SendPropInt( SENDINFO( m_iDamageIncrease ) ),
	SendPropInt( SENDINFO( m_iSlotOverride ) ),
	SendPropInt( SENDINFO( m_iPositionOverride ) ),
	SendPropInt( SENDINFO( m_iTeamNum ) ),
	SendPropFloat( SENDINFO( m_flBlastRadiusIncrease ) ),
	// World models have no animations so don't send these.
	SendPropExclude( "DT_BaseAnimating", "m_nSequence" ),
	SendPropExclude( "DT_AnimTimeMustBeFirst", "m_flAnimTime" ),
#endif 
#if !defined( CLIENT_DLL )
	SendPropTime( SENDINFO( m_flNextShotTime ) ),
	SendPropInt( SENDINFO( m_iShotsDue ), 9 ),
#else
	RecvPropTime( RECVINFO( m_flNextShotTime ) ),
	RecvPropInt( RECVINFO( m_iShotsDue )),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFWeaponBase )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE ),
	DEFINE_PRED_FIELD( m_bLowered, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iReloadMode, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bReloadedThroughAnimEvent, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bInBarrage, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bWindingUp, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iShotsDue, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flNextShotTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_PRED_FIELD_TOL( m_flWindTick, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),	
	DEFINE_PRED_FIELD( m_bSwapFire, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
#if defined( CLIENT_DLL )
	DEFINE_FIELD( m_iShotsDue, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextShotTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flWindTick, FIELD_FLOAT ),
	DEFINE_FIELD( m_bSwapFire, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iDamageIncrease, FIELD_INTEGER ),
	DEFINE_FIELD( m_flBlastRadiusIncrease, FIELD_FLOAT ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_base, CTFWeaponBase );

// Server specific.
#if !defined( CLIENT_DLL )

BEGIN_DATADESC( CTFWeaponBase )
DEFINE_FIELD( m_flNextShotTime, FIELD_TIME ),
DEFINE_FIELD( m_flWindTick, FIELD_TIME ),
DEFINE_FIELD( m_iShotsDue, FIELD_INTEGER ),
DEFINE_FIELD( m_bSwapFire, FIELD_BOOLEAN ),
DEFINE_FUNCTION( FallThink )
END_DATADESC()

// Client specific
#else
ConVar cl_dynamiccrosshair( "cl_dynamiccrosshair", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
ConVar cl_scalecrosshair( "cl_scalecrosshair", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );

int g_iScopeTextureID = 0;
int g_iScopeDustTextureID = 0;

#endif

//=============================================================================
//
// TFWeaponBase shared functions.
//

// -----------------------------------------------------------------------------
// Purpose: Constructor.
// -----------------------------------------------------------------------------
CTFWeaponBase::CTFWeaponBase()
{
	SetPredictionEligible( true );

	// Nothing collides with these, but they get touch calls.
	AddSolidFlags( FSOLID_TRIGGER );

	// Weapons can fire underwater.
	m_bFiresUnderwater = true;
	m_bAltFiresUnderwater = true;

	// Initialize the weapon modes.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	m_iReloadMode.Set( TF_RELOAD_START );

	m_iAltFireHint = 0;
	m_bInAttack = false;
	m_bInAttack2 = false;
	m_flCritTime = 0;
	m_flLastCritCheckTime = 0;
	m_iLastCritCheckFrame = 0;
	m_bCurrentAttackIsCrit = false;
	m_iCurrentSeed = -1;
	m_iGGLevel = -1;
	m_bNeverStrip = false;

	m_bQuakeRLHack = false;
	
	m_bWindingUp = false;
	
	m_iSlotOverride = -1;
	m_iPositionOverride = -1;
	
	m_iTeamNum = TEAM_UNASSIGNED;
}

CTFWeaponBase::~CTFWeaponBase()
{
#ifdef GAME_DLL
	CTFPlayer *pTFOwner = ToTFPlayer( GetOwner() );
	if ( !pTFOwner )
		return;
	WeaponHandle hHandle;
	hHandle = this;	
	if ( pTFOwner->m_hWeaponInSlot && pTFOwner->m_hWeaponInSlot[GetSlot()][GetPosition()] == hHandle )
		pTFOwner->m_hWeaponInSlot[GetSlot()][GetPosition()] = NULL;
#endif
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::Spawn()
{
	// Base class spawn.
	BaseClass::Spawn();

	// Set this here to allow players to shoot dropped weapons.
	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	// Get the weapon information.
	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( GetClassname() );
	Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
	CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	Assert( pWeaponInfo && "Failed to get CTFWeaponInfo in weapon spawn" );
	m_pWeaponInfo = pWeaponInfo;

	if ( GetPlayerOwner() )
	{
		ChangeTeam( GetPlayerOwner()->GetTeamNumber() );
	}

#ifdef GAME_DLL
	// Move it up a little bit, otherwise it'll be at the guy's feet, and its sound origin 
	// will be in the ground so its EmitSound calls won't do anything.
	Vector vecOrigin = GetAbsOrigin();
	SetAbsOrigin( Vector( vecOrigin.x, vecOrigin.y, vecOrigin.z + 5.0f ) );
#endif
	m_bNeverStrip = GetTFWpnData().m_bNeverStrip;
	m_szTracerName[0] = '\0';
}

//-----------------------------------------------------------------------------
// Purpose: Become a child of the owner (MOVETYPE_FOLLOW)
//			disables collisions, touch functions, thinking
// Input  : *pOwner - new owner/operator
//-----------------------------------------------------------------------------
void CTFWeaponBase::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
#ifdef GAME_DLL
	if( dynamic_cast<CTFWeaponBuilder*>(this) ) // Builders crash the game, and those are handled seperatley anyways
		return;
	CTFPlayer *pTFOwner = ToTFPlayer( pOwner );
	if ( !pTFOwner )
		return;
	WeaponHandle hHandle;
	hHandle = this;	
	if ( hHandle && pTFOwner->m_hWeaponInSlot && 
		pTFOwner->m_hWeaponInSlot[GetSlot()][GetPosition()] && 
		pTFOwner->m_hWeaponInSlot[GetSlot()][GetPosition()] 
		!= hHandle )
	{
		pTFOwner->DropWeapon( pTFOwner->m_hWeaponInSlot[GetSlot()][GetPosition()].Get(),
		true, false, 
		pTFOwner->m_hWeaponInSlot[GetSlot()][GetPosition()]->m_iClip1,
		pTFOwner->m_hWeaponInSlot[GetSlot()][GetPosition()]->m_iReserveAmmo );
		UTIL_Remove( pTFOwner->m_hWeaponInSlot[GetSlot()][GetPosition()] );
	}
	pTFOwner->m_hWeaponInSlot[GetSlot()][GetPosition()] = hHandle;
	
	if ( GetTFWpnData().m_bAlwaysDrop )
		pTFOwner->m_hSuperWeapons.AddToTail( hHandle );
#endif
}

int unequipable[10] =
{
	TF_WEAPON_BUILDER,
	TF_WEAPON_INVIS
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::DontAutoEquip( void ) const
{
	for ( int i = 0; i < ARRAYSIZE(unequipable); i++ )
	{
		if( GetWeaponID() == unequipable[i] )
			return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::LoadsManualy( void ) const
{
	return GetTFWpnData().m_bLoadsManualy;
	
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBase::GetSlot( void )
{
	if( GetSlotOverride() > -1 )
		return GetSlotOverride();
	
	if ( TFGameRules() && TFGameRules()->UsesDMBuckets() && !TFGameRules()->IsGGGamemode()  )
		return GetWpnData().iSlotDM;
	
	CTFPlayer *pOwner = ToTFPlayer ( GetOwner() );
	if ( !pOwner )
		return GetWpnData().iSlot;
	
	int ClassIndex = pOwner->GetPlayerClass()->GetClassIndex();
	
	if ( GetTFWpnData().m_iClassSlot[ ClassIndex ] != -1 )
		return GetTFWpnData().m_iClassSlot[ ClassIndex ];
	else
		return GetWpnData().iSlot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBase::GetPosition( void )
{
	if( GetPositionOverride() > -1 )
		return GetPositionOverride();
	
	if ( TFGameRules() && TFGameRules()->UsesDMBuckets() && !TFGameRules()->IsGGGamemode() )
		return GetWpnData().iPositionDM;	
	return GetWpnData().iPosition;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBase::GetDamage( void ) const
{
	int iDamage = ( TFGameRules()->IsMutator( NO_MUTATOR ) || TFGameRules()->GetMutator() > INSTAGIB_NO_MELEE ) ? m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nDamage : m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_nInstagibDamage;
	iDamage += m_iDamageIncrease;
	return iDamage;
	
}

//-----------------------------------------------------------------------------
// Purpose: Accessor for damage, so sniper etc can modify damage
//-----------------------------------------------------------------------------
float CTFWeaponBase::GetDamageRadius( void ) const
{
	return GetTFWpnData().m_flDamageRadius + m_flBlastRadiusIncrease;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::CanSecondaryAttack( void ) const
{
		return !GetTFWpnData().m_bDisableSecondaryAttack;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::CanDropManualy( void ) const
{
	if ( !GetTFWpnData().m_bAllowDrop && GetMaxReserveAmmo() <= 0 )
		return false;
	
	return true;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::FallInit( void )
{

}

#ifdef CLIENT_DLL
	#define SHARED_VarArgs VarArgs
#else
	#define SHARED_VarArgs UTIL_VarArgs
#endif
//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Precache()
{
	BaseClass::Precache();

	if ( GetMuzzleFlashModel() )
	{
		PrecacheModel( GetMuzzleFlashModel() );
	}

	const CTFWeaponInfo *pTFInfo = &GetTFWpnData();

	if ( pTFInfo->m_szExplosionSound && pTFInfo->m_szExplosionSound[0] )
	{
		CBaseEntity::PrecacheScriptSound( pTFInfo->m_szExplosionSound );
	}

	if ( pTFInfo->m_szBrassModel[0] )
	{
		PrecacheModel( pTFInfo->m_szBrassModel );
	}

	if (pTFInfo->m_szMagModel[0])
	{
		PrecacheModel( pTFInfo->m_szMagModel );
	}

	if ( pTFInfo->m_szMuzzleFlashParticleEffect && pTFInfo->m_szMuzzleFlashParticleEffect[0] )
	{
		if(pTFInfo->m_bTeamColorMuzzleFlash)
		{
			DevMsg("%s\n%s\n%s\n", pTFInfo->m_szMuzzleFlashParticleEffectRed, pTFInfo->m_szMuzzleFlashParticleEffectBlue, pTFInfo->m_szMuzzleFlashParticleEffectDM);
			PrecacheParticleSystem( pTFInfo->m_szMuzzleFlashParticleEffectRed );
			PrecacheParticleSystem( pTFInfo->m_szMuzzleFlashParticleEffectBlue );
			PrecacheParticleSystem( pTFInfo->m_szMuzzleFlashParticleEffectDM );
		}
		else
			PrecacheParticleSystem( pTFInfo->m_szMuzzleFlashParticleEffect );
	}	

	if ( pTFInfo->m_szExplosionEffect && pTFInfo->m_szExplosionEffect[0] )
	{
		if( pTFInfo->m_bTeamExplosion )
		{
			PrecacheParticleSystem( 
			SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionEffect, "_red") );
			PrecacheParticleSystem( 
			SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionEffect, "_blue") );
			PrecacheParticleSystem( 
			SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionEffect, "_dm") );
		}
		else
			PrecacheParticleSystem( pTFInfo->m_szExplosionEffect );
	}

	if ( pTFInfo->m_szExplosionPlayerEffect && pTFInfo->m_szExplosionPlayerEffect[0] )
	{
		if( pTFInfo->m_bTeamExplosion )
		{
			PrecacheParticleSystem( 
			SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionPlayerEffect, "_red") );
			PrecacheParticleSystem( 
			SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionPlayerEffect, "_blue") );
			PrecacheParticleSystem( 
			SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionPlayerEffect, "_dm") );
		}
		else
			PrecacheParticleSystem( pTFInfo->m_szExplosionPlayerEffect );
	}

	if ( pTFInfo->m_szExplosionWaterEffect && pTFInfo->m_szExplosionWaterEffect[0] )
	{
		if( pTFInfo->m_bTeamExplosion )
		{
			PrecacheParticleSystem( 
			SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionWaterEffect, "_red") );
			PrecacheParticleSystem( 
			SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionWaterEffect, "_blue") );
			PrecacheParticleSystem( 
			SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionWaterEffect, "_dm") );
		}
		else
			PrecacheParticleSystem( pTFInfo->m_szExplosionWaterEffect );
	}
	
	if( pTFInfo->m_bDropBomblets )
	{
		if ( pTFInfo->m_szExplosionEffectBomblets && pTFInfo->m_szExplosionEffectBomblets[0] )
		{
			if( pTFInfo->m_bTeamExplosion )
			{
				PrecacheParticleSystem( 
				SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionEffectBomblets, "_red") );
				PrecacheParticleSystem( 
				SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionEffectBomblets, "_blue") );
				PrecacheParticleSystem( 
				SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionEffectBomblets, "_dm") );
			}
			else
				PrecacheParticleSystem( pTFInfo->m_szExplosionEffectBomblets );
		}

		if ( pTFInfo->m_szExplosionPlayerEffectBomblets && pTFInfo->m_szExplosionPlayerEffectBomblets[0] )
		{
			if( pTFInfo->m_bTeamExplosion )
			{
				PrecacheParticleSystem( 
				SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionPlayerEffectBomblets, "_red") );
				PrecacheParticleSystem( 
				SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionPlayerEffectBomblets, "_blue") );
				PrecacheParticleSystem( 
				SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionPlayerEffectBomblets, "_dm") );
			}
			else
				PrecacheParticleSystem( pTFInfo->m_szExplosionPlayerEffectBomblets );
		}

		if ( pTFInfo->m_szExplosionWaterEffectBomblets && pTFInfo->m_szExplosionWaterEffectBomblets[0] )
		{
			if( pTFInfo->m_bTeamExplosion )
			{
				PrecacheParticleSystem( 
				SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionWaterEffectBomblets, "_red") );
				PrecacheParticleSystem( 
				SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionWaterEffectBomblets, "_blue") );
				PrecacheParticleSystem( 
				SHARED_VarArgs("%s%s", pTFInfo->m_szExplosionWaterEffectBomblets, "_dm") );
			}
			else
				PrecacheParticleSystem( pTFInfo->m_szExplosionWaterEffectBomblets );
		}		
	}

	if ( pTFInfo->m_szTracerEffect && pTFInfo->m_szTracerEffect[0] )
	{
		char pTracerEffect[128];
		char pTracerEffectCrit[128];

		Q_snprintf( pTracerEffect, sizeof(pTracerEffect), "%s_red", pTFInfo->m_szTracerEffect );
		Q_snprintf( pTracerEffectCrit, sizeof(pTracerEffectCrit), "%s_red_crit", pTFInfo->m_szTracerEffect );
		PrecacheParticleSystem( pTracerEffect );
		PrecacheParticleSystem( pTracerEffectCrit );

		Q_snprintf( pTracerEffect, sizeof(pTracerEffect), "%s_blue", pTFInfo->m_szTracerEffect );
		Q_snprintf( pTracerEffectCrit, sizeof(pTracerEffectCrit), "%s_blue_crit", pTFInfo->m_szTracerEffect );
		PrecacheParticleSystem( pTracerEffect );
		PrecacheParticleSystem( pTracerEffectCrit );
		
		Q_snprintf( pTracerEffect, sizeof(pTracerEffect), "%s_dm", pTFInfo->m_szTracerEffect );
		Q_snprintf( pTracerEffectCrit, sizeof(pTracerEffectCrit), "%s_dm_crit", pTFInfo->m_szTracerEffect );
		PrecacheParticleSystem( pTracerEffect );
		PrecacheParticleSystem( pTracerEffectCrit );		
	}

	if ( pTFInfo->szScoutViewModel && pTFInfo->szScoutViewModel[0] )
	{
		PrecacheModel( pTFInfo->szScoutViewModel );
	}
	if ( pTFInfo->szSoldierViewModel && pTFInfo->szSoldierViewModel[0] )
	{
		PrecacheModel( pTFInfo->szSoldierViewModel );
	}
	if ( pTFInfo->szPyroViewModel && pTFInfo->szPyroViewModel[0] )
	{
		PrecacheModel( pTFInfo->szPyroViewModel );
	}
	if ( pTFInfo->szDemomanViewModel && pTFInfo->szDemomanViewModel[0] )
	{
		PrecacheModel( pTFInfo->szDemomanViewModel );
	}
	if ( pTFInfo->szHeavyViewModel && pTFInfo->szHeavyViewModel[0] )
	{
		PrecacheModel( pTFInfo->szHeavyViewModel );
	}
	if ( pTFInfo->szEngineerViewModel && pTFInfo->szEngineerViewModel[0] )
	{
		PrecacheModel( pTFInfo->szEngineerViewModel );
	}
	if ( pTFInfo->szMedicViewModel && pTFInfo->szMedicViewModel[0] )
	{
		PrecacheModel( pTFInfo->szMedicViewModel );
	}
	if ( pTFInfo->szSniperViewModel && pTFInfo->szSniperViewModel[0] )
	{
		PrecacheModel( pTFInfo->szSniperViewModel );
	}
	if ( pTFInfo->szSpyViewModel && pTFInfo->szSpyViewModel[0] )
	{
		PrecacheModel( pTFInfo->szSpyViewModel );
	}
	if ( pTFInfo->szMercenaryViewModel && pTFInfo->szMercenaryViewModel[0] )
	{
		PrecacheModel( pTFInfo->szMercenaryViewModel );
	}
	if ( pTFInfo->szCivilianViewModel && pTFInfo->szCivilianViewModel[0] )
	{
		PrecacheModel( pTFInfo->szCivilianViewModel );
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const CTFWeaponInfo &CTFWeaponBase::GetTFWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CTFWeaponInfo *pTFInfo = dynamic_cast< const CTFWeaponInfo* >( pWeaponInfo );
	Assert( pTFInfo );
	return *pTFInfo;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
int CTFWeaponBase::GetWeaponID( void ) const
{
	Assert( false ); 
	return TF_WEAPON_NONE; 
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::IsWeapon( int iWeapon ) const
{ 
	return GetWeaponID() == iWeapon; 
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBase::GetViewModel( int iViewModel ) const
{
	if ( GetPlayerOwner() == NULL )
	{
		return BaseClass::GetViewModel();
	}

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	bool bScout = pPlayer->GetPlayerClass()->IsClass( TF_CLASS_SCOUT );
	bool bSoldier = pPlayer->GetPlayerClass()->IsClass( TF_CLASS_SOLDIER );
	bool bPyro = pPlayer->GetPlayerClass()->IsClass( TF_CLASS_PYRO );
	bool bDemo = pPlayer->GetPlayerClass()->IsClass( TF_CLASS_DEMOMAN );
	bool bHeavy = pPlayer->GetPlayerClass()->IsClass( TF_CLASS_HEAVYWEAPONS );
	bool bEngi = pPlayer->GetPlayerClass()->IsClass( TF_CLASS_ENGINEER );
	bool bMedic = pPlayer->GetPlayerClass()->IsClass( TF_CLASS_MEDIC );
	bool bSniper = pPlayer->GetPlayerClass()->IsClass( TF_CLASS_SNIPER );
	bool bSpy = pPlayer->GetPlayerClass()->IsClass( TF_CLASS_SPY );
	bool bCivilian = pPlayer->GetPlayerClass()->IsClass( TF_CLASS_CIVILIAN );
	bool bMercenary = pPlayer->GetPlayerClass()->IsClass( TF_CLASS_MERCENARY );
	bool bJuggernaut = pPlayer->GetPlayerClass()->IsClass( TF_CLASS_JUGGERNAUT );
	
	if ( bScout && GetTFWpnData().szScoutViewModel[0] != 0 )
		return GetTFWpnData().szScoutViewModel;
	if ( bSoldier && GetTFWpnData().szSoldierViewModel[0] != 0 )
		return GetTFWpnData().szSoldierViewModel;
	if ( bPyro && GetTFWpnData().szPyroViewModel[0] != 0 )
		return GetTFWpnData().szPyroViewModel;	
	if ( bDemo && GetTFWpnData().szDemomanViewModel[0] != 0 )
		return GetTFWpnData().szDemomanViewModel;		
	if ( bHeavy && GetTFWpnData().szHeavyViewModel[0] != 0  )
		return GetTFWpnData().szHeavyViewModel;	
	if ( bEngi && GetTFWpnData().szEngineerViewModel[0] != 0  )
		return GetTFWpnData().szEngineerViewModel;	
	if ( bMedic && GetTFWpnData().szMedicViewModel[0] != 0 )
		return GetTFWpnData().szMedicViewModel;	
	if ( bSniper && GetTFWpnData().szSniperViewModel[0] != 0 )
		return GetTFWpnData().szSniperViewModel;
	if ( bSpy && GetTFWpnData().szSpyViewModel[0] != 0  )
		return GetTFWpnData().szSpyViewModel;
	if ( bCivilian && GetTFWpnData().szCivilianViewModel[0] != 0  )
		return GetTFWpnData().szCivilianViewModel;
	if ( bMercenary && GetTFWpnData().szMercenaryViewModel[0] != 0 )
		return GetTFWpnData().szMercenaryViewModel;
	if ( bJuggernaut && GetTFWpnData().szJuggernautViewModel[0] != 0 )
		return GetTFWpnData().szJuggernautViewModel;
	return GetTFWpnData().szViewModel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFWeaponBase::GetDefaultClip1( void ) const
{
	int DefClip = GetWpnData().iDefaultClip1;
	if ( LoadsManualy() )
		DefClip = 0;
	return DefClip;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Drop( const Vector &vecVelocity )
{
#ifdef GAME_DLL
	if ( m_iAltFireHint )
	{
		CBasePlayer *pPlayer = GetPlayerOwner();
		if ( pPlayer )
		{
			pPlayer->StopHintTimer( m_iAltFireHint );
		}
	}
#endif

	BaseClass::Drop( vecVelocity );
}

void CTFWeaponBase::Detach()
{
#ifdef GAME_DLL
	CTFPlayer *pTFOwner = ToTFPlayer( GetOwner() );
	if ( !pTFOwner )
		return;
	WeaponHandle hHandle;
	hHandle = this;	
	if ( pTFOwner->m_hWeaponInSlot && pTFOwner->m_hWeaponInSlot[GetSlot()][GetPosition()] == hHandle )
		pTFOwner->m_hWeaponInSlot[GetSlot()][GetPosition()] = NULL;
	
	pTFOwner->m_hSuperWeapons.FindAndRemove(hHandle);
#endif
}

void CTFWeaponBase::PlayWeaponShootSound(void)
{
	if (IsCurrentAttackACrit())
	{
		WeaponSound(BURST);
	}
	else
	{
		WeaponSound(SINGLE);
	}
}

int CTFWeaponBase::IsCurrentAttackACrit()
{
	int nCritMod = m_bCurrentAttackIsCrit;
	if ( nCritMod )
	{
		return m_bCurrentAttackIsCrit;
	}
	else
	{
		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
		if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_CRIT_POWERUP ) )
			return 2;
		else
			return false;
	}
	return m_bCurrentAttackIsCrit;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if( !CanHolster() )
		return false;
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	
#ifdef GAME_DLL
	if ( m_iAltFireHint )
	{
		if ( pOwner )
		{
			pOwner->StopHintTimer( m_iAltFireHint );
		}
	}
#else
	if ( m_hMuzzleFlashModel[1] ) 
		m_hMuzzleFlashModel[1]->Release();
	if ( m_hMuzzleFlashModel[0] ) 
		m_hMuzzleFlashModel[0]->Release();
	
	if( pOwner && pOwner->GetViewModel( m_nViewModelIndex ) )
		pOwner->GetViewModel( m_nViewModelIndex )->ParticleProp()->StopEmission();
	
	if( pOwner )
		pOwner->m_Shared.UpdateCritParticle();
#endif

	// Negates the effects of long reloads not letting you fire after you've re-drawn your weapon
	if ( m_bInReload )
	{
		// Reset next player attack time (weapon independent).
		pOwner->m_flNextAttack = gpGlobals->curtime;

		// Reset next weapon attack times (based on reloading).
		m_flNextPrimaryAttack = gpGlobals->curtime;
		m_flNextSecondaryAttack = gpGlobals->curtime;
		
		// Reset the idle time
		SetWeaponIdleTime( gpGlobals->curtime );
	}

	AbortReload();
	
	if ( pOwner && GetTFWpnData().m_bDropOnNoAmmo && m_iClip1 <= 0 && m_iReserveAmmo<= 0 )
	{
#ifdef GAME_DLL 
		pOwner->DropWeapon( this, true, true, 0, 0 );
		UTIL_Remove ( this );
#endif
	}	
	
	return BaseClass::Holster( pSwitchingTo );
}

bool CTFWeaponBase::CanHolster( void ) const
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	if( !pPlayer )
		return false;
	
	if ( ( m_iShotsDue > 0 || m_bInBarrage || m_bWindingUp ) && !pPlayer->m_Shared.InCond( TF_COND_BERSERK )  )
		return false;

	if ( pPlayer->m_Shared.InCond( TF_COND_JAUGGERNAUGHT ) )
		return false;

	return BaseClass::CanHolster();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Deploy( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
#ifdef GAME_DLL
	if ( m_iAltFireHint )
	{
		if ( pPlayer )
		{
			pPlayer->StartHintTimer( m_iAltFireHint );
		}
	}
#else
	if ( pPlayer )
	{
		pPlayer->m_Shared.UpdateCritParticle();
	}
#endif

	float flOriginalPrimaryAttack = m_flNextPrimaryAttack;
	float flOriginalSecondaryAttack = m_flNextSecondaryAttack;

	bool bDeploy = BaseClass::Deploy();

	if ( bDeploy )
	{
		// Overrides the anim length for calculating ready time.
		// Don't override primary attacks that are already further out than this. This prevents
		// people exploiting weapon switches to allow weapons to fire faster.
		float flDeployTime = 0.67;
		if( pPlayer && pPlayer->m_Shared.InCond( TF_COND_HASTE ) )
		{
			flDeployTime *= of_haste_drawtime_multiplier.GetFloat();
			CBaseViewModel *vm = pPlayer->GetViewModel( m_nViewModelIndex );
			if ( vm )
			{
				vm->SetPlaybackRate( 1.0f / of_haste_drawtime_multiplier.GetFloat() );
			}
		}
		m_flNextPrimaryAttack = max( flOriginalPrimaryAttack, gpGlobals->curtime + flDeployTime );
		m_flNextSecondaryAttack = max( flOriginalSecondaryAttack, gpGlobals->curtime + flDeployTime );
		
		if (!pPlayer)
			return false;

		pPlayer->SetNextAttack( m_flNextPrimaryAttack );
		if( !CanSoftZoom() )
			pPlayer->SetFOV( pPlayer, 0, 0.1f );
	}
	return bDeploy;
}

void CTFWeaponBase::BurstFire( void )
{
	PrimaryAttack();
	PlayWeaponShootSound();
	if ( FiresInBursts() )
	m_iShotsDue--;
	if ( FiresInBursts() )
		m_flNextShotTime = gpGlobals->curtime + GetFireRate();
	else if ( LoadsManualy() )
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CTFWeaponBase::BeginBurstFire(void)
{
	if ( InBurst() )
		return;

	m_iShotsDue = GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nBurstSize;

	m_flNextPrimaryAttack = gpGlobals->curtime + GetBurstTotalTime() + GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flBurstFireDelay;
}

bool CTFWeaponBase::FiresInBursts( void )
{
	return GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flBurstFireDelay > 0;
}

bool CTFWeaponBase::InBarrage(void)
{
	return m_bInBarrage;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : 
//-----------------------------------------------------------------------------
void CTFWeaponBase::PrimaryAttack( void )
{
	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	if ( !CanAttack() )
		return;

	BaseClass::PrimaryAttack();

	if ( !InBarrage() && !InBurst() )
		m_bWindingUp = false;
	
	if ( m_bReloadsSingly )
	{
		m_iReloadMode.Set( TF_RELOAD_START );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CTFWeaponBase::SecondaryAttack( void )
{
	if ( !CanSecondaryAttack() )
		return;
	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if( !pOwner )
		return;

	// Don't hook secondary for now.
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Most calls use the prediction seed
//-----------------------------------------------------------------------------
void CTFWeaponBase::CalcIsAttackCritical( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( gpGlobals->framecount == m_iLastCritCheckFrame )
		return;
	m_iLastCritCheckFrame = gpGlobals->framecount;

	// if base entity seed has changed since last calculation, reseed with new seed
	int iSeed = CBaseEntity::GetPredictionRandomSeed();
	if ( iSeed != m_iCurrentSeed )
	{
		m_iCurrentSeed = iSeed;
		RandomSeed( m_iCurrentSeed );
	}
	
	if ( pPlayer->m_Shared.InCond( TF_COND_CRITBOOSTED ) )
	{
		m_bCurrentAttackIsCrit = true;
	}
	else
	{
		// call the weapon-specific helper method
		m_bCurrentAttackIsCrit = CalcIsAttackCriticalHelper();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Weapon-specific helper method to calculate if attack is crit
//-----------------------------------------------------------------------------
bool CTFWeaponBase::CalcIsAttackCriticalHelper()
{
	if ( !tf_weapon_criticals.GetBool() )
		return false;

	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	if ( !CanFireCriticalShot() )
		return false;

	float flPlayerCritMult = pPlayer->GetCritMult();

	if ( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_bUseRapidFireCrits )
	{
		if ( m_flCritTime > gpGlobals->curtime )
			return true;
		// only perform one crit check per second for rapid fire weapons
		if ( gpGlobals->curtime < m_flLastCritCheckTime + 1.0f )
			return false;
		m_flLastCritCheckTime = gpGlobals->curtime;

		// get the total crit chance (ratio of total shots fired we want to be crits)
		float flTotalCritChance = clamp( TF_DAMAGE_CRIT_CHANCE_RAPID * flPlayerCritMult, 0.01f, 0.99f );
		// get the fixed amount of time that we start firing crit shots for	
		float flCritDuration = TF_DAMAGE_CRIT_DURATION_RAPID;
		// calculate the amount of time, on average, that we want to NOT fire crit shots for in order to achive the total crit chance we want
		float flNonCritDuration = ( flCritDuration / flTotalCritChance ) - flCritDuration;
		// calculate the chance per second of non-crit fire that we should transition into critting such that on average we achieve the total crit chance we want
		float flStartCritChance = 1 / flNonCritDuration;

		// see if we should start firing crit shots
		int iRandom = RandomInt( 0, WEAPON_RANDOM_RANGE-1 );
		if ( iRandom <= flStartCritChance * WEAPON_RANDOM_RANGE )
		{
			m_flCritTime = gpGlobals->curtime + TF_DAMAGE_CRIT_DURATION_RAPID;
			return true;
		}
		
		return false;
	}
	else
	{
		// single-shot weapon, just use random pct per shot
		return ( RandomInt( 0.0, WEAPON_RANDOM_RANGE-1 ) < ( TF_DAMAGE_CRIT_CHANCE * flPlayerCritMult ) * WEAPON_RANDOM_RANGE );
	}
}

float CTFWeaponBase::GetFireRate()
{
	float flFireRate = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeFireDelay;
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( pOwner )
	{
		if ( pOwner->m_Shared.InCond( TF_COND_HASTE ) )
			flFireRate *= of_haste_fire_multiplier.GetFloat();
	}
	return flFireRate;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Reload( void )
{
	// If we're not already reloading, check to see if we have ammo to reload and check to see if we are max ammo.
	if ( m_iReloadMode == TF_RELOAD_START ) 
	{
		// If I don't have any spare ammo, I can't reload
		if ( ReserveAmmo() <= 0 )
			return false;

		if ( Clip1() >= GetMaxClip1() )
			return false;
	}
	
	// Reload one object at a time.
	if ( m_bReloadsSingly )
		return ReloadSingly();

	// Normal reload.
	DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	
	m_iDamageIncrease = 0;
	m_flBlastRadiusIncrease = 0;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::AbortReload( void )
{
	BaseClass::AbortReload();
	
	m_iReloadMode.Set( TF_RELOAD_START );
}

bool CTFWeaponBase::ReloadOrSwitchWeapons( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
	Assert( pOwner );

	m_bFireOnEmpty = false;

	// If we don't have any ammo, switch to the next best weapon
	if ( CanHolster() && !HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime && m_flNextSecondaryAttack < gpGlobals->curtime )
	{
		// weapon isn't useable, switch.
		if ( ( (GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) == false ) && ( g_pGameRules->SwitchToNextBestWeapon( pOwner, this ) ) )
		{
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.3;
			m_flNextSecondaryAttack = gpGlobals->curtime + 0.3;
			return true;
		}
	}
	else
	{
		// Weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
		if ( UsesClipsForAmmo1() && !LoadsManualy() && 
			 (GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) == false && 
			 m_flNextPrimaryAttack < gpGlobals->curtime && 
			 m_flNextSecondaryAttack < gpGlobals->curtime )
		{
			if ( pPlayer && pPlayer->ShouldAutoReload() )
			{
				if ( m_iClip1 < GetMaxClip1() )
				{
					if ( Reload() )
					return true;
				}
			}
			else if ( m_iClip1 == 0 )
			{
					if ( Reload() )
					return true;				
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTFWeaponBase::ReloadSingly( void )
{
	// Don't reload.
	if ( m_flNextPrimaryAttack > gpGlobals->curtime )
		return false;

	// Get the current player.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return false;

	// check to see if we're ready to reload
	switch ( m_iReloadMode )
	{
	case TF_RELOAD_START:
		{
			// Play weapon and player animations.
			if ( SendWeaponAnim( ACT_RELOAD_START ) )
			{
				if ( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeReloadStart )
					SetReloadTimer( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeReloadStart );
				else
					SetReloadTimer( SequenceDuration() );
			}
			else
			{
				// Update the reload timers with script values.
				UpdateReloadTimers( true );
			}

			// Next reload the shells.
			m_iReloadMode.Set( TF_RELOADING );

			m_iReloadStartClipAmount = Clip1();

			return true;
		}
	case TF_RELOADING:
		{
			// Did we finish the reload start?  Now we can reload a rocket.
			if ( m_flTimeWeaponIdle > gpGlobals->curtime )
				return false;

			// Play weapon reload animations and sound.
			if ( Clip1() == m_iReloadStartClipAmount )
			{
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
			}
			else
			{
				pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_LOOP );
			}

			m_bReloadedThroughAnimEvent = false;

			if ( SendWeaponAnim( ACT_VM_RELOAD ) )
			{
				if ( sv_reloadsync.GetFloat() <= 0 )
				{
					SetReloadTimer( GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload );
				}
				else
				{
					SetReloadTimer(sv_reloadsync.GetFloat());
				}
			}
			else
			{
				// Update the reload timers.
				UpdateReloadTimers( false );
			}

#ifdef GAME_DLL
			if (m_bQuakeRLHack)
				WeaponSound( RELOAD_NPC );
			else
				WeaponSound( RELOAD );
#endif

			// Next continue to reload shells?
			m_iReloadMode.Set( TF_RELOADING_CONTINUE );

			return true;
		}
	case TF_RELOADING_CONTINUE:
		{
			// Did we finish the reload start?  Now we can finish reloading the rocket.
			if ( m_flTimeWeaponIdle > gpGlobals->curtime )
				return false;

			m_iDamageIncrease = 0;
			m_flBlastRadiusIncrease = 0;

			// If we have ammo, remove ammo and add it to clip
			if ( ReserveAmmo() > 0 && !m_bReloadedThroughAnimEvent )
			{
				m_iClip1 = min( ( m_iClip1 + 1 ), GetMaxClip1() );
				if ( of_infiniteammo.GetBool() != 1 ) 
					m_iReserveAmmo -= 1;
			}

			if ( Clip1() == GetMaxClip1() || ReserveAmmo() <= 0 )
			{
				m_iReloadMode.Set( TF_RELOAD_FINISH );
			}
			else
			{
				m_iReloadMode.Set( TF_RELOADING );
			}

			return true;
		}

	case TF_RELOAD_FINISH:
	default:
		{
			if ( SendWeaponAnim( ACT_RELOAD_FINISH ) )
			{
				// We're done, allow primary attack as soon as we like
				//SetReloadTimer( SequenceDuration() );
			}

			pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_END );

			m_iReloadMode.Set( TF_RELOAD_START );
			
			return true;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	if ( (pEvent->type & AE_TYPE_NEWEVENTSYSTEM) /*&& (pEvent->type & AE_TYPE_SERVER)*/ )
	{
		if ( pEvent->event == AE_WPN_INCREMENTAMMO )
		{
			if ( m_iReserveAmmo > 0 && !m_bReloadedThroughAnimEvent )
			{
				m_iClip1 = min( ( m_iClip1 + 1 ), GetMaxClip1() );
				if ( of_infiniteammo.GetBool () != 1 )
					m_iReserveAmmo -= 1;
					
			}

			m_bReloadedThroughAnimEvent = true;
			return;
		}
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::DefaultReload( int iClipSize1, int iClipSize2, int iActivity )
{
	// The the owning local player.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return false;

	// Setup and check for reload.
	bool bReloadPrimary = false;
	bool bReloadSecondary = false;

	// If you don't have clips, then don't try to reload them.
	if ( UsesClipsForAmmo1() )
	{
		// need to reload primary clip?
		int primary	= min( iClipSize1 - m_iClip1, ReserveAmmo() );
		if ( primary != 0 )
		{
			bReloadPrimary = true;
		}
	}

	if ( UsesClipsForAmmo2() )
	{
		// need to reload secondary clip?
		int secondary = min( iClipSize2 - m_iClip2, pPlayer->GetAmmoCount( m_iSecondaryAmmoType ) );
		if ( secondary != 0 )
		{
			bReloadSecondary = true;
		}
	}

	// We didn't reload.
	if ( !( bReloadPrimary || bReloadSecondary )  )
		return false;

#ifdef GAME_DLL
		WeaponSound( RELOAD );
#endif

	// Play the player's reload animation
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );

	float flReloadTime;
	// First, see if we have a reload animation
	if ( SendWeaponAnim( iActivity ) )
	{
		flReloadTime = GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload;  
		if ( bReloadSecondary )
		{
			flReloadTime = GetTFWpnData().m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeReload;  
		}
		if ( sv_reloadsync.GetFloat() >0 )
			flReloadTime = sv_reloadsync.GetFloat();
	}
	else
	{
		// No reload animation. Use the script time.
		if ( sv_reloadsync.GetFloat() > 0 )
			flReloadTime = sv_reloadsync.GetFloat();
		else
			flReloadTime = GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload;  
		
		if ( bReloadSecondary )
		{
			flReloadTime = GetTFWpnData().m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeReload;  
		}
	}

	SetReloadTimer( flReloadTime );

	m_bInReload = true;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::UpdateReloadTimers( bool bStart )
{
	// Starting a reload?
	if ( bStart )
	{
		// Get the reload start time.
		SetReloadTimer( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeReloadStart );
	}
	// In reload.
	else
	{
		if (sv_reloadsync.GetFloat() > 0)
			SetReloadTimer( sv_reloadsync.GetFloat() );
		else
			SetReloadTimer( m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeReload );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::SetReloadTimer( float flReloadTime )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	float flSpeedMultiplier = 1.0f;
	if ( pPlayer->m_Shared.InCond( TF_COND_HASTE ) )
		flSpeedMultiplier = of_haste_reload_multiplier.GetFloat();
	
	CBaseViewModel *vm = pPlayer->GetViewModel( m_nViewModelIndex );
	if ( vm )
	{
		vm->SetPlaybackRate( 1.0f / flSpeedMultiplier );
	}
	
	float flTime = gpGlobals->curtime + ( flReloadTime * flSpeedMultiplier );

	// Set next player attack time (weapon independent).
	pPlayer->m_flNextAttack = flTime;

	// Set next weapon attack times (based on reloading).
	m_flNextPrimaryAttack = flTime;
	m_flNextSecondaryAttack = flTime;

	// Don't push out secondary attack, because our secondary fire
	// systems are all separate from primary fire (sniper zooming, demoman pipebomb detonating, etc)
	//m_flNextSecondaryAttack = flTime;

	// Set next idle time (based on reloading).
	if ( !m_bReloadsSingly )
		SetWeaponIdleTime( flTime - flReloadTime + SequenceDuration() );
	else
		SetWeaponIdleTime( flTime );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::PlayEmptySound()
{
	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();

	// TFTODO: Add default empty sound here!
//	EmitSound( filter, entindex(), "Default.ClipEmpty_Rifle" );

	return false;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::SendReloadEvents()
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( !pPlayer )
		return;

	// Make the player play his reload animation.
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ItemBusyFrame( void )
{
	// Call into the base ItemBusyFrame.
	BaseClass::ItemBusyFrame();

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
	{
		return;
	}
	
	bool bDidSkill = false;
	
	if ( (pOwner->m_nButtons & IN_ATTACK2) && m_bInReload == false && m_bInAttack2 == false )
	{
		if ( pOwner->DoClassSpecialSkill() )
		{
			bDidSkill = true;
			m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;
		}

		m_bInAttack2 = true;
	}
	else
	{
		m_bInAttack2 = false;
	}
	
	SoftZoomCheck();
	
	// Interrupt a reload on reload singly weapons.
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		if ( pPlayer->m_nButtons & IN_ATTACK )
		{
			if ( ( m_bInReload || ( ReloadsSingly() && m_iReloadMode!=TF_RELOAD_START ) ) && Clip1() > 0 )
			{
				AbortReload();
				
				pPlayer->m_flNextAttack = gpGlobals->curtime;
				m_flNextPrimaryAttack = gpGlobals->curtime;
				m_flNextSecondaryAttack = gpGlobals->curtime;
				SetWeaponIdleTime( gpGlobals->curtime + m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flTimeIdle );
			}
		}
	}
}

void CTFWeaponBase::ItemPreFrame( void )
{
	BaseClass::ItemPreFrame();
}

void CTFWeaponBase::SoftZoomCheck( void )
{
	
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	
	if( !pOwner )
		return;
	
	// Only merc has the eye of the TIGER DANCING THROUGH THE FIRE
	if( pOwner->GetPlayerClass()->GetClassIndex() != TF_CLASS_MERCENARY )
		return;
	
	float flZoomLevel = 0;
#ifdef GAME_DLL
	flZoomLevel = pOwner->IsFakeClient() ? 0 : Q_atof( engine->GetClientConVarValue( pOwner->entindex(), "fov_softzoom" ) );
#else
	flZoomLevel = fov_softzoom.GetFloat();
#endif
	float flFovDesired = 0;
#ifdef GAME_DLL
	flFovDesired = Q_atof( engine->GetClientConVarValue( pOwner->entindex(), "fov_desired" ) );
#else
	flFovDesired = fov_desired.GetFloat();
#endif
	
	if( (
	( pOwner->ShouldQuickZoom() && (pOwner->m_nButtons & IN_ATTACK2) && !pOwner->CheckSpecialSkill() && CanSoftZoom() )
	|| ( pOwner->m_nButtons & IN_ZOOM ) )
	&& pOwner->m_Shared.m_flNextZoomTime <= gpGlobals->curtime )
	{
		if( pOwner->GetFOV() == flFovDesired )
			pOwner->m_Shared.m_flNextZoomTime = gpGlobals->curtime + fConVarQuickZoomInTime;

		pOwner->SetFOV(pOwner, flZoomLevel, fConVarQuickZoomInTime);
	}
	else
	{
		if( ( pOwner->m_Shared.m_flNextZoomTime <= gpGlobals->curtime ) )
		{
			if( pOwner->GetFOV() == flZoomLevel )
				pOwner->m_Shared.m_flNextZoomTime = gpGlobals->curtime + fConVarQuickZoomOutTime;
			pOwner->SetFOV(pOwner, 0, fConVarQuickZoomOutTime);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
	{
		return;
	}

	if ( m_bWindingUp && gpGlobals->curtime >= m_flWindTick )
		PrimaryAttack();
	
	// debounce InAttack flags
	if ( m_bInAttack && !( pOwner->m_nButtons & IN_ATTACK ) )
	{
		m_bInAttack = false;
	}

	if ( m_bInAttack2 && !( pOwner->m_nButtons & IN_ATTACK2 ) )
	{
		m_bInAttack2 = false;
	}

	// If we're lowered, we're not allowed to fire
	if ( m_bLowered )
		return;

	if ( pOwner->m_Shared.IsLoser() )
		Lower();
	
	// Call the base item post frame.

	//Track the duration of the fire
	//FIXME: Check for IN_ATTACK2 as well?
	//FIXME: What if we're calling ItemBusyFrame?
	m_fFireDuration = ( pOwner->m_nButtons & IN_ATTACK ) ? ( m_fFireDuration + gpGlobals->frametime ) : 0.0f;

	if ( UsesClipsForAmmo1() )
	{
		CheckReload();
	}

	bool bFired = false;

	// Secondary attack has priority
	if ((pOwner->m_nButtons & IN_ATTACK2) && CanPerformSecondaryAttack() )
	{
		if (UsesSecondaryAmmo() && pOwner->GetAmmoCount(m_iSecondaryAmmoType)<=0 )
		{
			if (m_flNextEmptySoundTime < gpGlobals->curtime)
			{
				WeaponSound(EMPTY);
				m_flNextSecondaryAttack = m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
			}
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bAltFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// FIXME: This isn't necessarily true if the weapon doesn't have a secondary fire!
			// For instance, the crossbow doesn't have a 'real' secondary fire, but it still 
			// stops the crossbow from firing on the 360 if the player chooses to hold down their
			// zoom button. (sjb) Orange Box 7/25/2007
#if !defined(CLIENT_DLL)
			if( !IsX360() || !ClassMatches("weapon_crossbow") )
#endif
			{
				bFired = ShouldBlockPrimaryFire();
			}

			SecondaryAttack();

			// Secondary ammo doesn't have a reload animation
			if ( UsesClipsForAmmo2() )
			{
				// reload clip2 if empty
				if (m_iClip2 < 1)
				{
					if ( of_infiniteammo.GetBool() != 1 ) 
						pOwner->RemoveAmmo( 1, m_iSecondaryAmmoType );
					
					
					m_iClip2 = m_iClip2 + 1;
				}
			}
		}
	}
	
	SoftZoomCheck();
	
	if ( !bFired && !LoadsManualy() && (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		// Clip empty? Or out of ammo on a no-clip weapon?
		if ( !IsMeleeWeapon() &&  
			(( UsesClipsForAmmo1() && m_iClip1 <= 0 ) || ( !UsesClipsForAmmo1() && m_iReserveAmmo<=0 )) )
		{
			HandleFireOnEmpty();
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			//NOTENOTE: There is a bug with this code with regards to the way machine guns catch the leading edge trigger
			//			on the player hitting the attack key.  It relies on the gun catching that case in the same frame.
			//			However, because the player can also be doing a secondary attack, the edge trigger may be missed.
			//			We really need to hold onto the edge trigger and only clear the condition when the gun has fired its
			//			first shot.  Right now that's too much of an architecture change -- jdw
			
			// If the firing button was just pressed, or the alt-fire just released, reset the firing time
			if ( ( pOwner->m_afButtonPressed & IN_ATTACK ) || ( pOwner->m_afButtonReleased & IN_ATTACK2 ) )
			{
				 m_flNextPrimaryAttack = gpGlobals->curtime;
			}

			PrimaryAttack();

#ifdef CLIENT_DLL
			pOwner->SetFiredWeapon( true );
#endif
		}
	}

	// -----------------------
	//  Reload pressed / Clip Empty
	//  Can only start the Reload Cycle after the firing cycle
	if ( pOwner->m_nButtons & IN_RELOAD 
		&& m_flNextPrimaryAttack <= gpGlobals->curtime && UsesClipsForAmmo1() && !m_bInReload ) 
	{
		// reload when reload is pressed or if we're loading a barrage, or if no buttons are down and weapon is empty.
		Reload();
		m_fFireDuration = 0.0f;
	}

	// Check for reload singly interrupts.
	if ( m_bReloadsSingly )
	{
		ReloadSinglyPostFrame();
	}
	// -----------------------
	//  No buttons down
	// -----------------------
	if (!((pOwner->m_nButtons & IN_ATTACK) || (pOwner->m_nButtons & IN_ATTACK2) || (CanReload() && pOwner->m_nButtons & IN_RELOAD) || InBarrage()))
	{
		// no fire buttons down or reloading
		if ( !ReloadOrSwitchWeapons() && ( m_bInReload == false ) )
		{
			WeaponIdle();
		}
	}
}


//====================================================================================
// WEAPON RELOAD TYPES
//====================================================================================
void CTFWeaponBase::CheckReload( void )
{
	if ( m_bReloadsSingly )
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
		if ( !pOwner )
			return;

		if ((m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			if ( ( pOwner->m_nButtons & (IN_ATTACK | IN_ATTACK2) ) && m_iClip1 > 0 )
			{
				m_bInReload = false;
				return;
			}

			// If out of ammo end reload
			if ( m_iReserveAmmo <=0 )
			{
				FinishReload();
				return;
			}
			// If clip not full reload again
			else if (m_iClip1 < GetMaxClip1())
			{
				// Add them to the clip
				m_iClip1 += 1;
				if ( of_infiniteammo.GetBool() != 1 ) 
					m_iReserveAmmo -= 1;

				Reload();
				return;
			}
			// Clip full, stop reloading
			else
			{
				FinishReload();
				m_flNextPrimaryAttack	= gpGlobals->curtime;
				m_flNextSecondaryAttack = gpGlobals->curtime;
				return;
			}
		}
	}
	else
	{
		if ( (m_bInReload) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
		{
			FinishReload();
			m_flNextPrimaryAttack	= gpGlobals->curtime;
			m_flNextSecondaryAttack = gpGlobals->curtime;
			m_bInReload = false;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWeaponBase::ReloadSinglyPostFrame( void )
{
	if ( m_flTimeWeaponIdle > gpGlobals->curtime )
		return;
	// if the clip is empty and we have ammo remaining, 
	if ( ( ( Clip1() == 0 ) && ( GetOwner()->GetAmmoCount(m_iPrimaryAmmoType) > 0 ) ) ||
		// or we are already in the process of reloading but not finished
		( m_iReloadMode != TF_RELOAD_START ) )
	{
		// reload/continue reloading
		Reload();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::WeaponShouldBeLowered( void )
{
	// Can't be in the middle of another animation
	if ( GetIdealActivity() != ACT_VM_IDLE_LOWERED && GetIdealActivity() != ACT_VM_IDLE &&
		GetIdealActivity() != ACT_VM_IDLE_TO_LOWERED && GetIdealActivity() != ACT_VM_LOWERED_TO_IDLE )
		return false;

	if ( m_bLowered )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Ready( void )
{
	
	// If we don't have the anim, just hide for now
	if ( SelectWeightedSequence( ACT_VM_IDLE_LOWERED ) == ACTIVITY_NOT_AVAILABLE )
	{
		RemoveEffects( EF_NODRAW );
	}

	m_bLowered = false;	
	SendWeaponAnim( ACT_VM_IDLE );

	// Prevent firing until our weapon is back up
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	pPlayer->m_flNextAttack = gpGlobals->curtime + SequenceDuration();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFWeaponBase::Lower( void )
{
	AbortReload();

	// If we don't have the anim, just hide for now
	if ( SelectWeightedSequence( ACT_VM_IDLE_LOWERED ) == ACTIVITY_NOT_AVAILABLE )
	{
		AddEffects( EF_NODRAW );
	}

	m_bLowered = true;
	SendWeaponAnim( ACT_VM_IDLE_LOWERED );
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Show/hide weapon and corresponding view model if any
// Input  : visible - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::SetWeaponVisible( bool visible )
{
	if ( visible )
	{
		RemoveEffects( EF_NODRAW );
	}
	else
	{
		AddEffects( EF_NODRAW );
	}
	
#ifdef CLIENT_DLL
	UpdateVisibility();
#endif

}

//-----------------------------------------------------------------------------
// Purpose: Allows the weapon to choose proper weapon idle animation
//-----------------------------------------------------------------------------
void CTFWeaponBase::WeaponIdle( void )
{	
	//See if we should idle high or low
	if ( WeaponShouldBeLowered() )
	{
		// Move to lowered position if we're not there yet
		if ( GetActivity() != ACT_VM_IDLE_LOWERED && GetActivity() != ACT_VM_IDLE_TO_LOWERED && GetActivity() != ACT_TRANSITION )
		{
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
		}
		else if ( HasWeaponIdleTimeElapsed() )
		{
			// Keep idling low
			SendWeaponAnim( ACT_VM_IDLE_LOWERED );
		}
	}
	else
	{
		// See if we need to raise immediately
		if ( GetActivity() == ACT_VM_IDLE_LOWERED ) 
		{
			SendWeaponAnim( ACT_VM_IDLE );
		}
		else if ( HasWeaponIdleTimeElapsed() ) 
		{
			if ( !( m_bReloadsSingly && m_iReloadMode != TF_RELOAD_START ) )
			{
				SendWeaponAnim( ACT_VM_IDLE ); 
				m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();
			}
		}
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBase::GetMuzzleFlashModel( void )
{ 
	const char *pszModel = GetTFWpnData().m_szMuzzleFlashModel;

	if ( Q_strlen( pszModel ) > 0 )
	{
		return pszModel;
	}

	return NULL;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *CTFWeaponBase::GetMuzzleFlashParticleEffect( void )
{ 
	const char *pszPEffect = GetTFWpnData().m_szMuzzleFlashParticleEffect;
	
	if(GetTFWpnData().m_bTeamColorMuzzleFlash)
	{
		CTFPlayer *pPlayer = ToTFPlayer(GetOwner());
		if( pPlayer )
		{
			switch( pPlayer->GetTeamNumber() )
			{
				case TF_TEAM_RED:
					pszPEffect = GetTFWpnData().m_szMuzzleFlashParticleEffectRed;
					break;
				case TF_TEAM_BLUE:
					pszPEffect = GetTFWpnData().m_szMuzzleFlashParticleEffectBlue;
					break;
				default:
				case TF_TEAM_MERCENARY:
					pszPEffect = GetTFWpnData().m_szMuzzleFlashParticleEffectDM;
					break;
			}
		}
	}

	if ( Q_strlen( pszPEffect ) > 0 )
	{
		return pszPEffect;
	}

	return NULL;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
float CTFWeaponBase::GetMuzzleFlashModelLifetime( void )
{ 
	return GetTFWpnData().m_flMuzzleFlashModelDuration;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
float CTFWeaponBase::GetWindupTime( void )
{ 
	return GetTFWpnData().m_flWindupTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFWeaponBase::GetTracerType( void )
{ 
	if ( tf_useparticletracers.GetBool() && GetTFWpnData().m_szTracerEffect && GetTFWpnData().m_szTracerEffect[0] )
	{
		if ( !m_szTracerName[0] )
		{
			if( GetOwner() )
			{
				if ( GetOwner()->GetTeamNumber() == TF_TEAM_RED )
					Q_snprintf( m_szTracerName, MAX_TRACER_NAME, "%s_%s", GetTFWpnData().m_szTracerEffect, "red" );
				else if ( GetOwner()->GetTeamNumber() == TF_TEAM_BLUE )
					Q_snprintf( m_szTracerName, MAX_TRACER_NAME, "%s_%s", GetTFWpnData().m_szTracerEffect, "blue" );
				else
					Q_snprintf( m_szTracerName, MAX_TRACER_NAME, "%s_%s", GetTFWpnData().m_szTracerEffect, "dm" );					
			}
		}

		return m_szTracerName;
	}

	if ( GetWeaponID() == TF_WEAPON_MINIGUN || GetWeaponID() == TF_WEAPON_GATLINGGUN || GetWeaponID() == TFC_WEAPON_ASSAULTCANNON )
		return "BrightTracer";

	return BaseClass::GetTracerType();
}

//=============================================================================
//
// TFWeaponBase functions (Server specific).
//
#if !defined( CLIENT_DLL )

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::CheckRespawn()
{
	// Do not respawn.
	return;
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
CBaseEntity *CTFWeaponBase::Respawn()
{
	// make a copy of this weapon that is invisible and inaccessible to players (no touch function). The weapon spawn/respawn code
	// will decide when to make the weapon visible and touchable.
	CBaseEntity *pNewWeapon = CBaseEntity::Create( GetClassname(), g_pGameRules->VecWeaponRespawnSpot( this ), GetAbsAngles(), GetOwner() );

	if ( pNewWeapon )
	{
		pNewWeapon->AddEffects( EF_NODRAW );// invisible for now
		pNewWeapon->SetTouch( NULL );// no touch
		pNewWeapon->SetThink( &CTFWeaponBase::AttemptToMaterialize );

		UTIL_DropToFloor( this, MASK_SOLID );

		// not a typo! We want to know when the weapon the player just picked up should respawn! This new entity we created is the replacement,
		// but when it should respawn is based on conditions belonging to the weapon that was taken.
		pNewWeapon->SetNextThink( gpGlobals->curtime + g_pGameRules->FlWeaponRespawnTime( this ) );
	}
	else
	{
		Msg( "Respawn failed to create %s!\n", GetClassname() );
	}

	return pNewWeapon;
}

// -----------------------------------------------------------------------------
// Purpose: Make a weapon visible and tangible.
// -----------------------------------------------------------------------------
void CTFWeaponBase::Materialize()
{
	if ( IsEffectActive( EF_NODRAW ) )
	{
		RemoveEffects( EF_NODRAW );
		DoMuzzleFlash();
	}

	AddSolidFlags( FSOLID_TRIGGER );

	SetThink ( &CTFWeaponBase::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 1 );
}

// -----------------------------------------------------------------------------
// Purpose: The item is trying to materialize, should it do so now or wait longer?
// -----------------------------------------------------------------------------
void CTFWeaponBase::AttemptToMaterialize()
{
	float flTime = g_pGameRules->FlWeaponTryRespawn( this );

	if ( flTime == 0 )
	{
		Materialize();
		return;
	}

	SetNextThink( gpGlobals->curtime + flTime );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::SetDieThink( bool bDie )
{
	if( bDie )
	{
		SetContextThink( &CTFWeaponBase::Die, gpGlobals->curtime + 30.0f, "DieContext" );
	}
	else
	{
		SetContextThink( NULL, gpGlobals->curtime, "DieContext" );
	}
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
void CTFWeaponBase::Die( void )
{
	UTIL_Remove( this );
}

void CTFWeaponBase::WeaponReset( void )
{
	m_iReloadMode.Set( TF_RELOAD_START );

	m_bResetParity = !m_bResetParity;
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
const Vector &CTFWeaponBase::GetBulletSpread( void )
{
	static Vector cone = VECTOR_CONE_15DEGREES;
	return cone;
}

#else

void TE_DynamicLight( IRecipientFilter& filter, float delay,
					 const Vector* org, int r, int g, int b, int exponent, float radius, float time, float decay, int nLightIndex = LIGHT_INDEX_TE_DYNAMIC );

//=============================================================================
//
// TFWeaponBase functions (Client specific).
//
void CTFWeaponBase::CreateMuzzleFlashEffects(C_BaseEntity *pAttachEnt, int nIndex)
{
	Vector vecOrigin;
	QAngle angAngles;
	
	if ( !pAttachEnt )
		return;

	int iMuzzleFlashAttachment = pAttachEnt->LookupAttachment("muzzle");

	const char *pszMuzzleFlashEffect = NULL;
	const char *pszMuzzleFlashModel = GetMuzzleFlashModel();
	const char *pszMuzzleFlashParticleEffect = GetMuzzleFlashParticleEffect();
	
	CTFPlayer *pOwner = ToTFPlayer(GetOwnerEntity());

	// Pick the right muzzleflash (3rd / 1st person)
	CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if (pLocalPlayer && (GetOwnerEntity() == pLocalPlayer ||
		(pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE && pLocalPlayer->GetObserverTarget() == GetOwnerEntity())))
	{
		pszMuzzleFlashEffect = GetMuzzleFlashEffectName_1st();
	}
	else
	{
		pszMuzzleFlashEffect = GetMuzzleFlashEffectName_3rd();
	}

	// If we have an attachment, then stick a light on it.
	if (iMuzzleFlashAttachment > 0 && (pszMuzzleFlashEffect || pszMuzzleFlashModel || pszMuzzleFlashParticleEffect))
	{
		pAttachEnt->GetAttachment(iMuzzleFlashAttachment, vecOrigin, angAngles);

		// Muzzleflash light
		if ( of_muzzlelight.GetBool() )
		{
			CLocalPlayerFilter filter;
			TE_DynamicLight(filter, 0.0f, &vecOrigin, 255, 192, 64, 5, 70.0f, 0.05f, 70.0f / 0.05f, LIGHT_INDEX_MUZZLEFLASH);
		}

		if ( pszMuzzleFlashEffect )
		{
			// Using an muzzle flash dispatch effect
			CEffectData muzzleFlashData;
			muzzleFlashData.m_vOrigin = vecOrigin;
			muzzleFlashData.m_vAngles = angAngles;
			muzzleFlashData.m_hEntity = pAttachEnt->GetRefEHandle();
			muzzleFlashData.m_nAttachmentIndex = iMuzzleFlashAttachment;
			
			if( pOwner )
			{
				muzzleFlashData.m_bCustomColors = true;
				muzzleFlashData.m_CustomColors.m_vecColor1 = pOwner->m_vecPlayerColor;
			}
			//muzzleFlashData.m_nHitBox = GetDODWpnData().m_iMuzzleFlashType;
			//muzzleFlashData.m_flMagnitude = GetDODWpnData().m_flMuzzleFlashScale;
			muzzleFlashData.m_flMagnitude = 0.2;
			DispatchEffect( pszMuzzleFlashEffect, muzzleFlashData );
		}

		if ( of_beta_muzzleflash.GetBool() && pszMuzzleFlashModel )
		{
			float flEffectLifetime = GetMuzzleFlashModelLifetime();

			// Using a model as a muzzle flash.
			if ( m_hMuzzleFlashModel[nIndex] )
			{
				// Increase the lifetime of the muzzleflash
				m_hMuzzleFlashModel[nIndex]->SetLifetime( flEffectLifetime );
			}
			else
			{
				m_hMuzzleFlashModel[nIndex] = C_MuzzleFlashModel::CreateMuzzleFlashModel( pszMuzzleFlashModel, pAttachEnt, iMuzzleFlashAttachment, flEffectLifetime );

				// FIXME: This is an incredibly brutal hack to get muzzle flashes positioned correctly for recording
				m_hMuzzleFlashModel[nIndex]->SetIs3rdPersonFlash( nIndex == 1 );
				m_hMuzzleFlashModel[nIndex]->SetViewmodel( nIndex != 1 );
			}
		}

		if ( pszMuzzleFlashParticleEffect && !of_beta_muzzleflash.GetBool() ) 
		{
			if( pOwner )
				DispatchParticleEffect( pszMuzzleFlashParticleEffect, PATTACH_POINT_FOLLOW, pAttachEnt, "muzzle", pOwner->m_vecPlayerColor, pOwner->m_vecPlayerColor, true );
			else
				DispatchParticleEffect( pszMuzzleFlashParticleEffect, PATTACH_POINT_FOLLOW, pAttachEnt, "muzzle" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFWeaponBase::InternalDrawModel( int flags )
{
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );
	bool bNotViewModel = ( (pOwner && !pOwner->IsLocalPlayer()) || C_BasePlayer::ShouldDrawLocalPlayer() );
	bool bUseInvulnMaterial = (bNotViewModel && pOwner && pOwner->m_Shared.InCondUber());
	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( *pOwner->GetInvulnMaterialRef() );
	}

	int ret = BaseClass::InternalDrawModel( flags );

	if ( bUseInvulnMaterial )
	{
		modelrender->ForcedMaterialOverride( NULL );
	}

	return ret;
}

void CTFWeaponBase::ProcessMuzzleFlashEvent( void )
{
	C_BaseEntity *pAttachEnt;
	C_TFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	if ( pOwner == NULL )
		return;

	bool bDrawMuzzleFlashOnViewModel = ( pOwner->IsLocalPlayer() && !C_BasePlayer::ShouldDrawLocalPlayer() ) ||
		( IsLocalPlayerSpectator() && GetSpectatorMode() == OBS_MODE_IN_EYE && GetSpectatorTarget() == pOwner->entindex() );

	if ( bDrawMuzzleFlashOnViewModel )
	{
		pAttachEnt = pOwner->GetViewModel();
	}
	else
	{
		pAttachEnt = this;
	}

	{
		CRecordEffectOwner recordOwner( pOwner, bDrawMuzzleFlashOnViewModel );
		CreateMuzzleFlashEffects( pAttachEnt, 0 );
	}

	// Quasi-evil
	int nModelIndex = GetModelIndex();
	int nWorldModelIndex = GetWorldModelIndex();
	bool bInToolRecordingMode = ToolsEnabled() && clienttools->IsInRecordingMode();
	if ( bInToolRecordingMode && nModelIndex != nWorldModelIndex && pOwner->IsLocalPlayer() )
	{
		CRecordEffectOwner recordOwner( pOwner, false );

		SetModelIndex( nWorldModelIndex );
		CreateMuzzleFlashEffects( this, 1 );
		SetModelIndex( nModelIndex );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
bool CTFWeaponBase::ShouldPredict()
{
	if ( GetOwner() && GetOwner() == C_BasePlayer::GetLocalPlayer() )
	{
		return true;
	}

	return BaseClass::ShouldPredict();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::WeaponReset( void )
{
	UpdateVisibility();
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::OnPreDataChanged( DataUpdateType_t type )
{
	BaseClass::OnPreDataChanged( type );

	m_bOldResetParity = m_bResetParity;

}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
void CTFWeaponBase::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );	
	
	if ( GetPredictable() && !ShouldPredict() )
	{
		ShutdownPredictable();
	}

	//If its a world (held or dropped) model then set the correct skin color here.
	if ( m_nModelIndex == GetWorldModelIndex() )
	{
		m_nSkin = GetSkin();
	}

	if ( m_bResetParity != m_bOldResetParity )
	{
		WeaponReset();
	}

	//Here we go...
	//Since we can't get a repro for the invisible weapon thing, I'll fix it right up here:
	CTFPlayer *pOwner = ToTFPlayer( GetOwnerEntity() );

	//Our owner is alive
	if ( pOwner && pOwner->IsAlive() == true )
	{
		//And he is NOT taunting
		if ( pOwner->m_Shared.InCond ( TF_COND_TAUNTING ) == false )
		{
			//Then why the hell am I NODRAW?
			if ( pOwner->GetActiveWeapon() == this && IsEffectActive( EF_NODRAW ) )
			{
				RemoveEffects( EF_NODRAW );
				UpdateVisibility();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
int CTFWeaponBase::CalcOverrideModelIndex( void )
{
	if ( ShouldDrawUsingViewModel() )
	{
		return m_iViewModelIndex;
	}

	return GetWorldModelIndex();
}

//-----------------------------------------------------------------------------
// Purpose: hacky method to make clientside bodygroups work
// ----------------------------------------------------------------------------
C_BaseAnimating *C_TFWeaponBase::GetOwnModel( void )
{
	return this;
}

//-----------------------------------------------------------------------------
// Purpose:
// ----------------------------------------------------------------------------
int CTFWeaponBase::GetWorldModelIndex( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
	{
		// if we're a spy and we're disguised, we also
		// want to disguise our weapon's world model

		CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return 0;

		int iLocalTeam = pLocalPlayer->GetTeamNumber();

		// We only show disguise weapon to the enemy team when owner is disguised
		bool bUseDisguiseWeapon = ( pPlayer->GetTeamNumber() != iLocalTeam && iLocalTeam > LAST_SHARED_TEAM );

		if ( bUseDisguiseWeapon && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			int iModelIndex = pPlayer->m_Shared.GetDisguiseWeaponModelIndex();

			Assert( iModelIndex != -1 );

			return iModelIndex;
		}	
	}

	return BaseClass::GetWorldModelIndex();
}

bool CTFWeaponBase::ShouldDrawCrosshair( void )
{
	return GetTFWpnData().m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bDrawCrosshair;
}

void CTFWeaponBase::Redraw()
{
	if ( ShouldDrawCrosshair() && g_pClientMode->ShouldDrawCrosshair() )
	{
		DrawCrosshair();
	}
}

#endif

acttable_t CTFWeaponBase::m_acttablePrimary[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_PRIMARY,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_PRIMARY,				false },
	{ ACT_MP_DEPLOYED,			ACT_MP_DEPLOYED_PRIMARY,			false },
	{ ACT_MP_CROUCH_DEPLOYED,   ACT_MP_CROUCHWALK_DEPLOYED,         false },
	{ ACT_MP_RUN,				ACT_MP_RUN_PRIMARY,					false },
	{ ACT_MP_WALK,				ACT_MP_WALK_PRIMARY,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_PRIMARY,				false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_PRIMARY,			false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_PRIMARY,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_PRIMARY,			false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_PRIMARY,			false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_PRIMARY,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_PRIMARY,				false },
	{ ACT_MP_DEPLOYED,			ACT_MP_DEPLOYED_PRIMARY,			false },
	{ ACT_MP_SWIM_DEPLOYED,		ACT_MP_SWIM_DEPLOYED_PRIMARY,		false },
	{ ACT_MP_CROUCHWALK_DEPLOYED,		ACT_MP_CROUCHWALK_DEPLOYED, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_PRIMARY,	false },
	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED,		ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_PRIMARY,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED,	ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED,	false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_PRIMARY,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_PRIMARY,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_PRIMARY,		false },
	{ ACT_MP_RELOAD_STAND_LOOP,	ACT_MP_RELOAD_STAND_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_STAND_END,	ACT_MP_RELOAD_STAND_PRIMARY_END,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_PRIMARY,		false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_CROUCH_END,	ACT_MP_RELOAD_CROUCH_PRIMARY_END,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_PRIMARY,			false },
	{ ACT_MP_RELOAD_SWIM_LOOP,	ACT_MP_RELOAD_SWIM_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_SWIM_END,	ACT_MP_RELOAD_SWIM_PRIMARY_END,		false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_PRIMARY,		false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP,	ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_AIRWALK_END,ACT_MP_RELOAD_AIRWALK_PRIMARY_END,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_PRIMARY, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_PRIMARY_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_PRIMARY_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_PRIMARY_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_PRIMARY_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_PRIMARY_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_PRIMARY_GRENADE2_ATTACK,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_PRIMARY,	false },
};

acttable_t CTFWeaponBase::m_acttableSecondary[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_SECONDARY,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_SECONDARY,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_SECONDARY,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_SECONDARY,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_SECONDARY,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_SECONDARY,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_SECONDARY,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_SECONDARY,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_SECONDARY,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_SECONDARY,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_SECONDARY,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_SECONDARY,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_SECONDARY,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_SECONDARY,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_SECONDARY,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_SECONDARY,		false },
	{ ACT_MP_RELOAD_STAND_LOOP,	ACT_MP_RELOAD_STAND_SECONDARY_LOOP,	false },
	{ ACT_MP_RELOAD_STAND_END,	ACT_MP_RELOAD_STAND_SECONDARY_END,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_SECONDARY,		false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,ACT_MP_RELOAD_CROUCH_SECONDARY_LOOP,false },
	{ ACT_MP_RELOAD_CROUCH_END,	ACT_MP_RELOAD_CROUCH_SECONDARY_END,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_SECONDARY,		false },
	{ ACT_MP_RELOAD_SWIM_LOOP,	ACT_MP_RELOAD_SWIM_SECONDARY_LOOP,	false },
	{ ACT_MP_RELOAD_SWIM_END,	ACT_MP_RELOAD_SWIM_SECONDARY_END,	false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_SECONDARY,	false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP,	ACT_MP_RELOAD_AIRWALK_SECONDARY_LOOP,	false },
	{ ACT_MP_RELOAD_AIRWALK_END,ACT_MP_RELOAD_AIRWALK_SECONDARY_END,false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_SECONDARY, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_SECONDARY_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_SECONDARY_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_SECONDARY_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_SECONDARY_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_SECONDARY_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_SECONDARY_GRENADE2_ATTACK,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_SECONDARY,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_SECONDARY,	false },
};

acttable_t CTFWeaponBase::m_acttableMelee[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_MELEE,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_MELEE,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_MELEE,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_MELEE,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_MELEE,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_MELEE,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_MELEE,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_MELEE,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_MELEE,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_MELEE,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_MELEE,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_MELEE,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_MELEE,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE,	false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_ATTACK_STAND_MELEE_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY,false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_MELEE_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_MELEE_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_MELEE_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_MELEE_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_MELEE_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_MELEE_GRENADE2_ATTACK,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_MELEE,	false },
};

acttable_t CTFWeaponBase::m_acttableBuilding[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_BUILDING,			false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_BUILDING,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_BUILDING,			false },
	{ ACT_MP_WALK,				ACT_MP_WALK_BUILDING,			false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_BUILDING,		false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_BUILDING,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_BUILDING,			false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_BUILDING,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_BUILDING,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_BUILDING,		false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_BUILDING,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_BUILDING,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_BUILDING,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_BUILDING,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_BUILDING,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE_BUILDING,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_BUILDING,	false },
};

acttable_t CTFWeaponBase::m_acttablePDA[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_PDA,			false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_PDA,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_PDA,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_PDA,			false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_PDA,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_PDA,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_PDA,			false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_PDA,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_PDA,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_PDA,		false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_PDA,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MP_ATTACK_STAND_PDA, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MP_ATTACK_SWIM_PDA, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_PDA,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_PDA,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_PDA,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_PDA,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_PDA,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_PDA,	false },
};

ConVar mp_forceactivityset( "mp_forceactivityset", "-1", FCVAR_CHEAT|FCVAR_REPLICATED );

acttable_t *CTFWeaponBase::ActivityList( int &iActivityCount )
{
	int iClass = -1;
	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if (pPlayer)
		iClass = pPlayer->GetPlayerClass()->GetClassIndex();
	
	int iWeaponRole = GetTFWpnData().m_iWeaponType;
	if (!(iClass < 0 || GetTFWpnData().m_iClassWeaponType[iClass] < 0))
		iWeaponRole = GetTFWpnData().m_iClassWeaponType[iClass];

	if ( mp_forceactivityset.GetInt() >= 0 )
	{
		iWeaponRole = mp_forceactivityset.GetInt();
	}

#ifdef CLIENT_DLL
	// If we're disguised, we always show the primary weapon
	// even though our actual weapon may not be primary 
	if ( pPlayer && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->IsEnemyPlayer() )
	{
		CTFWeaponInfo *pWeaponInfo = pPlayer->m_Shared.GetDisguiseWeaponInfo();
		if ( pWeaponInfo )
		{
			iClass = pPlayer->m_Shared.GetDisguiseClass();
			if( iClass < 0 || pWeaponInfo->m_iClassWeaponType[iClass] < 0 )
				iWeaponRole = pWeaponInfo->m_iWeaponType;
			else
				iWeaponRole = pWeaponInfo->m_iClassWeaponType[iClass];
		}
	}
#endif

	acttable_t *pTable;

	switch( iWeaponRole )
	{
	case TF_WPN_TYPE_PRIMARY:
	default:
		pTable = m_acttablePrimary;
		iActivityCount = ARRAYSIZE(m_acttablePrimary);
		break;
	case TF_WPN_TYPE_SECONDARY:
		pTable = m_acttableSecondary;
		iActivityCount = ARRAYSIZE(m_acttableSecondary);
		break;
	case TF_WPN_TYPE_MELEE:
		pTable = m_acttableMelee;
		iActivityCount = ARRAYSIZE(m_acttableMelee);
		break;
	case TF_WPN_TYPE_BUILDING:
		pTable = m_acttableBuilding;
		iActivityCount = ARRAYSIZE(m_acttableBuilding);
		break;
	case TF_WPN_TYPE_PDA:
		pTable = m_acttablePDA;
		iActivityCount = ARRAYSIZE(m_acttablePDA);
		break;
	}

	return pTable;
}




// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
CBasePlayer *CTFWeaponBase::GetPlayerOwner() const
{
	return dynamic_cast<CBasePlayer*>( GetOwner() );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
CTFPlayer *CTFWeaponBase::GetTFPlayerOwner() const
{
	return dynamic_cast<CTFPlayer*>( GetOwner() );
}

#ifdef CLIENT_DLL
// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
C_BaseEntity *CTFWeaponBase::GetWeaponForEffect()
{
	//C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_TFPlayer *pOwner = GetTFPlayerOwner();

	//if ( !pLocalPlayer )
	//	return NULL;

	// This causes many problems!
	if ( pOwner && !pOwner->ShouldDrawThisPlayer() )
	{
		C_BaseViewModel *pViewModel = pOwner->GetViewModel();

		if (pViewModel)
			return pViewModel;
	}

	//if ( pLocalPlayer == GetTFPlayerOwner() )
	//	return pLocalPlayer->GetViewModel();

	return this;
}
#endif

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
bool CTFWeaponBase::CanAttack( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( pPlayer )
		return pPlayer->CanAttack();

	return false;
}


#if defined( CLIENT_DLL )

static ConVar	cl_bobcycle( "cl_bobcycle","0.8", FCVAR_ARCHIVE );
static ConVar	cl_bobup( "cl_bobup","0.5", FCVAR_ARCHIVE );

//-----------------------------------------------------------------------------
// Purpose: Helper function to calculate head bob
//-----------------------------------------------------------------------------
float CalcViewModelBobHelper( CBasePlayer *player, BobState_t *pBobState )
{
	Assert( pBobState );
	if ( !pBobState )
		return 0;

	float	cycle;

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ( ( !gpGlobals->frametime ) || ( player == NULL ) )
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	//Find the speed of the player
	float speed = player->GetLocalVelocity().Length2D();
	float flmaxSpeedDelta = max( 0, (gpGlobals->curtime - pBobState->m_flLastBobTime ) * 320.0f );

	// don't allow too big speed changes
	speed = clamp( speed, pBobState->m_flLastSpeed-flmaxSpeedDelta, pBobState->m_flLastSpeed+flmaxSpeedDelta );
	speed = clamp( speed, -320, 320 );

	pBobState->m_flLastSpeed = speed;

	//FIXME: This maximum speed value must come from the server.
	//		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

	float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );

	pBobState->m_flBobTime += ( gpGlobals->curtime - pBobState->m_flLastBobTime ) * bob_offset;
	pBobState->m_flLastBobTime = gpGlobals->curtime;

	//Calculate the vertical bob
	cycle = pBobState->m_flBobTime - (int)(pBobState->m_flBobTime/cl_bobcycle.GetFloat())*cl_bobcycle.GetFloat();
	cycle /= cl_bobcycle.GetFloat();

	if ( cycle < cl_bobup.GetFloat() )
	{
		cycle = M_PI * cycle / cl_bobup.GetFloat();
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-cl_bobup.GetFloat())/(1.0 - cl_bobup.GetFloat());
	}

	pBobState->m_flVerticalBob = speed*0.005f;
	pBobState->m_flVerticalBob = pBobState->m_flVerticalBob*0.3 + pBobState->m_flVerticalBob*0.7*sin(cycle);

	pBobState->m_flVerticalBob = clamp( pBobState->m_flVerticalBob, -7.0f, 4.0f );

	//Calculate the lateral bob
	cycle = pBobState->m_flBobTime - (int)(pBobState->m_flBobTime/cl_bobcycle.GetFloat()*2)*cl_bobcycle.GetFloat()*2;
	cycle /= cl_bobcycle.GetFloat()*2;

	if ( cycle < cl_bobup.GetFloat() )
	{
		cycle = M_PI * cycle / cl_bobup.GetFloat();
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-cl_bobup.GetFloat())/(1.0 - cl_bobup.GetFloat());
	}

	pBobState->m_flLateralBob = speed*0.005f;
	pBobState->m_flLateralBob = pBobState->m_flLateralBob*0.3 + pBobState->m_flLateralBob*0.7*sin(cycle);
	pBobState->m_flLateralBob = clamp( pBobState->m_flLateralBob, -7.0f, 4.0f );

	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to add head bob
//-----------------------------------------------------------------------------
void AddViewModelBobHelper( Vector &origin, QAngle &angles, BobState_t *pBobState )
{
	Assert( pBobState );
	if ( !pBobState )
		return;

	Vector	forward, right;
	AngleVectors( angles, &forward, &right, NULL );

	// Apply bob, but scaled down to 40%
	VectorMA( origin, pBobState->m_flVerticalBob * 0.4f, forward, origin );

	// Z bob a bit more
	origin[2] += pBobState->m_flVerticalBob * 0.1f;

	// bob the angles
	angles[ ROLL ]	+= pBobState->m_flVerticalBob * 0.5f;
	angles[ PITCH ]	-= pBobState->m_flVerticalBob * 0.4f;
	angles[ YAW ]	-= pBobState->m_flLateralBob  * 0.3f;

	VectorMA( origin, pBobState->m_flLateralBob * 0.2f, right, origin );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CTFWeaponBase::CalcViewmodelBob( void )
{
	CBasePlayer *player = ToBasePlayer( GetOwner() );
	//Assert( player );
	BobState_t *pBobState = GetBobState();
	if ( pBobState )
		return ::CalcViewModelBobHelper( player, pBobState );
	else
		return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CTFWeaponBase::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
	// call helper functions to do the calculation
	BobState_t *pBobState = GetBobState();
	if ( pBobState )
	{
		CalcViewmodelBob();
		::AddViewModelBobHelper( origin, angles, pBobState );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Returns the head bob state for this weapon, which is stored
//			in the view model.  Note that this this function can return
//			NULL if the player is dead or the view model is otherwise not present.
//-----------------------------------------------------------------------------
BobState_t *CTFWeaponBase::GetBobState()
{
	// get the view model for this weapon
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return NULL;
	CBaseViewModel *baseViewModel = pOwner->GetViewModel( m_nViewModelIndex );
	if ( baseViewModel == NULL )
		return NULL;
	CTFViewModel *viewModel = dynamic_cast<CTFViewModel *>(baseViewModel);
	Assert( viewModel );

	// get the bob state out of the view model
	return &( viewModel->GetBobState() );
}

//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
int CTFWeaponBase::GetSkin()
{
	int nSkin = 0;

	CTFPlayer *pPlayer = GetTFPlayerOwner();
	if ( pPlayer )
	{
		CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( !pLocalPlayer )
			return 0;

		int iLocalTeam = pLocalPlayer->GetTeamNumber();
		int iTeamNumber = pPlayer->GetTeamNumber();

		bool bHasTeamSkins = false;

		// We only show disguise weapon to the enemy team when owner is disguised
		bool bUseDisguiseWeapon = ( iTeamNumber != iLocalTeam && iLocalTeam > LAST_SHARED_TEAM );

		if ( bUseDisguiseWeapon && pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			CTFWeaponInfo *pInfo = pPlayer->m_Shared.GetDisguiseWeaponInfo();

			if ( pInfo )
			{
				bHasTeamSkins = pInfo->m_bHasTeamSkins_Worldmodel;
			}				

			if ( pLocalPlayer != pPlayer )
			{
				iTeamNumber = pPlayer->m_Shared.GetDisguiseTeam();
			}
		}
		else
		{
			 bHasTeamSkins = GetTFWpnData().m_bHasTeamSkins_Worldmodel;
		}

		if ( bHasTeamSkins )
		{
			switch( iTeamNumber )
			{
			case TF_TEAM_RED:
				nSkin = 0;
				break;
			case TF_TEAM_BLUE:
				nSkin = 1;
				break;
			case TF_TEAM_MERCENARY:
				nSkin = 2;
				break;
			}
		}
	}

	return nSkin;
}

bool CTFWeaponBase::OnFireEvent( C_BaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
	switch ( event )
	{

	case 6002:
	{
		CEffectData data;
		pViewModel->GetAttachment( atoi(options), data.m_vOrigin, data.m_vAngles );
		data.m_nHitBox = GetWeaponID();
		DispatchEffect( "TF_EjectBrass", data );
		return true;
	}
	break;
	case AE_WPN_INCREMENTAMMO:
	{
		CTFPlayer *pPlayer = GetTFPlayerOwner();

		if ( pPlayer && ReserveAmmo() > 0 && !m_bReloadedThroughAnimEvent )
		{
			m_iClip1 = min( ( m_iClip1 + 1 ), GetMaxClip1() );
			if( of_infiniteammo.GetBool() != 1 )
				m_iReserveAmmo -= 1;
		}

		m_bReloadedThroughAnimEvent = true;
		
		return true;
	}
	break;
	case AE_CL_CREATE_PARTICLE_EFFECT:
	{
		if( pViewModel )
		{
			int iAttachment = -1;
			int iAttachType = PATTACH_ABSORIGIN_FOLLOW;
			char token[256];
			char szParticleEffect[256];

			// Get the particle effect name
			const char *p = options;
			p = nexttoken( token, p, ' ', sizeof(token) );

			const char* mtoken = ModifyEventParticles( token );
			if ( !mtoken || mtoken[0] == '\0' )
				return false;
			Q_strncpy( szParticleEffect, mtoken, sizeof(szParticleEffect) );

			// Get the attachment type
			p = nexttoken( token, p, ' ', sizeof(token) );

			iAttachType = GetAttachTypeFromString( token );
			if ( iAttachType == -1 )
			{
				Warning("Invalid attach type specified for particle effect anim event. Trying to spawn effect '%s' with attach type of '%s'\n", szParticleEffect, token );
				return false;
			}

			// Get the attachment point index
			p = nexttoken( token, p, ' ', sizeof(token) );
			if ( token[0] )
			{
				iAttachment = atoi(token);

				// See if we can find any attachment points matching the name
				if ( token[0] != '0' && iAttachment == 0 )
				{
					iAttachment = pViewModel->LookupAttachment( token );
					if ( iAttachment <= 0 )
					{
						Warning( "Failed to find attachment point specified for particle effect anim event. Trying to spawn effect '%s' on attachment named '%s'\n", szParticleEffect, token );
						return false;
					}
				}
			}

			// Spawn the particle effect
			CTFPlayer *pPlayer = GetTFPlayerOwner();
			if( pPlayer )
				pPlayer->m_Shared.UpdateParticleColor(pViewModel->ParticleProp()->Create( szParticleEffect, (ParticleAttachment_t)iAttachType, iAttachment ));
			else
				pViewModel->ParticleProp()->Create( szParticleEffect, (ParticleAttachment_t)iAttachType, iAttachment );
		}
	}
	break;	
	case AE_CL_STOP_PARTICLE_EFFECT:
	{
		if( pViewModel )
		{
			char token[256];
			char szParticleEffect[256];

			// Get the particle effect name
			const char *p = options;
			p = nexttoken( token, p, ' ', sizeof(token) );

			const char* mtoken = ModifyEventParticles( token );
			if ( !mtoken || mtoken[0] == '\0' )
				return false;
			Q_strncpy( szParticleEffect, mtoken, sizeof(szParticleEffect) );

			pViewModel->ParticleProp()->StopParticlesNamed( szParticleEffect );
		}
	}
	break;
	case AE_CL_BODYGROUP_SET_VALUE:
	{
		if( pViewModel )
		{
			DevMsg("AE_CL_BODYGROUP_SET_VALUE event fired \n");
			int value;
			char token[256];
			char szBodygroupName[256];

			const char *p = options;

			// Bodygroup Name
			p = nexttoken( token, p, ' ', sizeof(token) );
			Q_strncpy( szBodygroupName, token, sizeof( szBodygroupName ) );

			// Get the desired value
			p = nexttoken( token, p, ' ', sizeof(token) );
			value = token[0] ? atoi( token ) : 0;

			int index = pViewModel->FindBodygroupByName( szBodygroupName );
			if ( index >= 0 )
			{
				pViewModel->SetBodygroup( index, value );
			}
			index = FindBodygroupByName( szBodygroupName );
			if ( index >= 0 )
			{
				SetBodygroup( index, value );
			}
		}
	}
	break;
	}

	return BaseClass::OnFireEvent( pViewModel, origin, angles, event, options );
}

ShadowType_t CTFWeaponBase::ShadowCastType( void )
{
	if ( IsEffectActive( EF_NODRAW | EF_NOSHADOW ) )
		return SHADOWS_NONE;

	if ( m_iState == WEAPON_IS_CARRIED_BY_PLAYER )
		return SHADOWS_NONE;

	return BaseClass::ShadowCastType();
}
//-----------------------------------------------------------------------------
// Purpose: Used for spy invisiblity material
//-----------------------------------------------------------------------------
class CWeaponInvisProxy : public CEntityMaterialProxy
{
public:

	CWeaponInvisProxy( void );
	virtual ~CWeaponInvisProxy( void );
	virtual bool Init( IMaterial *pMaterial, KeyValues* pKeyValues );
	virtual void OnBind( C_BaseEntity *pC_BaseEntity );
	virtual IMaterial * GetMaterial();

private:
	IMaterialVar *m_pPercentInvisible;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponInvisProxy::CWeaponInvisProxy( void )
{
	m_pPercentInvisible = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CWeaponInvisProxy::~CWeaponInvisProxy( void )
{

}

//-----------------------------------------------------------------------------
// Purpose: Get pointer to the color value
// Input : *pMaterial - 
//-----------------------------------------------------------------------------
bool CWeaponInvisProxy::Init( IMaterial *pMaterial, KeyValues* pKeyValues )
{
	Assert( pMaterial );

	// Need to get the material var
	bool bFound;
	m_pPercentInvisible = pMaterial->FindVar( "$cloakfactor", &bFound );

	return bFound;
}

extern ConVar tf_teammate_max_invis;
//-----------------------------------------------------------------------------
// Purpose: 
// Input :
//-----------------------------------------------------------------------------
void CWeaponInvisProxy::OnBind( C_BaseEntity *pEnt )
{
	if( !m_pPercentInvisible )
		return;

	if ( !pEnt )
		return;

	C_BaseEntity *pMoveParent = pEnt->GetMoveParent();
	if ( !pMoveParent || !pMoveParent->IsPlayer() )
	{
		m_pPercentInvisible->SetFloatValue( 0.0f );
		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer( pMoveParent );
	Assert( pPlayer );

	m_pPercentInvisible->SetFloatValue( pPlayer->GetEffectiveInvisibilityLevel() );
}

IMaterial *CWeaponInvisProxy::GetMaterial()
{
	if ( !m_pPercentInvisible )
		return NULL;

	return m_pPercentInvisible->GetOwningMaterial();
}

EXPOSE_INTERFACE( CWeaponInvisProxy, IMaterialProxy, "weapon_invis" IMATERIAL_PROXY_INTERFACE_VERSION );

#endif // CLIENT_DLL
