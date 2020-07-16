//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_gamerules.h"
#include "entity_capture_flag.h"
#include "baseobject_shared.h"
#include "tf_weapon_medigun.h"
#include "tf_weapon_pipebomblauncher.h"
#include "in_buttons.h"
#include "fmtstr.h"
#include "tf_viewmodel.h"
#include "tf_weapon_fists.h"

#ifdef CLIENT_DLL
	#include "c_tf_playerclass.h"
	#include "iviewrender.h"

	#define CTFPlayerClass C_TFPlayerClass
#else
	#include "te_effect_dispatch.h"
	#include "tf_gamestats.h"
	#include "tf_weapon_builder.h"
#endif

ConVar tf_spy_invis_time( "tf_spy_invis_time", "1.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Transition time in and out of spy invisibility", true, 0.1, true, 5.0 );
ConVar tf_spy_invis_unstealth_time( "tf_spy_invis_unstealth_time", "2.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Transition time in and out of spy invisibility", true, 0.1, true, 5.0 );

ConVar tf_spy_max_cloaked_speed( "tf_spy_max_cloaked_speed", "999", FCVAR_CHEAT | FCVAR_REPLICATED );	// no cap
ConVar tf_max_health_boost( "tf_max_health_boost", "1.5", FCVAR_CHEAT | FCVAR_REPLICATED, "Max health factor that players can be boosted to by healers.", true, 1.0, false, 0 );
ConVar of_dm_max_health_boost( "of_dm_max_health_boost", "2", FCVAR_CHEAT | FCVAR_REPLICATED, "Max health factor that players can be boosted to by pills and megahealths.", true, 1.0, false, 0 );
ConVar tf_invuln_time( "tf_invuln_time", "1.0", FCVAR_CHEAT | FCVAR_REPLICATED, "Time it takes for invulnerability to wear off." );

#ifdef GAME_DLL
ConVar tf_boost_drain_time( "tf_boost_drain_time", "15.0", FCVAR_CHEAT, "Time is takes for a full health boost to drain away from a player.", true, 0.1, false, 0 );
ConVar of_dm_boost_drain_time( "of_dm_boost_drain_time", "60.0", FCVAR_CHEAT, "Time is takes for a full pill health boost to drain away from a player in DM.", true, 0.1, false, 0 );
ConVar tf_debug_bullets( "tf_debug_bullets", "0", FCVAR_CHEAT, "Visualize bullet traces." );
ConVar tf_damage_events_track_for( "tf_damage_events_track_for", "30",  FCVAR_CHEAT );
#else
extern ConVar cl_quickzoom;
#endif
ConVar of_zombie_lunge_delay( "of_zombie_lunge_delay", "10", FCVAR_REPLICATED | FCVAR_NOTIFY, "How much delay, in seconds, before a zombie can lunge again." );

ConVar tf_useparticletracers( "tf_useparticletracers", "1", FCVAR_CHEAT | FCVAR_REPLICATED, "Use particle tracers instead of old style ones." );
ConVar tf_spy_cloak_consume_rate( "tf_spy_cloak_consume_rate", "10.0", FCVAR_CHEAT | FCVAR_REPLICATED, "cloak to use per second while cloaked, from 100 max )" );	// 10 seconds of invis
ConVar tf_spy_cloak_regen_rate( "tf_spy_cloak_regen_rate", "3.3", FCVAR_CHEAT | FCVAR_REPLICATED, "cloak to regen per second, up to 100 max" );		// 30 seconds to full charge
ConVar tf_spy_cloak_no_attack_time( "tf_spy_cloak_no_attack_time", "2.0", FCVAR_CHEAT | FCVAR_REPLICATED, "time after uncloaking that the spy is prohibited from attacking" );

//ConVar tf_spy_stealth_blink_time( "tf_spy_stealth_blink_time", "0.3", FCVAR_DEVELOPMENTONLY, "time after being hit the spy blinks into view" );
//ConVar tf_spy_stealth_blink_scale( "tf_spy_stealth_blink_scale", "0.85", FCVAR_DEVELOPMENTONLY, "percentage visible scalar after being hit the spy blinks into view" );

ConVar tf_damage_disablespread( "tf_damage_disablespread", "1", FCVAR_NOTIFY | FCVAR_REPLICATED, "Toggles the random damage spread applied to all player damage." );

ConVar of_allowteams( "of_allowteams", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Allow RED and BLU in DM." );
ConVar of_berserk_speed( "of_berserk_speed", "380", FCVAR_REPLICATED | FCVAR_NOTIFY, "Running speed while in berserk mode." );
ConVar of_berserk_speed_factor( "of_berserk_speed_factor", "1.33", FCVAR_REPLICATED | FCVAR_NOTIFY, "Running speed while in berserk mode. (factor mode)");
ConVar of_berserk_speed_mode( "of_berserk_speed_mode", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "TESTING Switch between strict mode and factor mode");
ConVar of_spawnprotecttime( "of_spawnprotecttime", "3", FCVAR_REPLICATED | FCVAR_NOTIFY , "How long the spawn protection lasts." );

ConVar of_allow_special_teams( "of_allow_special_teams", "0", FCVAR_REPLICATED | FCVAR_NOTIFY , "Allow special teams outside their gamemodes." );

extern ConVar of_infiniteammo;
extern ConVar of_dynamic_color_update;

ConVar of_infection_allow_sentry	 ( "of_infection_allow_sentry", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Allow Engineer to build sentries in Infection?" );
ConVar of_infection_allow_dispenser	 ( "of_infection_allow_dispenser", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Allow Engineer to build dispensers in Infection?" );
ConVar of_infection_allow_teleporter ( "of_infection_allow_teleporter", "1", FCVAR_REPLICATED | FCVAR_NOTIFY, "Allow Engineer to build teleporters in Infection?" );

ConVar of_haste_movespeed_multplier("of_haste_movespeed_multplier", "1.5",FCVAR_REPLICATED | FCVAR_NOTIFY, "By how much move speed should be multiplied when in Haste.");

#define TF_SPY_STEALTH_BLINKTIME   0.3f
#define TF_SPY_STEALTH_BLINKSCALE  0.85f

#define COND_FIRST_POWERUP TF_COND_BERSERK
#define COND_LAST_POWERUP TF_COND_JAUGGERNAUGHT

#define TF_PLAYER_CONDITION_CONTEXT	"TFPlayerConditionContext"

#define MAX_DAMAGE_EVENTS		128

const char *g_pszBDayGibs[22] = 
{
	"models/effects/bday_gib01.mdl",
	"models/effects/bday_gib02.mdl",
	"models/effects/bday_gib03.mdl",
	"models/effects/bday_gib04.mdl",
	"models/player/gibs/gibs_balloon.mdl",
	"models/player/gibs/gibs_burger.mdl",
	"models/player/gibs/gibs_boot.mdl",
	"models/player/gibs/gibs_bolt.mdl",
	"models/player/gibs/gibs_can.mdl",
	"models/player/gibs/gibs_clock.mdl",
	"models/player/gibs/gibs_fish.mdl",
	"models/player/gibs/gibs_gear1.mdl",
	"models/player/gibs/gibs_gear2.mdl",
	"models/player/gibs/gibs_gear3.mdl",
	"models/player/gibs/gibs_gear4.mdl",
	"models/player/gibs/gibs_gear5.mdl",
	"models/player/gibs/gibs_hubcap.mdl",
	"models/player/gibs/gibs_licenseplate.mdl",
	"models/player/gibs/gibs_spring1.mdl",
	"models/player/gibs/gibs_spring2.mdl",
	"models/player/gibs/gibs_teeth.mdl",
	"models/player/gibs/gibs_tire.mdl"
};

//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

BEGIN_RECV_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerSharedLocal )
	RecvPropInt( RECVINFO( m_nDesiredDisguiseTeam ) ),
	RecvPropInt( RECVINFO( m_nDesiredDisguiseClass ) ),
	RecvPropTime( RECVINFO( m_flStealthNoAttackExpire ) ),
	RecvPropTime( RECVINFO( m_flStealthNextChangeTime ) ),
	RecvPropTime( RECVINFO( m_flNextLungeTime ) ),
	RecvPropBool( RECVINFO( m_bIsLunging ) ),
	RecvPropTime( RECVINFO( m_flNextZoomTime ) ),	
	RecvPropFloat( RECVINFO( m_flCloakMeter) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominated ), RecvPropBool( RECVINFO( m_bPlayerDominated[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_bPlayerDominatingMe ), RecvPropBool( RECVINFO( m_bPlayerDominatingMe[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY( m_flCondExpireTimeLeft ), RecvPropFloat( RECVINFO( m_flCondExpireTimeLeft[0] ) ) ),
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerShared )
	RecvPropInt( RECVINFO( m_nPlayerCond ) ),
	RecvPropInt( RECVINFO( m_nPlayerCondEx ) ),
	RecvPropInt( RECVINFO( m_nPlayerCondEx2 ) ),
	RecvPropInt( RECVINFO( m_nPlayerCondEx3 ) ),
	RecvPropInt( RECVINFO( m_nPlayerCondEx4 ) ),
	RecvPropInt( RECVINFO( m_bJumping) ),
	RecvPropBool( RECVINFO( m_bIsTopThree ) ),
	RecvPropBool( RECVINFO( bWatchReady ) ),
	RecvPropBool( RECVINFO( m_bIsZombie ) ),
	RecvPropInt( RECVINFO( m_nNumHealers ) ),
	RecvPropInt( RECVINFO( m_iCritMult) ),
	RecvPropInt( RECVINFO( m_bAirDash) ),
	RecvPropInt( RECVINFO( m_iAirDashCount) ),
	RecvPropFloat( RECVINFO( m_flGHookProp ) ),
	RecvPropInt( RECVINFO( m_nPlayerState ) ),
	RecvPropInt( RECVINFO( m_iDesiredPlayerClass ) ),
	RecvPropInt( RECVINFO( m_iRespawnEffect ) ),
	RecvPropTime( RECVINFO( m_flMegaOverheal ) ),
	// Spy.
	RecvPropTime( RECVINFO( m_flInvisChangeCompleteTime ) ),
	RecvPropInt( RECVINFO( m_nDisguiseTeam ) ),
	RecvPropInt( RECVINFO( m_nDisguiseClass ) ),
	RecvPropInt( RECVINFO( m_iDisguiseTargetIndex ) ),
	RecvPropInt( RECVINFO( m_iDisguiseHealth ) ),
	// Local Data.
	RecvPropDataTable( "tfsharedlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_TFPlayerSharedLocal) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( CTFPlayerShared )
	DEFINE_PRED_FIELD( m_nPlayerState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCond, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx2, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx3, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nPlayerCondEx4, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flCloakMeter, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bJumping, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bIsTopThree, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( bWatchReady, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bIsZombie, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bAirDash, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iAirDashCount, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flGHookProp, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flMegaOverheal, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iRespawnEffect, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flNextZoomTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

// Server specific.
#else

BEGIN_SEND_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerSharedLocal )
	SendPropInt( SENDINFO( m_nDesiredDisguiseTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDesiredDisguiseClass ), 4, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flStealthNoAttackExpire ) ),
	SendPropTime( SENDINFO( m_flStealthNextChangeTime ) ),
	SendPropTime( SENDINFO( m_flNextLungeTime ) ),
	SendPropBool( SENDINFO( m_bIsLunging ) ),
	SendPropTime( SENDINFO( m_flNextZoomTime ) ),	
	SendPropFloat( SENDINFO( m_flCloakMeter ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN, 0.0, 100.0 ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominated ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominated ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_bPlayerDominatingMe ), SendPropBool( SENDINFO_ARRAY( m_bPlayerDominatingMe ) ) ),
	SendPropArray3( SENDINFO_ARRAY3( m_flCondExpireTimeLeft ), SendPropFloat( SENDINFO_ARRAY( m_flCondExpireTimeLeft ) ) ),
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CTFPlayerShared, DT_TFPlayerShared )
	SendPropInt( SENDINFO( m_nPlayerCond ), -1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nPlayerCondEx ), -1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nPlayerCondEx2 ), -1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nPlayerCondEx3 ), -1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nPlayerCondEx4 ), -1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_bJumping ), 1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_bIsTopThree ), 1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( bWatchReady ), 1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_bIsZombie ), 1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_nNumHealers ), 5, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iCritMult ), 8, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_bAirDash ), 1, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO( m_iAirDashCount ), 8, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN ),
	SendPropFloat( SENDINFO( m_flGHookProp ), 0, SPROP_NOSCALE | SPROP_CHANGES_OFTEN ), 
	SendPropInt( SENDINFO( m_nPlayerState ), Q_log2( TF_STATE_COUNT )+1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDesiredPlayerClass ), Q_log2( TF_CLASS_COUNT_ALL )+1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iRespawnEffect ), -1, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flMegaOverheal ) ),
	// Spy
	SendPropTime( SENDINFO( m_flInvisChangeCompleteTime ) ),
	SendPropInt( SENDINFO( m_nDisguiseTeam ), 3, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nDisguiseClass ), 4, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDisguiseTargetIndex ), 7, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iDisguiseHealth ), 10 ),
	// Local Data.
	SendPropDataTable( "tfsharedlocaldata", 0, &REFERENCE_SEND_TABLE( DT_TFPlayerSharedLocal ), SendProxy_SendLocalDataTable ),	
END_SEND_TABLE()

#endif

// --------------------------------------------------------------------------------------------------- //
// Shared CTFPlayer implementation.
// --------------------------------------------------------------------------------------------------- //

// --------------------------------------------------------------------------------------------------- //
// CTFPlayerShared implementation.
// --------------------------------------------------------------------------------------------------- //

CTFPlayerShared::CTFPlayerShared()
{
	m_nPlayerState.Set( TF_STATE_WELCOME );
	m_bJumping = false;
	m_bIsTopThree = false,
	bWatchReady = false;
	m_bIsZombie = false,
	m_bAirDash = false;
	m_iAirDashCount = 0;
	m_bBlockJump = false;
	m_Hook = NULL;
	m_flGHookProp = 0.f;
	m_flStealthNoAttackExpire = 0.0f;
	m_flStealthNextChangeTime = 0.0f;
	m_flNextLungeTime = 0.0f;
	m_bIsLunging = false;
	m_flNextZoomTime = 0.0f;
	m_iCritMult = 0;
	m_flInvisibility = 0.0f;

	m_flStepSoundDelay = 0.f;
	m_flJumpSoundDelay = 0.f;
	
	m_iDesiredPlayerClass = 0;

#ifdef CLIENT_DLL
	m_iDisguiseWeaponModelIndex = -1;
	m_pDisguiseWeaponInfo = NULL;
#endif

	m_iJauggernaughtOldClass = TF_CLASS_UNDEFINED;
}

