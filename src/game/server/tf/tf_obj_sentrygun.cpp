//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Engineer's Sentrygun OMG
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_obj_sentrygun.h"
#include "bot/tf_bot.h"
#include "tf_team.h"
#include "te_effect_dispatch.h"
#include "tf_gamerules.h"
#include "ammodef.h"
#include "ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool IsInCommentaryMode();

// Ground placed version
#define SENTRY_MODEL_PLACEMENT			"models/buildables/sentry1_blueprint.mdl"
#define SENTRY_MODEL_LEVEL_1			"models/buildables/sentry1.mdl"
#define SENTRY_MODEL_LEVEL_1_UPGRADE	"models/buildables/sentry1_heavy.mdl"
#define SENTRY_MODEL_LEVEL_2			"models/buildables/sentry2.mdl"
#define SENTRY_MODEL_LEVEL_2_UPGRADE	"models/buildables/sentry2_heavy.mdl"
#define SENTRY_MODEL_LEVEL_3			"models/buildables/sentry3.mdl"
#define SENTRY_MODEL_LEVEL_3_UPGRADE	"models/buildables/sentry3_heavy.mdl"

#define SENTRY_ROCKET_MODEL "models/buildables/sentry3_rockets.mdl"

#define SENTRYGUN_MINS			Vector(-20, -20, 0)
#define SENTRYGUN_MAXS			Vector( 20,  20, 66)

#define SENTRYGUN_MAX_HEALTH	150

#define SENTRYGUN_ADD_SHELLS	40
#define SENTRYGUN_ADD_ROCKETS	8

#define SENTRY_THINK_DELAY	0.05

#define	SENTRYGUN_CONTEXT	"SentrygunContext"

#define SENTRYGUN_RECENTLY_ATTACKED_TIME 2.0

#define SENTRYGUN_MINIGUN_RESIST_LVL_1		0.0
#define SENTRYGUN_MINIGUN_RESIST_LVL_2		0.2
#define SENTRYGUN_MINIGUN_RESIST_LVL_3		0.33

#define SENTRYGUN_SAPPER_OWNER_DAMAGE_MODIFIER	0.33f

enum
{	
	SENTRYGUN_ATTACHMENT_MUZZLE = 0,
	SENTRYGUN_ATTACHMENT_MUZZLE_ALT,
	SENTRYGUN_ATTACHMENT_ROCKET_L,
	SENTRYGUN_ATTACHMENT_ROCKET_R,
};

enum target_ranges
{
	RANGE_MELEE,
	RANGE_NEAR,
	RANGE_MID,
	RANGE_FAR,
};

#define VECTOR_CONE_TF_SENTRY		Vector( 0.1, 0.1, 0 )

//-----------------------------------------------------------------------------
// Purpose: Only send the LocalWeaponData to the player carrying the weapon
//-----------------------------------------------------------------------------
void* SendProxy_SendLocalObjectDataTable( const SendProp *pProp, const void *pStruct, const void *pVarData, CSendProxyRecipients *pRecipients, int objectID )
{
	// Get the weapon entity
	CBaseObject *pObject = (CBaseObject*)pVarData;
	if ( pObject )
	{
		// Only send this chunk of data to the player carrying this weapon
		CBasePlayer *pPlayer = ToBasePlayer( pObject->GetOwner() );
		if ( pPlayer )
		{
			pRecipients->SetOnly( pPlayer->GetClientIndex() );
			return (void*)pVarData;
		}
	}

	return NULL;
}
REGISTER_SEND_PROXY_NON_MODIFIED_POINTER( SendProxy_SendLocalObjectDataTable );

BEGIN_NETWORK_TABLE_NOBASE( CObjectSentrygun, DT_SentrygunLocalData )
	SendPropInt( SENDINFO(m_iKills), 12, SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO(m_iAssists), 12, SPROP_CHANGES_OFTEN ),
END_NETWORK_TABLE()

IMPLEMENT_SERVERCLASS_ST( CObjectSentrygun, DT_ObjectSentrygun )
	SendPropInt( SENDINFO(m_iAmmoShells), 9, SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO(m_iAmmoRockets), 6, SPROP_CHANGES_OFTEN ),
	SendPropInt( SENDINFO(m_iState), Q_log2( SENTRY_NUM_STATES ) + 1, SPROP_UNSIGNED ),
	SendPropDataTable( "SentrygunLocalData", 0, &REFERENCE_SEND_TABLE( DT_SentrygunLocalData ), SendProxy_SendLocalObjectDataTable ),
END_SEND_TABLE()

BEGIN_DATADESC( CObjectSentrygun )
END_DATADESC()

LINK_ENTITY_TO_CLASS(obj_sentrygun, CObjectSentrygun);
//PRECACHE_REGISTER(obj_sentrygun);

ConVar tf_sentrygun_damage( "tf_sentrygun_damage", "16", FCVAR_CHEAT );
ConVar tf_sentrygun_ammocheat( "tf_sentrygun_ammocheat", "0", FCVAR_CHEAT );
ConVar tf_sentrygun_upgrade_per_hit( "tf_sentrygun_upgrade_per_hit", "25", FCVAR_CHEAT );
ConVar tf_sentrygun_newtarget_dist( "tf_sentrygun_newtarget_dist", "200", FCVAR_CHEAT );
ConVar tf_sentrygun_metal_per_shell( "tf_sentrygun_metal_per_shell", "1", FCVAR_CHEAT );
ConVar tf_sentrygun_metal_per_rocket( "tf_sentrygun_metal_per_rocket", "2", FCVAR_CHEAT );
ConVar tf_sentrygun_notarget( "tf_sentrygun_notarget", "0", FCVAR_CHEAT );

