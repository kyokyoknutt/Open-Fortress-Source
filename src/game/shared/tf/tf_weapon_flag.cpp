//====== Copyright © 1996-2006, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_flag.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
	#include "tf_team.h"
	#include "func_bomb_target.h"
	#include "tf_gamestats.h"
#endif

//=============================================================================
//
// Weapon Flag tables.
//
IMPLEMENT_NETWORKCLASS_ALIASED( TFFlag, DT_TFWeaponFlag )

BEGIN_NETWORK_TABLE( CTFFlag, DT_TFWeaponFlag )
// Client specific.
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bInPlantZone ) ),
	RecvPropBool( RECVINFO( m_bOldInPlantZone ) ),
	RecvPropBool( RECVINFO( m_bPlanting ) ),
	RecvPropBool( RECVINFO( m_bPlanted ) ),
	RecvPropTime( RECVINFO( m_flPlantStart ) ),
// Server specific.
#else
	SendPropBool( SENDINFO( m_bInPlantZone ) ),
	SendPropBool( SENDINFO( m_bOldInPlantZone ) ),
	SendPropBool( SENDINFO( m_bPlanting ) ),
	SendPropBool( SENDINFO( m_bPlanted ) ),
	SendPropTime( SENDINFO( m_flPlantStart ) ),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFFlag )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD( m_bInPlantZone, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bPlanting, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
#endif
#if defined( CLIENT_DLL )
	DEFINE_FIELD(m_bInPlantZone, FIELD_INTEGER ),
	DEFINE_FIELD(m_bPlanting, FIELD_INTEGER ),
	DEFINE_FIELD(m_bOldInPlantZone, FIELD_INTEGER ),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_c4, CTFFlag );
//PRECACHE_WEAPON_REGISTER( tf_weapon_c4 );

// Server specific.

BEGIN_DATADESC( CTFFlag )
DEFINE_FIELD( m_bInPlantZone, FIELD_BOOLEAN ),
DEFINE_FIELD( m_bOldInPlantZone, FIELD_BOOLEAN ),
DEFINE_FIELD( m_bPlanting, FIELD_BOOLEAN ),
END_DATADESC()

ConVar of_cs_blast_radius("of_cs_blast_radius", "1024", FCVAR_NONE);
ConVar of_cs_bomb_time("of_cs_bomb_time", "45", FCVAR_NONE);
ConVar of_cs_bomb_damage("of_cs_bomb_damage", "400", FCVAR_NONE);

#define BOMB_BLAST_RADIUS	of_cs_blast_radius.GetFloat()
#define BOMB_TICK_TIME		of_cs_bomb_time.GetFloat()
#define BOMB_DAMAGE			of_cs_bomb_damage.GetInt()

//=============================================================================
//
// Weapon Flag functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFFlag::CTFFlag()
{
	m_bPlanting = false;
	m_bOldInPlantZone = false;
	m_bPlanted = false;
	
	m_flExplodeTick = -1;
}

void CTFFlag::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
	
	if( pOwner )
		SetTeamNum( pOwner->GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFFlag::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
#ifdef GAME_DLL
        TFTeamMgr()->PlayerCenterPrint( ToTFPlayer( GetOwner() ), "#TF_Flag_AltFireToDrop" );
#endif
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFFlag::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( BaseClass::Holster( pSwitchingTo ) )
	{
		ResetBomb();
		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if( pPlayer )
			pPlayer->TeamFortress_SetSpeed();
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFFlag::PrimaryAttack( void )
{	
	// Get the player owning the weapon.
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );
	if ( !pPlayer )
		return;

	if ( !CanAttack() )
		return;
	
	if ( !m_bInPlantZone )
		return;
	
	if ( m_bPlanting = false || gpGlobals->curtime < m_flPlantStart + 3.0f )
		return;

#ifdef GAME_DLL

	pPlayer->SpeakWeaponFire();

	CTF_GameStats.Event_PlayerFiredWeapon( pPlayer, IsCurrentAttackACrit() );
#endif

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;
	
	pPlayer->SwitchToNextBestWeapon( this );

	pPlayer->TeamFortress_SetSpeed();
	
	PlantBomb();
	// Set the idle animation times based on the sequence duration, so that we play full fire animations
	// that last longer than the refire rate may allow.
}	

void CTFFlag::ItemPostFrame( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner( ) );
	if ( !pOwner )
		return;

#ifdef GAME_DLL	
	m_bInPlantZone = InBombTargetZone(pOwner->GetAbsOrigin());
#endif
	
	if ( m_bInPlantZone )
	{
		if( !m_bPlanting && pOwner->m_nButtons & IN_ATTACK )
		{
			m_bPlanting = true;
			m_flPlantStart = gpGlobals->curtime;
			SendWeaponAnim( ACT_VM_HITCENTER );
			SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
		}
		else if( m_bPlanting && !( pOwner->m_nButtons & IN_ATTACK ) )
		{
			ResetBomb();
		}

		if( m_bPlanting )
		{
			pOwner->SetMaxSpeed( 1 );
			if( gpGlobals->curtime >= m_flPlantStart + 3.0f )
			{
				PrimaryAttack();
				m_bPlanting = false;
				m_flPlantStart = gpGlobals->curtime;
			}
		}
	}
	else
	{
		ResetBomb();
	}	
	
	BaseClass::ItemPostFrame();	
}

