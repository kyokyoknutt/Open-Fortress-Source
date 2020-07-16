//========= Copyright � Valve LLC, All rights reserved. =======================
//
// Purpose:		
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "team_control_point_master.h"
#include "tf_gamerules.h"
#include "tf_obj_sentrygun.h"
#include "entity_capture_flag.h"
#include "func_capture_zone.h"
#include "tf_bot.h"
#include "tf_bot_components.h"
#include "tf_bot_squad.h"
#include "tf_bot_manager.h"
#include "tf_weapon_medigun.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "behavior/tf_bot_behavior.h"
#include "func_respawnroom.h"
#include "team_train_watcher.h"
#include "NextBotUtil.h"

void DifficultyChanged( IConVar *var, const char *pOldValue, float flOldValue );
void PrefixNameChanged( IConVar *var, const char *pOldValue, float flOldValue );

ConVar tf_bot_difficulty( "tf_bot_difficulty", "2", FCVAR_GAMEDLL, "Defines the skill of bots joining the game.  Values are: 0=easy, 1=normal, 2=hard, 3=expert.", &DifficultyChanged );
ConVar tf_bot_force_class( "tf_bot_force_class", "", FCVAR_GAMEDLL, "If set to a class name, all TFBots will respawn as that class" );
ConVar tf_bot_keep_class_after_death( "tf_bot_keep_class_after_death", "0", FCVAR_GAMEDLL );
ConVar tf_bot_prefix_name_with_difficulty( "tf_bot_prefix_name_with_difficulty", "0", FCVAR_GAMEDLL, "Append the skill level of the bot to the bot's name", &PrefixNameChanged );
ConVar tf_bot_path_lookahead_range( "tf_bot_path_lookahead_range", "300", FCVAR_GAMEDLL, "", true, 0.0f, false, 0.0f );
ConVar tf_bot_near_point_travel_distance( "tf_bot_near_point_travel_distance", "750", FCVAR_CHEAT );
ConVar tf_bot_pyro_shove_away_range( "tf_bot_pyro_shove_away_range", "250", FCVAR_CHEAT, "If a Pyro bot's target is closer than this, compression blast them away" );
ConVar tf_bot_pyro_always_reflect( "tf_bot_pyro_always_reflect", "0", FCVAR_CHEAT, "Pyro bots will always reflect projectiles fired at them. For tesing/debugging purposes.", true, 0.0f, true, 1.0f );
ConVar tf_bot_pyro_deflect_tolerance( "tf_bot_pyro_deflect_tolerance", "0.5", FCVAR_CHEAT );
ConVar tf_bot_sniper_spot_min_range( "tf_bot_sniper_spot_min_range", "1000", FCVAR_CHEAT );
ConVar tf_bot_sniper_spot_max_count( "tf_bot_sniper_spot_max_count", "10", FCVAR_CHEAT, "Stop searching for sniper spots when each side has found this many" );
ConVar tf_bot_sniper_spot_search_count( "tf_bot_sniper_spot_search_count", "10", FCVAR_CHEAT, "Search this many times per behavior update frame" );
ConVar tf_bot_sniper_spot_point_tolerance( "tf_bot_sniper_spot_point_tolerance", "750", FCVAR_CHEAT );
ConVar tf_bot_sniper_spot_epsilon( "tf_bot_sniper_spot_epsilon", "100", FCVAR_CHEAT );
ConVar tf_bot_sniper_goal_entity_move_tolerance( "tf_bot_sniper_goal_entity_move_tolerance", "500", FCVAR_CHEAT );
ConVar tf_bot_suspect_spy_touch_interval( "tf_bot_suspect_spy_touch_interval", "5", FCVAR_CHEAT, "How many seconds back to look for touches against suspicious spies", true, 0.0f, false, 0.0f );
ConVar tf_bot_suspect_spy_forget_cooldown( "tf_bot_suspect_spy_forced_cooldown", "5", FCVAR_CHEAT, "How long to consider a suspicious spy as suspicious", true, 0.0f, false, 0.0f );

extern ConVar of_mutator;

LINK_ENTITY_TO_CLASS( tf_bot, CTFBot )

CBasePlayer *CTFBot::AllocatePlayerEntity( edict_t *edict, const char *playerName )
{
	CTFPlayer::s_PlayerEdict = edict;
	return (CTFBot *)CreateEntityByName( "tf_bot" );
}


class SelectClosestPotentiallyVisible
{
public:
	SelectClosestPotentiallyVisible( const Vector &origin )
		: m_vecOrigin( origin )
	{
		m_pSelected = NULL;
		m_flMinDist = FLT_MAX;
	}

	bool operator()( CNavArea *area )
	{
		Vector vecClosest;
		area->GetClosestPointOnArea( m_vecOrigin, &vecClosest );
		float flDistance = ( vecClosest - m_vecOrigin ).LengthSqr();

		if ( flDistance < m_flMinDist )
		{
			m_flMinDist = flDistance;
			m_pSelected = area;
		}

		return true;
	}

	Vector m_vecOrigin;
	CNavArea *m_pSelected;
	float m_flMinDist;
};


class CollectReachableObjects : public ISearchSurroundingAreasFunctor
{
public:
	CollectReachableObjects( CTFBot *actor, CUtlVector<EHANDLE> *selectedHealths, CUtlVector<EHANDLE> *outVector, float flMaxLength )
	{
		m_pBot = actor;
		m_flMaxRange = flMaxLength;
		m_pHealths = selectedHealths;
		m_pVector = outVector;
	}

	virtual bool operator() ( CNavArea *area, CNavArea *priorArea, float travelDistanceSoFar )
	{
		for ( int i=0; i<m_pHealths->Count(); ++i )
		{
			CBaseEntity *pEntity = ( *m_pHealths )[i];
			if ( !pEntity || !area->Contains( pEntity->WorldSpaceCenter() ) )
				continue;

			for ( int j=0; j<m_pVector->Count(); ++j )
			{
				CBaseEntity *pSelected = ( *m_pVector )[j];
				if ( ENTINDEX( pEntity ) == ENTINDEX( pSelected ) )
					return true;
			}

			EHANDLE hndl( pEntity );
			m_pVector->AddToTail( hndl );
		}

		return true;
	}

	virtual bool ShouldSearch( CNavArea *adjArea, CNavArea *currentArea, float travelDistanceSoFar )
	{
		if ( adjArea->IsBlocked( m_pBot->GetTeamNumber() ) || travelDistanceSoFar > m_flMaxRange )
			return false;

		return currentArea->IsContiguous( adjArea );
	}

private:
	CTFBot *m_pBot;
	CUtlVector<EHANDLE> *m_pHealths;
	CUtlVector<EHANDLE> *m_pVector;
	float m_flMaxRange;
};


class CountClassMembers
{
public:
	CountClassMembers( CTFBot *bot, int teamNum )
		: m_pBot( bot ), m_iTeam( teamNum )
	{
		Q_memset( &m_aClassCounts, 0, sizeof( m_aClassCounts ) );
	}

	bool operator()( CBasePlayer *player )
	{
		if ( player->GetTeamNumber() == m_iTeam )
		{
			++m_iTotal;
			CTFPlayer *pTFPlayer = static_cast<CTFPlayer *>( player );
			if ( !m_pBot->IsSelf( player ) )
				++m_aClassCounts[ pTFPlayer->GetPlayerClass()->GetClassIndex() ];
		}

		return true;
	}

	CTFBot *m_pBot;
	int m_iTeam;
	int m_aClassCounts[TF_CLASS_COUNT_ALL];
	int m_iTotal;
};


IMPLEMENT_INTENTION_INTERFACE( CTFBot, CTFBotMainAction )


CTFBot::CTFBot( CTFPlayer *player )
{
	m_controlling = player;

	m_body = new CTFBotBody( this );
	m_vision = new CTFBotVision( this );
	m_locomotor = new CTFBotLocomotion( this );
	m_intention = new CTFBotIntention( this );

	ListenForGameEvent( "teamplay_point_startcapture" );
	ListenForGameEvent( "teamplay_point_captured" );
	ListenForGameEvent( "teamplay_round_win" );
	ListenForGameEvent( "teamplay_flag_event" );
}