extern ConVar tf_cheapobjects;
extern ConVar of_infiniteammo;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CObjectSentrygun::CObjectSentrygun()
{
	SetMaxHealth( SENTRYGUN_MAX_HEALTH );
	m_iHealth = SENTRYGUN_MAX_HEALTH;
	SetType( OBJ_SENTRYGUN );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::Spawn()
{
	m_iPitchPoseParameter = -1;
	m_iYawPoseParameter = -1;

	SetModel( SENTRY_MODEL_PLACEMENT );
	
	m_takedamage = DAMAGE_AIM;

	SetMaxHealth( SENTRYGUN_MAX_HEALTH );
	SetHealth( SENTRYGUN_MAX_HEALTH );

	// Rotate Details
	m_iRightBound = 45;
	m_iLeftBound = 315;
	m_iBaseTurnRate = 6;
	m_flFieldOfView = VIEW_FIELD_NARROW;

	// Give the Gun some ammo
	m_iAmmoShells = 0;
	m_iAmmoRockets = 0;
	m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_1;
	m_iMaxAmmoRockets = SENTRYGUN_MAX_ROCKETS;

	m_iAmmoType = GetAmmoDef()->Index( "TF_AMMO_PRIMARY" );

	// Start searching for enemies
	m_hEnemy = NULL;

	m_flLastAttackedTime = 0;

	m_flHeavyBulletResist = SENTRYGUN_MINIGUN_RESIST_LVL_1;

	m_fireTimer.Start();

	BaseClass::Spawn();

	SetViewOffset( SENTRYGUN_EYE_OFFSET_LEVEL_1 );

	UTIL_SetSize( this, SENTRYGUN_MINS, SENTRYGUN_MAXS );

	m_iState.Set( SENTRY_STATE_INACTIVE );

	SetContextThink( &CObjectSentrygun::SentryThink, gpGlobals->curtime + SENTRY_THINK_DELAY, SENTRYGUN_CONTEXT );
}

void CObjectSentrygun::SentryThink( void )
{
	switch( m_iState )
	{
	case SENTRY_STATE_INACTIVE:
		break;

	case SENTRY_STATE_SEARCHING:
		SentryRotate();
		break;

	case SENTRY_STATE_ATTACKING:
		Attack();
		break;

	case SENTRY_STATE_UPGRADING:
		UpgradeThink();
		break;

	default:
		Assert( 0 );
		break;
	}

	SetContextThink( &CObjectSentrygun::SentryThink, gpGlobals->curtime + SENTRY_THINK_DELAY, SENTRYGUN_CONTEXT );
}

void CObjectSentrygun::StartPlacement( CTFPlayer *pPlayer )
{
	BaseClass::StartPlacement( pPlayer );

	// Set my build size
	m_vecBuildMins = SENTRYGUN_MINS;
	m_vecBuildMaxs = SENTRYGUN_MAXS;
	m_vecBuildMins -= Vector( 4,4,0 );
	m_vecBuildMaxs += Vector( 4,4,0 );
}

void CObjectSentrygun::StartHauling( void )
{
	BaseClass::StartHauling();

	SetModel( SENTRY_MODEL_PLACEMENT );

	m_iState.Set( SENTRY_STATE_INACTIVE );
	m_hEnemy = NULL;

	// Set my build size
	m_vecBuildMins = SENTRYGUN_MINS;
	m_vecBuildMaxs = SENTRYGUN_MAXS;
	m_vecBuildMins -= Vector( 4,4,0 );
	m_vecBuildMaxs += Vector( 4,4,0 );
}

//-----------------------------------------------------------------------------
// Purpose: Start building the object
//-----------------------------------------------------------------------------
bool CObjectSentrygun::StartBuilding( CBaseEntity *pBuilder )
{
	SetModel( SENTRY_MODEL_LEVEL_1_UPGRADE );

	CreateBuildPoints();

	SetPoseParameter( m_iPitchPoseParameter, 0.0 );
	SetPoseParameter( m_iYawPoseParameter, 0.0 );

	return BaseClass::StartBuilding( pBuilder );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::OnGoActive( void )
{
	SetModel( SENTRY_MODEL_LEVEL_1 );

	m_iState.Set( SENTRY_STATE_SEARCHING );

	// Orient it
	QAngle angles = GetAbsAngles();

	m_vecCurAngles.y = UTIL_AngleMod( angles.y );
	m_iRightBound = UTIL_AngleMod( (int)angles.y - 50 );
	m_iLeftBound = UTIL_AngleMod( (int)angles.y + 50 );
	if ( m_iRightBound > m_iLeftBound )
	{
		m_iRightBound = m_iLeftBound;
		m_iLeftBound = UTIL_AngleMod( (int)angles.y - 50);
	}

	// Start it rotating
	m_vecGoalAngles.y = m_iRightBound;
	m_vecGoalAngles.x = m_vecCurAngles.x = 0;
	m_bTurningRight = true;

	EmitSound( "Building_Sentrygun.Built" );

	// if our eye pos is underwater, we're waterlevel 3, else 0
	bool bUnderwater = ( UTIL_PointContents( EyePosition() ) & MASK_WATER ) ? true : false;
	SetWaterLevel( ( bUnderwater ) ? 3 : 0 );	

	m_iAmmoShells = m_iMaxAmmoShells;

	// Init attachments for level 1 sentry gun
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE] = LookupAttachment( "muzzle" );
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT] = 0;
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_L] = 0;
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_R] = 0;

	BaseClass::OnGoActive();

	// this is for the defaultupgrade keyvalue useable in hammer
	// if the keyvalue has a number more than 0 it will change the building to this level
    // you can use levels higher than 3 with this to cause havoc
	if ( m_iDefaultUpgrade > 0 )
	{
		for ( int i = 0; i < m_iDefaultUpgrade; i++ )
		{
			StartUpgrading();
			FinishUpgrading();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::Precache()
{
	BaseClass::Precache();

	int iModelIndex;

	// Models
	PrecacheModel( SENTRY_MODEL_PLACEMENT );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_1 );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_1_UPGRADE );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_2 );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_2_UPGRADE );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_3 );
	PrecacheGibsForModel( iModelIndex );

	iModelIndex = PrecacheModel( SENTRY_MODEL_LEVEL_3_UPGRADE );
	PrecacheGibsForModel( iModelIndex );

	PrecacheModel( SENTRY_ROCKET_MODEL );
	PrecacheModel( "models/effects/sentry1_muzzle/sentry1_muzzle.mdl" );

	// Sounds
	PrecacheScriptSound( "Building_Sentrygun.Fire" );
	PrecacheScriptSound( "Building_Sentrygun.Fire2" );	// level 2 sentry
	PrecacheScriptSound( "Building_Sentrygun.Fire3" );	// level 3 sentry
	PrecacheScriptSound( "Building_Sentrygun.FireRocket" );
	PrecacheScriptSound( "Building_Sentrygun.Alert" );
	PrecacheScriptSound( "Building_Sentrygun.AlertTarget" );
	PrecacheScriptSound( "Building_Sentrygun.Idle" );
	PrecacheScriptSound( "Building_Sentrygun.Idle2" );	// level 2 sentry
	PrecacheScriptSound( "Building_Sentrygun.Idle3" );	// level 3 sentry
	PrecacheScriptSound( "Building_Sentrygun.Built" );
	PrecacheScriptSound( "Building_Sentrygun.Empty" );

	PrecacheParticleSystem( "sentrydamage_1" );
	PrecacheParticleSystem( "sentrydamage_2" );
	PrecacheParticleSystem( "sentrydamage_3" );
	PrecacheParticleSystem( "sentrydamage_4" );
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool CObjectSentrygun::CanBeUpgraded( CTFPlayer *pPlayer )
{
	// Already upgrading
	if ( m_iState == SENTRY_STATE_UPGRADING )
	{
		return false;
	}

	// can't upgrade while in a hauling state
	if ( m_bHauling )
		return false;

	// only people who have a pda can upgrade (always engineers)
	CTFWeaponBase *pWeapon = NULL;

	if ( pPlayer )
		pWeapon = pPlayer->Weapon_OwnsThisID( TF_WEAPON_PDA_ENGINEER_BUILD );

	if ( pPlayer && !pWeapon )
	{
		return false;	
	}

	// max upgraded
	if ( m_iUpgradeLevel >= 3 )
	{
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Raises the Sentrygun one level
//-----------------------------------------------------------------------------
void CObjectSentrygun::StartUpgrading( void )
{
	// safety measure to stop the game from crashing if the upgrade level somehow attempts to go over 3
	if ( m_iUpgradeLevel >= 3 && !( m_iDefaultUpgrade > 3 ) )
		return;

	m_bUpgrading = true;

	// Increase level
	m_iUpgradeLevel++;

	// more health
	if ( !m_bHauling )
	{
		int iMaxHealth = GetMaxHealth();
		SetMaxHealth( iMaxHealth * 1.2 );
		SetHealth( iMaxHealth * 1.2 );
	}

	EmitSound( "Building_Sentrygun.Built" );
		
	switch( m_iUpgradeLevel )
	{
	case 2:
		SetModel( SENTRY_MODEL_LEVEL_2_UPGRADE );
		m_flHeavyBulletResist = SENTRYGUN_MINIGUN_RESIST_LVL_2;
		SetViewOffset( SENTRYGUN_EYE_OFFSET_LEVEL_2 );
		if ( !m_bHauling )
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_2;
		break;
	case 3:
		SetModel( SENTRY_MODEL_LEVEL_3_UPGRADE );
		if ( !m_bHauling )
			m_iAmmoRockets = SENTRYGUN_MAX_ROCKETS;
		m_flHeavyBulletResist = SENTRYGUN_MINIGUN_RESIST_LVL_3;
		SetViewOffset( SENTRYGUN_EYE_OFFSET_LEVEL_3 );
		if ( !m_bHauling )
			m_iMaxAmmoShells = SENTRYGUN_MAX_SHELLS_3;
		break;
	default:
		Assert(0);
		break;
	}

	// more ammo capability
	if ( !m_bHauling )
		m_iAmmoShells = m_iMaxAmmoShells;

	m_iState.Set( SENTRY_STATE_UPGRADING );

	SetActivity( ACT_OBJ_UPGRADING );

	m_flUpgradeCompleteTime = gpGlobals->curtime + m_flUpgradeDuration;

	RemoveAllGestures();
}

void CObjectSentrygun::FinishUpgrading( void )
{
	if ( m_iUpgradeLevel < m_iOldUpgradeLevel )
	{
		StartUpgrading();
		return;
	}

	m_bUpgrading = false;
	m_bHauling = false;

	m_iState.Set( SENTRY_STATE_SEARCHING );
	m_hEnemy = NULL;

	switch( m_iUpgradeLevel )
	{
	case 2:
		SetModel( SENTRY_MODEL_LEVEL_2 );
		break;
	case 3:
		SetModel( SENTRY_MODEL_LEVEL_3 );
		break;
	default:
		Assert(0);
		break;
	}

	// Look up the new attachments
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE] = LookupAttachment( "muzzle_l" );
	m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT] = LookupAttachment( "muzzle_r" );
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_L] = LookupAttachment( "rocket_l" );
	m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_R] = LookupAttachment( "rocket_r" );

	EmitSound( "Building_Sentrygun.Built" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::FinishedBuilding( void )
{
	if ( m_bHauling )
		m_iState.Set( SENTRY_STATE_SEARCHING );

	BaseClass::FinishedBuilding();
}

//-----------------------------------------------------------------------------
// Playing the upgrade animation
//-----------------------------------------------------------------------------
void CObjectSentrygun::UpgradeThink( void )
{
	if ( gpGlobals->curtime > m_flUpgradeCompleteTime )
	{
		FinishUpgrading();
	}
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectSentrygun::IsUpgrading( void ) const
{
	return ( m_iState == SENTRY_STATE_UPGRADING );
}

//-----------------------------------------------------------------------------
// Hit by a friendly engineer's wrench
//-----------------------------------------------------------------------------
bool CObjectSentrygun::OnWrenchHit( CTFPlayer *pPlayer )
{
	bool bDidWork = false;

	// If the player repairs it at all, we're done
	if ( GetHealth() < GetMaxHealth() )
	{
		if ( Command_Repair( pPlayer ) )
		{
			bDidWork = true;
		}
	}

	// Don't put in upgrade metal until the sentry is fully healed
	if ( !bDidWork && CanBeUpgraded( pPlayer ) )
	{
		int iPlayerMetal = pPlayer->GetAmmoCount( TF_AMMO_METAL );
		int iAmountToAdd = min( tf_sentrygun_upgrade_per_hit.GetInt(), iPlayerMetal );

		if ( iAmountToAdd > ( m_iUpgradeMetalRequired - m_iUpgradeMetal ) )
			iAmountToAdd = ( m_iUpgradeMetalRequired - m_iUpgradeMetal );

		if ( tf_cheapobjects.GetBool() == false )
		{
		
			if ( of_infiniteammo.GetBool() != 1 )
				pPlayer->RemoveAmmo( iAmountToAdd, TF_AMMO_METAL );
		
		}
		m_iUpgradeMetal += iAmountToAdd;

		if ( iAmountToAdd > 0 )
		{
			bDidWork = true;
		}

		if ( m_iUpgradeMetal >= m_iUpgradeMetalRequired )
		{
			StartUpgrading();
			m_iUpgradeMetal = 0;
		}
	}

	if ( !IsUpgrading() )
	{
		// player ammo into rockets
		//	1 ammo = 1 shell
		//  2 ammo = 1 rocket
		//  only fill rockets if we have extra shells

		int iPlayerMetal = pPlayer->GetAmmoCount( TF_AMMO_METAL );

		// If the sentry has less that 100% ammo, put some ammo in it
		if ( m_iAmmoShells < m_iMaxAmmoShells && iPlayerMetal > 0 )
		{
			int iMaxShellsPlayerCanAfford = (int)( (float)iPlayerMetal / tf_sentrygun_metal_per_shell.GetFloat() );

			// cap the amount we can add
			int iAmountToAdd = min( SENTRYGUN_ADD_SHELLS, iMaxShellsPlayerCanAfford );

			iAmountToAdd = min( ( m_iMaxAmmoShells - m_iAmmoShells ), iAmountToAdd );

			if ( of_infiniteammo.GetBool() != 1 )
				pPlayer->RemoveAmmo( iAmountToAdd * tf_sentrygun_metal_per_shell.GetInt(), TF_AMMO_METAL );
			
			m_iAmmoShells += iAmountToAdd;

			if ( iAmountToAdd > 0 )
			{
				bDidWork = true;
			}
		}

		// One rocket per two ammo
		iPlayerMetal = pPlayer->GetAmmoCount( TF_AMMO_METAL );

		if ( m_iAmmoRockets < m_iMaxAmmoRockets && m_iUpgradeLevel == 3 && iPlayerMetal > 0  )
		{
			int iMaxRocketsPlayerCanAfford = (int)( (float)iPlayerMetal / tf_sentrygun_metal_per_rocket.GetFloat() );

			int iAmountToAdd = min( ( SENTRYGUN_ADD_ROCKETS ), iMaxRocketsPlayerCanAfford );
			iAmountToAdd = min( ( m_iMaxAmmoRockets - m_iAmmoRockets ), iAmountToAdd );

			if ( of_infiniteammo.GetBool() != 1 )
				pPlayer->RemoveAmmo( iAmountToAdd * tf_sentrygun_metal_per_rocket.GetFloat(), TF_AMMO_METAL );
			m_iAmmoRockets += iAmountToAdd;

			if ( iAmountToAdd > 0 )
			{
				bDidWork = true;
			}
		}
	}

	return bDidWork;
}

//-----------------------------------------------------------------------------
// Debug infos
//-----------------------------------------------------------------------------
int CObjectSentrygun::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf( tempstr, sizeof( tempstr ), "Level: %d", m_iUpgradeLevel.Get() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Q_snprintf( tempstr, sizeof( tempstr ), "Shells: %d / %d", m_iAmmoShells.Get(), m_iMaxAmmoShells.Get() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		if ( m_iUpgradeLevel == 3 )
		{
			Q_snprintf( tempstr, sizeof( tempstr ), "Rockets: %d / %d", m_iAmmoRockets.Get(), m_iMaxAmmoRockets.Get() );
			EntityText(text_offset,tempstr,0);
			text_offset++;
		}

		Q_snprintf( tempstr, sizeof( tempstr ), "Upgrade metal %d", m_iUpgradeMetal.Get() );
		EntityText(text_offset,tempstr,0);
		text_offset++;

		Vector vecSrc = EyePosition();
		Vector forward;

		// m_vecCurAngles
		AngleVectors( m_vecCurAngles, &forward );
		NDebugOverlay::Line( vecSrc, vecSrc + forward * 200, 0, 255, 0, false, 0.1 );

		// m_vecGoalAngles
		AngleVectors( m_vecGoalAngles, &forward );
		NDebugOverlay::Line( vecSrc, vecSrc + forward * 200, 0, 0, 255, false, 0.1 );
	}

	return text_offset;
}

//-----------------------------------------------------------------------------
// Returns the sentry targeting range the target is in
//-----------------------------------------------------------------------------
int CObjectSentrygun::Range( CBaseEntity *pTarget )
{
	Vector vecOrg = EyePosition();
	Vector vecTargetOrg = pTarget->EyePosition();

	int iDist = ( vecTargetOrg - vecOrg ).Length();

	if (iDist < 132)
		return RANGE_MELEE;
	if (iDist < 550)
		return RANGE_NEAR;
	if (iDist < 1100)
		return RANGE_MID;
	return RANGE_FAR;
}

//-----------------------------------------------------------------------------
// Look for a target
//-----------------------------------------------------------------------------
bool CObjectSentrygun::FindTarget()
{
	// Disable the sentry guns for ifm.
	if ( tf_sentrygun_notarget.GetBool() )
		return false;

	if ( IsInCommentaryMode() )
		return false;

	// Sapper, etc.
	if ( IsDisabled() )
		return false;

	// Loop through players within 1100 units (sentry range).
	Vector vecSentryOrigin = EyePosition();
	
	// Find the opposing team list.
	CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );

	// this will be used to test which team we can shoot happily
	CUtlVector<CTFTeam *> pTeamTest;

	// CTFTeam *pTeam = pPlayer->GetOpposingTFTeam();
	CTFTeam *pTeam = NULL;

	// this isn't compatible with hammer-placed sentries
	//CTFTeam *pTeam = pPlayer->GetOpposingTFTeam();

	if ( pPlayer )
		pTeam = pPlayer->GetTFTeam(); 		// test the team of the engineer who built us first
	else
		pTeam = GetTFTeam(); // no builder means spawned manually so get team number of sentry itself instead

	// test who we can shoot
	// if the sentry has no team it can shoot everyone
	if ( pTeam )
		pTeam->GetOpposingTFTeam( &pTeamTest );
	else
		return false;

	// If we have an enemy get his minimum distance to check against.
	Vector vecSegment;
	Vector vecTargetCenter;
	float flMinDist2 = 1100.0f * 1100.0f;
	CBaseEntity *pTargetCurrent = NULL;
	CBaseEntity *pTargetOld = m_hEnemy.Get();
	float flOldTargetDist2 = FLT_MAX;

	CUtlVector<INextBot *> bots;
	TheNextBots().CollectAllBots( &bots );

	// Sentries will try to target players first, then objects, then AI entities.  However, if the enemy held was an object it will continue
	// to try and attack it first.	
	// the teams are further tested from above
	for ( int i = 0; i < pTeamTest.Size(); i++ )
	{
		if ( !TFGameRules()->IsCoopEnabled() )
		{
			int nTeamCount = pTeamTest[i]->GetNumPlayers();
			for ( int iPlayer = 0; iPlayer < nTeamCount; ++iPlayer )
			{
				CTFPlayer *pTargetPlayer = static_cast<CTFPlayer*>( pTeamTest[i]->GetPlayer( iPlayer ) );

				if ( pTargetPlayer == NULL )
					continue;

				if (pTargetPlayer == pPlayer)
					continue;

				// Make sure the player is alive.
				if ( !pTargetPlayer->IsAlive() )
					continue;

				if ( pTargetPlayer->GetFlags() & FL_NOTARGET )
					continue;

				vecTargetCenter = pTargetPlayer->GetAbsOrigin();
				vecTargetCenter += pTargetPlayer->GetViewOffset();
				VectorSubtract( vecTargetCenter, vecSentryOrigin, vecSegment );
				float flDist2 = vecSegment.LengthSqr();

				// Store the current target distance if we come across it
				if ( pTargetPlayer == pTargetOld )
				{
					flOldTargetDist2 = flDist2;
				}

				// Check to see if the target is closer than the already validated target.
				if ( flDist2 > flMinDist2 )
					continue;

				// It is closer, check to see if the target is valid.
				if ( ValidTargetPlayer( pTargetPlayer, vecSentryOrigin, vecTargetCenter ) )
				{
					flMinDist2 = flDist2;
					pTargetCurrent = pTargetPlayer;
				}
			}

			// If we already have a target, don't check objects.
			if ( pTargetCurrent == NULL )
			{
				int nTeamObjectCount = pTeamTest[i]->GetNumObjects();
				for ( int iObject = 0; iObject < nTeamObjectCount; ++iObject )
				{
					CBaseObject *pTargetObject = pTeamTest[i]->GetObject( iObject );
					if ( !pTargetObject )
						continue;

					// don't attack ourselves
					if (pTargetObject == this)
						continue;
					if (pTargetObject->GetOwner() == pPlayer)
						continue;

					vecTargetCenter = pTargetObject->GetAbsOrigin();
					vecTargetCenter += pTargetObject->GetViewOffset();
					VectorSubtract( vecTargetCenter, vecSentryOrigin, vecSegment );
					float flDist2 = vecSegment.LengthSqr();

					// Store the current target distance if we come across it
					if ( pTargetObject == pTargetOld )
					{
						flOldTargetDist2 = flDist2;
					}

					// Check to see if the target is closer than the already validated target.
					if ( flDist2 > flMinDist2 )
						continue;

					// It is closer, check to see if the target is valid.
					if ( ValidTargetObject( pTargetObject, vecSentryOrigin, vecTargetCenter ) )
					{
						flMinDist2 = flDist2;
						pTargetCurrent = pTargetObject;
					}
				}
			}
		}

		if ( pTargetCurrent == NULL )
		{
			for ( int iBot=0; iBot<bots.Count(); ++iBot )
			{
				CBaseCombatCharacter *pTargetActor = bots[iBot]->GetEntity();
				if ( pTargetActor == NULL )
					continue;

				VectorSubtract( pTargetActor->WorldSpaceCenter(), vecSentryOrigin, vecSegment );
				float flDist2 = vecSegment.LengthSqr();

				// Store the current target distance if we come across it
				if ( pTargetActor == pTargetOld )
				{
					flOldTargetDist2 = flDist2;
				}

				// Check to see if the target is closer than the already validated target.
				if ( flDist2 > flMinDist2 )
					continue;

				// It is closer, check to see if the target is valid.
				if ( ValidTargetBot( pTargetActor ) )
				{
					flMinDist2 = flDist2;
					pTargetCurrent = pTargetActor;
				}
			}
		}
		
		// We have a target.
		if ( pTargetCurrent )
		{
			if ( pTargetCurrent != pTargetOld )
			{
				// flMinDist2 is the new target's distance
				// flOldTargetDist2 is the old target's distance
				// Don't switch unless the new target is closer by some percentage
				if ( flMinDist2 < ( flOldTargetDist2 * 0.75f ) )
				{
					FoundTarget( pTargetCurrent, vecSentryOrigin );
				}
			}
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CObjectSentrygun::ValidTargetPlayer( CTFPlayer *pPlayer, const Vector &vecStart, const Vector &vecEnd )
{
	// Keep shooting at spies that go invisible after we acquire them as a target.
	if ( pPlayer->m_Shared.GetPercentInvisible() > 0.5 )
		return false;

	// Keep shooting at spies that disguise after we acquire them as at a target.
	if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->m_Shared.GetDisguiseTeam() == GetTeamNumber() && pPlayer != m_hEnemy )
		return false;

	// Not across water boundary.
	if ( ( GetWaterLevel() == 0 && pPlayer->GetWaterLevel() >= 3 ) || ( GetWaterLevel() == 3 && pPlayer->GetWaterLevel() <= 0 ) )
		return false;

	// Ray trace!!!
	return FVisible( pPlayer, MASK_SHOT | CONTENTS_GRATE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CObjectSentrygun::ValidTargetObject( CBaseObject *pObject, const Vector &vecStart, const Vector &vecEnd )
{
	// Ignore objects being placed, they are not real objects yet.
	if ( pObject->IsPlacing() )
		return false;

	// Ignore sappers.
	if ( pObject->MustBeBuiltOnAttachmentPoint() )
		return false;

	// Not across water boundary.
	if ( ( GetWaterLevel() == 0 && pObject->GetWaterLevel() >= 3 ) || ( GetWaterLevel() == 3 && pObject->GetWaterLevel() <= 0 ) )
		return false;

	// Ray trace.
	return FVisible( pObject, MASK_SHOT | CONTENTS_GRATE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CObjectSentrygun::ValidTargetNPC( CAI_BaseNPC *pNPC, const Vector &vecStart, const Vector &vecEnd )
{
	// Not across water boundary.
	if ( ( GetWaterLevel() == 0 && pNPC->GetWaterLevel() >= 3) || ( GetWaterLevel() == 3 && pNPC->GetWaterLevel() <= 0) )
		return false;

	// Ray trace.
	return FVisible( pNPC, MASK_SHOT | CONTENTS_GRATE );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CObjectSentrygun::ValidTargetBot( CBaseCombatCharacter *pActor )
{
	// Players should already be checked, ignore
	if ( pActor->IsPlayer() )
		return false;

	// Ignore the dead
	if ( !pActor->IsAlive() )
		return false;

	// Make sure it's an enemy
	if ( InSameTeam( pActor ) )
		return false;

	// Make sure we can even hit it
	if ( !pActor->IsSolid() )
		return false;

	// Ray trace with respect to parents
	CBaseEntity *pBlocker = nullptr;
	if ( !FVisible( pActor, MASK_SHOT|CONTENTS_GRATE, &pBlocker ) )
	{
		if ( pActor->GetMoveParent() == pBlocker )
			return true;

		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Found a Target
//-----------------------------------------------------------------------------
void CObjectSentrygun::FoundTarget( CBaseEntity *pTarget, const Vector &vecSoundCenter )
{
	m_hEnemy = pTarget;

	if ( ( m_iAmmoShells > 0 ) || ( m_iAmmoRockets > 0 && m_iUpgradeLevel == 3 ) )
	{
		// Play one sound to everyone but the target.
		CPASFilter filter( vecSoundCenter );

		if ( pTarget->IsPlayer() )
		{
			CTFPlayer *pPlayer = ToTFPlayer( pTarget );

			// Play a specific sound just to the target and remove it from the genral recipient list.
			CSingleUserRecipientFilter singleFilter( pPlayer );
			EmitSound( singleFilter, entindex(), "Building_Sentrygun.AlertTarget" );
			filter.RemoveRecipient( pPlayer );

			CTFBot *pBot = ToTFBot( pTarget );
			if ( pBot )
			{
				pBot->m_hTargetSentry = this;
				pBot->m_vecLastHurtBySentry = GetAbsOrigin();
			}			
		}

		EmitSound( filter, entindex(), "Building_Sentrygun.Alert" );
	}

	// Update timers, we are attacking now!
	m_iState.Set( SENTRY_STATE_ATTACKING );
	m_flNextAttack = gpGlobals->curtime + SENTRY_THINK_DELAY;
	if ( m_flNextRocketAttack < gpGlobals->curtime )
	{
		m_flNextRocketAttack = gpGlobals->curtime + 0.5;
	}
}

//-----------------------------------------------------------------------------
// FInViewCone - returns true is the passed ent is in
// the caller's forward view cone. The dot product is performed
// in 2d, making the view cone infinitely tall. 
//-----------------------------------------------------------------------------
bool CObjectSentrygun::FInViewCone ( CBaseEntity *pEntity )
{
	Vector forward;
	AngleVectors( m_vecCurAngles, &forward );

	Vector2D vec2LOS = ( pEntity->GetAbsOrigin() - GetAbsOrigin() ).AsVector2D();
	vec2LOS.NormalizeInPlace();

	float flDot = vec2LOS.Dot( forward.AsVector2D() );

	if ( flDot > m_flFieldOfView )
	{
		return true;
	}
	else
	{
		return false;
	}
}

//-----------------------------------------------------------------------------
// Make sure our target is still valid, and if so, fire at it
//-----------------------------------------------------------------------------
void CObjectSentrygun::Attack()
{
	StudioFrameAdvance( );

	if ( !FindTarget() )
	{
		m_iState.Set( SENTRY_STATE_SEARCHING );
		m_hEnemy = NULL;
		return;
	}

	// Track enemy
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = m_hEnemy->WorldSpaceCenter();
	Vector vecDirToEnemy = vecMidEnemy - vecMid;

	QAngle angToTarget;
	VectorAngles( vecDirToEnemy, angToTarget );

	angToTarget.y = UTIL_AngleMod( angToTarget.y );
	if (angToTarget.x < -180)
		angToTarget.x += 360;
	if (angToTarget.x > 180)
		angToTarget.x -= 360;

	// now all numbers should be in [1...360]
	// pin to turret limitations to [-50...50]
	if (angToTarget.x > 50)
		angToTarget.x = 50;
	else if (angToTarget.x < -50)
		angToTarget.x = -50;
	m_vecGoalAngles.y = angToTarget.y;
	m_vecGoalAngles.x = angToTarget.x;

	MoveTurret();

	// Fire on the target if it's within 10 units of being aimed right at it
	if ( m_flNextAttack <= gpGlobals->curtime && (m_vecGoalAngles - m_vecCurAngles).Length() <= 10 )
	{
		Fire();

		if ( m_iUpgradeLevel == 1 )
		{
			// Level 1 sentries fire slower
			m_flNextAttack = gpGlobals->curtime + 0.2;
		}
		else
		{
			m_flNextAttack = gpGlobals->curtime + 0.1;
		}
	}
	else
	{
		// SetSentryAnim( TFTURRET_ANIM_SPIN );
	}
}

//-----------------------------------------------------------------------------
// Fire on our target
//-----------------------------------------------------------------------------
bool CObjectSentrygun::Fire()
{
	//NDebugOverlay::Cross3D( m_hEnemy->WorldSpaceCenter(), 10, 255, 0, 0, false, 0.1 );

	Vector vecAimDir;

	// Level 3 Turrets fire rockets every 3 seconds
	if ( m_iUpgradeLevel == 3 &&
		m_iAmmoRockets > 0 &&
		m_flNextRocketAttack < gpGlobals->curtime )
	{
		Vector vecSrc;
		QAngle vecAng;

		// alternate between the 2 rocket launcher ports.
		if ( m_iAmmoRockets & 1 )
		{
			GetAttachment( m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_L], vecSrc, vecAng );
		}
		else
		{
			GetAttachment( m_iAttachments[SENTRYGUN_ATTACHMENT_ROCKET_R], vecSrc, vecAng );
		}

		vecAimDir = m_hEnemy->WorldSpaceCenter() - vecSrc;
		vecAimDir.NormalizeInPlace();

		// NOTE: vecAng is not actually set by GetAttachment!!!
		QAngle angDir;
		VectorAngles( vecAimDir, angDir );

		EmitSound( "Building_Sentrygun.FireRocket" );

		AddGesture( ACT_RANGE_ATTACK2 );

		QAngle angAimDir;
		VectorAngles( vecAimDir, angAimDir );
		CTFProjectile_SentryRocket *pProjectile = CTFProjectile_SentryRocket::Create( vecSrc, angAimDir, this, GetBuilder() );
		if ( pProjectile )
		{
			if ( TFGameRules() && TFGameRules()->IsInfGamemode() )
				pProjectile->SetDamage( 20 );
			else
				pProjectile->SetDamage( 100 );
		}

		// Setup next rocket shot
		m_flNextRocketAttack = gpGlobals->curtime + 3;

		m_fireTimer.Start();

		//if ( !tf_sentrygun_ammocheat.GetBool() )
		if ( !tf_sentrygun_ammocheat.GetBool() && !HasSpawnFlags( SF_SENTRY_INFINITE_AMMO ) )
		{
			m_iAmmoRockets--;
		}

		if (m_iAmmoRockets == 10)
			ClientPrint( GetBuilder(), HUD_PRINTNOTIFY, "#Sentry_rocketslow");
		if (m_iAmmoRockets == 0)
			ClientPrint( GetBuilder(), HUD_PRINTNOTIFY, "#Sentry_rocketsout");
	}

	// All turrets fire shells
	if ( m_iAmmoShells > 0)
	{
		if ( !IsPlayingGesture( ACT_RANGE_ATTACK1 ) )
		{
			RemoveGesture( ACT_RANGE_ATTACK1_LOW );
			AddGesture( ACT_RANGE_ATTACK1 );
		}

		Vector vecSrc;
		QAngle vecAng;

		int iAttachment;

		if ( m_iUpgradeLevel > 1 && (m_iAmmoShells & 1) )
		{
			// level 2 and 3 turrets alternate muzzles each time they fizzy fizzy fire.
			iAttachment = m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE_ALT];
		}
		else
		{
			iAttachment = m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE];
		}

		GetAttachment( iAttachment, vecSrc, vecAng );

		Vector vecMidEnemy = m_hEnemy->WorldSpaceCenter();

		// If we cannot see their WorldSpaceCenter ( possible, as we do our target finding based
		// on the eye position of the target ) then fire at the eye position
		trace_t tr;
		UTIL_TraceLine( vecSrc, vecMidEnemy, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);

		if ( !tr.m_pEnt || tr.m_pEnt->IsWorld() )
		{
			// Hack it lower a little bit..
			// The eye position is not always within the hitboxes for a standing TF Player
			vecMidEnemy = m_hEnemy->EyePosition() + Vector(0,0,-5);
		}

		vecAimDir = vecMidEnemy - vecSrc;

		float flDistToTarget = vecAimDir.Length();

		vecAimDir.NormalizeInPlace();

		//NDebugOverlay::Cross3D( vecSrc, 10, 255, 0, 0, false, 0.1 );

		FireBulletsInfo_t info;

		info.m_vecSrc = vecSrc;
		info.m_vecDirShooting = vecAimDir;
		info.m_iTracerFreq = 1;
		info.m_iShots = 1;
		info.m_pAttacker = GetBuilder();
		info.m_vecSpread = vec3_origin;
		info.m_flDistance = flDistToTarget + 100;
		info.m_iAmmoType = m_iAmmoType;

		if ( TFGameRules() && TFGameRules()->IsInfGamemode() )
			info.m_flDamage = tf_sentrygun_damage.GetFloat() * 0.2;
		else
			info.m_flDamage = tf_sentrygun_damage.GetFloat();

		FireBullets( info );

		//NDebugOverlay::Line( vecSrc, vecSrc + vecAimDir * 1000, 255, 0, 0, false, 0.1 );

		CEffectData data;
		data.m_nEntIndex = entindex();
		data.m_nAttachmentIndex = iAttachment;
		data.m_fFlags = m_iUpgradeLevel;
		data.m_vOrigin = vecSrc;
		DispatchEffect( "TF_3rdPersonMuzzleFlash_SentryGun", data );

		switch( m_iUpgradeLevel )
		{
		case 1:
		default:
			EmitSound( "Building_Sentrygun.Fire" );
			break;
		case 2:
			EmitSound( "Building_Sentrygun.Fire2" );
			break;
		case 3:
			EmitSound( "Building_Sentrygun.Fire3" );
			break;
		}

		//if ( !tf_sentrygun_ammocheat.GetBool() )
		if ( !tf_sentrygun_ammocheat.GetBool() && !HasSpawnFlags( SF_SENTRY_INFINITE_AMMO ) )
		{
			m_iAmmoShells--;
		}
	}
	else
	{
		if ( m_iUpgradeLevel > 1 )
		{
			if ( !IsPlayingGesture( ACT_RANGE_ATTACK1_LOW ) )
			{
				RemoveGesture( ACT_RANGE_ATTACK1 );
				AddGesture( ACT_RANGE_ATTACK1_LOW );
			}
		}

		// Out of ammo, play a click
		EmitSound( "Building_Sentrygun.Empty" );
		m_flNextAttack = gpGlobals->curtime + 0.2;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CObjectSentrygun::GetTimeSinceLastFired( void ) const
{
	return m_fireTimer.GetElapsedTime();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	trace_t tmptrace;
	tmptrace.endpos = tr.endpos + RandomVector(-10,10);

	// Sentryguns are perfectly accurate, but this doesn't look good for tracers.
	// Add a little noise to them, but not enough so that it looks like they're missing.
	BaseClass::MakeTracer( vecTracerSrc, tmptrace, iTracerType );
}

//-----------------------------------------------------------------------------
// Purpose: MakeTracer asks back for the attachment index
//-----------------------------------------------------------------------------
int	CObjectSentrygun::GetTracerAttachment( void )
{
	return m_iAttachments[SENTRYGUN_ATTACHMENT_MUZZLE];
}

//-----------------------------------------------------------------------------
// Rotate and scan for targets
//-----------------------------------------------------------------------------
void CObjectSentrygun::SentryRotate( void )
{
	// if we're playing a fire gesture, stop it
	if ( IsPlayingGesture( ACT_RANGE_ATTACK1 ) )
	{
		RemoveGesture( ACT_RANGE_ATTACK1 );
	}

	if ( IsPlayingGesture( ACT_RANGE_ATTACK1_LOW ) )
	{
		RemoveGesture( ACT_RANGE_ATTACK1_LOW );
	}

	// animate
	StudioFrameAdvance();

	// Look for a target
	if ( FindTarget() )
		return;

	// Rotate
	if ( !MoveTurret() )
	{
		// Change direction

		if ( IsDisabled() )
		{
			EmitSound( "Building_Sentrygun.Disabled" );
			m_vecGoalAngles.x = 30;
		}
		else
		{
			switch( m_iUpgradeLevel )
			{
			case 1:
			default:
				EmitSound( "Building_Sentrygun.Idle" );
				break;
			case 2:
				EmitSound( "Building_Sentrygun.Idle2" );
				break;
			case 3:
				EmitSound( "Building_Sentrygun.Idle3" );
				break;
			}

			// Switch rotation direction
			if ( m_bTurningRight )
			{
				m_bTurningRight = false;
				m_vecGoalAngles.y = m_iLeftBound;
			}
			else
			{
				m_bTurningRight = true;
				m_vecGoalAngles.y = m_iRightBound;
			}

			// Randomly look up and down a bit
			if (random->RandomFloat(0, 1) < 0.3)
			{
				m_vecGoalAngles.x = (int)random->RandomFloat(-10,10);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add the EMP effect
//-----------------------------------------------------------------------------
void CObjectSentrygun::OnStartDisabled( void )
{
	// stay at current rotation, angle down
	m_vecGoalAngles.x = m_vecCurAngles.x;
	m_vecGoalAngles.y = m_vecCurAngles.y;

	// target = nULL

	BaseClass::OnStartDisabled();
}

//-----------------------------------------------------------------------------
// Purpose: Remove the EMP effect
//-----------------------------------------------------------------------------
void CObjectSentrygun::OnEndDisabled( void )
{
	// return to normal rotations
	if ( m_bTurningRight )
	{
		m_bTurningRight = false;
		m_vecGoalAngles.y = m_iLeftBound;
	}
	else
	{
		m_bTurningRight = true;
		m_vecGoalAngles.y = m_iRightBound;
	}

	m_vecGoalAngles.x = 0;

	BaseClass::OnEndDisabled();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CObjectSentrygun::GetBaseTurnRate( void )
{
	return m_iBaseTurnRate;
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
bool CObjectSentrygun::MoveTurret( void )
{
	bool bMoved = false;

	int iBaseTurnRate = GetBaseTurnRate();

	// any x movement?
	if ( m_vecCurAngles.x != m_vecGoalAngles.x )
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1 : -1 ;

		m_vecCurAngles.x += SENTRY_THINK_DELAY * ( iBaseTurnRate * 5 ) * flDir;

		// if we started below the goal, and now we're past, peg to goal
		if ( flDir == 1 )
		{
			if (m_vecCurAngles.x > m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		} 
		else
		{
			if (m_vecCurAngles.x < m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}

		SetPoseParameter( m_iPitchPoseParameter, -m_vecCurAngles.x );

		bMoved = true;
	}

	if ( m_vecCurAngles.y != m_vecGoalAngles.y )
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1 : -1 ;
		float flDist = fabs( m_vecGoalAngles.y - m_vecCurAngles.y );
		bool bReversed = false;

		if ( flDist > 180 )
		{
			flDist = 360 - flDist;
			flDir = -flDir;
			bReversed = true;
		}

		if ( m_hEnemy.Get() == NULL )
		{
			if ( flDist > 30 )
			{
				if ( m_flTurnRate < iBaseTurnRate * 10 )
				{
					m_flTurnRate += iBaseTurnRate;
				}
			}
			else
			{
				// Slow down
				if ( m_flTurnRate > (iBaseTurnRate * 5) )
					m_flTurnRate -= iBaseTurnRate;
			}
		}
		else
		{
			// When tracking enemies, move faster and don't slow
			if ( flDist > 30 )
			{
				if (m_flTurnRate < iBaseTurnRate * 30)
				{
					m_flTurnRate += iBaseTurnRate * 3;
				}
			}
		}

		m_vecCurAngles.y += SENTRY_THINK_DELAY * m_flTurnRate * flDir;

		// if we passed over the goal, peg right to it now
		if (flDir == -1)
		{
			if ( (bReversed == false && m_vecGoalAngles.y > m_vecCurAngles.y) ||
				(bReversed == true && m_vecGoalAngles.y < m_vecCurAngles.y) )
			{
				m_vecCurAngles.y = m_vecGoalAngles.y;
			}
		} 
		else
		{
			if ( (bReversed == false && m_vecGoalAngles.y < m_vecCurAngles.y) ||
                (bReversed == true && m_vecGoalAngles.y > m_vecCurAngles.y) )
			{
				m_vecCurAngles.y = m_vecGoalAngles.y;
			}
		}

		if ( m_vecCurAngles.y < 0 )
		{
			m_vecCurAngles.y += 360;
		}
		else if ( m_vecCurAngles.y >= 360 )
		{
			m_vecCurAngles.y -= 360;
		}

		if ( flDist < ( SENTRY_THINK_DELAY * 0.5 * iBaseTurnRate ) )
		{
			m_vecCurAngles.y = m_vecGoalAngles.y;
		}

		QAngle angles = GetAbsAngles();

		float flYaw = m_vecCurAngles.y - angles.y;

		SetPoseParameter( m_iYawPoseParameter, -flYaw );

		InvalidatePhysicsRecursive( ANIMATION_CHANGED );

		bMoved = true;
	}

	if ( !bMoved || m_flTurnRate <= 0 )
	{
		m_flTurnRate = iBaseTurnRate * 5;
	}

	return bMoved;
}

//-----------------------------------------------------------------------------
// Purpose: Note our last attacked time
//-----------------------------------------------------------------------------
int CObjectSentrygun::OnTakeDamage( const CTakeDamageInfo &info )
{
	CTakeDamageInfo newInfo = info;

	// As we increase in level, we get more resistant to minigun bullets, to compensate for
	// our increased surface area taking more minigun hits.
	if ( ( info.GetDamageType() & DMG_BULLET ) && ( info.GetDamageCustom() == TF_DMG_CUSTOM_MINIGUN ) )
	{
		float flDamage = newInfo.GetDamage();

		flDamage *= ( 1.0 - m_flHeavyBulletResist );

		newInfo.SetDamage( flDamage );
	}

	// Check to see if we are being sapped.
	if ( HasSapper() )
	{
		// Get the sapper owner.
		CBaseObject *pSapper = GetObjectOfTypeOnMe( OBJ_ATTACHMENT_SAPPER );
		Assert( pSapper );

		// Take less damage if the owner is causing additional damage.
		if ( pSapper && ( info.GetAttacker() == pSapper->GetOwner() ) )
		{
			float flDamage = newInfo.GetDamage() * SENTRYGUN_SAPPER_OWNER_DAMAGE_MODIFIER;
			newInfo.SetDamage( flDamage );
		}
	}

	int iDamageTaken = BaseClass::OnTakeDamage( newInfo );

	if ( iDamageTaken > 0 )
	{
		m_flLastAttackedTime = gpGlobals->curtime;
	}

	return iDamageTaken;
}

//-----------------------------------------------------------------------------
// Purpose: Called when this object is destroyed
//-----------------------------------------------------------------------------
void CObjectSentrygun::Killed( const CTakeDamageInfo &info )
{
	// do normal handling
	BaseClass::Killed( info );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CObjectSentrygun::SetModel( const char *pModel )
{
	float flPoseParam0 = 0.0;
	float flPoseParam1 = 0.0;

	// Save pose parameters across model change
	if ( m_iPitchPoseParameter >= 0 )
	{
		flPoseParam0 = GetPoseParameter( m_iPitchPoseParameter );
	}

	if ( m_iYawPoseParameter >= 0 )
	{
		flPoseParam1 = GetPoseParameter( m_iYawPoseParameter );
	}

	BaseClass::SetModel( pModel );

	// Reset this after model change
	UTIL_SetSize( this, SENTRYGUN_MINS, SENTRYGUN_MAXS );
	SetSolid( SOLID_BBOX );

	// Restore pose parameters
	m_iPitchPoseParameter = LookupPoseParameter( "aim_pitch" );
	m_iYawPoseParameter = LookupPoseParameter( "aim_yaw" );

	SetPoseParameter( m_iPitchPoseParameter, flPoseParam0 );
	SetPoseParameter( m_iYawPoseParameter, flPoseParam1 );

	CreateBuildPoints();

	ReattachChildren();

	ResetSequenceInfo();
}

LINK_ENTITY_TO_CLASS( tf_projectile_sentryrocket, CTFProjectile_SentryRocket );

IMPLEMENT_NETWORKCLASS_ALIASED( TFProjectile_SentryRocket, DT_TFProjectile_SentryRocket )

BEGIN_NETWORK_TABLE( CTFProjectile_SentryRocket, DT_TFProjectile_SentryRocket )
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: Creation
//-----------------------------------------------------------------------------
CTFProjectile_SentryRocket *CTFProjectile_SentryRocket::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer )
{
	CTFProjectile_SentryRocket *pRocket = static_cast<CTFProjectile_SentryRocket*>( CTFBaseRocket::Create( NULL, "tf_projectile_sentryrocket", vecOrigin, vecAngles, pOwner ) );

	if ( pRocket )
	{
		pRocket->SetScorer( pScorer );
	}

	return pRocket;
}

CTFProjectile_SentryRocket::CTFProjectile_SentryRocket()
{
	UseClientSideAnimation();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFProjectile_SentryRocket::Spawn()
{
	BaseClass::Spawn();

	SetModel( SENTRY_ROCKET_MODEL );

	ResetSequence( LookupSequence("idle") );
}
