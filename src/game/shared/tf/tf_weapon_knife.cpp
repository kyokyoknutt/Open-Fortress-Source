//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: Weapon Knife.
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_knife.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
	#include "tf_gamestats.h"
	#include "ilagcompensationmanager.h"
#endif

//=============================================================================
//
// Weapon Bottle tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFKnife, DT_TFWeaponKnife )

BEGIN_NETWORK_TABLE( CTFKnife, DT_TFWeaponKnife )
#if defined( CLIENT_DLL )
	RecvPropBool( RECVINFO( m_bReady ) ),
	RecvPropBool( RECVINFO( m_bBlood ) ),
#else
	SendPropBool( SENDINFO( m_bReady ) ),
	SendPropBool( SENDINFO( m_bBlood ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFKnife )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_knife, CTFKnife );
//PRECACHE_WEAPON_REGISTER( tf_weapon_knife );


IMPLEMENT_NETWORKCLASS_ALIASED( TFCKnife, DT_TFCWeaponKnife )

BEGIN_NETWORK_TABLE( CTFCKnife, DT_TFCWeaponKnife )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFCKnife )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tfc_weapon_knife, CTFCKnife );
//PRECACHE_WEAPON_REGISTER( tfc_weapon_knife );

IMPLEMENT_NETWORKCLASS_ALIASED(TFCombatKnife, DT_TFCombatWeaponKnife)

BEGIN_NETWORK_TABLE(CTFCombatKnife, DT_TFCombatWeaponKnife)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFCombatKnife)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_combatknife, CTFCombatKnife);
//PRECACHE_WEAPON_REGISTER( tf_weapon_combatknife );

//=============================================================================
//
// Weapon Knife functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFKnife::CTFKnife()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFKnife::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		m_bReady = false;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFKnife::ItemPostFrame( void )
{
	KnifeThink();

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: Set stealth attack bool
//-----------------------------------------------------------------------------
void CTFKnife::PrimaryAttack( void )
{
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );

	if ( !CanAttack() )
		return;

	// Set the weapon usage mode - primary, secondary.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

#if !defined (CLIENT_DLL)
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#endif

	trace_t trace;
	if ( DoSwingTrace( trace ) == true )
	{
		// we will hit something with the attack
		if ( trace.m_pEnt && trace.m_pEnt->IsPlayer() )
		{
			CTFPlayer *pTarget = ToTFPlayer( trace.m_pEnt );

			if ( pTarget && ( pTarget->GetTeamNumber() != pPlayer->GetTeamNumber() || pTarget->GetTeamNumber() == TF_TEAM_MERCENARY ) )
			{
				// Deal extra damage to players when stabbing them from behind
				if ( IsBehindAndFacingTarget( pTarget ) )
				{
					// this will be a backstab, do the strong anim
					m_iWeaponMode = TF_WEAPON_SECONDARY_MODE;

					// store the victim to compare when we do the damage
					m_hBackstabVictim = trace.m_pEnt;
				}
			}

		}
	}

	m_bReady = false;

#ifdef GAME_DLL
	pPlayer->RemoveInvisibility();
	pPlayer->RemoveDisguise();
#endif

#if !defined (CLIENT_DLL)
	lagcompensation->FinishLagCompensation( pPlayer );
#endif

	// Swing the weapon, instantly.
	Swing( pPlayer );
	Smack();
	m_flSmackTime = -1.0;

#if !defined( CLIENT_DLL ) 
	pPlayer->SpeakWeaponFire();
	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACritical() );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Do backstab damage
//-----------------------------------------------------------------------------
float CTFKnife::GetMeleeDamage( CBaseEntity *pTarget, int &iCustomDamage )
{
	float flBaseDamage = BaseClass::GetMeleeDamage( pTarget, iCustomDamage );

	if ( pTarget->IsPlayer() )
	{
		// This counts as a backstab if:
		// a ) we are behind the target player
		// or b) we were behind this target player when we started the stab

		// removed this check as knife hits instantly anyway
		//if ( IsBehindTarget( pTarget ) ||
		//	( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE && m_hBackstabVictim.Get() == pTarget ) )
		if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE && m_hBackstabVictim.Get() == pTarget )
		{
			// this will be a backstab, do the strong anim.
			// Do twice the target's health so that random modification will still kill him.
			flBaseDamage = pTarget->GetHealth() * 2;

			// Declare a backstab.
			iCustomDamage = TF_DMG_CUSTOM_BACKSTAB;

			m_bBlood = true;
			SwitchBodyGroups();
		}
		/*
		else
		{
			m_bCurrentAttackIsCrit = false;	// don't do a crit if we failed the above checks.
		}
		*/
	}

	return flBaseDamage;
}