void CTFFlag::ResetBomb( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );
	if ( !pOwner )
		return;

	m_bPlanting = false;
	m_flPlantStart = gpGlobals->curtime;
	pOwner->TeamFortress_SetSpeed();

	if( m_bOldInPlantZone )
	{
		SetWeaponIdleTime( gpGlobals->curtime );
		SendWeaponAnim( ACT_VM_IDLE );
		m_bOldInPlantZone = false;
	}
}

void CTFFlag::PlantBomb()
{
	m_bPlanted = true;
	
	CTFPlayer *pOwner = ToTFPlayer( GetPlayerOwner() );

	if( pOwner )
	{
#ifdef GAME_DLL
		pOwner->Weapon_Detach(this);

		WeaponHandle hHandle;
		hHandle = this;	
		if ( pOwner->m_hWeaponInSlot && pOwner->m_hWeaponInSlot[GetSlot()][GetPosition()] == hHandle )
			pOwner->m_hWeaponInSlot[GetSlot()][GetPosition()] = NULL;
#endif
	}

	Vector vecZero( 0,0,0 );
	Drop( vecZero );
#ifdef GAME_DLL
	SetExplodeTick( gpGlobals->curtime + BOMB_TICK_TIME );
	
	SetTouch(NULL);
	SetThink( &CTFFlag::PlantedThink );
	SetNextThink( gpGlobals->curtime + 0.1 );
	
	DrawRadius( BOMB_BLAST_RADIUS );
#endif
}

#ifdef GAME_DLL
void CTFFlag::PlantedThink()
{
	DevMsg("Peee\n");
	
	if( gpGlobals->curtime > m_flExplodeTick )
	{
		Explode();
		return;
	}

	// Increase timer accuracy when we're close to going boom
	if( gpGlobals->curtime + 1.0 > m_flExplodeTick )
		SetNextThink( gpGlobals->curtime );
	else
		SetNextThink( gpGlobals->curtime + 0.1 );
	
}
#endif

void CTFFlag::Explode()
{
#ifdef GAME_DLL

	UTIL_ScreenShake( GetAbsOrigin(), 2400.0f, 480.0f, 3, BOMB_BLAST_RADIUS * 1.5f, SHAKE_START );

	Vector vecSrc = GetAbsOrigin();
	Vector vecEnd = vecSrc;
	
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( pPlayer && pPlayer->IsAlive() )
		{
			vecEnd = pPlayer->GetAbsOrigin();
			float flDist = vecEnd.DistTo(vecSrc);
			if( flDist > BOMB_BLAST_RADIUS )
				continue;
			
			Vector vecCenter = CollisionProp()->WorldSpaceCenter();
			
			Vector vecDamagePos;
			pPlayer->CollisionProp()->CalcNearestPoint( vecCenter, &vecDamagePos );

			CTakeDamageInfo info( this, this, BOMB_DAMAGE * (1.0f - (flDist / BOMB_BLAST_RADIUS)), DMG_CLUB );
			info.SetDamagePosition( vecDamagePos );
			GuessDamageForce( &info, ( vecDamagePos - vecCenter ), vecDamagePos );
			
			pPlayer->TakeDamage( info );
		}
	}
#endif
}

#ifdef GAME_DLL
void CTFFlag::DrawRadius( float flRadius )
{
	Vector pos = GetAbsOrigin();
	int r = 255;
	int g = 0, b = 0;
	float flLifetime = BOMB_TICK_TIME + 5;
	bool bDepthTest = true;

	Vector edge, lastEdge;
	NDebugOverlay::Line( pos, pos + Vector( 0, 0, 50 ), r, g, b, !bDepthTest, flLifetime );

	lastEdge = Vector( flRadius + pos.x, pos.y, pos.z );
	float angle;
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = pos.x;
		edge.y = flRadius * cos( angle ) + pos.y;
		edge.z = flRadius * sin( angle ) + pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}

	lastEdge = Vector( pos.x, flRadius + pos.y, pos.z );
	for( angle=0.0f; angle <= 360.0f; angle += 22.5f )
	{
		edge.x = flRadius * cos( angle ) + pos.x;
		edge.y = flRadius * sin( angle ) + pos.y;
		edge.z = pos.z;

		NDebugOverlay::Line( edge, lastEdge, r, g, b, !bDepthTest, flLifetime );

		lastEdge = edge;
	}
}


void CTFFlag::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( m_bPlanted )
		DevMsg("Poo\n");
	else
		DevMsg("Unpoo\n");
}
#endif