void CTFPlayerShared::Init( CTFPlayer *pPlayer )
{
	m_pOuter = pPlayer;

	m_flNextBurningSound = 0;

	SetJumping( false );
}

//-----------------------------------------------------------------------------
// Purpose: Add a condition and duration
// duration of PERMANENT_CONDITION means infinite duration
//-----------------------------------------------------------------------------
void CTFPlayerShared::AddCond( int nCond, float flDuration /* = PERMANENT_CONDITION */ )
{
	Assert( nCond >= 0 && nCond < TF_COND_LAST );

	int nCond2 = nCond;
	int *pCond2 = NULL;

	// ...... bruh

	// how this works: we store the appropiate m_nPlayerCond into pCond2, 
	// and then pCond2 gets its values transferred back to the m_nPlayerCond
	if ( nCond < 128 )
	{
		if ( nCond < 96 )
		{
			if ( nCond < 64 )
			{
				if ( nCond < 32 )
					pCond2 = &m_nPlayerCond.GetForModify();
				else
				{
					pCond2 = &m_nPlayerCondEx.GetForModify();
					nCond2 -= 32;
				}
			}
			else
			{
				pCond2 = &m_nPlayerCondEx2.GetForModify();
				nCond2 -= 64;
			}
		}
		else
		{
			pCond2 = &m_nPlayerCondEx3.GetForModify();
			nCond2 -= 96;
		}
	}
	else
	{
		pCond2 = &m_nPlayerCondEx4.GetForModify();
		nCond2 -= 128;
	}

	*pCond2 |= ( 1 << nCond2 );

	m_flCondExpireTimeLeft.Set( nCond, flDuration );

	OnConditionAdded( nCond );
}