//
// https://gist.github.com/sigsegv-mvm/bda5c53af428878af6889635cd787332/revisions
// reverse engineered from TF2 ServerLinux 20151007a by sigsegv
//

bool CTFKnife::IsBehindAndFacingTarget( CBaseEntity *pTarget )
{
	CTFPlayer *pTargetPlayer = ToTFPlayer(pTarget);
	if ( !pTargetPlayer ) 
		return false;

	Vector2D wsc_spy_to_victim = ( pTargetPlayer->WorldSpaceCenter() - GetOwner()->WorldSpaceCenter() ).AsVector2D();
	wsc_spy_to_victim.NormalizeInPlace();

	Vector temp1;
	
	pTargetPlayer->EyeVectors( &temp1 );
	Vector2D eye_spy = temp1.AsVector2D();
	eye_spy.NormalizeInPlace();

	Vector temp2; 
	
	pTargetPlayer->EyeVectors( &temp2 );
	Vector2D eye_victim = temp2.AsVector2D();
	eye_victim.NormalizeInPlace();

	if ( DotProduct2D( wsc_spy_to_victim, eye_victim ) <=  0.0f ) 
		return false;
	if ( DotProduct2D( wsc_spy_to_victim, eye_spy )    <=  0.5f ) 
		return false;
	if ( DotProduct2D( eye_spy,           eye_victim ) <= -0.3f ) 
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFKnife::SendWeaponAnim( int iActivity )
{
	if ( iActivity == ACT_VM_IDLE && m_bReady )
	{
		return BaseClass::SendWeaponAnim( ACT_BACKSTAB_VM_IDLE );
	}

	return BaseClass::SendWeaponAnim( iActivity );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFKnife::KnifeThink( void )
{
	CTFPlayer *pPlayer = GetTFPlayerOwner();

	if ( !pPlayer )
		return;

	if ( pPlayer->GetWaterLevel() == WL_Eyes )
	{
		m_bBlood = false;
		SwitchBodyGroups();
	}

	if ( GetActivity() == ACT_VM_IDLE || GetActivity() == ACT_BACKSTAB_VM_IDLE )
	{
		trace_t trace;
		if ( CanAttack() && DoSwingTrace( trace ) && trace.m_pEnt && trace.m_pEnt->IsPlayer() &&
			( trace.m_pEnt->GetTeamNumber() != pPlayer->GetTeamNumber() || pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY ) && IsBehindAndFacingTarget( trace.m_pEnt ) )
		{
			// we will hit something with the attack
			// Deal extra damage to players when stabbing them from behind
			if ( !m_bReady )
			{
				m_bReady = true;

				SendWeaponAnim( ACT_BACKSTAB_VM_UP );

			}
		}
		else
		{
			if ( m_bReady )
			{
				m_bReady = false;

				SendWeaponAnim( ACT_BACKSTAB_VM_DOWN );
			}
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFKnife::CalcIsAttackCriticalHelper( void )
{
	// Always crit from behind, never from front
	return ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE );
}

//-----------------------------------------------------------------------------
// Purpose: Allow melee weapons to send different anim events
// Input  :  - 
//-----------------------------------------------------------------------------
void CTFKnife::SendPlayerAnimEvent( CTFPlayer *pPlayer )
{
	if ( m_iWeaponMode == TF_WEAPON_SECONDARY_MODE )
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_MP_ATTACK_STAND_SECONDARYFIRE );
	}
	else
	{
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
	}
}


// blooood
void CTFKnife::WeaponReset( void )
{
	BaseClass::WeaponReset();

	m_bBlood = false;
}

bool CTFKnife::DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt )
{
	bool bRet = BaseClass::DefaultDeploy( szViewModel, szWeaponModel, iActivity, szAnimExt );

	if ( bRet )
	{
		SwitchBodyGroups();
	}

	return bRet;
}

void CTFKnife::SwitchBodyGroups( void )
{
	int iState = 0;

	if ( m_bBlood == true )
	{
		iState = 1;
	}

	SetBodygroup( 0, iState );

	CTFPlayer *pTFPlayer = ToTFPlayer( GetOwner() );

	if ( pTFPlayer && pTFPlayer->GetActiveWeapon() == this )
	{
		if ( pTFPlayer->GetViewModel() )
		{
			pTFPlayer->GetViewModel()->SetBodygroup( 0, iState );
		}
	}
}

void CTFKnife::Smack( void )
{
	BaseClass::Smack();

	if ( ConnectedHit() && IsCurrentAttackACritical() )
	{
		m_bBlood = true;
		SwitchBodyGroups();
	}
}