CTFBot::~CTFBot()
{
	if ( m_body )
		delete m_body;
	if ( m_vision )
		delete m_vision;
	if ( m_locomotor )
		delete m_locomotor;
	if ( m_intention )
		delete m_intention;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::Spawn( void )
{
	BaseClass::Spawn();

	m_iSkill = (DifficultyType)tf_bot_difficulty.GetInt();
	m_nBotAttrs = AttributeType::NONE;

	m_useWeaponAbilityTimer.Start( 5.0f );
	m_bLookingAroundForEnemies = true;

	m_bPickedUpWeapon = false;
	m_bPickedUpPowerup = false;
	m_bTouchedJumpPad = false;
	m_bTouchedTeleport = false;

	m_suspectedSpies.PurgeAndDeleteElements();
	m_cpChangedTimer.Invalidate();
	m_requiredEquipStack.RemoveAll();
	m_hMyControlPoint = NULL;
	m_hMyCaptureZone = NULL;

	GetVisionInterface()->ForgetAllKnownEntities();

	ClearSniperSpots();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::Event_Killed( const CTakeDamageInfo &info )
{
	OnKilled( info );
	CTFPlayer::Event_Killed( info );

	LeaveSquad();

	if ( !tf_bot_keep_class_after_death.GetBool() && TFGameRules()->CanBotChangeClass( this ) )
	{
		const char *pszClassname = GetNextSpawnClassname();

		HandleCommand_JoinClass( pszClassname, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::UpdateOnRemove( void )
{
	LeaveSquad();

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: Notify my components
//-----------------------------------------------------------------------------
void CTFBot::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "teamplay_point_startcapture" ) )
	{
		int iCPIndex = event->GetInt( "cp" );
		OnTerritoryContested( iCPIndex );
	}
	else if ( FStrEq( event->GetName(), "teamplay_point_captured" ) )
	{
		ClearMyControlPoint();

		int iCPIndex = event->GetInt( "cp" );
		int iTeam = event->GetInt( "team" );
		if ( iTeam == GetTeamNumber() )
		{
			OnTerritoryCaptured( iCPIndex );
		}
		else
		{
			OnTerritoryLost( iCPIndex );
			m_cpChangedTimer.Start( RandomFloat( 10.0f, 20.0f ) );
		}
	}
	else if ( FStrEq( event->GetName(), "teamplay_flag_event" ) )
	{
		if ( event->GetInt( "eventtype" ) == TF_FLAGEVENT_PICKUP )
		{
			int iPlayer = event->GetInt( "player" );
			if ( iPlayer == GetUserID() )
				OnPickUp( nullptr, nullptr );
		}
	}
	else if ( FStrEq( event->GetName(), "teamplay_round_win" ) )
	{
		int iWinningTeam = event->GetInt( "team" );
		if ( event->GetBool( "full_round" ) )
		{
			if ( iWinningTeam == GetTeamNumber() )
				OnWin();
			else
				OnLose();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFBot::DrawDebugTextOverlays( void )
{
	int text_offset = CTFPlayer::DrawDebugTextOverlays();

	if ( m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		EntityText( text_offset, CFmtStr( "FOV: %.2f (%i)", GetVisionInterface()->GetFieldOfView(), GetFOV() ), 0 );
		text_offset++;
	}

	return text_offset;
}

//-----------------------------------------------------------------------------
// Purpose: Perform some updates on physics step
//-----------------------------------------------------------------------------
void CTFBot::PhysicsSimulate( void )
{
	BaseClass::PhysicsSimulate();

	if ( m_HomeArea == nullptr )
		m_HomeArea = GetLastKnownArea();

	TeamFortress_SetSpeed();

	if ( m_pSquad && ( m_pSquad->GetMemberCount() <= 1 || !m_pSquad->GetLeader() ) )
		LeaveSquad();
}

//-----------------------------------------------------------------------------
// Purpose: Alert us and others we bumped a spy
//-----------------------------------------------------------------------------
void CTFBot::Touch( CBaseEntity *other )
{
	BaseClass::Touch( other );

	CTFPlayer *pOther = ToTFPlayer( other );
	if ( !pOther )
		return;

	if ( IsEnemy( pOther ) )
	{
		if ( pOther->m_Shared.InCondInvis() || pOther->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			RealizeSpy( pOther );
		}

		// hack nearby bots into reacting to bumping someone
		TheNextBots().OnWeaponFired( pOther, pOther->GetActiveTFWeapon() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsAllowedToPickUpFlag( void )
{
	//if ( BaseClass::IsAllowedToPickUpFlag() ) 
	//{
		if ( !m_pSquad || this == m_pSquad->GetLeader() )
		{
			//return DWORD( this + 2468 ) == 0;
		}

		return true;
	//}
	//
	//return false;
}

//-----------------------------------------------------------------------------
// Purpose: Disguise as a dead enemy for maximum espionage
//-----------------------------------------------------------------------------
void CTFBot::DisguiseAsEnemy( void )
{
	CUtlVector<CTFPlayer *> enemies;
	CollectPlayers( &enemies, GetEnemyTeam( this ), false );

	int iClass = TF_CLASS_UNDEFINED;
	for ( int i=0; i < enemies.Count(); ++i )
	{
		if ( !enemies[i]->IsAlive() )
			iClass = enemies[i]->GetPlayerClass()->GetClassIndex();
	}

	if ( iClass == TF_CLASS_UNDEFINED )
		// BOTTODO: Mercenary and civilian
		iClass = RandomInt( TF_FIRST_NORMAL_CLASS, TF_CLASS_ENGINEER );

	m_Shared.Disguise( GetEnemyTeam( this ), iClass );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsCombatWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == nullptr )
	{
		weapon = GetActiveTFWeapon();
		if ( weapon == nullptr )
		{
			return true;
		}
	}

	switch ( weapon->GetWeaponID() )
	{
		case TF_WEAPON_PDA:
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_DISPENSER:
		case TF_WEAPON_MEDIGUN:
		case TF_WEAPON_INVIS:
		//case TF_WEAPON_LUNCHBOX:
		//case TF_WEAPON_BUFF_ITEM:
			return false;

		default:
			return true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsQuietWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == nullptr )
	{
		weapon = GetActiveTFWeapon();
		if ( weapon == nullptr )
		{
			return false;
		}
	}

	switch ( weapon->GetWeaponID() )
	{
		case TF_WEAPON_KNIFE:
		case TF_WEAPON_FISTS:
		case TF_WEAPON_PDA:
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
		case TF_WEAPON_PDA_SPY:
		case TF_WEAPON_BUILDER:
		case TF_WEAPON_MEDIGUN:
		case TF_WEAPON_DISPENSER:
		case TF_WEAPON_INVIS:
		//case TF_WEAPON_LUNCHBOX:
		//case TF_WEAPON_JAR:
		//case TF_WEAPON_COMPOUND_BOW:
		// open fortress weaponry
		case TFC_WEAPON_KNIFE:
			return true;

		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsHitScanWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == nullptr )
	{
		weapon = GetActiveTFWeapon();
		if ( weapon == nullptr )
		{
			return false;
		}
	}

	if ( !IsCombatWeapon( weapon ) )
	{
		return false;
	}

	switch ( weapon->GetWeaponID() )
	{
		//case TF_WEAPON_SHOTGUN_PRIMARY:
		//case TF_WEAPON_SHOTGUN_SOLDIER:
		//case TF_WEAPON_SHOTGUN_HWG:
		//case TF_WEAPON_SHOTGUN_PYRO:
		case TF_WEAPON_SHOTGUN:
		case TF_WEAPON_SCATTERGUN:
		case TF_WEAPON_SNIPERRIFLE:
		case TF_WEAPON_MINIGUN:
		case TF_WEAPON_SMG:
		case TF_WEAPON_PISTOL:
		case TF_WEAPON_PISTOL_SCOUT:
		case TF_WEAPON_REVOLVER:
		case TF_WEAPON_SENTRY_BULLET:
		//case TF_WEAPON_HANDGUN_SCOUT_PRIMARY:
		// open fortress weaponry
		case TF_WEAPON_RAILGUN:
		case TF_WEAPON_SUPERSHOTGUN:
		case TF_WEAPON_ETERNALSHOTGUN:
		case TF_WEAPON_PISTOL_MERCENARY:
		case TF_WEAPON_REVOLVER_MERCENARY:
		case TF_WEAPON_GATLINGGUN:
		case TF_WEAPON_PISTOL_AKIMBO:
		case TF_WEAPON_SMG_MERCENARY:
		case TF_WEAPON_TOMMYGUN:
		case TF_WEAPON_ASSAULTRIFLE:
		case TF_WEAPON_PHYSCANNON:
		case TF_WEAPON_LIGHTNING_GUN:
		case TFC_WEAPON_SHOTGUN_SB:
		case TFC_WEAPON_SHOTGUN_DB:
		case TFC_WEAPON_RAILPISTOL:
		case TFC_WEAPON_ASSAULTCANNON:
		case TFC_WEAPON_SNIPER_RIFLE:
		case TFC_WEAPON_ASSAULT_RIFLE:
			return true;

		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsExplosiveProjectileWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == nullptr )
	{
		weapon = GetActiveTFWeapon();
		if ( weapon == nullptr )
		{
			return false;
		}
	}

	switch ( weapon->GetWeaponID() )
	{
		case TF_WEAPON_ROCKETLAUNCHER:
		case TF_WEAPON_GRENADELAUNCHER:
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_SENTRY_ROCKET:
		//case TF_WEAPON_JAR:
		// open fortress weaponry
		case TF_WEAPON_GRENADELAUNCHER_MERCENARY:
		case TF_WEAPON_ROCKETLAUNCHER_DM:
		case TF_WEAPON_SUPER_ROCKETLAUNCHER:
		case TF_WEAPON_DYNAMITE_BUNDLE:
		case TF_WEAPON_GIB:
		case TFC_WEAPON_INCENDIARYCANNON:
		case TFC_WEAPON_PIPEBOMBLAUNCHER:
		case TFC_WEAPON_GRENADELAUNCHER:
		case TFC_WEAPON_RPG:
			return true;

		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsContinuousFireWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == nullptr )
	{
		weapon = GetActiveTFWeapon();
		if ( weapon == nullptr )
		{
			return false;
		}
	}

	if ( !IsCombatWeapon( weapon ) )
	{
		return false;
	}

	switch ( weapon->GetWeaponID() )
	{
		case TF_WEAPON_MINIGUN:
		case TF_WEAPON_SMG:
		case TF_WEAPON_PISTOL:
		case TF_WEAPON_PISTOL_SCOUT:
		case TF_WEAPON_FLAMETHROWER:
		//case TF_WEAPON_LASER_POINTER:
		// open fortress weaponry
		case TF_WEAPON_PISTOL_MERCENARY:
		case TF_WEAPON_GATLINGGUN:
		case TF_WEAPON_PISTOL_AKIMBO:
		case TF_WEAPON_SMG_MERCENARY:
		case TF_WEAPON_TOMMYGUN:
		case TF_WEAPON_ASSAULTRIFLE:
		case TF_WEAPON_LIGHTNING_GUN:
		case TF_WEAPON_CHAINSAW:
		case TFC_WEAPON_RAILPISTOL:
		case TFC_WEAPON_ASSAULTCANNON:
		case TFC_WEAPON_ASSAULT_RIFLE:
			return true;

		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsBarrageAndReloadWeapon( CTFWeaponBase *weapon ) const
{
	if ( weapon == nullptr )
	{
		weapon = GetActiveTFWeapon();
		if ( weapon == nullptr )
		{
			return false;
		}
	}

	switch ( weapon->GetWeaponID() )
	{
		case TF_WEAPON_SCATTERGUN:
		case TF_WEAPON_ROCKETLAUNCHER:
		case TF_WEAPON_GRENADELAUNCHER:
		case TF_WEAPON_PIPEBOMBLAUNCHER:
		case TF_WEAPON_GRENADELAUNCHER_MERCENARY:
		// open fortress weaponry
		case TF_WEAPON_ROCKETLAUNCHER_DM:
		case TF_WEAPON_SUPER_ROCKETLAUNCHER:
		case TF_WEAPON_DYNAMITE_BUNDLE:
		case TFC_WEAPON_SHOTGUN_SB:
		case TFC_WEAPON_SHOTGUN_DB:
		case TFC_WEAPON_RPG:
		case TFC_WEAPON_INCENDIARYCANNON:
		case TFC_WEAPON_GRENADELAUNCHER:
		case TFC_WEAPON_PIPEBOMBLAUNCHER:
			return true;

		default:
			return false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the weapon in the specified slot
//-----------------------------------------------------------------------------
CTFWeaponBase *CTFBot::GetWeaponInSlot( int iSlot, int iSlotPos )
{
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBase *pWeapon =(CTFWeaponBase*)GetWeapon(i);

		if ( pWeapon == NULL )
			continue;

		if ( pWeapon->GetSlot() == iSlot && pWeapon->GetPosition() == iSlotPos )
			return pWeapon;
	}
	
	return NULL;
}

//TODO: why does this only care about the current weapon?
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsAmmoLow( void ) const
{
	CTFWeaponBase *weapon = GetActiveTFWeapon();
	if ( weapon == nullptr )
		return false;

	if ( weapon->GetWeaponID() != ( TF_WEAPON_WRENCH ) && weapon->GetWeaponID() != ( TFC_WEAPON_WRENCH ) )
	{
		if ( !weapon->IsMeleeWeapon() )
		{
			// int ammoType = weapon->GetPrimaryAmmoType();
			int current = ( weapon->ReserveAmmo() );

			return ( current / weapon->GetMaxReserveAmmo() < 0.2f );
			//return current / const_cast<CTFBot *>( this )->GetMaxAmmo( 1 ) < 0.2f;
		}

		return false;
	}

	return GetAmmoCount( TF_AMMO_METAL ) < 50;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsAmmoFull( void ) const
{
	CTFWeaponBase *weapon = GetActiveTFWeapon();
	if ( weapon == nullptr )
		return false;

	//int primaryCount = GetAmmoCount( TF_AMMO_PRIMARY );
	//bool primaryFull = primaryCount >= const_cast<CTFBot *>( this )->GetMaxAmmo( TF_AMMO_PRIMARY );

	//int secondaryCount = GetAmmoCount( TF_AMMO_SECONDARY );
	//bool secondaryFull = secondaryCount >= const_cast<CTFBot *>( this )->GetMaxAmmo( TF_AMMO_SECONDARY );



	CTFWeaponBase *primary = const_cast<CTFBot *>( this )->GetWeaponInSlot( 0, 0 );
	CTFWeaponBase *secondary = const_cast<CTFBot *>( this )->GetWeaponInSlot( 1, 0 );

	bool primaryFull = false;
	bool secondaryFull = false;

	if ( primary )
		primaryFull = primary->GetMaxReserveAmmo() >= ( primary->ReserveAmmo() );
	if ( secondary )
		secondaryFull = secondary->GetMaxReserveAmmo() >= ( secondary->ReserveAmmo() );

	// OFBOT: Allclass support
	if ( TFGameRules()->IsFreeRoam() || !IsPlayerClass( TF_CLASS_ENGINEER ) )
		return primaryFull && secondaryFull;

	return GetAmmoCount( TF_AMMO_METAL ) >= 200;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::AreAllPointsUncontestedSoFar( void ) const
{
	if ( g_hControlPointMasters.IsEmpty() )
		return true;

	if ( !g_hControlPointMasters[0].IsValid() )
		return true;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
	for ( int i=0; i<pMaster->GetNumPoints(); ++i )
	{
		CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
		if ( pPoint && pPoint->HasBeenContested() )
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsNearPoint( CTeamControlPoint *point ) const
{
	CTFNavArea *myArea = GetLastKnownArea();
	if ( myArea )
	{
		int iPointIdx = point->GetPointIndex();
		if ( iPointIdx < MAX_CONTROL_POINTS )
		{
			CTFNavArea *cpArea = TFNavMesh()->GetMainControlPointArea( iPointIdx );
			if ( cpArea )
				return ( abs( myArea->GetIncursionDistance( GetTeamNumber() ) - cpArea->GetIncursionDistance( GetTeamNumber() ) ) < tf_bot_near_point_travel_distance.GetFloat() );
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Return a CP that we desire to defend or capture
//-----------------------------------------------------------------------------
CTeamControlPoint *CTFBot::GetMyControlPoint( void )
{
	if ( !m_hMyControlPoint || m_myCPValidDuration.IsElapsed() )
	{
		m_myCPValidDuration.Start( RandomFloat( 1.0f, 2.0f ) );

		CUtlVector<CTeamControlPoint *> defensePoints;
		CUtlVector<CTeamControlPoint *> attackPoints;
		TFGameRules()->CollectDefendPoints( this, &defensePoints );
		TFGameRules()->CollectCapturePoints( this, &attackPoints );

		CTFWeaponBase *pWeapon = GetActiveTFWeapon();
		
		// OFBOT: Allclass Support
		if ( (
				( pWeapon && WeaponID_IsSniperRifle( pWeapon->GetWeaponID() ) )
				|| IsPlayerClass( TF_CLASS_ENGINEER )
				/* || BYTE( this + 10061 ) & ( 1 << 4 ) */
			) && !defensePoints.IsEmpty()
		) {
			CTeamControlPoint *pPoint = SelectPointToDefend( defensePoints );
			if ( pPoint )
			{
				m_hMyControlPoint = pPoint;
				return pPoint;
			}
		}
		else
		{
			CTeamControlPoint *pPoint = SelectPointToCapture( attackPoints );
			if ( pPoint )
			{
				m_hMyControlPoint = pPoint;
				return pPoint;
			}
			else
			{
				m_myCPValidDuration.Invalidate();

				pPoint = SelectPointToDefend( defensePoints );
				if ( pPoint )
				{
					m_hMyControlPoint = pPoint;
					return pPoint;
				}
			}
		}

		m_myCPValidDuration.Invalidate();

		return nullptr;
	}

	return m_hMyControlPoint;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsAnyPointBeingCaptured( void ) const
{
	if ( g_hControlPointMasters.IsEmpty() )
		return false;

	CTeamControlPointMaster *pMaster = g_hControlPointMasters[0];
	if ( pMaster )
	{
		for ( int i=0; i<pMaster->GetNumPoints(); ++i )
		{
			CTeamControlPoint *pPoint = pMaster->GetControlPoint( i );
			if ( IsPointBeingContested( pPoint ) )
				return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsPointBeingContested( CTeamControlPoint *point ) const
{
	if ( point )
	{
		if ( ( point->LastContestedAt() + 5.0f ) > gpGlobals->curtime )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::GetTimeLeftToCapture( void )
{
	int iTeam = GetTeamNumber();

	if ( TFGameRules()->IsInKothMode() )
	{
		if ( iTeam != TF_TEAM_RED )
		{
			if ( iTeam != TF_TEAM_BLUE )
				return 0.0f;

			CTeamRoundTimer *pBlueTimer = TFGameRules()->GetBlueKothRoundTimer();
			if ( pBlueTimer )
				return pBlueTimer->GetTimeRemaining();
		}
		else
		{
			CTeamRoundTimer *pRedTimer = TFGameRules()->GetRedKothRoundTimer();
			if ( pRedTimer )
				return pRedTimer->GetTimeRemaining();
		}
	}
	else
	{
		CTeamRoundTimer *pTimer = TFGameRules()->GetActiveRoundTimer();
		if ( pTimer )
			return pTimer->GetTimeRemaining();
	}

	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamControlPoint *CTFBot::SelectPointToCapture( CUtlVector<CTeamControlPoint *> const &candidates )
{
	if ( candidates.IsEmpty() )
		return nullptr;

	if ( candidates.Count() == 1 )
		return candidates[0];

	if ( IsCapturingPoint() )
	{
		CTriggerAreaCapture *pCapArea = GetControlPointStandingOn();
		if ( pCapArea )
			return pCapArea->GetControlPoint();
	}

	CTeamControlPoint *pClose = SelectClosestPointByTravelDistance( candidates );
	if ( pClose && IsPointBeingContested( pClose ) )
		return pClose;

	float flMaxDanger = FLT_MIN;
	bool bInCombat = false;
	CTeamControlPoint *pDangerous = nullptr;

	for ( int i=0; i<candidates.Count(); ++i )
	{
		CTeamControlPoint *pPoint = candidates[i];
		if ( IsPointBeingContested( pPoint ) )
			return pPoint;

		CTFNavArea *pCPArea = TFNavMesh()->GetMainControlPointArea( pPoint->GetPointIndex() );
		if ( pCPArea == nullptr )
			continue;

		float flDanger = pCPArea->GetCombatIntensity();
		bInCombat = flDanger > 0.1f ? true : false;

		if ( flMaxDanger < flDanger )
		{
			flMaxDanger = flDanger;
			pDangerous = pPoint;
		}
	}

	if ( bInCombat )
		return pDangerous;

	// Probaly some Min/Max going on here
	int iSelection = candidates.Count() - 1;
	if ( iSelection >= 0 )
	{
		int iRandSel = candidates.Count() * TransientlyConsistentRandomValue( 60.0f, 0 );
		if ( iRandSel < 0 )
			return candidates[0];

		if ( iRandSel <= iSelection )
			iSelection = iRandSel;
	}

	return candidates[iSelection];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTeamControlPoint *CTFBot::SelectPointToDefend( CUtlVector<CTeamControlPoint *> const &candidates )
{
	if ( candidates.IsEmpty() )
		return nullptr;

	if ( ( m_nBotAttrs & CTFBot::AttributeType::DISABLEDODGE ) != 0 )
		return SelectClosestPointByTravelDistance( candidates );

	return candidates.Random();
}

//-----------------------------------------------------------------------------
// Purpose: Return the closest control point to us
//-----------------------------------------------------------------------------
CTeamControlPoint *CTFBot::SelectClosestPointByTravelDistance( CUtlVector<CTeamControlPoint *> const &candidates ) const
{
	CTeamControlPoint *pClosest = nullptr;
	float flMinDist = FLT_MAX;
	CTFPlayerPathCost cost( (CTFPlayer *)this );

	if ( GetLastKnownArea() )
	{
		for ( int i=0; i<candidates.Count(); ++i )
		{
			CTFNavArea *pCPArea = TFNavMesh()->GetMainControlPointArea( candidates[i]->GetPointIndex() );
			float flDist = NavAreaTravelDistance( GetLastKnownArea(), pCPArea, cost );

			if ( flDist >= 0.0f && flMinDist > flDist )
			{
				flMinDist = flDist;
				pClosest = candidates[i];
			}
		}
	}

	return pClosest;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCaptureZone *CTFBot::GetFlagCaptureZone( void )
{
	if ( !m_hMyCaptureZone && TFGameRules()->InGametype( TF_GAMETYPE_CTF ) )
	{
		for ( int i=0; i<ICaptureZoneAutoList::AutoList().Count(); ++i )
		{
			CCaptureZone *pZone = static_cast<CCaptureZone *>( ICaptureZoneAutoList::AutoList()[i] );
			if ( pZone && pZone->GetTeamNumber() == GetTeamNumber() )
				m_hMyCaptureZone = pZone;
		}
	}

	return m_hMyCaptureZone;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCaptureFlag *CTFBot::GetFlagToFetch( void )
{
	CUtlVector<CCaptureFlag *> flags;

	// HACK: ficool2 - this variable causes bots to not move when their own flag gets taken
	//int nNumStolen = 0;

	for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
	{
		CCaptureFlag *pFlag = static_cast<CCaptureFlag *>( ICaptureFlagAutoList::AutoList()[i] );
		if ( !pFlag || pFlag->IsDisabled() )
			continue;

		if ( HasTheFlag(/* 0, 0 */) && pFlag->GetOwnerEntity() == this )
			return pFlag;

		if ( pFlag->GetGameType() >= TF_FLAGTYPE_CTF && pFlag->GetGameType() <= TF_FLAGTYPE_INVADE )
		{
			if ( pFlag->GetTeamNumber() == GetEnemyTeam( this ) )
				flags.AddToTail( pFlag );

			//nNumStolen += pFlag->IsStolen();
		}
	}

	float flMinDist = FLT_MAX;
	float flMinStolenDist = FLT_MAX;
	CCaptureFlag *pClosest = NULL;
	CCaptureFlag *pClosestStolen = NULL;

	for ( int i=0; i<flags.Count(); ++i )
	{
		CCaptureFlag *pFlag = flags[i];
		if ( !pFlag )
			continue;

		float flDistance = ( pFlag->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
		if ( flDistance > flMinStolenDist )
		{
			flMinStolenDist = flDistance;
			pClosestStolen = pFlag;
		}

		if ( flags.Count() > /* nNumStolen*/ 0 )
		{
			if ( pFlag->IsStolen() || flMinDist <= flDistance )
				continue;

			flMinDist = flDistance;
			pClosest = pFlag;
		}
	}

	if ( pClosest )
		return pClosest;

	return pClosestStolen;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsLineOfFireClear( CBaseEntity *to )
{
	return IsLineOfFireClear( EyePosition(), to );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsLineOfFireClear( const Vector &to )
{
	return IsLineOfFireClear( EyePosition(), to );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsLineOfFireClear( const Vector &from, CBaseEntity *to )
{
	NextBotTraceFilterIgnoreActors filter( nullptr, COLLISION_GROUP_NONE );

	trace_t trace;
	UTIL_TraceLine( from, to->WorldSpaceCenter(), MASK_SOLID_BRUSHONLY, &filter, &trace );

	return !trace.DidHit() || trace.m_pEnt == to;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsLineOfFireClear( const Vector &from, const Vector &to )
{
	NextBotTraceFilterIgnoreActors filter( nullptr, COLLISION_GROUP_NONE );

	trace_t trace;
	UTIL_TraceLine( from, to, MASK_SOLID_BRUSHONLY, &filter, &trace );

	return !trace.DidHit();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsAnyEnemySentryAbleToAttackMe( void ) const
{
	for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *obj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );
		if ( obj == nullptr )
			continue;

		if ( obj->ObjectType() == OBJ_SENTRYGUN && !obj->IsPlacing() &&
			!obj->IsBuilding() && !obj->IsHauling() && !obj->HasSapper() )
		{
			if ( ( GetAbsOrigin() - obj->GetAbsOrigin() ).LengthSqr() < Square( SENTRYGUN_BASE_RANGE ) &&
				IsThreatAimingTowardsMe( obj, 0.95f ) && IsLineOfSightClear( obj, CBaseCombatCharacter::IGNORE_ACTORS ) )
			{
				return true;
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsThreatAimingTowardsMe( CBaseEntity *threat, float dotTolerance ) const
{
	if ( threat == nullptr )
		return false;

	Vector vecToActor = GetAbsOrigin() - threat->GetAbsOrigin();
	vecToActor.NormalizeInPlace();

	CTFPlayer *player = ToTFPlayer( threat );
	if ( player )
	{
		Vector fwd;
		player->EyeVectors( &fwd );

		return vecToActor.Dot( fwd ) > dotTolerance;
	}

	CObjectSentrygun *sentry = dynamic_cast<CObjectSentrygun *>( threat );
	if ( sentry )
	{
		Vector fwd;
		AngleVectors( sentry->GetTurretAngles(), &fwd );

		return vecToActor.Dot( fwd ) > dotTolerance;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsThreatFiringAtMe( CBaseEntity *threat ) const
{
	if ( !IsThreatAimingTowardsMe( threat ) )
		return false;

	// looking at me, but has it shot at me yet
	if ( threat->IsPlayer() )
		return ( (CBasePlayer *)threat )->IsFiringWeapon();

	CObjectSentrygun *sentry = dynamic_cast<CObjectSentrygun *>( threat );
	if ( sentry )
	{
		// if it hasn't fired recently then it's clearly not shooting at me
		return sentry->GetTimeSinceLastFired() < 1.0f;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsEntityBetweenTargetAndSelf( CBaseEntity *blocker, CBaseEntity *target ) const
{
	Vector vecToTarget = ( target->GetAbsOrigin() - GetAbsOrigin() );
	Vector vecToEntity = ( blocker->GetAbsOrigin() - GetAbsOrigin() );
	if ( vecToEntity.NormalizeInPlace() < vecToTarget.NormalizeInPlace() )
	{
		if ( vecToTarget.Dot( vecToEntity ) > 0.7071f )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::TransientlyConsistentRandomValue( float duration, int seed ) const
{
	CTFNavArea *area = GetLastKnownArea();
	if ( area == nullptr )
	{
		return 0.0f;
	}

	int time_seed = (int)( gpGlobals->curtime / duration ) + 1;
	seed += ( area->GetID() * time_seed * entindex() );

	return fabs( FastCos( (float)seed ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::GetMaxAttackRange() const
{
	CTFWeaponBase *weapon = GetActiveTFWeapon();
	if ( weapon == nullptr )
	{
		return 0.0f;
	}

	if ( weapon->IsMeleeWeapon() )
	{
		return 100.0f;
	}

	// OFBot: hack so bots stop trying to act like a sniper with their pistol
	if ( weapon->IsWeapon( TF_WEAPON_PISTOL_MERCENARY ) || weapon->IsWeapon( TF_WEAPON_PISTOL_AKIMBO ) )
	{
		return 1024.0f;
	}

	if ( weapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) || weapon->IsWeapon( TF_WEAPON_LIGHTNING_GUN ) )
	{
		return 256.0f;
	}

	if ( IsExplosiveProjectileWeapon( weapon ) )
	{
		return 3000.0f;
	}

	if ( WeaponID_IsSniperRifle( weapon->GetWeaponID() ) )
	{
		return FLT_MAX;
	}

	return 2048.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::GetDesiredAttackRange( void ) const
{
	CTFWeaponBase *weapon = GetActiveTFWeapon();
	if ( weapon == nullptr )
		return 0.0f;

	if ( weapon->IsWeapon( TF_WEAPON_KNIFE ) || weapon->IsWeapon( TFC_WEAPON_KNIFE ) )
		return 70.0f;

	if ( !weapon->IsMeleeWeapon() && ( !weapon->IsWeapon( TF_WEAPON_FLAMETHROWER ) || !weapon->IsWeapon( TF_WEAPON_LIGHTNING_GUN ) ) )
	{
		if ( !WeaponID_IsSniperRifle( weapon->GetWeaponID() ) )
		{
			if ( !weapon->IsWeapon( TF_WEAPON_ROCKETLAUNCHER ) || !weapon->IsWeapon( TFC_WEAPON_RPG ) || !weapon->IsWeapon( TF_WEAPON_ROCKETLAUNCHER_DM ) )
				return 500.0f; // this will make pretty much every weapon use this as the desired range, not sure if intended/correct

			return 1250.0f; // rocket launchers apperantly have a larger desired range than hitscan
		}

		return FLT_MAX;
	}

	return 100.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::GetDesiredPathLookAheadRange( void ) const
{
	return GetModelScale() * tf_bot_path_lookahead_range.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsDebugFilterMatch( const char *name ) const
{
	if ( !Q_stricmp( name, const_cast<CTFBot *>( this )->GetPlayerClass()->GetName() ) )
		return true;

	return INextBot::IsDebugFilterMatch( name );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::PressFireButton( float duration )
{
	if ( m_Shared.IsControlStunned() || m_Shared.IsLoser() )
		return;

	BaseClass::PressFireButton( duration );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::PressAltFireButton( float duration )
{
	if ( m_Shared.IsControlStunned() || m_Shared.IsLoser() )
		return;

	BaseClass::PressAltFireButton( duration );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::PressSpecialFireButton( float duration )
{
	if ( m_Shared.IsControlStunned() || m_Shared.IsLoser() )
		return;

	BaseClass::PressSpecialFireButton( duration );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::ShouldFireCompressionBlast( void )
{
	if ( !tf_bot_pyro_always_reflect.GetBool() )
	{
		if ( TFGameRules()->IsInTraining() || m_iSkill == CTFBot::EASY )
			return false;

		if ( m_iSkill == CTFBot::NORMAL && TransientlyConsistentRandomValue( 1.0, 0 ) < 0.5f )
			return false;

		if ( m_iSkill == CTFBot::HARD && TransientlyConsistentRandomValue( 1.0, 0 ) < 0.1f )
			return false;
	}

	const CKnownEntity *threat = GetVisionInterface()->GetPrimaryKnownThreat( true );
	if ( threat && threat->GetEntity() && threat->GetEntity()->IsPlayer() )
	{
		CTFPlayer *pTarget = ToTFPlayer( threat->GetEntity() );
		if ( IsRangeLessThan( pTarget, tf_bot_pyro_shove_away_range.GetFloat() ) )
		{
			if ( pTarget->m_Shared.InCondUber() )
				return true;

			if ( pTarget->GetGroundEntity() )
			{
				if ( pTarget->IsCapturingPoint() && TransientlyConsistentRandomValue( 3.0, 0 ) < 0.5f )
					return true;
			}

			// this doesn't work well, it just makes them look like they are firing randomly
			//return TransientlyConsistentRandomValue( 0.5, 0 ) < 0.5f;
		}
	}

	CUtlVector<CKnownEntity> knowns;
	GetVisionInterface()->CollectKnownEntities( &knowns );

	for ( int i=0; i<knowns.Count(); ++i )
	{
		CTFPlayer *pKnownPlayer = dynamic_cast<CTFPlayer *>( knowns[i].GetEntity() );
		if ( pKnownPlayer && IsFriend( pKnownPlayer ) )
		{
			if ( IsRangeLessThan( pKnownPlayer, tf_bot_pyro_shove_away_range.GetFloat() ) )
			{
				// extinguish teammates that are on fire!
				if ( pKnownPlayer->m_Shared.InCond( TF_COND_BURNING ) )
					return true;
			}
		}
	}

	CBaseEntity *pList[128];
	Vector vecOrig = EyePosition();
	QAngle angDir = EyeAngles();

	Vector vecFwd;
	AngleVectors( angDir, &vecFwd );
	vecOrig += vecFwd * 128.0f;

	Vector vecMins, vecMaxs;
	vecMins = vecOrig - Vector( 128.0f, 128.0f, 64.0f );
	vecMaxs = vecOrig + Vector( 128.0f, 128.0f, 64.0f );

	int count = UTIL_EntitiesInBox( pList, 128, vecMins, vecMaxs, FL_CLIENT|FL_GRENADE );
	for ( int i=0; i<count; ++i )
	{
		CBaseEntity *pEnt = pList[i];
		if ( pEnt != this && !pEnt->IsPlayer() && pEnt->IsDeflectable() )
		{
			// wow...
			char szClassname[48];		
			Q_strncpy( szClassname, pEnt->GetClassname(), sizeof( szClassname ) );
			
			if ( !Q_strncmp( szClassname, "tf_p", 4 ) /*FClassnameIs( pEnt, "tf_projectile_rocket" ) || FClassnameIs( pEnt, "tf_projectile_energy_ball" )*/ )
			{
				if ( GetVisionInterface()->IsLineOfSightClear( pEnt->WorldSpaceCenter() ) )
					return true;
			}

			Vector vecVel = pEnt->GetAbsVelocity();
			vecVel.NormalizeInPlace();

			if ( vecVel.Dot( vecFwd.Normalized() ) <= abs( tf_bot_pyro_deflect_tolerance.GetFloat() ) )
			{
				if ( GetVisionInterface()->IsLineOfSightClear( pEnt->WorldSpaceCenter() ) )
					return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::ShouldFireCompressionBlastAtFriend( void )
{
	// stripped down version of the normal function just for friends

	if ( m_iSkill == CTFBot::EASY )
		return false;

	CUtlVector<CKnownEntity> knowns;
	GetVisionInterface()->CollectKnownEntities( &knowns );

	for ( int i=0; i<knowns.Count(); ++i )
	{
		CTFPlayer *pKnownPlayer = dynamic_cast<CTFPlayer *>( knowns[i].GetEntity() );
		if ( pKnownPlayer && IsFriend( pKnownPlayer ) )
		{
			if ( IsRangeLessThan( pKnownPlayer, tf_bot_pyro_shove_away_range.GetFloat() ) )
			{
				// extinguish teammates that are on fire!
				if ( pKnownPlayer->m_Shared.InCond( TF_COND_BURNING ) )
					return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFNavArea *CTFBot::FindVantagePoint( float flMaxDist )
{
	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Action<CTFBot> *CTFBot::OpportunisticallyUseWeaponAbilities( void )
{
	// this function just does nothing, so don't call it for now
		return nullptr;
		
	if ( !m_useWeaponAbilityTimer.IsElapsed() )
		return nullptr;

	m_useWeaponAbilityTimer.Start( RandomFloat( 0.1f, 0.4f ) );

	for ( int i = 0; i < MAX_WEAPONS; ++i )
	{
		CTFWeaponBase *weapon = dynamic_cast<CTFWeaponBase *>( GetWeapon( i ) );
		if ( weapon == nullptr )
			continue;

		/*if ( weapon->GetWeaponID() == TF_WEAPON_BUFF_ITEM )
		{
			CTFBuffItem *buff = static_cast<CTFBuffItem *>( weapon );
			if ( buff->IsFull() )
				return new CTFBotUseItem( weapon );

			continue;
		}
		

		if ( weapon->GetWeaponID() == TF_WEAPON_LUNCHBOX || weapon->GetWeaponID() == TF_WEAPON_LUNCHBOX_DRINK )
		{
			if ( !weapon->HasAmmo() || ( IsPlayerClass( TF_CLASS_SCOUT ) && weapon->GetEffectBarProgress() < 1.0f ) )
				continue;

			return new CTFBotUseItem( weapon );
		}

		if ( weapon->GetWeaponID() == TF_WEAPON_BAT_WOOD && GetAmmoCount( weapon->GetSecondaryAmmoType() ) > 0 )
		{
			const CKnownEntity *threat = GetVisionInterface()->GetPrimaryKnownThreat( false );
			if ( threat == nullptr || !threat->IsVisibleRecently() )
				continue;

			this->PressAltFireButton();
		}
		*/
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBaseObject *CTFBot::GetNearestKnownSappableTarget( void ) const
{
	CUtlVector<CKnownEntity> knowns;
	GetVisionInterface()->CollectKnownEntities( &knowns );

	float flMinDist = Square( 500.0f );
	CBaseObject *ret = nullptr;
	for ( int i=0; i<knowns.Count(); ++i )
	{
		CBaseObject *obj = dynamic_cast<CBaseObject *>( knowns[i].GetEntity() );
		if ( obj && !obj->HasSapper() && this->IsEnemy( knowns[i].GetEntity() ) )
		{
			float flDist = this->GetRangeSquaredTo( obj );
			if ( flDist < flMinDist )
			{
				ret = obj;
				flMinDist = flDist;
			}
		}
	}

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::DelayedThreatNotice( CHandle<CBaseEntity> ent, float delay )
{
	const float when = gpGlobals->curtime + delay;

	FOR_EACH_VEC( m_delayedThreatNotices, i )
	{
		DelayedNoticeInfo *info = &m_delayedThreatNotices[i];

		if ( ent == info->m_hEnt )
		{
			if ( when < info->m_flWhen )
			{
				info->m_flWhen = when;
			}

			return;
		}
	}

	m_delayedThreatNotices.AddToTail( {ent, delay} );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::UpdateDelayedThreatNotices()
{
	FOR_EACH_VEC_BACK( m_delayedThreatNotices, i )
	{
		DelayedNoticeInfo *info = &m_delayedThreatNotices[i];

		if ( gpGlobals->curtime >= info->m_flWhen )
		{
			CBaseEntity *ent = info->m_hEnt;
			if ( ent )
			{
				CTFPlayer *player = ToTFPlayer( ent );
				if ( player && player->IsPlayerClass( TF_CLASS_SPY ) )
				{
					RealizeSpy( player );
				}

				GetVisionInterface()->AddKnownEntity( ent );
			}

			m_delayedThreatNotices.Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFBot::SuspectedSpyInfo *CTFBot::IsSuspectedSpy( CTFPlayer *spy )
{
	FOR_EACH_VEC( m_suspectedSpies, i )
	{
		SuspectedSpyInfo *info = m_suspectedSpies[i];
		if ( info->m_hSpy == spy )
		{
			return info;
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::SuspectSpy( CTFPlayer *spy )
{
	SuspectedSpyInfo *info = IsSuspectedSpy( spy );
	if ( info == nullptr )
	{
		info = new SuspectedSpyInfo;
		info->m_hSpy = spy;
		m_suspectedSpies.AddToHead( info );
	}

	info->Suspect();
	if ( info->TestForRealizing() )
	{
		RealizeSpy( spy );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::StopSuspectingSpy( CTFPlayer *spy )
{
	FOR_EACH_VEC( m_suspectedSpies, i )
	{
		SuspectedSpyInfo *info = m_suspectedSpies[i];
		if ( info->m_hSpy == spy )
		{
			delete info;
			m_suspectedSpies.Remove( i );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsKnownSpy( CTFPlayer *spy ) const
{
	return m_knownSpies.HasElement( spy );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::RealizeSpy( CTFPlayer *spy )
{
	if ( IsKnownSpy( spy ) )
		return;

	m_knownSpies.AddToHead( spy );

	SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_CLOAKEDSPY );

	SuspectedSpyInfo *info = IsSuspectedSpy( spy );
	if ( info && info->IsCurrentlySuspected() )
	{
		CUtlVector<CTFPlayer *> teammates;
		CollectPlayers( &teammates, GetTeamNumber(), true );

		FOR_EACH_VEC( teammates, i )
		{
			CTFBot *teammate = ToTFBot( teammates[i] );
			if ( teammate && !teammate->IsKnownSpy( spy ) )
			{
				if ( EyePosition().DistToSqr( teammate->EyePosition() ) < Square( 512.0f ) )
				{
					teammate->SuspectSpy( spy );
					teammate->RealizeSpy( spy );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::ForgetSpy( CTFPlayer *spy )
{
	StopSuspectingSpy( spy );

	CHandle<CTFPlayer> hndl( spy );
	m_knownSpies.FindAndFastRemove( hndl );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::UpdateLookingAroundForEnemies( void )
{
	if ( !m_bLookingAroundForEnemies || m_Shared.IsControlStunned() || ( m_nBotAttrs & AttributeType::DONTLOOKAROUND ) == AttributeType::DONTLOOKAROUND )
		return;

	const CKnownEntity *threat = GetVisionInterface()->GetPrimaryKnownThreat();
	if ( !threat || !threat->GetEntity() )
	{
		UpdateLookingForIncomingEnemies( true );
		return;
	}

	if ( threat->IsVisibleInFOVNow() )
	{
		if ( IsPlayerClass( TF_CLASS_SPY ) && m_Shared.InCond( TF_COND_DISGUISED ) && !m_Shared.InCond( TF_COND_STEALTHED ) )
		{
			UpdateLookingForIncomingEnemies( false );
		}
		else
		{
			GetBodyInterface()->AimHeadTowards( threat->GetEntity(), IBody::CRITICAL, 1.0f, nullptr, "Aiming at a visible threat" );
		}
		return;
	}
	else if ( IsLineOfSightClear( threat->GetEntity(), CBaseCombatCharacter::IGNORE_ACTORS ) )
	{
		// ???
		Vector vecToThreat = threat->GetEntity()->GetAbsOrigin() - GetAbsOrigin();
		float sin, trash;
		FastSinCos( BitsToFloat( 0x3F060A92 ), &sin, &trash );
		float flAdjustment = vecToThreat.NormalizeInPlace() * sin;

		Vector vecToTurnTo = threat->GetEntity()->WorldSpaceCenter() + Vector( RandomFloat( -flAdjustment, flAdjustment ), RandomFloat( -flAdjustment, flAdjustment ), 0 );

		GetBodyInterface()->AimHeadTowards( vecToTurnTo, IBody::IMPORTANT, 1.0f, nullptr, "Turning around to find threat out of our FOV" );
		return;
	}

	CTFWeaponBase *pWeapon = GetActiveTFWeapon();

	if ( /* IsPlayerClass( TF_CLASS_SNIPER )*/ pWeapon && WeaponID_IsSniperRifle( pWeapon->GetWeaponID() ) ) // OFBOT: Allclass support
	{
		UpdateLookingForIncomingEnemies( true );
		return;
	}

	CTFNavArea *pArea = GetLastKnownArea();
	if ( pArea )
	{
		SelectClosestPotentiallyVisible functor( threat->GetLastKnownPosition() );
		pArea->ForAllPotentiallyVisibleAreas( functor );

		if ( functor.m_pSelected )
		{
			for ( int i = 0; i < 10; ++i )
			{
				const Vector vSpot = functor.m_pSelected->GetRandomPoint() + Vector( 0, 0, HumanHeight * 0.75f );
				if ( GetVisionInterface()->IsLineOfSightClear( vSpot ) )
				{
					GetBodyInterface()->AimHeadTowards( vSpot, IBody::IMPORTANT, 1.0f, nullptr, "Looking toward potentially visible area near known but hidden threat" );
					return;
				}
			}

			DebugConColorMsg( NEXTBOT_ERRORS|NEXTBOT_VISION, Color( 0xFF, 0xFF, 0, 0xFF ), "%3.2f: %s can't find clear line to look at potentially visible near known but hidden entity %s(#%d)\n",
				gpGlobals->curtime, GetPlayerName(), threat->GetEntity()->GetClassname(), ENTINDEX( threat->GetEntity() ) );

			return;
		}
	}

	DebugConColorMsg( NEXTBOT_ERRORS|NEXTBOT_VISION, Color( 0xFF, 0xFF, 0, 0xFF ), "%3.2f: %s no potentially visible area to look toward known but hidden entity %s(#%d)\n",
		gpGlobals->curtime, GetPlayerName(), threat->GetEntity()->GetClassname(), ENTINDEX( threat->GetEntity() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::UpdateLookingForIncomingEnemies( bool enemy )
{
	if ( !m_lookForEnemiesTimer.IsElapsed() )
		return;

	m_lookForEnemiesTimer.Start( RandomFloat( 0.3f, 1.0f ) );

	CTFNavArea *area = GetLastKnownArea();
	if ( area == nullptr )
		return;

	int iTeam = enemy ? GetTeamNumber() : GetEnemyTeam( this );
	// really shouldn't happen
	if ( iTeam < 0 || iTeam > 4 )
		iTeam = 0;

	float fRange = 150.0f;
	if ( m_Shared.InCond( TF_COND_AIMING ) )
		fRange = 750.0f;

	const CUtlVector<CTFNavArea *> &areas = area->GetInvasionAreasForTeam( iTeam );
	if ( !areas.IsEmpty() )
	{
		for ( int i = 0; i < 20; ++i )
		{
			const Vector vSpot = areas.Random()->GetRandomPoint();
			if ( this->IsRangeGreaterThan( vSpot, fRange ) )
			{
				if ( GetVisionInterface()->IsLineOfSightClear( vSpot ) )
				{
					GetBodyInterface()->AimHeadTowards( vSpot, IBody::INTERESTING, 1.0f, nullptr, "Looking toward enemy invasion areas" );
					return;
				}
			}
		}
	}

	DebugConColorMsg( NEXTBOT_ERRORS|NEXTBOT_VISION, Color( 0xFF, 0, 0, 0xFF ), "%3.2f: %s no invasion areas to look toward to predict oncoming enemies\n",
		gpGlobals->curtime, GetPlayerName() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::EquipBestWeaponForThreat( const CKnownEntity *threat )
{
	if ( !threat )
		return false;

	if ( !EquipRequiredWeapon() )
	{
		CTFWeaponBase *primary = const_cast<CTFBot *>( this )->GetWeaponInSlot( 0, 0 );
		CTFWeaponBase *secondary = const_cast<CTFBot *>( this )->GetWeaponInSlot( 1, 0 );
		CTFWeaponBase *melee = const_cast<CTFBot *>( this )->GetWeaponInSlot( 2, 0 );

		if ( !IsCombatWeapon( primary ) )
			primary = nullptr;
		if ( !IsCombatWeapon( secondary ) )
			secondary = nullptr;
		if ( !IsCombatWeapon( melee ) )
			melee = nullptr;

		CTFWeaponBase *pWeapon = primary;
		if ( !primary )
		{
			pWeapon = secondary;
			if ( !secondary )
				pWeapon = melee;
		}

		if ( m_iSkill != EASY )
		{
			if ( threat->WasEverVisible() && threat->GetTimeSinceLastSeen() <= 5.0f )
			{
				if ( primary && primary->ReserveAmmo() <= 0 )
					primary = nullptr;
				if ( secondary && secondary->ReserveAmmo() <= 0 )
					secondary = nullptr;

				switch ( m_PlayerClass.GetClassIndex() )
				{
					case TF_CLASS_SNIPER:
						if ( secondary && IsRangeLessThan( threat->GetLastKnownPosition(), 750.0f ) )
						{
							pWeapon = secondary;
						}
						break;
					case TF_CLASS_SOLDIER:
						if ( pWeapon && pWeapon->Clip1() <= 0 )
						{
							if ( secondary && secondary->Clip1() != 0 && IsRangeLessThan( threat->GetLastKnownPosition(), 500.0f ) )
								pWeapon = secondary;
						}
						break;
					case TF_CLASS_PYRO:
						if ( secondary && IsRangeGreaterThan( threat->GetLastKnownPosition(), 750.0f ) )
						{
							pWeapon = secondary;
						}
						else if ( primary )
							pWeapon = primary;
						/*
						if ( threat->GetEntity() )
						{
							CTFPlayer *pPlayer = ToTFPlayer( threat->GetEntity() );
							if ( pPlayer )
							{
								if ( pPlayer->IsPlayerClass( TF_CLASS_SOLDIER ) || pPlayer->IsPlayerClass( TF_CLASS_DEMOMAN ) )
									pWeapon = primary;
							}
						}
						break;
						*/
					case TF_CLASS_SCOUT:
						if ( pWeapon && pWeapon->Clip1() <= 0 )
						{
							pWeapon = secondary;
						}
						break;
				}
			}
		}

		if ( pWeapon ) 
		{
			return Weapon_Switch( pWeapon );
		}
		else if( of_mutator.GetInt() > 0 )
		{
			// iterate through all weapons if we didn't find anything (weapon isn't primary/secondary/melee)
			// prioritize non melee weapons first, if none found, then swap to the melee
			// this is kinda terrible as it just picks the first found weapon

			CTFWeaponBase *pMelee = nullptr;

			for ( int iWeapon = 0; iWeapon < TF_WEAPON_COUNT; iWeapon++ )
			{
				CTFWeaponBase *pWeaponAll = (CTFWeaponBase *)GetWeapon( iWeapon );

				if ( pWeaponAll )
				{
					// HACK: railgun is always first slotwise and its redundant in these mutators, so just pick something else
					if ( ( TFGameRules()->IsMutator( UNHOLY_TRINITY ) || TFGameRules()->IsMutator( CLAN_ARENA ) ) 
						&& pWeaponAll->GetWeaponID() == TF_WEAPON_RAILGUN )
						continue;

					if ( pWeaponAll->IsMeleeWeapon() )
						pMelee = pWeaponAll;
					else
						return Weapon_Switch( pWeaponAll );
				}
			}

			if ( pMelee )
				return Weapon_Switch( pMelee );
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Swap to a weapon our class uses for range
//-----------------------------------------------------------------------------
bool CTFBot::EquipLongRangeWeapon( void )
{	
	// This is so terrible
	// OFBOT TODO:
	if ( ( TFGameRules()->IsFreeRoam() ) || ( IsPlayerClass( TF_CLASS_SOLDIER ) || IsPlayerClass( TF_CLASS_DEMOMAN ) || IsPlayerClass( TF_CLASS_SNIPER ) || IsPlayerClass( TF_CLASS_HEAVYWEAPONS ) ) ) 
	{
		CBaseCombatWeapon *pWeapon = Weapon_GetSlot( 0 );
		if ( pWeapon )
		{
			if ( pWeapon->ReserveAmmo() > 0 )
			{
				Weapon_Switch( pWeapon );
				return true;
			}
		}
	}

	CBaseCombatWeapon *pWeapon = Weapon_GetSlot( 1 );
	if ( pWeapon )
	{
		if ( pWeapon->ReserveAmmo() > 0 )
		{
			Weapon_Switch( pWeapon );
			return true;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::PushRequiredWeapon( CTFWeaponBase *weapon )
{
	CHandle<CTFWeaponBase> hndl;
	if ( weapon ) hndl.Set( weapon );

	m_requiredEquipStack.AddToTail( hndl );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::PopRequiredWeapon( void )
{
	m_requiredEquipStack.RemoveMultipleFromTail( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::EquipRequiredWeapon( void )
{
	if ( m_requiredEquipStack.Count() <= 0 )
		return false;

	CHandle<CTFWeaponBase> &hndl = m_requiredEquipStack.Tail();
	CTFWeaponBase *weapon = hndl.Get();

	return Weapon_Switch( weapon );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFBot::IsSquadmate( CTFPlayer *player ) const
{
	if ( m_pSquad == nullptr )
		return false;

	CTFBot *bot = ToTFBot( player );
	if ( bot )
		return m_pSquad == bot->m_pSquad;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::JoinSquad( CTFBotSquad *squad )
{
	if ( squad )
	{
		squad->Join( this );
		m_pSquad = squad;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::LeaveSquad( void )
{
	if ( m_pSquad )
	{
		m_pSquad->Leave( this );
		m_pSquad = nullptr;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::TouchJumpPad( void )
{
	PressForwardButton( 4.0f ); // so that the bot doesn't just get stuck on the jumppad
	int rng = random->RandomInt( 0, 2 ); // and add some variation each time this fella goes onto a jumppad
	switch ( rng )
	{
		case 0:
			PressLeftButton( random->RandomFloat( 0.2f, 1.2f ) );
			break;
		case 1:
			PressRightButton( random->RandomFloat( 0.2f, 1.2f ) );
			break;
		case 2:
			PressLeftButton( random->RandomFloat( 0.2f, 1.2f ) );
			PressRightButton( random->RandomFloat( 0.2f, 1.2f ) );
			break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::AccumulateSniperSpots( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	SetupSniperSpotAccumulation();

	if ( m_sniperStandAreas.IsEmpty() || m_sniperLookAreas.IsEmpty() )
	{
		if ( m_sniperSpotTimer.IsElapsed() )
			ClearSniperSpots();

		return;
	}

	for ( int i=0; i<tf_bot_sniper_spot_search_count.GetInt(); ++i )
	{
		SniperSpotInfo newInfo{};
		newInfo.m_pHomeArea = m_sniperStandAreas.Random();
		newInfo.m_vecHome = newInfo.m_pHomeArea->GetRandomPoint();
		newInfo.m_pForwardArea = m_sniperLookAreas.Random();
		newInfo.m_vecForward = newInfo.m_pForwardArea->GetRandomPoint();

		newInfo.m_flRange = ( newInfo.m_vecHome - newInfo.m_vecForward ).Length();

		if ( newInfo.m_flRange < tf_bot_sniper_spot_min_range.GetFloat() )
			continue;

		if ( !IsLineOfFireClear( newInfo.m_vecHome + Vector( 0, 0, 60.0f ), newInfo.m_vecForward + Vector( 0, 0, 60.0f ) ) )
			continue;

		float flIncursion1 = newInfo.m_pHomeArea->GetIncursionDistance( GetEnemyTeam( this ) );
		float flIncursion2 = newInfo.m_pForwardArea->GetIncursionDistance( GetEnemyTeam( this ) );

		newInfo.m_flIncursionDiff = flIncursion1 - flIncursion2;

		if ( m_sniperSpots.Count() < tf_bot_sniper_spot_max_count.GetInt() )
			m_sniperSpots.AddToTail( newInfo );

		for ( int j=0; j<m_sniperSpots.Count(); ++j )
		{
			SniperSpotInfo *info = &m_sniperSpots[j];

			if ( flIncursion1 - flIncursion2 <= info->m_flIncursionDiff )
				continue;

			*info = newInfo;
		}
	}

	if ( IsDebugging( NEXTBOT_BEHAVIOR ) )
	{
		for ( int i=0; i<m_sniperSpots.Count(); ++i )
		{
			NDebugOverlay::Cross3D( m_sniperSpots[i].m_vecHome, 5.0f, 255, 0, 255, true, 0.1f );
			NDebugOverlay::Line( m_sniperSpots[i].m_vecHome, m_sniperSpots[i].m_vecForward, 0, 200, 0, true, 0.1f );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::SetupSniperSpotAccumulation( void )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	CBaseEntity *pObjective = nullptr;
	if ( TFGameRules()->InGametype( TF_GAMETYPE_PAYLOAD ) )
	{
		CTeamTrainWatcher *pWatcher = TFGameRules()->GetPayloadToPush( GetTeamNumber() );
		if ( !pWatcher )
		{
			pWatcher = TFGameRules()->GetPayloadToBlock( GetTeamNumber() );
			if ( !pWatcher )
			{
				ClearSniperSpots();
				return;
			}
		}

		pObjective = pWatcher->GetTrainEntity();
	}
	else
	{
		if ( !TFGameRules()->InGametype( TF_GAMETYPE_CP ) )
		{
			ClearSniperSpots();
			return;
		}

		pObjective = GetMyControlPoint();
	}

	if ( pObjective == nullptr )
	{
		ClearSniperSpots();
		return;
	}

	if ( pObjective == m_sniperGoalEnt && Square( tf_bot_sniper_goal_entity_move_tolerance.GetFloat() ) > ( pObjective->WorldSpaceCenter() - m_sniperGoal ).LengthSqr() )
		return;

	ClearSniperSpots();

	const int iMyTeam = GetTeamNumber();
	const int iEnemyTeam = GetEnemyTeam( this );
	bool bCheckForward = false;
	CTFNavArea *pObjectiveArea = nullptr;

	m_sniperStandAreas.RemoveAll();
	m_sniperLookAreas.RemoveAll();

	if ( TFGameRules()->InGametype( TF_GAMETYPE_PAYLOAD ) )
	{
		pObjectiveArea = static_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( pObjective->WorldSpaceCenter(), true, 500.0f ) );
		bCheckForward = iEnemyTeam != pObjective->GetTeamNumber();
	}
	else
	{
		if ( GetMyControlPoint()->GetPointIndex() >= MAX_CONTROL_POINTS )
			return;

		pObjectiveArea = TFNavMesh()->GetMainControlPointArea( GetMyControlPoint()->GetPointIndex() );
		bCheckForward = GetMyControlPoint()->GetOwner() == iMyTeam;
	}

	if ( !pObjectiveArea )
		return;

	for ( int i=0; i<TheNavAreas.Count(); ++i )
	{
		CTFNavArea *area = static_cast<CTFNavArea *>( TheNavAreas[i] );

		float flMyIncursion = area->GetIncursionDistance( iMyTeam );
		if ( flMyIncursion < 0.0f )
			continue;

		float flEnemyIncursion = area->GetIncursionDistance( iEnemyTeam );
		if ( flEnemyIncursion < 0.0f )
			continue;

		if ( flEnemyIncursion <= pObjectiveArea->GetIncursionDistance( iEnemyTeam ) )
			m_sniperLookAreas.AddToTail( area );

		if ( bCheckForward )
		{
			if ( pObjectiveArea->GetIncursionDistance( iMyTeam ) + tf_bot_sniper_spot_point_tolerance.GetFloat() >= flMyIncursion )
				m_sniperStandAreas.AddToTail( area );
		}
		else
		{
			if ( pObjectiveArea->GetIncursionDistance( iMyTeam ) - tf_bot_sniper_spot_point_tolerance.GetFloat() >= flMyIncursion )
				m_sniperStandAreas.AddToTail( area );
		}
	}

	m_sniperGoalEnt = pObjective;
	m_sniperGoal = pObjective->WorldSpaceCenter();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::ClearSniperSpots( void )
{
	m_sniperSpots.Purge();
	m_sniperStandAreas.RemoveAll();
	m_sniperLookAreas.RemoveAll();
	m_sniperGoalEnt = nullptr;

	m_sniperSpotTimer.Start( RandomFloat( 5.0f, 10.0f ) );
}

//-----------------------------------------------------------------------------
// Purpose: Seperate ourselves with minor push forces from teammates
//-----------------------------------------------------------------------------
void CTFBot::AvoidPlayers( CUserCmd *pCmd )
{
	if ( !tf_avoidteammates.GetBool() /*|| !tf_avoidteammates_pushaway.GetBool()*/ )
		return;

	Vector vecFwd, vecRight;
	this->EyeVectors( &vecFwd, &vecRight );

	Vector vecAvoidCenter = vec3_origin;
	const float flRadius = 50.0;

	CUtlVector<CTFPlayer *> teammates;
	CollectPlayers( &teammates, GetTeamNumber(), true );
	for ( int i=0; i<teammates.Count(); i++ )
	{
		if ( IsSelf( teammates[i] ) || HasTheFlag() )
			continue;

		Vector vecToTeamMate = GetAbsOrigin() - teammates[i]->GetAbsOrigin();
		if ( Square( flRadius ) > vecToTeamMate.LengthSqr() )
		{
			vecAvoidCenter += vecToTeamMate.Normalized() * ( 1.0f - ( 1.0f / flRadius ) );
		}
	}

	if ( !vecAvoidCenter.IsZero() )
	{
		vecAvoidCenter.NormalizeInPlace();

		m_Shared.SetSeparation( true );
		m_Shared.SetSeparationVelocity( vecAvoidCenter * flRadius );
		pCmd->forwardmove += vecAvoidCenter.Dot( vecFwd ) * flRadius;
		pCmd->sidemove += vecAvoidCenter.Dot( vecRight ) * flRadius;
	}
	else
	{
		m_Shared.SetSeparation( false );
		m_Shared.SetSeparationVelocity( vec3_origin );
	}
}

//-----------------------------------------------------------------------------
// Purpose: If we were assigned to take over a real player, return them
//-----------------------------------------------------------------------------
CBaseCombatCharacter *CTFBot::GetEntity( void ) const
{
	return ToBasePlayer( m_controlling ) ? m_controlling : (CTFPlayer *)this;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFBot::SelectReachableObjects( CUtlVector<EHANDLE> const &knownHealth, CUtlVector<EHANDLE> *outVector, INextBotFilter const &func, CNavArea *pStartArea, float flMaxRange )
{
	if ( !pStartArea || !outVector )
		return;

	CUtlVector<EHANDLE> selectedHealths;
	for ( int i=0; i<knownHealth.Count(); ++i )
	{
		CBaseEntity *pEntity = knownHealth[i];
		if ( !pEntity || !func.IsSelected( pEntity ) )
			continue;

		EHANDLE hndl( pEntity );
		selectedHealths.AddToTail( hndl );
	}

	outVector->RemoveAll();

	CollectReachableObjects collector( this, &selectedHealths, outVector, flMaxRange );
	SearchSurroundingAreas( pStartArea, collector, flMaxRange );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFPlayer *CTFBot::SelectRandomReachableEnemy( void )
{
	// ficool2 - This function originally collected every enemy, and it only checked if they were in a respawn room
	// It didn't care if the enemy in question was invis, disguised, or even alive!
	// I have changed this, although I'm not sure to which extent this will make bots in CTF perform better

	CUtlVector<CTFPlayer *> enemies;
	CollectPlayers( &enemies, GetEnemyTeam( this ), true );

	CUtlVector<CTFPlayer *> validEnemies;
	for ( int i=0; i< enemies.Count(); ++i )
	{
		CTFPlayer *pEnemy = enemies[i];

		if ( !pEnemy )
			continue;

		if ( !pEnemy->IsAlive() )
			continue;

		if ( pEnemy->m_Shared.InCondInvis() || pEnemy->m_Shared.InCond( TF_COND_DISGUISED ) )
			continue;

		if ( PointInRespawnRoom( pEnemy, pEnemy->WorldSpaceCenter() ) )
			continue;

		validEnemies.AddToTail( pEnemy );
	}

	if ( !validEnemies.IsEmpty() )
		return validEnemies.Random();

	return nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Can we change class? If nested or have uber then no
//-----------------------------------------------------------------------------
bool CTFBot::CanChangeClass( void )
{
	if ( IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		if ( !GetObjectOfType( OBJ_SENTRYGUN, 0 ) && !GetObjectOfType( OBJ_TELEPORTER, TELEPORTER_TYPE_EXIT ) )
			return true;

		return false;
	}

	// OFBOT: Allclass support
	// if ( !IsPlayerClass( TF_CLASS_MEDIC ) )
	// 	return true;
	
	CTFWeaponBase *pWeapon = Weapon_OwnsThisID( TF_WEAPON_MEDIGUN );

	if ( pWeapon )
	{
		CWeaponMedigun *pMedigun = dynamic_cast<CWeaponMedigun *>( pWeapon );

		if ( pMedigun )
			return pMedigun->GetChargeLevel() <= 0.25f;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFBot::GetNextSpawnClassname( void )
{	
	const char *szClassName = tf_bot_force_class.GetString();
	if ( !FStrEq( szClassName, "" ) )
	{
		// BOTTODO: Mercenary and Civilian
		const int iClassIdx = GetClassIndexFromString( szClassName, TF_CLASS_CIVILIAN );
		if ( iClassIdx != TF_CLASS_UNDEFINED )
			return GetPlayerClassData( iClassIdx )->m_szClassName;
	}

	// OFBOT Todo: Brutal hack to stop this function from crashing
	return "random";

	// The operations below this line cause a frequent crash on bot death
	// VS reports the returned string as "Error reading characters of string", I do not know why

	static const int offenseRoster[] ={
		TF_CLASS_SCOUT,
		TF_CLASS_ENGINEER,
		TF_CLASS_SOLDIER,
		TF_CLASS_HEAVYWEAPONS,
		TF_CLASS_MEDIC,
		TF_CLASS_DEMOMAN,
		TF_CLASS_SCOUT,
		TF_CLASS_PYRO,
		TF_CLASS_SOLDIER,
		TF_CLASS_DEMOMAN,
		TF_CLASS_SNIPER,
		TF_CLASS_MEDIC,
		TF_CLASS_SPY
	};
	static const int defenseRoster[] ={
		TF_CLASS_MEDIC,
		TF_CLASS_ENGINEER,
		TF_CLASS_SOLDIER,
		TF_CLASS_DEMOMAN,
		TF_CLASS_SCOUT,
		TF_CLASS_HEAVYWEAPONS,
		TF_CLASS_MEDIC,
		TF_CLASS_SNIPER,
		TF_CLASS_ENGINEER,
		TF_CLASS_SOLDIER,
		TF_CLASS_MEDIC,
		TF_CLASS_PYRO,
		TF_CLASS_SPY
	};
	static const int rosterSize = ( ARRAYSIZE( offenseRoster ) + ARRAYSIZE( defenseRoster ) ) / 2;

	if ( !CanChangeClass() )
		return m_PlayerClass.GetName();

	CountClassMembers func( this, GetTeamNumber() );
	ForEachPlayer( func );

	const int *pRoster = NULL;
	if ( !TFGameRules()->IsInKothMode() )
	{
		if ( !TFGameRules()->InGametype( TF_GAMETYPE_CP ) )
		{
			if ( TFGameRules()->InGametype( TF_GAMETYPE_PAYLOAD ) && GetTeamNumber() == TF_TEAM_RED )
				pRoster = defenseRoster;
			else
				pRoster = offenseRoster;
		}
		else
		{
			CUtlVector<CTeamControlPoint *> defensePoints;
			CUtlVector<CTeamControlPoint *> attackPoints;
			TFGameRules()->CollectCapturePoints( this, &attackPoints );
			TFGameRules()->CollectDefendPoints( this, &defensePoints );

			if ( attackPoints.IsEmpty() && !defensePoints.IsEmpty() )
				pRoster = defenseRoster;
			else
				pRoster = offenseRoster;
		}
	}
	else
	{
		CTeamControlPoint *pPoint = this->GetMyControlPoint();
		if ( pPoint )
		{
			if ( GetTeamNumber() == ObjectiveResource()->GetOwningTeam( pPoint->GetPointIndex() ) )
				pRoster = defenseRoster;
			else
				pRoster = offenseRoster;
		}
		else
			pRoster = offenseRoster;
	}

	if ( pRoster == NULL )
		return "random";

	// BOT TODO: Mercenary and civilian here!
	float flClassWeight[ TF_CLASS_ENGINEER ] = { 1.0f };
	for ( int i=0; i < rosterSize; ++i )
	{
		if ( !TFGameRules()->CanBotChooseClass( this, pRoster[i] ) )
			continue;

		flClassWeight[ pRoster[i] ] += 1.0f;

		if ( m_PlayerClass.GetClassIndex() == pRoster[i] )
			flClassWeight[ pRoster[i] ] *= 0.1f;

		flClassWeight[ pRoster[i] ] /= func.m_aClassCounts[ pRoster[i] ] + 1;
	}

	float flTotalFitness = 0;

	// BOT TODO: Mercenary and civilian here!
	for ( int i = TF_FIRST_NORMAL_CLASS; i < ( TF_CLASS_ENGINEER ); i++ )
		flTotalFitness += flClassWeight[i];

	float flRandom = RandomFloat(0.0f, flTotalFitness);

	flTotalFitness = 0;

	int iDesiredClass = TF_CLASS_UNDEFINED;

	// BOT TODO: Mercenary and civilian here!
	for ( int i = TF_FIRST_NORMAL_CLASS; i < ( TF_CLASS_ENGINEER ); i++ )
	{
		flTotalFitness += flClassWeight[i];

		if ( flRandom <= flTotalFitness )
		{
			iDesiredClass = i;
			break;
		}
	}

	
	if ( iDesiredClass > TF_CLASS_UNDEFINED )
	{
		szClassName = GetPlayerClassData( iDesiredClass )->m_szClassName;

		if ( szClassName )
			return szClassName;
	}
	
	return "random";
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::GetUberDeployDelayDuration( void ) const
{
	float flDelay = 0.0f;
	//CALL_ATTRIB_HOOK_FLOAT( flDelay, bot_medic_uber_deploy_delay_duration );
	return flDelay;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CTFBot::GetUberHealthThreshold( void ) const
{
	float flThreshold = 0.0f;
	//CALL_ATTRIB_HOOK_FLOAT( flThreshold, bot_medic_uber_health_threshold );
	return flThreshold > 0.0f ? flThreshold : 50.0f;
}



CTFBotPathCost::CTFBotPathCost( CTFBot *actor, RouteType routeType )
	: m_Actor( actor ), m_iRouteType( routeType )
{
	const ILocomotion *loco = m_Actor->GetLocomotionInterface();
	m_flStepHeight = loco->GetStepHeight();
	m_flMaxJumpHeight = loco->GetMaxJumpHeight();
	m_flDeathDropHeight = loco->GetDeathDropHeight();
}

float CTFBotPathCost::operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	if ( fromArea == nullptr )
	{
		// first area in path; zero cost
		return 0.0f;
	}

	if ( !m_Actor->GetLocomotionInterface()->IsAreaTraversable( area ) )
	{
		// dead end
		return -1.0f;
	}

	float fDist;
	if ( ladder != nullptr )
		fDist = ladder->m_length;
	else if ( length != 0.0f )
		fDist = length;
	else
		fDist = ( area->GetCenter() - fromArea->GetCenter() ).Length();

	const float dz = fromArea->ComputeAdjacentConnectionHeightChange( area );
	if ( dz >= m_flStepHeight )
	{
		// too high!
		if ( dz >= m_flMaxJumpHeight )
			return -1.0f;

		// jumping is slow
		fDist *= 2;
	}
	else
	{
		// yikes, this drop will hurt too much!
		if ( dz < -m_flDeathDropHeight )
			return -1.0f;
	}

	// consistently random pathing with huge cost modifier
	float fMultiplier = 1.0f;
	if ( m_iRouteType == DEFAULT_ROUTE )
	{
		const float rand = m_Actor->TransientlyConsistentRandomValue( 10.0f, 0 );
		fMultiplier += ( rand + 1.0f ) * 50.0f;
	}

	const int iOtherTeam = GetEnemyTeam( m_Actor );

	for ( int i=0; i < IBaseObjectAutoList::AutoList().Count(); ++i )
	{
		CBaseObject *obj = static_cast<CBaseObject *>( IBaseObjectAutoList::AutoList()[i] );

		if ( obj->GetType() == OBJ_SENTRYGUN && obj->GetTeamNumber() == iOtherTeam )
		{
			obj->UpdateLastKnownArea();
			if ( area == obj->GetLastKnownArea() )
			{
				if ( m_iRouteType == SAFEST_ROUTE )
					fDist *= 5.0f;
				else if ( m_Actor->IsPlayerClass( TF_CLASS_SPY ) ) // spies always consider sentryguns to avoid
					fDist *= 10.0f;
			}
		}
	}

	// we need to be sneaky, try to take routes where no players are
	if ( m_Actor->IsPlayerClass( TF_CLASS_SPY ) && !( TFGameRules()->IsFreeRoam() ) ) //OFBOT TODO: I don't know if this is a good idea
		fDist += ( fDist * 10.0f * area->GetPlayerCount( m_Actor->GetTeamNumber() ) );

	float fCost = fDist * fMultiplier;

	if ( area->HasAttributes( NAV_MESH_FUNC_COST ) )
		fCost *= area->ComputeFuncNavCost( m_Actor );

	return fromArea->GetCostSoFar() + fCost;
}


void DifficultyChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
	if ( tf_bot_difficulty.GetInt() >= CTFBot::EASY && tf_bot_difficulty.GetInt() <= CTFBot::EXPERT )
	{
		CUtlVector<INextBot *> bots;
		TheNextBots().CollectAllBots( &bots );
		for ( int i=0; i<bots.Count(); ++i )
		{
			CTFBot *pBot = dynamic_cast<CTFBot *>( bots[i]->GetEntity() );
			if ( pBot == nullptr )
				continue;

			pBot->m_iSkill = (CTFBot::DifficultyType)tf_bot_difficulty.GetInt();
		}
	}
	else
		Warning( "tf_bot_difficulty value out of range [0,4]: %d", tf_bot_difficulty.GetInt() );
}

void PrefixNameChanged( IConVar *var, const char *pOldValue, float flOldValue )
{
	CUtlVector<INextBot *> bots;
	TheNextBots().CollectAllBots( &bots );
	for ( int i=0; i<bots.Count(); ++i )
	{
		CTFBot *pBot = dynamic_cast<CTFBot *>( bots[i]->GetEntity() );
		if ( pBot == nullptr )
			continue;

		if ( tf_bot_prefix_name_with_difficulty.GetBool() )
		{
			const char *szSkillName = DifficultyToName( pBot->m_iSkill );
			const char *szCurrentName = pBot->GetPlayerName();

			engine->SetFakeClientConVarValue( pBot->edict(), "name", CFmtStr( "%s%s", szSkillName, szCurrentName ) );
		}
		else
		{
			const char *szSkillName = DifficultyToName( pBot->m_iSkill );
			const char *szCurrentName = pBot->GetPlayerName();

			engine->SetFakeClientConVarValue( pBot->edict(), "name", &szCurrentName[Q_strlen( szSkillName )] );
		}
	}
}


CON_COMMAND_F( tf_bot_add, "Add a bot.", FCVAR_GAMEDLL )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() )
	{
		int count = Clamp( Q_atoi( args.Arg( 1 ) ), 1, gpGlobals->maxClients );
		for ( int i = 0; i < count; ++i )
		{
			char szBotName[32];
			if ( args.ArgC() > 4 )
				Q_strncpy( szBotName, args.Arg( 4 ), sizeof(szBotName) );
			else
				Q_strncpy( szBotName, TheTFBots().GetRandomBotName(), sizeof(szBotName) );

			CTFBot *bot = NextBotCreatePlayerBot<CTFBot>( szBotName );
			if ( bot == nullptr )
				return;

			char szTeam[10];
			if ( args.ArgC() > 2 )
			{
				if ( IsTeamName( args.Arg( 2 ) ) )
					Q_strncpy( szTeam, args.Arg( 2 ), sizeof(szTeam) );
				else
				{
					Warning( "Invalid argument '%s'\n", args.Arg( 2 ) );
					Q_strncpy( szTeam, "auto", sizeof(szTeam) );
				}
			}
			else
				Q_strncpy( szTeam, "auto", sizeof(szTeam) );

			bot->HandleCommand_JoinTeam( szTeam );

			// pick a random color!
			Vector m_vecPlayerColor = vec3_origin;

			if ( TFGameRules() && TFGameRules()->IsDMGamemode() )
			{
				float flColors[ 3 ];

				for ( int i = 0; i < ARRAYSIZE( flColors ); i++ )
					flColors[ i ] = RandomFloat( 1, 255 );

				m_vecPlayerColor.Init( flColors[0], flColors[1], flColors[2] );

				m_vecPlayerColor /= 255.0f;

				bot->m_vecPlayerColor = m_vecPlayerColor;
			}

			char szClassName[32];

			const char *szForceClassName = tf_bot_force_class.GetString();
			if ( !FStrEq( szForceClassName, "" ) )
			{
				// BOTTODO: Mercenary and Civilian
				const int iClassIdx = GetClassIndexFromString( szForceClassName, TF_CLASS_CIVILIAN );
				if ( iClassIdx != TF_CLASS_UNDEFINED )
					Q_strncpy( szClassName, szForceClassName, sizeof(szClassName) );
				else 
					Q_strncpy( szClassName, "random", sizeof(szClassName) );
			}
			else
			{
				if ( args.ArgC() > 3 )
				{
					if ( IsPlayerClassName( args.Arg( 3 ) ) )
						Q_strncpy( szClassName, args.Arg( 3 ), sizeof(szClassName) );
					else
					{
						Warning( "Invalid argument '%s'\n", args.Arg( 3 ) );
						Q_strncpy( szClassName, "random", sizeof(szClassName) );
					}
				}
				else
					Q_strncpy( szClassName, "random", sizeof(szClassName) );
			}

			bot->HandleCommand_JoinClass( szClassName );

		}

		TheTFBots().OnForceAddedBots( count );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Only removes INextBots that are CTFBot derivatives with the CTFBotManager
//-----------------------------------------------------------------------------
class TFBotDestroyer
{
public:
	TFBotDestroyer( int team=TEAM_ANY ) : m_team( team ) { }

	bool operator()( CBaseCombatCharacter *bot )
	{
		if ( m_team == TEAM_ANY || bot->GetTeamNumber() == m_team )
		{
			CTFBot *pBot = ToTFBot( bot->GetBaseEntity() );
			if ( pBot == nullptr )
				return true;

			engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pBot->GetUserID() ) );
			TheTFBots().OnForceKickedBots( 1 );
		}

		return true;
	}

private:
	int m_team;
};

CON_COMMAND_F( tf_bot_kick, "Remove a TFBot by name, or all bots (\"all\").", FCVAR_GAMEDLL )
{
	if ( UTIL_IsCommandIssuedByServerAdmin() )
	{
		const char *arg = args.Arg( 1 );
		if ( !Q_strncmp( arg, "all", 3 ) )
		{
			TFBotDestroyer func;
			TheNextBots().ForEachCombatCharacter( func );
		}
		else
		{
			CBasePlayer *pBot = UTIL_PlayerByName( arg );
			if ( pBot && pBot->IsFakeClient() )
			{
				engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pBot->GetUserID() ) );
				TheTFBots().OnForceKickedBots( 1 );
			}
			else if ( IsTeamName( arg ) )
			{
				TFBotDestroyer func;
				if ( !Q_stricmp( arg, "red" ) )
					func = TFBotDestroyer( TF_TEAM_RED );
				else if ( !Q_stricmp( arg, "blue" ) )
					func = TFBotDestroyer( TF_TEAM_BLUE );

				TheNextBots().ForEachCombatCharacter( func );
			}
			else
			{
				Msg( "No bot or team with that name\n" );
			}
		}
	}
}