//-----------------------------------------------------------------------------
// Purpose: Forcibly remove a condition
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveCond( int nCond )
{
	Assert( nCond >= 0 && nCond < TF_COND_LAST );

	int nCond2 = nCond;
	int *pCond2 = NULL;

	// ...... bruh

	// how this works: we store the appropiate m_nPlayerCond into pCond2, 
	// and then pCond2 gets its values transferred back to the m_nPlayerCond
	if ( nCond < 128 )
	{
		if ( nCond < 96 )
		{
			if ( nCond < 64 )
			{
				if ( nCond < 32 )
					pCond2 = &m_nPlayerCond.GetForModify();
				else
				{
					pCond2 = &m_nPlayerCondEx.GetForModify();
					nCond2 -= 32;
				}
			}
			else
			{
				pCond2 = &m_nPlayerCondEx2.GetForModify();
				nCond2 -= 64;
			}
		}
		else
		{
			pCond2 = &m_nPlayerCondEx3.GetForModify();
			nCond2 -= 96;
		}
	}
	else
	{
		pCond2 = &m_nPlayerCondEx4.GetForModify();
		nCond2 -= 128;
	}

	*pCond2 &= ~( 1 << nCond2 );

	m_flCondExpireTimeLeft.Set( nCond, 0 );

	OnConditionRemoved( nCond );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::InCond( int nCond )
{
	Assert(nCond >= 0 && nCond < TF_COND_LAST);

	int nCond2 = nCond;
	const int *pCond2 = NULL;

	// ...... bruh

	// how this works: we store the appropiate m_nPlayerCond into pCond2, 
	// and then pCond2 gets its values transferred back to the m_nPlayerCond
	if ( nCond < 128 )
	{
		if ( nCond < 96 )
		{
			if ( nCond < 64 )
			{
				if ( nCond < 32 )
					pCond2 = &m_nPlayerCond.GetForModify();
				else
				{
					pCond2 = &m_nPlayerCondEx.GetForModify();
					nCond2 -= 32;
				}
			}
			else
			{
				pCond2 = &m_nPlayerCondEx2.GetForModify();
				nCond2 -= 64;
			}
		}
		else
		{
			pCond2 = &m_nPlayerCondEx3.GetForModify();
			nCond2 -= 96;
		}
	}
	else
	{
		pCond2 = &m_nPlayerCondEx4.GetForModify();
		nCond2 -= 128;
	}

	return ( ( *pCond2 & ( 1 << nCond2 ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetConditionDuration( int nCond )
{
	Assert( nCond >= 0 && nCond < TF_COND_LAST );

	if ( InCond( nCond ) )
	{
		return m_flCondExpireTimeLeft[nCond];
	}
	
	return 0.0f;
}

void CTFPlayerShared::DebugPrintConditions( void )
{
#ifdef GAME_DLL
	const char *szDll = "Server";
#else
	const char *szDll = "Client";
#endif

	Msg( "( %s ) Conditions for player ( %d )\n", szDll, m_pOuter->entindex() );

	int i;
	int iNumFound = 0;
	for ( i=0;i<TF_COND_LAST;i++ )
	{
		if ( InCond(i) )
		{
			if ( m_flCondExpireTimeLeft[i] == PERMANENT_CONDITION )
			{
				Msg( "( %s ) Condition %d - ( permanent cond )\n", szDll, i );
			}
			else
			{
				Msg( "( %s ) Condition %d - ( %.1f left )\n", szDll, i, m_flCondExpireTimeLeft[i] );
			}

			iNumFound++;
		}
	}

	if ( iNumFound == 0 )
	{
		Msg( "( %s ) No active conditions\n", szDll );
	}
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnPreDataChanged( void )
{
	m_nOldConditions = m_nPlayerCond;
	m_nOldConditionsEx = m_nPlayerCondEx;
	m_nOldConditionsEx2 = m_nPlayerCondEx2;
	m_nOldConditionsEx3 = m_nPlayerCondEx3;
	m_nOldConditionsEx4 = m_nPlayerCondEx4;

	m_nOldDisguiseClass = GetDisguiseClass();
	m_iOldDisguiseWeaponModelIndex = m_iDisguiseWeaponModelIndex;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnDataChanged( void )
{
	// Update conditions from last network change

	// 1 is cond, 2 is old cond, 3 is the offset of 32
	UpdateConditions( m_nPlayerCond, m_nOldConditions, 0 );
	UpdateConditions( m_nPlayerCondEx, m_nOldConditionsEx, 32 );
	UpdateConditions( m_nPlayerCondEx2, m_nOldConditionsEx2, 64 );
	UpdateConditions( m_nPlayerCondEx3, m_nOldConditionsEx3, 96 );
	UpdateConditions( m_nPlayerCondEx4, m_nOldConditionsEx4, 128 );

	m_nOldConditions = m_nPlayerCond;
	m_nOldConditionsEx = m_nPlayerCondEx;
	m_nOldConditionsEx2 = m_nPlayerCondEx2;
	m_nOldConditionsEx3 = m_nPlayerCondEx3;
	m_nOldConditionsEx4 = m_nPlayerCondEx4;

	if ( m_iDisguiseWeaponModelIndex != m_iOldDisguiseWeaponModelIndex )
	{
		C_BaseCombatWeapon *pWeapon = m_pOuter->GetActiveWeapon();

		if ( pWeapon )
		{
			pWeapon->SetModelIndex( pWeapon->GetWorldModelIndex() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: check the newly networked conditions for changes
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateConditions( int nCond, int nCondOld, int nCondOffset )
{
	if ( nCondOld == nCond )
		return;

	int nCondChanged = nCond ^ nCondOld;
	int nCondAdded = nCondChanged & nCond;
	int nCondRemoved = nCondChanged & nCondOld;

	int i;
	for ( i=0;i<32;i++ ) // safely presume its within 32 now, with no offsets
	{
		if ( nCondAdded & (1<<i) )
		{
			OnConditionAdded( i + nCondOffset ); // ... and offset it again
		}
		else if ( nCondRemoved & (1<<i) )
		{
			OnConditionRemoved( i + nCondOffset ); // ... and offset it again
		}
	}
}

#endif // CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: Remove any conditions affecting players
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveAllCond(CTFPlayer *pPlayer)
{
	int i;
	for (i = 0; i < TF_COND_LAST; i++)
	{
		if ( InCond( i ) )
			RemoveCond( i );
	}

	// Now remove all the rest
	m_nPlayerCond = 0;
	m_nPlayerCondEx = 0;
	m_nPlayerCondEx2 = 0;
	m_nPlayerCondEx3 = 0;
	m_nPlayerCondEx4 = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we add the bit,
// and client when it recieves the new cond bits and finds one added
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnConditionAdded( int nCond )
{
	switch( nCond )
	{
	case TF_COND_HEALTH_BUFF:
#ifdef GAME_DLL
		m_flHealFraction = 0;
		m_flDisguiseHealFraction = 0;
#endif
		break;

	case TF_COND_STEALTHED:
	case TF_COND_INVIS_POWERUP:
		OnAddStealthed();
		break;

	case TF_COND_INVULNERABLE:
		OnAddInvulnerable();
		break;
		
	case TF_COND_SPAWNPROTECT:
		OnAddInvulnerable();
		break;

	case TF_COND_SHIELD:
		OnAddShield();
		break;
		
	case TF_COND_TELEPORTED:
		OnAddTeleported();
		break;

	case TF_COND_BURNING:
		OnAddBurning();
		break;

	case TF_COND_DISGUISING:
		OnAddDisguising();
		break;

	case TF_COND_DISGUISED:
		OnAddDisguised();
		break;

	case TF_COND_TAUNTING:
		{
			CTFWeaponBase *pWpn = m_pOuter->GetActiveTFWeapon();
			if ( pWpn )
			{
				// cancel any reload in progress.
				pWpn->AbortReload();
			}
			m_pOuter->TeamFortress_SetSpeed();
		}
		break;

	case TF_COND_CRITBOOSTED:
	case TF_COND_CRIT_POWERUP:
	case TF_COND_CRITBOOSTED_DEMO_CHARGE:
		OnAddCritBoosted();
		break;

	case TF_COND_BERSERK:
		OnAddBerserk();
		break;		

	case TF_COND_SHIELD_CHARGE:
		OnAddShieldCharge();
		break;	

	case TF_COND_HASTE:
		OnAddHaste();
		break;

	case TF_COND_JAUGGERNAUGHT:
		OnAddJauggernaught();
		break;

	case TF_COND_POISON:
		OnAddPoison();
		break;

	case TF_COND_TRANQ:
		OnAddTranq();
		break;

	default:
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on both client and server. Server when we remove the bit,
// and client when it recieves the new cond bits and finds one removed
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnConditionRemoved( int nCond )
{
	switch( nCond )
	{
	case TF_COND_ZOOMED:
		OnRemoveZoomed();
		break;

	case TF_COND_BURNING:
		OnRemoveBurning();
		break;

	case TF_COND_HEALTH_BUFF:
#ifdef GAME_DLL
		m_flHealFraction = 0;
		m_flDisguiseHealFraction = 0;
#endif
		break;

	case TF_COND_STEALTHED:
	case TF_COND_INVIS_POWERUP:
		OnRemoveStealthed();
		break;

	case TF_COND_DISGUISED:
		OnRemoveDisguised();
		break;

	case TF_COND_DISGUISING:
		OnRemoveDisguising();
		break;

	case TF_COND_INVULNERABLE:
		OnRemoveInvulnerable();
		break;
		
	case TF_COND_SPAWNPROTECT:
		OnRemoveInvulnerable();
		break;
	
	case TF_COND_SHIELD:
		OnRemoveShield();
		break;	
	
	case TF_COND_TAUNTING:
		OnRemoveTaunting();
		break;	
		
	case TF_COND_TELEPORTED:
		OnRemoveTeleported();
		break;

	case TF_COND_CRITBOOSTED:
	case TF_COND_CRIT_POWERUP:
	case TF_COND_CRITBOOSTED_DEMO_CHARGE:
		OnRemoveCritBoosted();
		break;

	case TF_COND_BERSERK:
		OnRemoveBerserk();
		break;

	case TF_COND_SHIELD_CHARGE:
		OnRemoveShieldCharge();
		break;

	case TF_COND_HASTE:
		OnRemoveHaste();
		break;

	case TF_COND_JAUGGERNAUGHT:
		OnRemoveJauggernaught();
		break;

	case TF_COND_POISON:
		OnRemovePoison();
		break;

	case TF_COND_TRANQ:
		OnRemoveTranq();
		break;

	default:
		break;
	}
}

int CTFPlayerShared::GetMaxBuffedHealth( void )
{
	float flBoostMax;

	flBoostMax = m_pOuter->GetPlayerClass()->GetMaxHealth() * tf_max_health_boost.GetFloat();

	int iRoundDown = floor( flBoostMax / 5 );
	iRoundDown = iRoundDown * 5;

	return iRoundDown;
}

int CTFPlayerShared::GetMaxBuffedHealthDM( void )
{
	float flBoostMax;

	flBoostMax = m_pOuter->GetPlayerClass()->GetMaxHealth() * of_dm_max_health_boost.GetFloat();

	int iRoundDown = floor( flBoostMax / 5 );
	iRoundDown = iRoundDown * 5;

	return iRoundDown;
}

int CTFPlayerShared::GetDefaultHealth( void )
{
	return m_pOuter->GetPlayerClass()->GetMaxHealth();
}

//-----------------------------------------------------------------------------
// Purpose: Runs SERVER SIDE only Condition Think
// If a player needs something to be updated no matter what do it here (invul, etc).
//-----------------------------------------------------------------------------
void CTFPlayerShared::ConditionGameRulesThink( void )
{
#ifdef GAME_DLL
	if ( m_flNextCritUpdate < gpGlobals->curtime )
	{
		UpdateCritMult();
		m_flNextCritUpdate = gpGlobals->curtime + 0.5;
	}

	int i;
	for ( i=0;i<TF_COND_LAST;i++ )
	{
		if ( InCond( i ) )
		{
			// Ignore permanent conditions
			if ( m_flCondExpireTimeLeft[i] != PERMANENT_CONDITION )
			{
				float flReduction = gpGlobals->frametime;

				// If we're being healed, we reduce bad conditions faster
				if ( i > TF_COND_HEALTH_BUFF && m_aHealers.Count() > 0 )
				{
					flReduction += (m_aHealers.Count() * flReduction * 4);
				}

				m_flCondExpireTimeLeft.Set( i, max( m_flCondExpireTimeLeft[i] - flReduction, 0 ) );

				if ( m_flCondExpireTimeLeft[i] == 0 )
				{
					RemoveCond( i );
				}
			}
		}
	}

	// Our health will only decay ( from being medic buffed ) if we are not being healed by a medic
	// Dispensers can give us the TF_COND_HEALTH_BUFF, but will not maintain or give us health above 100%s
	bool bDecayHealth = true;

	// If we're being healed, heal ourselves
	if ( InCond( TF_COND_HEALTH_BUFF ) )
	{
		// Heal faster if we haven't been in combat for a while
		float flTimeSinceDamage = gpGlobals->curtime - m_pOuter->GetLastDamageTime();
		float flScale = RemapValClamped( flTimeSinceDamage, 10, 15, 1.0, 3.0 );

		bool bHasFullHealth = m_pOuter->GetHealth() >= m_pOuter->GetMaxHealth();

		float fTotalHealAmount = 0.0f;
		for ( int i = 0; i < m_aHealers.Count(); i++ )
		{
			Assert( m_aHealers[i].pPlayer );

			// Dispensers don't heal above 100%
			if ( bHasFullHealth && m_aHealers[i].bDispenserHeal )
			{
				continue;
			}

			// Being healed by a medigun, don't decay our health
			bDecayHealth = false;

			// Dispensers heal at a constant rate
			if ( m_aHealers[i].bDispenserHeal )
			{
				// Dispensers heal at a slower rate, but ignore flScale
				m_flHealFraction += gpGlobals->frametime * m_aHealers[i].flAmount;
			}
			else	// player heals are affected by the last damage time
			{
				m_flHealFraction += gpGlobals->frametime * m_aHealers[i].flAmount * flScale;
			}

			fTotalHealAmount += m_aHealers[i].flAmount;
		}

		int nHealthToAdd = (int)m_flHealFraction;
		if ( nHealthToAdd > 0 )
		{
			m_flHealFraction -= nHealthToAdd;

			int iBoostMax = GetMaxBuffedHealth() + m_flMegaOverheal;
			if ( iBoostMax > GetMaxBuffedHealthDM() )
				iBoostMax = GetMaxBuffedHealthDM();
			
			if ( InCond( TF_COND_DISGUISED ) )
			{
				// Separate cap for disguised health
				int nFakeHealthToAdd = clamp( nHealthToAdd, 0, iBoostMax - m_iDisguiseHealth );
				m_iDisguiseHealth += nFakeHealthToAdd;
			}

			// Cap it to the max we'll boost a player's health
			nHealthToAdd = clamp( nHealthToAdd, 0, iBoostMax - m_pOuter->GetHealth() );
			
			m_pOuter->TakeHealth( nHealthToAdd, DMG_IGNORE_MAXHEALTH );

			// split up total healing based on the amount each healer contributes
			for ( int i = 0; i < m_aHealers.Count(); i++ )
			{
				Assert( m_aHealers[i].pPlayer );
				if ( m_aHealers[i].pPlayer.IsValid () )
				{
					CTFPlayer *pPlayer = static_cast<CTFPlayer *>( static_cast<CBaseEntity *>( m_aHealers[i].pPlayer ) );
					if ( IsAlly( pPlayer ) )
					{
						CTF_GameStats.Event_PlayerHealedOther( pPlayer, nHealthToAdd * ( m_aHealers[i].flAmount / fTotalHealAmount ) );
					}
					else
					{
						CTF_GameStats.Event_PlayerLeachedHealth( m_pOuter, m_aHealers[i].bDispenserHeal, nHealthToAdd * ( m_aHealers[i].flAmount / fTotalHealAmount ) );
					}
				}
			}
		}

		// Reduce the duration of this burn
		if ( InCond( TF_COND_BURNING ) )
			m_flFlameRemoveTime -= 2.f * gpGlobals->frametime;  // ( flReduction + 1 ) x faster reduction

		// Reduce the duration of this poison
		if ( InCond( TF_COND_POISON ) )
			m_flPoisonRemoveTime -= 2.f * gpGlobals->frametime;
	}

	if ( bDecayHealth )
	{
		// If we're not being buffed, our health drains back to our max
		if ( m_pOuter->GetHealth() > m_pOuter->GetMaxHealth() )
		{
			float flBoostMaxAmount = GetMaxBuffedHealth() - m_pOuter->GetMaxHealth();
			float flDrainTime = (m_pOuter->GetHealth() <= GetDefaultHealth() + m_flMegaOverheal) ? of_dm_boost_drain_time.GetFloat() : tf_boost_drain_time.GetFloat();
			m_flHealFraction += (gpGlobals->frametime * (flBoostMaxAmount / flDrainTime));

			int nHealthToDrain = (int)m_flHealFraction;
			if ( nHealthToDrain > 0 )
			{
				m_flHealFraction -= nHealthToDrain;
				// Drain our Pill overheal if thats the only thing left
				if( flDrainTime == of_dm_boost_drain_time.GetFloat() )
					m_flMegaOverheal -= nHealthToDrain;

				// Manually subtract the health so we don't generate pain sounds / etc
				m_pOuter->m_iHealth -= nHealthToDrain;
			}
		}

		if ( InCond( TF_COND_DISGUISED ) && m_iDisguiseHealth > m_pOuter->GetMaxHealth() )
		{
			float flBoostMaxAmount = GetMaxBuffedHealth() - m_pOuter->GetMaxHealth();
			m_flDisguiseHealFraction += (gpGlobals->frametime * (flBoostMaxAmount / tf_boost_drain_time.GetFloat()));

			int nHealthToDrain = (int)m_flDisguiseHealFraction;
			if ( nHealthToDrain > 0 )
			{
				m_flDisguiseHealFraction -= nHealthToDrain;
				// Reduce our fake disguised health by roughly the same amount
				m_iDisguiseHealth -= nHealthToDrain;
			}
		}
	}

	// Taunt
	if ( InCond( TF_COND_TAUNTING ) )
	{
		if ( gpGlobals->curtime > m_flTauntRemoveTime )
		{
			m_pOuter->ResetTauntHandle();

			m_pOuter->SnapEyeAngles( m_pOuter->m_angTauntCamera );
			m_pOuter->SetAbsAngles( m_pOuter->m_angTauntCamera );
			m_pOuter->SetLocalAngles( m_pOuter->m_angTauntCamera );

			RemoveCond( TF_COND_TAUNTING );
		}
	}

	if ( InCond( TF_COND_BURNING ) /*&& ( m_pOuter->m_flPowerPlayTime gpGlobals->curtime )*/ )
	{
		// If we're underwater, put the fire out
		if ( gpGlobals->curtime > m_flFlameRemoveTime || m_pOuter->GetWaterLevel() >= WL_Waist )
		{
			RemoveCond( TF_COND_BURNING );
		}
		else if ( ( gpGlobals->curtime >= m_flFlameBurnTime ) && ( TF_CLASS_PYRO != m_pOuter->GetPlayerClass()->GetClassIndex() ) )
		{
			// Burn the player (if not pyro, who does not take persistent burning damage)
			CTakeDamageInfo info( m_hBurnAttacker, m_hBurnAttacker, TF_BURNING_DMG, DMG_BURN | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_BURNING );
			m_pOuter->TakeDamage( info );
			m_flFlameBurnTime = gpGlobals->curtime + TF_BURNING_FREQUENCY;
		}

		if ( m_flNextBurningSound < gpGlobals->curtime )
		{
			m_pOuter->SpeakConceptIfAllowed( MP_CONCEPT_ONFIRE );
			m_flNextBurningSound = gpGlobals->curtime + 2.5;
		}
	}

	if (InCond(TF_COND_POISON))
	{
		if (gpGlobals->curtime >= m_flPoisonTime)
		{
			CTakeDamageInfo info(m_hPoisonAttacker, m_hPoisonAttacker, TF_POISON_DMG, DMG_SLASH | DMG_PREVENT_PHYSICS_FORCE, TF_DMG_CUSTOM_POISON);
			m_pOuter->TakeDamage(info);
			m_flPoisonTime = gpGlobals->curtime + TF_POISON_FREQUENCY;
		}
	}

	if ( InCond( TF_COND_DISGUISING ) )
	{
		if ( gpGlobals->curtime > m_flDisguiseCompleteTime )
		{
			CompleteDisguise();
		}
	}

	// Stops the drain hack.
	CWeaponMedigun *pWeapon = ( CWeaponMedigun* )m_pOuter->Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );
	if ( pWeapon && pWeapon->IsReleasingCharge() )
	{
		pWeapon->DrainCharge();
	}

	if ( InCondUber()  )
	{
		bool bRemoveInvul = false;

		if ( ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN ) && ( TFGameRules()->GetWinningTeam() != m_pOuter->GetTeamNumber()  || ( ( TFGameRules()->GetWinningTeam() == TF_TEAM_MERCENARY && !IsTopThree() ) ) ) )
		{
			bRemoveInvul = true;
		}
		
		if ( m_flInvulnerableOffTime )
		{
			if ( gpGlobals->curtime > m_flInvulnerableOffTime )
			{
				bRemoveInvul = true;
			}
		}

		if ( bRemoveInvul == true )
		{
			m_flInvulnerableOffTime = 0;
			RemoveCond( TF_COND_INVULNERABLE_WEARINGOFF );
			RemoveCondUber();//Stickynote
		}
	}
	
	if ( InCond( TF_COND_STEALTHED_BLINK ) )
	{
		if ( TF_SPY_STEALTH_BLINKTIME/*tf_spy_stealth_blink_time.GetFloat()*/ < ( gpGlobals->curtime - m_flLastStealthExposeTime ) )
		{
			RemoveCond( TF_COND_STEALTHED_BLINK );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Do CLIENT/SERVER SHARED condition thinks.
//-----------------------------------------------------------------------------
void CTFPlayerShared::ConditionThink( void )
{
	bool bIsLocalPlayer = false;
#ifdef CLIENT_DLL
	bIsLocalPlayer = m_pOuter->IsLocalPlayer();
#else
	bIsLocalPlayer = true;
#endif

	if ( m_pOuter->IsPlayerClass( TF_CLASS_SPY ) && bIsLocalPlayer )
	{
		if ( InCond( TF_COND_STEALTHED ) )
		{
			m_flCloakMeter -= gpGlobals->frametime * tf_spy_cloak_consume_rate.GetFloat();

			if ( m_flCloakMeter <= 0.0f )
			{
				FadeInvis( tf_spy_invis_unstealth_time.GetFloat() );
			}
		} 
		else
		{
			m_flCloakMeter += gpGlobals->frametime * tf_spy_cloak_regen_rate.GetFloat();

			if ( m_flCloakMeter >= 100.0f )
			{
				m_flCloakMeter = 100.0f;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveZoomed( void )
{
#ifdef GAME_DLL
	m_pOuter->SetFOV( m_pOuter, 0, 0.1f );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDisguising( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pDisguisingEffect && InCondInvis() )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
	}

	if ( !m_pOuter->IsLocalPlayer() && ( !InCondInvis() || !m_pOuter->IsEnemyPlayer() ) )
	{
		const char *pEffectName;
		if ( m_pOuter->GetTeamNumber() == TF_TEAM_RED )
			pEffectName = "spy_start_disguise_red";
		else if ( m_pOuter->GetTeamNumber() == TF_TEAM_BLUE )
			pEffectName = "spy_start_disguise_blue";
		else
			pEffectName = "spy_start_disguise_mercenary";
		m_pOuter->m_pDisguisingEffect = m_pOuter->ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );
		m_pOuter->m_flDisguiseEffectStartTime = gpGlobals->curtime;
	}

	m_pOuter->EmitSound( "Player.Spy_Disguise" );

#endif
}

//-----------------------------------------------------------------------------
// Purpose: set up effects for when player finished disguising
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddDisguised( void )
{
#ifdef CLIENT_DLL
	UpdateCritParticle();
	if ( m_pOuter->m_pDisguisingEffect )
	{
		// turn off disguising particles
//		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
		m_pOuter->m_pDisguisingEffect = NULL;
	}
	m_pOuter->m_flDisguiseEndEffectStartTime = gpGlobals->curtime;
#endif
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: start, end, and changing disguise classes
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnDisguiseChanged( void )
{
	m_pOuter->UpdateSpyMask();
	// recalc disguise model index
	RecalcDisguiseWeapon();
}
#endif

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCritParticle()
{
	if( m_pOuter->m_Shared.InCondCrit()  // We need to be critboosted in the first place
	&& !m_pOuter->m_Shared.InCondInvis() // And we cant be invisible
	)
	{
		CBaseEntity *pTarget = NULL;
		if( m_pOuter->IsLocalPlayer() )
			pTarget = m_pOuter->GetViewModel(0);
		else
			pTarget = m_pOuter->GetActiveWeapon();

		if ( pTarget )
		{
			char *pEffect = NULL;
			
			int iTeam = (InCond( TF_COND_DISGUISED ) && !m_pOuter->IsLocalPlayer()) ? GetDisguiseTeam() : m_pOuter->GetTeamNumber();

			if( m_pOuter->IsLocalPlayer() )
			{
				// Get the viewmodel and use it instead
				switch( iTeam )
				{
					case TF_TEAM_BLUE:
						pEffect = "critgun_firstperson_weaponmodel_blu";
						break;
					case TF_TEAM_RED:
						pEffect = "critgun_firstperson_weaponmodel_red";
						break;
					default:
					case TF_TEAM_MERCENARY:
						pEffect = "critgun_firstperson_weaponmodel_dm";
						break;
				}
			}
			else
			{
				switch( iTeam )
				{
					case TF_TEAM_BLUE:
						pEffect = "critgun_weaponmodel_blu";
						break;
					case TF_TEAM_RED:
						pEffect = "critgun_weaponmodel_red";
						break;
					default:
					case TF_TEAM_MERCENARY:
						pEffect = "critgun_weaponmodel_dm";
						break;
				}
			}

			if ( m_pCritBoostEffect )
			{
				m_pCritBoostEffect->StartEmission();
			}
			else
			{
				m_pCritBoostEffect = pTarget->ParticleProp()->Create( pEffect, PATTACH_ABSORIGIN_FOLLOW );
			}

			if ( m_pCritBoostEffect )
				UpdateParticleColor( m_pCritBoostEffect );
		}	
	}
	else
	{
		if ( m_pCritBoostEffect )
		{
			if ( m_pCritBoostEffect->GetOwner() )
				m_pCritBoostEffect->GetOwner()->ParticleProp()->StopEmissionAndDestroyImmediately( m_pCritBoostEffect );
			else
				m_pCritBoostEffect->StopEmission();

			m_pCritBoostEffect = NULL;
		}
	}
}
#endif

void CTFPlayerShared::OnAddCritBoosted( void )
{
#ifdef GAME_DLL
	CTFPlayer *pTFPlayer = ToTFPlayer( m_pOuter );
	if ( pTFPlayer )
		pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_POSITIVE );
#else
	UpdateCritParticle();
#endif
}

void CTFPlayerShared::OnRemoveCritBoosted( void )
{
#ifdef GAME_DLL
	CTFPlayer *pTFPlayer = ToTFPlayer( m_pOuter );
	if ( pTFPlayer && pTFPlayer->IsAlive() )
	{
		CFmtStrN<128> modifiers( "inpowerup:yes");
		pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_NEGATIVE, modifiers );
	}
#else
	UpdateCritParticle();
#endif
}
void CTFPlayerShared::OnAddBerserk( void )
{	
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		char *pEffectName = NULL;

		pEffectName = "effects/berserk_overlay";

		IMaterial *pMaterial = materials->FindMaterial( pEffectName, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#else
	CTFPlayer *pTFPlayer = ToTFPlayer( m_pOuter );
	if ( pTFPlayer )
	{
		CFmtStrN<128> modifiers( "poweruptype:TF_COND_BERSERK");	
		pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_POSITIVE, modifiers );
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)pTFPlayer->GiveNamedItem( "TF_WEAPON_BERSERK" );
		if ( pWeapon )
		{
			pWeapon->GiveTo( pTFPlayer );
			pTFPlayer->Weapon_Switch(pWeapon, pWeapon->GetSlot() );
		}
	}
	m_pOuter->TeamFortress_SetSpeed();
	m_pOuter->EmitSound("HeartbeatLoop");
#endif
}

void CTFPlayerShared::OnRemoveBerserk( void )
{		
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		view->SetScreenOverlayMaterial( NULL );
	}
#else
	CTFPlayer *pTFPlayer = ToTFPlayer( m_pOuter );	
	if ( pTFPlayer && pTFPlayer->IsAlive() )
	{
		CFmtStrN<128> modifiers( "inpowerup:yes");
		pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_NEGATIVE, modifiers );
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)pTFPlayer->GetActiveWeapon();
		if ( pWeapon )
		{
			pTFPlayer->Weapon_Detach( pWeapon );
			UTIL_Remove( pWeapon );
			if ( pTFPlayer->GetLastWeapon() )
				pTFPlayer->Weapon_Switch( pTFPlayer->GetLastWeapon() );
			else
				pTFPlayer->SwitchToNextBestWeapon( NULL );
		}
	}
	m_pOuter->TeamFortress_SetSpeed();
	m_pOuter->StopSound("HeartbeatLoop");
#endif
}

void CTFPlayerShared::OnAddJauggernaught( void )
{
#ifdef GAME_DLL

	m_iJauggernaughtOldClass = m_pOuter->GetPlayerClass()->GetClass();

	m_pOuter->UpdatePlayerClass( TF_CLASS_JUGGERNAUT, true );

	int iHealth;

	iHealth = m_pOuter->GetPlayerClass()->GetMaxHealth() * ( TFGameRules()->CountActivePlayers() - 1 ) ;

	//fixes a solo juggernaught having no health...
	if ( iHealth == 0 )
		iHealth = m_pOuter->GetPlayerClass()->GetMaxHealth();

	m_pOuter->SetMaxHealth(iHealth);

	OnAddHaste();

#endif
}

void CTFPlayerShared::OnRemoveJauggernaught( void )
{
#ifdef GAME_DLL
	
	m_pOuter->UpdatePlayerClass( m_iJauggernaughtOldClass, true );

	m_iJauggernaughtOldClass = TF_CLASS_UNDEFINED;

	OnRemoveHaste();

#endif
}

void CTFPlayerShared::OnAddShieldCharge( void )
{
	m_pOuter->TeamFortress_SetSpeed();
}
void CTFPlayerShared::OnRemoveShieldCharge( void )
{
	m_pOuter->TeamFortress_SetSpeed();
}

void CTFPlayerShared::OnAddHaste( void )
{
#ifdef CLIENT_DLL
	const char *pszTeamName;
	switch ( m_pOuter->GetTeamNumber() )
	{
		case TF_TEAM_RED:
			pszTeamName = "red";
			break;
		case TF_TEAM_BLUE:
			pszTeamName = "blue";
			break;
		default:
			pszTeamName = "dm";
			break;
	}
	m_pOuter->ParticleProp()->Create( "demo_charge_socks", PATTACH_BONE_FOLLOW, "bip_foot_L" );
	m_pOuter->ParticleProp()->Create( "demo_charge_socks", PATTACH_BONE_FOLLOW, "bip_foot_R" );
	m_pOuter->ParticleProp()->Create( "speed_boost_trail", PATTACH_ABSORIGIN_FOLLOW );
	char pszParticleEffect[32];
	Q_snprintf( pszParticleEffect, sizeof(pszParticleEffect), "demo_charge_pelvis_%s", pszTeamName );
	UpdateParticleColor ( m_pOuter->ParticleProp()->Create( pszParticleEffect, PATTACH_BONE_FOLLOW, "bip_spine_2" ) );
	Q_snprintf( pszParticleEffect, sizeof(pszParticleEffect), "demo_charge_root_%s", pszTeamName );
	UpdateParticleColor ( m_pOuter->ParticleProp()->Create( pszParticleEffect, PATTACH_ABSORIGIN_FOLLOW ) );
	
#endif
	m_pOuter->TeamFortress_SetSpeed();
}

void CTFPlayerShared::OnRemoveHaste( void )
{
#ifdef CLIENT_DLL
	m_pOuter->ParticleProp()->StopParticlesNamed( "demo_charge_socks", true );
	m_pOuter->ParticleProp()->StopParticlesNamed( "speed_boost_trail", true );
	
	for( int i = TF_TEAM_RED; i <= TF_TEAM_MERCENARY; i++ )
	{
		const char *pszTeamName;
		switch ( i )
		{
			case TF_TEAM_RED:
				pszTeamName = "red";
				break;
			case TF_TEAM_BLUE:
				pszTeamName = "blue";
				break;
			default:
				pszTeamName = "dm";
				break;
		}
		char pszParticleEffect[64];
		Q_snprintf( pszParticleEffect, sizeof(pszParticleEffect), "demo_charge_pelvis_%s", pszTeamName );
		m_pOuter->ParticleProp()->StopParticlesNamed( pszParticleEffect, true );
		Q_snprintf( pszParticleEffect, sizeof(pszParticleEffect), "demo_charge_root_%s", pszTeamName );
		m_pOuter->ParticleProp()->StopParticlesNamed( pszParticleEffect, true );
	}
#endif
	m_pOuter->TeamFortress_SetSpeed();
}

void CTFPlayerShared::OnAddTranq(void)
{
#ifdef CLIENT_DLL
	m_pOuter->ParticleProp()->Create("sleepy_overhead", PATTACH_POINT_FOLLOW, "head");
#endif
	m_pOuter->TeamFortress_SetSpeed();
}

void CTFPlayerShared::OnRemoveTranq(void)
{
#ifdef CLIENT_DLL
	m_pOuter->ParticleProp()->StopParticlesNamed("sleepy_overhead", true);
#endif
	m_pOuter->TeamFortress_SetSpeed();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddInvulnerable( void )
{
#ifdef CLIENT_DLL

	if ( m_pOuter->IsLocalPlayer() )
	{
		char *pEffectName = NULL;

		switch( m_pOuter->GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			pEffectName = "effects/invuln_overlay_blue";
			break;
		case TF_TEAM_RED:
			pEffectName =  "effects/invuln_overlay_red";
			break;
		case TF_TEAM_MERCENARY:
			pEffectName = "effects/invuln_overlay_mercenary";
			break;
		default:
			pEffectName = "effects/invuln_overlay_blue";
			break;
		}

		IMaterial *pMaterial = materials->FindMaterial( pEffectName, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveInvulnerable( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->IsLocalPlayer() )
	{
		view->SetScreenOverlayMaterial( NULL );
	}
#endif
}

void CTFPlayerShared::OnAddShield( void )
{
#ifdef CLIENT_DLL

	if ( m_pOuter->IsLocalPlayer() )
	{
		char *pEffectName = NULL;

		switch( m_pOuter->GetTeamNumber() )
		{
		case TF_TEAM_BLUE:
			pEffectName = "effects/shield_overlay_blue";
			break;
		case TF_TEAM_RED:
			pEffectName =  "effects/shield_overlay_red";
			break;
		case TF_TEAM_MERCENARY:
			pEffectName = "effects/shield_overlay_dm";
			break;
		default:
			pEffectName = "effects/shield_overlay_blue";
			break;
		}

		IMaterial *pMaterial = materials->FindMaterial( pEffectName, TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
	m_pOuter->UpdatePlayerAttachedModels();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveShield( void )
{
#ifdef CLIENT_DLL
	m_pOuter->UpdatePlayerAttachedModels();
	if ( m_pOuter->IsLocalPlayer() )
	{
		view->SetScreenOverlayMaterial( NULL );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddTeleported( void )
{
#ifdef CLIENT_DLL
	m_pOuter->OnAddTeleported();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveTeleported( void )
{
#ifdef CLIENT_DLL
	m_pOuter->OnRemoveTeleported();
#endif
}

void CTFPlayerShared::OnRemoveTaunting( void )
{
#ifdef GAME_DLL
	m_pOuter->m_iTaunt = -1;
	m_pOuter->m_iTauntLayer = 0;
#endif
	m_pOuter->TeamFortress_SetSpeed();
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::Burn( CTFPlayer *pAttacker, float flTime )
{
#ifdef GAME_DLL
	// Don't bother igniting players who have just been killed by the fire damage.
	if ( !m_pOuter->IsAlive() )
		return;

	// pyros don't burn persistently or take persistent burning damage, but we show brief burn effect so attacker can tell they hit
	bool bVictimIsPyro = ( TF_CLASS_PYRO == m_pOuter->GetPlayerClass()->GetClassIndex() );

	if ( !InCond( TF_COND_BURNING ) )
	{
		// Start burning
		AddCond( TF_COND_BURNING );
		m_flFlameBurnTime = gpGlobals->curtime;	//asap
		// let the attacker know he burned me
		if ( pAttacker && !bVictimIsPyro )
		{
			pAttacker->OnBurnOther( m_pOuter );
		}
	}

	if ( flTime > 0.0f )
	{
		if ( bVictimIsPyro )
			m_flFlameRemoveTime = gpGlobals->curtime + TF_BURNING_FLAME_LIFE_PYRO;
		else 
			m_flFlameRemoveTime = gpGlobals->curtime + flTime;
	}
	else
	{
		float flFlameLife = bVictimIsPyro ? TF_BURNING_FLAME_LIFE_PYRO : TF_BURNING_FLAME_LIFE;
		m_flFlameRemoveTime = gpGlobals->curtime + flFlameLife;
	}

	m_hBurnAttacker = pAttacker;
#endif
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::Poison(CTFPlayer *pAttacker, float flTime)
{
#ifdef GAME_DLL
	// Don't bother igniting players who have just been killed by the fire damage.
	if (!m_pOuter->IsAlive())
		return;

	if (!InCond(TF_COND_POISON))
	{
		// Start posioning
		AddCond(TF_COND_POISON, flTime);
		m_flPoisonTime = gpGlobals->curtime;    //asap
	}

	if (flTime > 0.f)
		m_flPoisonRemoveTime = gpGlobals->curtime + flTime;
	else
		m_flPoisonRemoveTime = gpGlobals->curtime + TF_POISON_STING_LIFE;

	m_hPoisonAttacker = pAttacker;
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsControlStunned( void )
{
	if ( InCond( TF_COND_STUNNED ) )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveBurning( void )
{
#ifdef CLIENT_DLL
	m_pOuter->StopBurningSound();

	if ( m_pOuter->m_pBurningEffect )
	{
		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pBurningEffect );
		m_pOuter->m_pBurningEffect = NULL;
	}

	if ( m_pOuter->IsLocalPlayer() )
	{
		view->SetScreenOverlayMaterial( NULL );
	}

	m_pOuter->m_flBurnEffectStartTime = 0;
	m_pOuter->m_flBurnEffectEndTime = 0;
#else
	m_hBurnAttacker = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemovePoison(void)
{
#ifdef CLIENT_DLL
	if (m_pOuter->IsLocalPlayer())
		view->SetScreenOverlayMaterial(NULL);
	m_pOuter->ParticleProp()->StopParticlesNamed("poison_overhead", true);

	m_pOuter->m_flPoisonEffectStartTime = 0.f;
	m_pOuter->m_flPoisonEffectEndTime = 0.f;
#else
	m_hPoisonAttacker = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddStealthed( void )
{
#ifdef CLIENT_DLL
	m_pOuter->EmitSound( "Player.Spy_Cloak" );
	m_pOuter->RemoveAllDecals();
	UpdateCritParticle();
#endif

	m_flInvisChangeCompleteTime = gpGlobals->curtime + tf_spy_invis_time.GetFloat();

	if ( InCond( TF_COND_STEALTHED ) )
	{
		// set our offhand weapon to be the invis weapon
		int i;
		for (i = 0; i < m_pOuter->WeaponCount(); i++) 
		{
			CTFWeaponBase *pWpn = ( CTFWeaponBase *)m_pOuter->GetWeapon(i);
			if ( !pWpn )
				continue;

			if ( pWpn->GetWeaponID() != TF_WEAPON_INVIS )
				continue;

			// try to switch to this weapon
			m_pOuter->SetOffHandWeapon( pWpn );
			break;
		}
	}
	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveStealthed( void )
{
#ifdef CLIENT_DLL
	m_pOuter->EmitSound( "Player.Spy_UnCloak" );
	UpdateCritParticle();
#endif

	m_pOuter->HolsterOffHandWeapon();

	m_pOuter->TeamFortress_SetSpeed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDisguising( void )
{
#ifdef CLIENT_DLL
	if ( m_pOuter->m_pDisguisingEffect )
	{
//		m_pOuter->ParticleProp()->StopEmission( m_pOuter->m_pDisguisingEffect );
		m_pOuter->m_pDisguisingEffect = NULL;
	}
#else
	m_nDesiredDisguiseTeam = TF_SPY_UNDEFINED;

	// Do not reset this value, we use the last desired disguise class for the
	// 'lastdisguise' command

	//m_nDesiredDisguiseClass = TF_CLASS_UNDEFINED;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnRemoveDisguised( void )
{
#ifdef CLIENT_DLL
	UpdateCritParticle();
	// if local player is on the other team, reset the model of this player
	CTFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !m_pOuter->InSameTeam( pLocalPlayer ) )
	{
		TFPlayerClassData_t *pData = GetPlayerClassData( TF_CLASS_SPY );
		int iIndex = modelinfo->GetModelIndex( pData->GetModelName() );

		m_pOuter->SetModelIndex( iIndex );
	}

	m_pOuter->EmitSound( "Player.Spy_Disguise" );

	// They may have called for medic and created a visible medic bubble
	m_pOuter->ParticleProp()->StopParticlesNamed( "speech_mediccall", true );

#else
	m_nDisguiseTeam  = TF_SPY_UNDEFINED;
	m_nDisguiseClass.Set( TF_CLASS_UNDEFINED );
	m_hDisguiseTarget.Set( NULL );
	m_iDisguiseTargetIndex = TF_DISGUISE_TARGET_INDEX_NONE;
	m_iDisguiseHealth = 0;

	// Update the player model and skin.
	m_pOuter->UpdateModel();

	m_pOuter->TeamFortress_SetSpeed();

	m_pOuter->ClearExpression();

#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddBurning( void )
{
#ifdef CLIENT_DLL
	// Start the burning effect
	if ( !m_pOuter->m_pBurningEffect )
	{	
		const char *pEffectName;
		if ( m_pOuter->GetTeamNumber() == TF_TEAM_RED )
			pEffectName = "burningplayer_red";
		else if ( m_pOuter->GetTeamNumber() == TF_TEAM_BLUE )
			pEffectName = "burningplayer_blue";
		else
			pEffectName = "burningplayer_dm";
		m_pOuter->m_pBurningEffect = m_pOuter->ParticleProp()->Create( pEffectName, PATTACH_ABSORIGIN_FOLLOW );

		m_pOuter->m_flBurnEffectStartTime = gpGlobals->curtime;
		m_pOuter->m_flBurnEffectEndTime = gpGlobals->curtime + TF_BURNING_FLAME_LIFE;
	}
	// set the burning screen overlay
	if ( m_pOuter->IsLocalPlayer() )
	{
		IMaterial *pMaterial = materials->FindMaterial( "effects/imcookin", TEXTURE_GROUP_CLIENT_EFFECTS, false );
		if ( !IsErrorMaterial( pMaterial ) )
		{
			view->SetScreenOverlayMaterial( pMaterial );
		}
	}
#endif

	/*
#ifdef GAME_DLL
	
	if ( player == robin || player == cook )
	{
		CSingleUserRecipientFilter filter( m_pOuter );
		TFGameRules()->SendHudNotification( filter, HUD_NOTIFY_SPECIAL );
	}

#endif
	*/

	// play a fire-starting sound
	m_pOuter->EmitSound( "Fire.Engulf" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnAddPoison(void)
{
#ifdef CLIENT_DLL
	// set the poison screen overlay
	if (m_pOuter->IsLocalPlayer())
	{
		IMaterial *pMaterial = materials->FindMaterial("effects/poison_overlay", TEXTURE_GROUP_CLIENT_EFFECTS, false);
		if (!IsErrorMaterial(pMaterial))
			view->SetScreenOverlayMaterial(pMaterial);
	}

	m_pOuter->ParticleProp()->Create("poison_overhead", PATTACH_POINT_FOLLOW, "head");
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetStealthNoAttackExpireTime( void )
{
	return m_flStealthNoAttackExpire;
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is dominating the specified other player
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPlayerDominated( CTFPlayer *pPlayer, bool bDominated )
{
	int iPlayerIndex = pPlayer->entindex();
	m_bPlayerDominated.Set( iPlayerIndex, bDominated );
	pPlayer->m_Shared.SetPlayerDominatingMe( m_pOuter, bDominated );
}

//-----------------------------------------------------------------------------
// Purpose: Sets whether this player is being dominated by the other player
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetPlayerDominatingMe( CTFPlayer *pPlayer, bool bDominated )
{
	int iPlayerIndex = pPlayer->entindex();
	m_bPlayerDominatingMe.Set( iPlayerIndex, bDominated );
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether this player is dominating the specified other player
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsPlayerDominated( int iPlayerIndex )
{
#ifdef CLIENT_DLL
	// On the client, we only have data for the local player.
	// As a result, it's only valid to ask for dominations related to the local player
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer )
		return false;

	Assert( m_pOuter->IsLocalPlayer() || pLocalPlayer->entindex() == iPlayerIndex );

	if ( m_pOuter->IsLocalPlayer() )
		return m_bPlayerDominated.Get( iPlayerIndex );

	return pLocalPlayer->m_Shared.IsPlayerDominatingMe( m_pOuter->entindex() );
#else
	// Server has all the data.
	return m_bPlayerDominated.Get( iPlayerIndex );
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsPlayerDominatingMe( int iPlayerIndex )
{
	return m_bPlayerDominatingMe.Get( iPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::NoteLastDamageTime( int nDamage )
{
	// we took damage
	if ( nDamage > 5 )
	{
		m_flLastStealthExposeTime = gpGlobals->curtime;
		AddCond( TF_COND_STEALTHED_BLINK );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::OnSpyTouchedByEnemy( void )
{
	m_flLastStealthExposeTime = gpGlobals->curtime;
	AddCond( TF_COND_STEALTHED_BLINK );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayerShared::FadeInvis( float flInvisFadeTime )
{
	bool bNoAttack = false;
	if ( InCond( TF_COND_STEALTHED ) )
		bNoAttack = true;
	RemoveCondInvis();

	if ( flInvisFadeTime > 0.15 && bNoAttack ) // this was a force respawn, they can attack whenever
	{
		// next attack in some time
		m_flStealthNoAttackExpire = gpGlobals->curtime + tf_spy_cloak_no_attack_time.GetFloat();
	}

	m_flInvisChangeCompleteTime = gpGlobals->curtime + flInvisFadeTime;
}

//-----------------------------------------------------------------------------
// Purpose: Approach our desired level of invisibility
//-----------------------------------------------------------------------------
void CTFPlayerShared::InvisibilityThink( void )
{
	float flTargetInvis = 0.0f;
	float flTargetInvisScale = 1.0f;
	if ( InCond( TF_COND_STEALTHED_BLINK ) )
	{
		// We were bumped into or hit for some damage.
		flTargetInvisScale = TF_SPY_STEALTH_BLINKSCALE;/*tf_spy_stealth_blink_scale.GetFloat();*/
	}

	// Go invisible or appear.
	if ( m_flInvisChangeCompleteTime > gpGlobals->curtime )
	{
		if ( InCondInvis() )
		{
			flTargetInvis = 1.0f - ( ( m_flInvisChangeCompleteTime - gpGlobals->curtime ) );
		}
		else
		{
			flTargetInvis = ( ( m_flInvisChangeCompleteTime - gpGlobals->curtime ) * 0.5f );
		}
	}
	else
	{
		if ( InCondInvis() )
		{
			flTargetInvis = 1.0f;
		}
		else
		{
			flTargetInvis = 0.0f;
		}
	} 

	flTargetInvis *= flTargetInvisScale;
	m_flInvisibility = clamp( flTargetInvis, 0.0f, 1.0f );
}


//-----------------------------------------------------------------------------
// Purpose: How invisible is the player [0..1]
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetPercentInvisible( void )
{
	return m_flInvisibility;
}

//-----------------------------------------------------------------------------
// Purpose: Start the process of disguising
//-----------------------------------------------------------------------------
void CTFPlayerShared::Disguise( int nTeam, int nClass )
{
#ifdef GAME_DLL
	int nRealTeam = m_pOuter->GetTeamNumber();
	int nRealClass = m_pOuter->GetPlayerClass()->GetClassIndex();

	Assert ( ( nClass >= TF_CLASS_SCOUT ) && ( nClass <= TF_CLASS_ENGINEER ) );

	// we're not a spy
	if ( nRealClass != TF_CLASS_SPY )
	{
		return;
	}

	// disguise concommand exploit fix
	if ( InCond( TF_COND_TAUNTING ) )
	{
		return;
	}

	// we're not disguising as anything but ourselves (so reset everything)
	if ( nRealTeam == nTeam && nRealClass == nClass )
	{
		RemoveDisguise();
		return;
	}

	// Ignore disguise of the same type
	if ( nTeam == m_nDisguiseTeam && nClass == m_nDisguiseClass )
	{
		return;
	}

	// invalid team
	if ( nTeam <= TEAM_SPECTATOR || nTeam >= TF_TEAM_COUNT )
	{
		return;
	}

	// invalid class
	if ( nClass <= TF_CLASS_UNDEFINED || nClass >= TF_CLASS_COUNT )
	{
		return;
	}

	m_nDesiredDisguiseClass = nClass;
	m_nDesiredDisguiseTeam = nTeam;

	AddCond( TF_COND_DISGUISING );

	// Start the think to complete our disguise

	// Disguise faster if already disguised
	if ( InCond( TF_COND_DISGUISED ) )
		m_flDisguiseCompleteTime = gpGlobals->curtime + ( TF_TIME_TO_DISGUISE / 4 );
	else
		m_flDisguiseCompleteTime = gpGlobals->curtime + TF_TIME_TO_DISGUISE;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Set our target with a player we've found to emulate
//-----------------------------------------------------------------------------
#ifdef GAME_DLL
void CTFPlayerShared::FindDisguiseTarget( void )
{
	m_hDisguiseTarget = m_pOuter->TeamFortress_GetDisguiseTarget( m_nDisguiseTeam, m_nDisguiseClass );
	if ( m_hDisguiseTarget )
	{
		m_iDisguiseTargetIndex.Set( m_hDisguiseTarget.Get()->entindex() );
		Assert( m_iDisguiseTargetIndex >= 1 && m_iDisguiseTargetIndex <= MAX_PLAYERS );
	}
	else
	{
		m_iDisguiseTargetIndex.Set( TF_DISGUISE_TARGET_INDEX_NONE );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Complete our disguise
//-----------------------------------------------------------------------------
void CTFPlayerShared::CompleteDisguise( void )
{
#ifdef GAME_DLL
	AddCond( TF_COND_DISGUISED );

	m_nDisguiseClass = m_nDesiredDisguiseClass;
	m_nDisguiseTeam = m_nDesiredDisguiseTeam;

	RemoveCond( TF_COND_DISGUISING );

	FindDisguiseTarget();

	int iMaxHealth = m_pOuter->GetMaxHealth();
	m_iDisguiseHealth = (int)random->RandomInt( iMaxHealth / 2, iMaxHealth );

	// Update the player model and skin.
	m_pOuter->UpdateModel();

	m_pOuter->TeamFortress_SetSpeed();

	m_pOuter->ClearExpression();
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetDisguiseHealth( int iDisguiseHealth )
{
	m_iDisguiseHealth = iDisguiseHealth;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveDisguise( void )
{
#ifdef CLIENT_DLL


#else
	RemoveCond( TF_COND_DISGUISED );
	RemoveCond( TF_COND_DISGUISING );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::DoLungeCheck( void )
{
	if ( IsZombie() || m_pOuter->GetPlayerClass()->GetClass() == TF_CLASS_JUGGERNAUT )
	{
		bool OnGround = m_pOuter->GetGroundEntity() != NULL;

		if (m_bIsLunging && OnGround)
			m_bIsLunging = false;

		CTFClaws *pWeapon = dynamic_cast<CTFClaws*>(m_pOuter->GetActiveWeapon());

		if (!pWeapon)
			return false;

		if ((m_pOuter->m_nButtons & IN_ATTACK2) && pWeapon->CanPrimaryAttack() &&
			m_pOuter->GetWaterLevel() < 2 && !m_pOuter->m_Shared.InCond(TF_COND_TAUNTING) && OnGround)
		{
			if (m_flNextLungeTime <= gpGlobals->curtime)
			{
				m_bIsLunging = true;
				return true;
			}
			else
			{
				CSingleUserRecipientFilter filter(m_pOuter);
				m_pOuter->EmitSound(filter, m_pOuter->entindex(), "Player.DenyWeaponSelection");
				return false;
			}
		}
	}

	return false;
}

#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecalcDisguiseWeapon( void )
{
	if ( !InCond( TF_COND_DISGUISED ) ) 
	{
		m_iDisguiseWeaponModelIndex = -1;
		m_pDisguiseWeaponInfo = NULL;
		return;
	}

	Assert( m_pOuter->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY );

	CTFWeaponInfo *pDisguiseWeaponInfo = NULL;

	TFPlayerClassData_t *pData = GetPlayerClassData( m_nDisguiseClass );

	Assert( pData );

	// Find the weapon in the same slot
	int i;
	for ( i=0;i<TF_PLAYER_WEAPON_COUNT;i++ )
	{
		if ( pData->m_aWeapons[i] != TF_WEAPON_NONE )
		{
			const char *pWpnName = WeaponIdToAlias( pData->m_aWeapons[i] );

			WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pWpnName );
			Assert( hWpnInfo != GetInvalidWeaponInfoHandle() );
			CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );

			// find the primary weapon
			if ( pWeaponInfo && pWeaponInfo->iSlot == 0 )
			{
				pDisguiseWeaponInfo = pWeaponInfo;
				break;
			}
		}
	}

	Assert( pDisguiseWeaponInfo != NULL && "Cannot find slot 0 primary weapon for desired disguise class\n" );

	m_pDisguiseWeaponInfo = pDisguiseWeaponInfo;
	m_iDisguiseWeaponModelIndex = -1;

	if ( pDisguiseWeaponInfo )
	{
		m_iDisguiseWeaponModelIndex = modelinfo->GetModelIndex( pDisguiseWeaponInfo->szWorldModel );
	}
}


CTFWeaponInfo *CTFPlayerShared::GetDisguiseWeaponInfo( void )
{
	if ( InCond( TF_COND_DISGUISED ) && m_pDisguiseWeaponInfo == NULL )
	{
		RecalcDisguiseWeapon();
	}

	return m_pDisguiseWeaponInfo;
}

bool CTFPlayerShared::UpdateParticleColor( CNewParticleEffect *pParticle )
{
	if ( pParticle && m_pOuter )
	{
		pParticle->SetControlPoint( CUSTOM_COLOR_CP1, m_pOuter->m_vecPlayerColor );
		return true;
	}
	return false;
}

#endif

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: Heal players.
// pPlayer is person who healed us
//-----------------------------------------------------------------------------
void CTFPlayerShared::Heal( CTFPlayer *pPlayer, float flAmount, bool bDispenserHeal /* = false */ )
{
	Assert( FindHealerIndex(pPlayer) == m_aHealers.InvalidIndex() );

	healers_t newHealer;
	newHealer.pPlayer = pPlayer;
	newHealer.flAmount = flAmount;
	newHealer.bDispenserHeal = bDispenserHeal;
	m_aHealers.AddToTail( newHealer );

	AddCond( TF_COND_HEALTH_BUFF, PERMANENT_CONDITION );

	RecalculateInvuln();

	m_nNumHealers = m_aHealers.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Heal players.
// pPlayer is person who healed us
//-----------------------------------------------------------------------------
void CTFPlayerShared::StopHealing( CTFPlayer *pPlayer )
{
	int iIndex = FindHealerIndex(pPlayer);
	if ( iIndex == m_aHealers.InvalidIndex() )
			return;

	m_aHealers.Remove( iIndex );

	if ( !m_aHealers.Count() )
	{
		RemoveCond( TF_COND_HEALTH_BUFF );
	}

	RecalculateInvuln();

	m_nNumHealers = m_aHealers.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsProvidingInvuln( CTFPlayer *pPlayer )
{
	CTFWeaponBase *pWpn = pPlayer->GetActiveTFWeapon();
	if ( !pWpn )
		return false;

	CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun*>(pWpn);
	if ( pMedigun && pMedigun->IsReleasingCharge() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecalculateInvuln( bool bInstantRemove )
{
	bool bShouldBeInvuln = false;

	/*if ( m_pOuter->m_flPowerPlayTime > gpGlobals->curtime )
	{
		bShouldBeInvuln = true;
	}*/

	// If we're not carrying the flag, and we're being healed by a medic 
	// who's generating invuln, then we should get invuln.
	if ( !m_pOuter->HasTheFlag() )
	{
		if ( IsProvidingInvuln( m_pOuter ) )
		{
			bShouldBeInvuln = true;
		}
		else
		{
			for ( int i = 0; i < m_aHealers.Count(); i++ )
			{
				if ( !m_aHealers[i].pPlayer )
					continue;

				CTFPlayer *pPlayer = ToTFPlayer( m_aHealers[i].pPlayer );
				if ( !pPlayer )
					continue;

				if ( IsProvidingInvuln( pPlayer ) )
				{
					bShouldBeInvuln = true;
					break;
				}
			}
		}
	}

	SetInvulnerable( bShouldBeInvuln, bInstantRemove );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetInvulnerable( bool bState, bool bInstant )
{
	bool bCurrentState = InCond( TF_COND_INVULNERABLE );
	if ( bCurrentState == bState )
	{
		if ( bState && m_flInvulnerableOffTime )
		{
			m_flInvulnerableOffTime = 0;
			RemoveCond( TF_COND_INVULNERABLE_WEARINGOFF );
		}
		return;
	}

	if ( bState )
	{
		Assert( !m_pOuter->HasTheFlag() );

		if ( m_flInvulnerableOffTime )
		{
			m_pOuter->StopSound( "TFPlayer.InvulnerableOff" );

			m_flInvulnerableOffTime = 0;
			RemoveCond( TF_COND_INVULNERABLE_WEARINGOFF );
		}

		// Invulnerable turning on
		AddCond( TF_COND_INVULNERABLE );

		// remove any persistent damaging conditions
		if ( InCond( TF_COND_BURNING ))
			RemoveCond( TF_COND_BURNING );
		if(InCond( TF_COND_POISON ))
			RemoveCond(TF_COND_POISON);

		CSingleUserRecipientFilter filter( m_pOuter );
		m_pOuter->EmitSound( filter, m_pOuter->entindex(), "TFPlayer.InvulnerableOn" );
	}
	else
	{
		if ( !m_flInvulnerableOffTime )
		{
			CSingleUserRecipientFilter filter( m_pOuter );
			m_pOuter->EmitSound( filter, m_pOuter->entindex(), "TFPlayer.InvulnerableOff" );
		}

		if ( bInstant )
		{
			m_flInvulnerableOffTime = 0;
			RemoveCondUber();
			RemoveCond( TF_COND_INVULNERABLE_WEARINGOFF );
		}
		else
		{
			// We're already in the process of turning it off
			if ( m_flInvulnerableOffTime )
				return;

			AddCond( TF_COND_INVULNERABLE_WEARINGOFF );
			m_flInvulnerableOffTime = gpGlobals->curtime + tf_invuln_time.GetFloat();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
EHANDLE	CTFPlayerShared::GetHealerByIndex( int index )
{
	if (m_aHealers.IsValidIndex( index ))
		return m_aHealers[index].pPlayer;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerShared::FindHealerIndex( CTFPlayer *pPlayer )
{
	for ( int i = 0; i < m_aHealers.Count(); i++ )
	{
		if ( m_aHealers[i].pPlayer == pPlayer )
			return i;
	}

	return m_aHealers.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Purpose: Returns the first healer in the healer array.  Note that this
//		is an arbitrary healer.
//-----------------------------------------------------------------------------
EHANDLE CTFPlayerShared::GetFirstHealer()
{
	if ( m_aHealers.Count() > 0 )
		return m_aHealers.Head().pPlayer;

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::HealthKitPickupEffects( int iAmount )
{
	if ( InCond( TF_COND_BURNING ) )
		RemoveCond( TF_COND_BURNING );
	if ( InCond( TF_COND_BLEEDING ) )
		RemoveCond( TF_COND_BLEEDING );

	if ( InCondInvis() || !m_pOuter )
		return;

	IGameEvent *event = gameeventmanager->CreateEvent( "player_healonhit" );
	if ( event )
	{
		event->SetInt( "amount", iAmount );
		event->SetInt( "entindex", m_pOuter->entindex() );

		gameeventmanager->FireEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::HealerIsDispenser( int index ) const
{
	if( !m_aHealers.IsValidIndex(index) )
		return false;

	return m_aHealers[index].bDispenserHeal;
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFPlayerShared::GetActiveTFWeapon() const
{
	return m_pOuter->GetActiveTFWeapon();
}

//-----------------------------------------------------------------------------
// Purpose: Team check.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsLoser( void )
{
	if ( TFGameRules() && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		int iWinner = TFGameRules()->GetWinningTeam();

		if ( iWinner == TF_TEAM_MERCENARY )
		{
			if( IsTopThree() )
				return false;
			else
				return true;
		}
		
		
		//if ( iWinner == GetTeamNumber() )
		if ( iWinner != TEAM_UNASSIGNED && iWinner != m_pOuter->GetTeamNumber() )
		{
			return true;
		}
		if ( iWinner == TEAM_UNASSIGNED )
		{
			return true;
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Engineer hauling check
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsHauling( void )
{
	if ( m_pOuter->m_bHauling )
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Top three for DM
//-----------------------------------------------------------------------------

bool CTFPlayerShared::IsTopThree( void )
{
	return m_bIsTopThree;
}

void CTFPlayerShared::SetTopThree( bool bTop3 )
{
	m_bIsTopThree = bTop3;
}

//-----------------------------------------------------------------------------
// Purpose: Zombie mode
//-----------------------------------------------------------------------------

bool CTFPlayerShared::IsZombie( void )
{
	return m_bIsZombie;
}

void CTFPlayerShared::SetZombie( bool bZombie )
{
	m_bIsZombie = bZombie;
}

//-----------------------------------------------------------------------------
// Purpose: Team check.
//-----------------------------------------------------------------------------
bool CTFPlayerShared::IsAlly( CBaseEntity *pEntity )
{
	return ( pEntity->GetTeamNumber() == m_pOuter->GetTeamNumber() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::GetDesiredPlayerClassIndex( void )
{
	return m_iDesiredPlayerClass;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFPlayerShared::PlayDeathAnimation( CBaseAnimating *pAnim, int iDamageCustom, bool bDissolve )
{
	//const char *pszSequence = -1;
	const char *pszSequence = NULL;

	if ( bDissolve )
	{
		// fizzly wizzly
		pszSequence = "dieviolent";
	}
	else
	{
		// play a custom death animation depending on the type of damage it was
		switch ( iDamageCustom )
		{
		case TF_DMG_CUSTOM_HEADSHOT:
		case TF_DMG_CUSTOM_DECAPITATION_BOSS:
			pszSequence = "primary_death_headshot";
			break;
		case TF_DMG_CUSTOM_BACKSTAB:
			pszSequence = "primary_death_backstab";
			break;
		}
	}

//	if ( pszSequence != -1 )
	if ( pszSequence != NULL )
	{
		return pAnim->LookupSequence( pszSequence );
	}
	else
	{
		return -1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::SetJumping( bool bJumping )
{
	m_bJumping = bJumping;
}

void CTFPlayerShared::SetAirDash( bool bAirDash )
{
	m_bAirDash = bAirDash;
}

void CTFPlayerShared::AddAirDashCount()
{
	m_iAirDashCount++;
}

void CTFPlayerShared::SetAirDashCount( int iAirDashCount )
{
	m_iAirDashCount = iAirDashCount;
}

void CTFPlayerShared::SetHook(CBaseEntity *hook)
{
	m_Hook = hook;
}

void CTFPlayerShared::SetHookProperty(float pull)
{
	m_flGHookProp = pull;
}

void CTFPlayerShared::SetJumpBuffer(bool buffer)
{
	m_bBlockJump = buffer;
}

void CTFPlayerShared::SetCSlideDuration(float duration)
{
	m_flCSlideDuration = duration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFPlayerShared::GetCritMult( void )
{
	float flRemapCritMul = RemapValClamped( m_iCritMult, 0, 255, 1.0, 4.0 );
/*#ifdef CLIENT_DLL
	Msg("CLIENT: Crit mult %.2f - %d\n",flRemapCritMul, m_iCritMult);
#else
	Msg("SERVER: Crit mult %.2f - %d\n", flRemapCritMul, m_iCritMult );
#endif*/

	return flRemapCritMul;
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::UpdateCritMult( void )
{
	const float flMinMult = 1.0;
	const float flMaxMult = TF_DAMAGE_CRITMOD_MAXMULT;

	if ( m_DamageEvents.Count() == 0 )
	{
		m_iCritMult = RemapValClamped( flMinMult, 1.0, 4.0, 0, 255 );
		return;
	}

	//Msg( "Crit mult update for %s\n", m_pOuter->GetPlayerName() );
	//Msg( "   Entries: %d\n", m_DamageEvents.Count() );

	// Go through the damage multipliers and remove expired ones, while summing damage of the others
	float flTotalDamage = 0;
	for ( int i = m_DamageEvents.Count() - 1; i >= 0; i-- )
	{
		float flDelta = gpGlobals->curtime - m_DamageEvents[i].flTime;
		if ( flDelta > tf_damage_events_track_for.GetFloat() )
		{
			//Msg( "      Discarded (%d: time %.2f, now %.2f)\n", i, m_DamageEvents[i].flTime, gpGlobals->curtime );
			m_DamageEvents.Remove(i);
			continue;
		}

		// Ignore damage we've just done. We do this so that we have time to get those damage events
		// to the client in time for using them in prediction in this code.
		if ( flDelta < TF_DAMAGE_CRITMOD_MINTIME )
		{
			//Msg( "      Ignored (%d: time %.2f, now %.2f)\n", i, m_DamageEvents[i].flTime, gpGlobals->curtime );
			continue;
		}

		if ( flDelta > TF_DAMAGE_CRITMOD_MAXTIME )
			continue;

		//Msg( "      Added %.2f (%d: time %.2f, now %.2f)\n", m_DamageEvents[i].flDamage, i, m_DamageEvents[i].flTime, gpGlobals->curtime );

		flTotalDamage += m_DamageEvents[i].flDamage;
	}

	float flMult = RemapValClamped( flTotalDamage, 0, TF_DAMAGE_CRITMOD_DAMAGE, flMinMult, flMaxMult );

	//Msg( "   TotalDamage: %.2f   -> Mult %.2f\n", flTotalDamage, flMult );

	m_iCritMult = (int)RemapValClamped( flMult, 1.0, 4.0, 0, 255 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RecordDamageEvent( const CTakeDamageInfo &info, bool bKill )
{
	if ( m_DamageEvents.Count() >= MAX_DAMAGE_EVENTS )
	{
		// Remove the oldest event
		m_DamageEvents.Remove( m_DamageEvents.Count()-1 );
	}

	int iIndex = m_DamageEvents.AddToTail();
	m_DamageEvents[iIndex].flDamage = info.GetDamage();
	m_DamageEvents[iIndex].flTime = gpGlobals->curtime;
	m_DamageEvents[iIndex].bKill = bKill;

	// Don't count critical damage
	if ( info.GetDamageType() & DMG_CRITICAL )
	{
		m_DamageEvents[iIndex].flDamage /= TF_DAMAGE_CRIT_MULTIPLIER;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerShared::GetNumKillsInTime( float flTime )
{
	if ( tf_damage_events_track_for.GetFloat() < flTime )
	{
		Warning("Player asking for damage events for time %.0f, but tf_damage_events_track_for is only tracking events for %.0f\n", flTime, tf_damage_events_track_for.GetFloat() );
	}

	int iKills = 0;
	for ( int i = m_DamageEvents.Count() - 1; i >= 0; i-- )
	{
		float flDelta = gpGlobals->curtime - m_DamageEvents[i].flTime;
		if ( flDelta < flTime )
		{
			if ( m_DamageEvents[i].bKill )
			{
				iKills++;
			}
		}
	}

	return iKills;
}

#endif

//=============================================================================
//
// Shared player code that isn't CTFPlayerShared
//

//-----------------------------------------------------------------------------
// Purpose:
//   Input: info
//          bDoEffects - effects (blood, etc.) should only happen client-side.
//-----------------------------------------------------------------------------
void CTFPlayer::FireBullet( const FireBulletsInfo_t &info, bool bDoEffects, int nDamageType, int nCustomDamageType /*= TF_DMG_CUSTOM_NONE*/ )
{
	// Fire a bullet (ignoring the shooter).
	Vector vecStart = info.m_vecSrc;
	Vector vecEnd = vecStart + info.m_vecDirShooting * info.m_flDistance;
	trace_t trace;
	UTIL_TraceLine( vecStart, vecEnd, ( MASK_SOLID | CONTENTS_HITBOX ), this, COLLISION_GROUP_NONE, &trace );

#ifdef GAME_DLL
	if ( tf_debug_bullets.GetBool() )
	{
		NDebugOverlay::Line( vecStart, trace.endpos, 0,255,0, true, 30 );
	}
#endif

	if( trace.fraction < 1.0 )
	{
		// Verify we have an entity at the point of impact.
		Assert( trace.m_pEnt );

		if( bDoEffects )
		{
			// If shot starts out of water and ends in water
			if ( !( enginetrace->GetPointContents( trace.startpos ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) &&
				( enginetrace->GetPointContents( trace.endpos ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) )
			{	
				// Water impact effects.
				ImpactWaterTrace( trace, vecStart );
			}
			else
			{
				// Regular impact effects.

				// don't decal your teammates or objects on your team
				if ( trace.m_pEnt->GetTeamNumber() != GetTeamNumber() )
				{
					UTIL_ImpactTrace( &trace, nDamageType );
				}
			}

#ifdef CLIENT_DLL
			static int	tracerCount;
			if ( ( info.m_iTracerFreq != 0 ) && ( tracerCount++ % info.m_iTracerFreq ) == 0 )
			{
				// if this is a local player, start at attachment on view model
				// else start on attachment on weapon model

				int iEntIndex = entindex();
				int iUseAttachment = TRACER_DONT_USE_ATTACHMENT;
				int iAttachment = 1;

				C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

				if( pWeapon )
				{
					iAttachment = pWeapon->LookupAttachment( "muzzle" );
				}

				//C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

				bool bInToolRecordingMode = clienttools->IsInRecordingMode();

				// try to align tracers to actual weapon barrel if possible
				//if ( IsLocalPlayer() && !bInToolRecordingMode )
				if ( !ShouldDrawThisPlayer() && !bInToolRecordingMode )
				{
					/*
					C_BaseViewModel *pViewModel = GetViewModel(0);

					if ( pViewModel )
					{
						iEntIndex = pViewModel->entindex();
						pViewModel->GetAttachment( iAttachment, vecStart );
					}
				}
				else if ( pLocalPlayer &&
					pLocalPlayer->GetObserverTarget() == this &&
					pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
				{	
					// get our observer target's view model
					*/

					C_TFViewModel *pViewModel = dynamic_cast<C_TFViewModel *>( GetViewModel() );

					if ( pViewModel )
					{
						iEntIndex = pViewModel->entindex();
						pViewModel->GetAttachment( iAttachment, vecStart );
					}
				}
				else if ( !IsDormant() )
				{
					// fill in with third person weapon model index
					C_BaseCombatWeapon *pWeapon = GetActiveWeapon();

					if( pWeapon )
					{
						iEntIndex = pWeapon->entindex();

						int nModelIndex = pWeapon->GetModelIndex();
						int nWorldModelIndex = pWeapon->GetWorldModelIndex();
						if ( bInToolRecordingMode && nModelIndex != nWorldModelIndex )
						{
							pWeapon->SetModelIndex( nWorldModelIndex );
						}

						pWeapon->GetAttachment( iAttachment, vecStart );

						if ( bInToolRecordingMode && nModelIndex != nWorldModelIndex )
						{
							pWeapon->SetModelIndex( nModelIndex );
						}
					}
				}

				if ( tf_useparticletracers.GetBool() )
				{
					const char *pszTracerEffect = GetTracerType();
					if ( pszTracerEffect && pszTracerEffect[0] )
					{
						char szTracerEffect[128];
						if ( nDamageType & DMG_CRITICAL )
						{
							Q_snprintf( szTracerEffect, sizeof(szTracerEffect), "%s_crit", pszTracerEffect );
							pszTracerEffect = szTracerEffect;
						}

						UTIL_ParticleTracer( pszTracerEffect, vecStart, trace.endpos, entindex(), iUseAttachment, true );
					}
				}
				else
				{
					UTIL_Tracer( vecStart, trace.endpos, entindex(), iUseAttachment, 5000, true, GetTracerType() );
				}
			}
#endif

		}

		// Server specific.
#ifdef GAME_DLL
		// See what material we hit.
		CTakeDamageInfo dmgInfo( this, info.m_pAttacker, GetActiveWeapon(), info.m_flDamage, nDamageType );
		dmgInfo.SetDamageCustom( nCustomDamageType );
		CalculateBulletDamageForce( &dmgInfo, info.m_iAmmoType, info.m_vecDirShooting, trace.endpos, 1.0 );	//MATTTODO bullet forces
		trace.m_pEnt->DispatchTraceAttack( dmgInfo, info.m_vecDirShooting, &trace );
#endif
	}
}

#ifdef CLIENT_DLL
static ConVar tf_impactwatertimeenable( "tf_impactwatertimeenable", "0", FCVAR_CHEAT, "Draw impact debris effects." );
static ConVar tf_impactwatertime( "tf_impactwatertime", "1.0f", FCVAR_CHEAT, "Draw impact debris effects." );
#endif

//-----------------------------------------------------------------------------
// Purpose: Trace from the shooter to the point of impact (another player,
//          world, etc.), but this time take into account water/slime surfaces.
//   Input: trace - initial trace from player to point of impact
//          vecStart - starting point of the trace 
//-----------------------------------------------------------------------------
void CTFPlayer::ImpactWaterTrace( trace_t &trace, const Vector &vecStart )
{
#ifdef CLIENT_DLL
	if ( tf_impactwatertimeenable.GetBool() )
	{
		if ( m_flWaterImpactTime > gpGlobals->curtime )
			return;
	}
#endif 

	trace_t traceWater;
	UTIL_TraceLine( vecStart, trace.endpos, ( MASK_SHOT | CONTENTS_WATER | CONTENTS_SLIME ), 
		this, COLLISION_GROUP_NONE, &traceWater );
	if( traceWater.fraction < 1.0f )
	{
		CEffectData	data;
		data.m_vOrigin = traceWater.endpos;
		data.m_vNormal = traceWater.plane.normal;
		data.m_flScale = random->RandomFloat( 8, 12 );
		if ( traceWater.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		const char *pszEffectName = "tf_gunshotsplash";
		CTFWeaponBase *pWeapon = GetActiveTFWeapon();
		if ( pWeapon && ( TF_WEAPON_MINIGUN == pWeapon->GetWeaponID() || TF_WEAPON_GATLINGGUN == pWeapon->GetWeaponID() || TFC_WEAPON_ASSAULTCANNON == pWeapon->GetWeaponID() ) )
		{
			// for the minigun, use a different, cheaper splash effect because it can create so many of them
			pszEffectName = "tf_gunshotsplash_minigun";
		}		
		DispatchEffect( pszEffectName, data );

#ifdef CLIENT_DLL
		if ( tf_impactwatertimeenable.GetBool() )
		{
			m_flWaterImpactTime = gpGlobals->curtime + tf_impactwatertime.GetFloat();
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFPlayer::GetActiveTFWeapon( void ) const
{
	CBaseCombatWeapon *pRet = GetActiveWeapon();
	if ( pRet )
	{
		Assert( dynamic_cast< CTFWeaponBase* >( pRet ) != NULL );
		return static_cast< CTFWeaponBase * >( pRet );
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: How much build resource ( metal ) does this player have
//-----------------------------------------------------------------------------
int CTFPlayer::GetBuildResources( void )
{
	return GetAmmoCount( TF_AMMO_METAL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::TeamFortress_SetSpeed()
{
	int playerclass = GetPlayerClass()->GetClassIndex();
	float maxfbspeed;

	// Spectators can move while in Classic Observer mode
	if ( IsObserver() )
	{
		if ( GetObserverMode() == OBS_MODE_ROAMING )
			SetMaxSpeed( GetPlayerClassData( TF_CLASS_SCOUT )->m_flMaxSpeed );
		else
			SetMaxSpeed( 0 );
		return;
	}

	// Check for any reason why they can't move at all
	if ( playerclass == TF_CLASS_UNDEFINED || TFGameRules()->InRoundRestart() )
	{
		SetAbsVelocity( vec3_origin );
		SetMaxSpeed( 1 );
		return;
	}

	if( m_Shared.InCond( TF_COND_TAUNTING ) )
	{
		SetMaxSpeed( 1 );
		return;
	}	

	// First, get their max class speed
	maxfbspeed = GetPlayerClass()->GetData()->m_flMaxSpeed;

	// Slow us down if we're disguised as a slower class
	// unless we're cloaked..
	if ( m_Shared.InCond( TF_COND_DISGUISED ) && !m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		float flMaxDisguiseSpeed = GetPlayerClassData( m_Shared.GetDisguiseClass() )->m_flMaxSpeed;
		maxfbspeed = min( flMaxDisguiseSpeed, maxfbspeed );
	}

	// Second, see if any flags are slowing them down
	if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*>( GetItem() );

		if ( pFlag )
		{
			if ( pFlag->GetGameType() == TF_FLAGTYPE_ATTACK_DEFEND || pFlag->GetGameType() == TF_FLAGTYPE_TERRITORY_CONTROL )
			{
				maxfbspeed *= 0.5;
			}
		}
	}
	if ( m_Shared.InCond( TF_COND_BERSERK ) )
	{
		if ( of_berserk_speed_mode.GetFloat() )
		{
			if ( maxfbspeed < maxfbspeed * of_berserk_speed_factor.GetFloat() )
				maxfbspeed *= of_berserk_speed_factor.GetFloat();
		}
		else
		{
			if ( maxfbspeed < of_berserk_speed.GetFloat() )
				maxfbspeed = of_berserk_speed.GetFloat();
		}

	}
	// if they're a sniper, and they're aiming, their speed must be 80 or less
	if ( m_Shared.InCond( TF_COND_AIMING ) )
	{
		// Pyro's move faster while firing their flamethrower
		if ( playerclass == TF_CLASS_PYRO )
		{
			if (maxfbspeed > 200)
				maxfbspeed = 200;
		}
		else
		{
			if (maxfbspeed > 80)
				maxfbspeed = 80;
		}
	}

	if ( m_Shared.InCond( TF_COND_STEALTHED ) )
	{
		if (maxfbspeed > tf_spy_max_cloaked_speed.GetFloat() )
			maxfbspeed = tf_spy_max_cloaked_speed.GetFloat();
	}
	
	if ( m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
			maxfbspeed = 750.0f;

	// if we're in bonus time because a team has won, give the winners 110% speed and the losers 90% speed
	if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		int iWinner = TFGameRules()->GetWinningTeam();
		
		if ( iWinner != TEAM_UNASSIGNED )
		{
			if ( iWinner == TF_TEAM_MERCENARY )
			{
				if ( m_Shared.IsTopThree() )
					maxfbspeed *= 1.1f;
				else
					maxfbspeed *= 0.9f;
			}
			else if ( iWinner == GetTeamNumber() )
			{
				maxfbspeed *= 1.1f;
			}
			else
			{
				maxfbspeed *= 0.9f;
			}
		}
	}
	
	// hauling engineers move slower
	if ( m_bHauling )
		maxfbspeed *= 0.9f;

	// zombies move slightly faster
	if ( m_Shared.IsZombie() )
		maxfbspeed *= 1.1f;

	if ( m_Shared.InCond( TF_COND_HASTE ) )
		maxfbspeed *= of_haste_movespeed_multplier.GetFloat();

	if (m_Shared.InCond(TF_COND_TRANQ))
			maxfbspeed *= 0.5f;

	// Set the speed
	SetMaxSpeed( maxfbspeed );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayer::HasItem( void )
{
	return ( m_hItem != NULL );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFPlayer::SetItem( CTFItem *pItem )
{
	m_hItem = pItem;

#ifdef GAME_DLL
	if ( pItem && pItem->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		RemoveInvisibility();
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFItem	*CTFPlayer::GetItem( void )
{
	return m_hItem;
}

//-----------------------------------------------------------------------------
// Purpose: Is the player allowed to use a teleporter ?
//-----------------------------------------------------------------------------
bool CTFPlayer::HasTheFlag( void )
{
	if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Get the number of objects of the specified type that this player has
//-----------------------------------------------------------------------------
int CTFPlayer::GetNumObjects( int iObjectType, int iAltMode )
{
	int iCount = 0;
	for (int i = 0; i < GetObjectCount(); i++)
	{
		if ( !GetObject(i) )
			continue;

		if ( GetObject(i)->GetType() == iObjectType && GetObject(i)->GetAltMode() == iAltMode )
		{
			iCount++;
		}
	}

	return iCount;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::ItemPostFrame()
{
	if ( m_hOffHandWeapon.Get() && m_hOffHandWeapon->IsWeaponVisible() )
	{
		if ( gpGlobals->curtime < m_flNextAttack )
		{
			m_hOffHandWeapon->ItemBusyFrame();
		}
		else
		{
#if defined( CLIENT_DLL )
			// Not predicting this weapon
			if ( m_hOffHandWeapon->IsPredicted() )
#endif
			{
				m_hOffHandWeapon->ItemPostFrame( );
			}
		}
	}

	BaseClass::ItemPostFrame();
}

void CTFPlayer::SetOffHandWeapon( CTFWeaponBase *pWeapon )
{

	m_hOffHandWeapon = pWeapon;
	if ( m_hOffHandWeapon.Get() )
	{
				m_hOffHandWeapon->Deploy();
	}
}

// Set to NULL at the end of the holster?
void CTFPlayer::HolsterOffHandWeapon( void )
{
	if ( m_hOffHandWeapon.Get() )
	{
		m_hOffHandWeapon->Holster();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Return true if we should record our last weapon when switching between the two specified weapons
//-----------------------------------------------------------------------------
bool CTFPlayer::Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	// if the weapon doesn't want to be auto-switched to, don't!	
	CTFWeaponBase *pTFWeapon = dynamic_cast< CTFWeaponBase * >( pOldWeapon );
	
	if (pTFWeapon && pTFWeapon->AllowsAutoSwitchTo() == false)
	{
		return false;
	}

	return BaseClass::Weapon_ShouldSetLast( pOldWeapon, pNewWeapon );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFPlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	m_PlayerAnimState->ResetGestureSlot( GESTURE_SLOT_ATTACK_AND_RELOAD );
	return BaseClass::Weapon_Switch( pWeapon, viewmodelindex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::GetStepSoundVelocities( float *velwalk, float *velrun )
{
	float flMaxSpeed = MaxSpeed();

	if ( ( GetFlags() & FL_DUCKING ) || ( GetMoveType() == MOVETYPE_LADDER ) )
	{
		*velwalk = flMaxSpeed * 0.25;
		*velrun = flMaxSpeed * 0.3;		
	}
	else
	{
		*velwalk = flMaxSpeed * 0.3;
		*velrun = flMaxSpeed * 0.8;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayer::SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking )
{
	float flMaxSpeed = MaxSpeed();

	switch ( iStepSoundTime )
	{
	case STEPSOUNDTIME_NORMAL:
	case STEPSOUNDTIME_WATER_FOOT:
		m_flStepSoundTime = RemapValClamped( flMaxSpeed, 200, 450, 400, 200 );
		if ( bWalking )
		{
			m_flStepSoundTime += 100;
		}
		break;

	case STEPSOUNDTIME_ON_LADDER:
		m_flStepSoundTime = 350;
		break;

	case STEPSOUNDTIME_WATER_KNEE:
		m_flStepSoundTime = RemapValClamped( flMaxSpeed, 200, 450, 600, 400 );
		break;

	default:
		Assert(0);
		break;
	}

	if ( ( GetFlags() & FL_DUCKING) || ( GetMoveType() == MOVETYPE_LADDER ) )
	{
		m_flStepSoundTime += 100;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanAttack( void )
{
	CTFGameRules *pRules = TFGameRules();

	Assert( pRules );

	if( m_Shared.InCond( TF_COND_TAUNTING ) )
		return false;

	if ( TFGameRules()->InRoundRestart() && TFGameRules()->IsDMGamemode() )
		return false;

	if ( m_Shared.GetStealthNoAttackExpireTime() > gpGlobals->curtime || ( m_Shared.InCond( TF_COND_STEALTHED ) ) )
		return false;

	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this player's allowed to build another one of the specified object
//-----------------------------------------------------------------------------
int CTFPlayer::CanBuild( int iObjectType, int iAltMode )
{
	if ( TFGameRules() && TFGameRules()->IsInfGamemode() )
	{
		if ( iObjectType == OBJ_SENTRYGUN && !of_infection_allow_sentry.GetBool() )
			return CB_CANNOT_BUILD;
		else if ( iObjectType == OBJ_DISPENSER && !of_infection_allow_dispenser.GetBool() )
			return CB_CANNOT_BUILD;
		else if ( iObjectType == OBJ_TELEPORTER && !of_infection_allow_teleporter.GetBool() )
			return CB_CANNOT_BUILD;
	}

	if ( iObjectType < 0 || iObjectType >= OBJ_LAST )
		return CB_UNKNOWN_OBJECT;

	if ( iObjectType != OBJ_TELEPORTER && iAltMode > 0 )
		return CB_UNKNOWN_OBJECT;

	if ( iObjectType == OBJ_TELEPORTER && iAltMode > 1 )
		return CB_UNKNOWN_OBJECT;

#ifdef GAME_DLL
	CTFPlayerClass *pCls = GetPlayerClass();
	if ( pCls )
	{
		if( pCls->CanBuildObject( iObjectType ) == false )
			return CB_CANNOT_BUILD;
	}
#endif

	int iObjectCount = GetNumObjects( iObjectType, iAltMode );

	// Make sure we haven't hit maximum number
	if ( iObjectCount >= GetObjectInfo( iObjectType )->m_nMaxObjects && 
		GetObjectInfo( iObjectType )->m_nMaxObjects != -1 )
	{
		return CB_LIMIT_REACHED;
	}

	// Find out how much the object should cost
	int iCost = CalculateObjectCost( iObjectType );
	// Make sure we have enough resources
	if ( GetBuildResources() < iCost && of_infiniteammo.GetBool() == 0 )
	{
		return CB_NEED_RESOURCES;
	}

	return CB_CAN_BUILD;
}



//-----------------------------------------------------------------------------
// Purpose: Weapons can call this on secondary attack and it will link to the class
// ability
//-----------------------------------------------------------------------------
bool CTFPlayer::DoClassSpecialSkill( void )
{
	bool bDoSkill = false;

	if ( GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY )
	{
		if ( !m_Shared.InCond( TF_COND_INVIS_POWERUP ) )	
		{
			if ( m_Shared.m_flStealthNextChangeTime <= gpGlobals->curtime )
			{
				bool bStealthed = false;
				// Toggle invisibility
				if ( m_Shared.InCond( TF_COND_STEALTHED ) )
				{
					m_Shared.FadeInvis( tf_spy_invis_unstealth_time.GetFloat() );
					bStealthed = true;
				}
				else if ( CanGoInvisible() && ( m_Shared.GetSpyCloakMeter() > 8.0f ) )	// must have over 10% cloak to start
				{
					m_Shared.AddCond( TF_COND_STEALTHED );
					bStealthed = true;
				}

				if ( bStealthed )
				{
					m_Shared.m_flStealthNextChangeTime = gpGlobals->curtime + 0.5;
					bDoSkill = true;
				}
			}
		}
	}

#ifdef GAME_DLL
	if ( GetPlayerClass()->GetClassIndex() == TF_CLASS_ENGINEER )
	{
		CTFWeaponBuilder *pBuilder = static_cast<CTFWeaponBuilder*>( Weapon_OwnsThisID( TF_WEAPON_BUILDER ) );

		if ( pBuilder )
		{
			pBuilder->HaulingAttack();
			bDoSkill = true;
		}
	}
#endif

	CTFPipebombLauncher *pPipebombLauncher = static_cast<CTFPipebombLauncher*>( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) );
	CTFCPipebombLauncher *pPipebombLauncherTFC = static_cast<CTFCPipebombLauncher*>( Weapon_OwnsThisID( TFC_WEAPON_PIPEBOMBLAUNCHER ) );

	if ( pPipebombLauncher )
	{
		pPipebombLauncher->SecondaryAttack();
		bDoSkill = true;
	}
	else if ( pPipebombLauncherTFC )
	{
		pPipebombLauncherTFC->SecondaryAttack();
		bDoSkill = true;
	}
	return bDoSkill;
}

bool CTFPlayer::CheckSpecialSkill( void )
{

	if ( m_Shared.IsZombie() )
	{
		return true;
	}

	if ( GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY )
	{
		if ( !m_Shared.InCond( TF_COND_INVIS_POWERUP ) )	
		{
			if ( m_Shared.m_flStealthNextChangeTime <= gpGlobals->curtime )
			{
				bool bStealthed = false;
				// Toggle invisibility
				if ( m_Shared.InCond( TF_COND_STEALTHED ) )
				{
					bStealthed = true;
				}
				else if ( CanGoInvisible() && ( m_Shared.GetSpyCloakMeter() > 8.0f ) )	// must have over 10% cloak to start
				{
					bStealthed = true;
				}

				if ( bStealthed )
				{
					return true;
				}
			}
		}
	}

#ifdef GAME_DLL
	if ( GetPlayerClass()->GetClassIndex() == TF_CLASS_ENGINEER )
	{
		CTFWeaponBuilder *pBuilder = static_cast<CTFWeaponBuilder*>( Weapon_OwnsThisID( TF_WEAPON_BUILDER ) );

		if ( pBuilder )
			return true;
	}
#endif

//	Weapon specific skills

	if( m_Shared.InCond( TF_COND_SHIELD_CHARGE ) )
		return true;
	
	if( m_Shared.InCond( TF_COND_AIMING ) )
		return true;
	
	CTFPipebombLauncher *pPipebombLauncher = static_cast<CTFPipebombLauncher*>( Weapon_OwnsThisID( TF_WEAPON_PIPEBOMBLAUNCHER ) );
	CTFCPipebombLauncher *pPipebombLauncherTFC = static_cast<CTFCPipebombLauncher*>( Weapon_OwnsThisID( TFC_WEAPON_PIPEBOMBLAUNCHER ) );

	if ( pPipebombLauncher && pPipebombLauncher->m_Pipebombs.Count() )
		return true;
	else if ( pPipebombLauncherTFC && pPipebombLauncherTFC->m_Pipebombs.Count() )
		return true;

	return false;
}

bool CTFPlayer::ShouldQuickZoom( void )
{	
	bool bAllow = false;
#ifdef GAME_DLL
	
	bAllow = IsFakeClient() ? 0 : Q_atoi( engine->GetClientConVarValue( entindex(), "cl_quickzoom" ) ) > 0;
#else
	bAllow = cl_quickzoom.GetBool();
#endif
	return bAllow;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayer::CanGoInvisible( void )
{
	if ( HasItem() && GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		HintMessage( HINT_CANNOT_CLOAK_WITH_FLAG );
		return false;
	}

	CTFGameRules *pRules = TFGameRules();

	Assert( pRules );

	if ( ( pRules->State_Get() == GR_STATE_TEAM_WIN ) && ( pRules->GetWinningTeam() != GetTeamNumber() ) )
	{
		return false;
	}

	return true;
}

//ConVar testclassviewheight( "testclassviewheight", "0", FCVAR_DEVELOPMENTONLY );
//Vector vecTestViewHeight(0,0,0);

//-----------------------------------------------------------------------------
// Purpose: Return class-specific standing eye height
//-----------------------------------------------------------------------------
Vector CTFPlayer::GetClassEyeHeight( void )
{
	CTFPlayerClass *pClass = GetPlayerClass();

	if ( !pClass )
		return VEC_VIEW;

	//if ( testclassviewheight.GetFloat() > 0 )
	//{
	//	vecTestViewHeight.z = test.GetFloat();
	//	return vecTestViewHeight;
	//}

	int iClassIndex = pClass->GetClassIndex();

	if ( iClassIndex < TF_FIRST_NORMAL_CLASS || iClassIndex > TF_CLASS_COUNT_ALL )
		return VEC_VIEW;

	Vector ViewHeight(0, 0, 0);
	ViewHeight.z = pClass->GetData()->m_nViewVector;

	return ( ViewHeight * GetModelScale());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseEntity *CTFPlayer::MedicGetHealTarget( void )
{
	CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun *>( GetActiveTFWeapon() );
	if (pMedigun)
		return pMedigun->GetHealTarget();

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFWeaponBase *CTFPlayer::Weapon_OwnsThisID( int iWeaponID )
{
	for (int i = 0;i < WeaponCount(); i++) 
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon( i );

		if ( pWpn == NULL )
			continue;

		if ( pWpn->GetWeaponID() == iWeaponID )
		{
			return pWpn;
		}
	}

	return NULL;
}

CTFWeaponBase *CTFPlayer::Weapon_GetWeaponByType( int iType )
{
	for (int i = 0;i < WeaponCount(); i++) 
	{
		CTFWeaponBase *pWpn = ( CTFWeaponBase *)GetWeapon( i );

		if ( pWpn == NULL )
			continue;
	
		int iWeaponRole = pWpn->GetTFWpnData().m_iWeaponType;
	int	iClass = GetPlayerClass()->GetClassIndex();
	
	if (pWpn->GetTFWpnData().m_iClassWeaponType[iClass] >= 0)
		iWeaponRole = pWpn->GetTFWpnData().m_iClassWeaponType[iClass];
	
		if ( iWeaponRole == iType )
		{
			return pWpn;
		}
	}

	return NULL;

}

int CTFPlayer::GetMaxAmmo( int iAmmoIndex, int iClassNumber /*= -1*/ )
{
	if ( !GetPlayerClass()->GetData() )
		return 0;

	int iMaxAmmo = 0;

	if ( iClassNumber != -1 )
	{
		iMaxAmmo = GetPlayerClassData( iClassNumber )->m_aAmmoMax[iAmmoIndex];
	}
	else
	{
		iMaxAmmo = GetPlayerClass()->GetData()->m_aAmmoMax[iAmmoIndex];
	}

	// If we have a weapon that overrides max ammo, use its value.
	// BUG: If player has multiple weapons using same ammo type then only the first one's value is used.
	for ( int i = 0; i < WeaponCount(); i++ )
	{
		CTFWeaponBase *pWpn = (CTFWeaponBase *)GetWeapon( i );

		if ( !pWpn )
			continue;

		if ( pWpn->GetPrimaryAmmoType() != iAmmoIndex )
			continue;
	}

	switch ( iAmmoIndex )
	{
	case TF_AMMO_PRIMARY:
		//CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_primary );
		break;

	case TF_AMMO_SECONDARY:
		//CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_secondary );
		break;

	case TF_AMMO_METAL:
		//CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_metal );
		break;

	case TF_AMMO_GRENADES1:
		//CALL_ATTRIB_HOOK_INT( iMaxAmmo, mult_maxammo_grenades1 );
		break;

	case 6:
	default:
		iMaxAmmo = 1;
		break;
	}

	return iMaxAmmo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::InCondUber( void )
{
	if ( InCond( TF_COND_INVULNERABLE ) || InCond( TF_COND_SPAWNPROTECT ) )
		return true;
	else
		return false;
}
void CTFPlayerShared::RemoveCondUber( void )
{
	RemoveCond( TF_COND_INVULNERABLE );
	RemoveCond( TF_COND_SPAWNPROTECT );
}

bool CTFPlayerShared::InCondShield( void )
{
	if ( InCond( TF_COND_SHIELD ) )
		return true;
	else
		return false;
}
void CTFPlayerShared::RemoveCondShield( void )
{
	RemoveCond( TF_COND_SHIELD );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::InCondCrit( void )
{
	if ( InCond( TF_COND_CRITBOOSTED ) || InCond( TF_COND_CRIT_POWERUP ) || InCond( TF_COND_CRITBOOSTED_DEMO_CHARGE ) )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveCondCrit( void )
{
	RemoveCond( TF_COND_CRITBOOSTED );
	RemoveCond( TF_COND_CRIT_POWERUP );
	RemoveCond( TF_COND_CRITBOOSTED_DEMO_CHARGE );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerShared::InCondInvis( void )
{
	if ( InCond( TF_COND_STEALTHED ) || InCond( TF_COND_INVIS_POWERUP ) )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerShared::RemoveCondInvis( void )
{
	RemoveCond( TF_COND_STEALTHED );
	RemoveCond( TF_COND_INVIS_POWERUP );
}

//-----------------------------------------------------------------------------
// Purpose: check if player has any powerup
//-----------------------------------------------------------------------------

bool CTFPlayerShared::InPowerupCond()
{
	for (int i = COND_FIRST_POWERUP; i < COND_LAST_POWERUP; i++)
	{
		if (InCond(i))
			return true;
	}
	return false;
}