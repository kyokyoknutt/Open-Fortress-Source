#include "cbase.h"
#include "../tf_bot.h"
#include "tf_gamerules.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_get_powerup.h"
#include "entity_condpowerup.h"

ConVar tf_bot_powerup_search_range( "tf_bot_powerup_search_range", "2000", FCVAR_CHEAT, "How far bots will search to find powerups around them" );
ConVar tf_bot_debug_powerup_scavanging( "tf_bot_debug_powerup_scavanging", "0", FCVAR_CHEAT );


static CHandle<CBaseEntity> s_possiblePowerup;
static CTFBot *s_possibleBot;
static int s_possibleFrame;


CTFBotGetPowerup::CTFBotGetPowerup()
{
	m_PathFollower.Invalidate();
	m_hPowerup = nullptr;
}

CTFBotGetPowerup::~CTFBotGetPowerup()
{
}


const char *CTFBotGetPowerup::GetName() const
{
	return "GetPowerup";
}


ActionResult<CTFBot> CTFBotGetPowerup::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	me->m_bLookingAroundForEnemies = false;
	me->m_bPickedUpPowerup = false;

	if ( ( gpGlobals->framecount != s_possibleFrame || s_possibleBot != me ) && ( !IsPossible( me ) || !s_possiblePowerup ) )
		return Action<CTFBot>::Done( "Can't get powerup" );

	m_hPowerup = s_possiblePowerup;

	CTFBotPathCost cost( me, FASTEST_ROUTE );
	if ( !m_PathFollower.Compute( me, m_hPowerup->WorldSpaceCenter(), cost ) )
		return Action<CTFBot>::Done( "No path to powerup" );

	return Action<CTFBot>::Continue();
}

void CTFBotGetPowerup::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	me->m_bLookingAroundForEnemies = true;
}

ActionResult<CTFBot> CTFBotGetPowerup::OnSuspend( CTFBot *me, Action<CTFBot> *newAction )
{
	me->m_bLookingAroundForEnemies = true;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotGetPowerup::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	me->m_bLookingAroundForEnemies = false;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotGetPowerup::Update( CTFBot *me, float dt )
{
	if ( me->m_bPickedUpPowerup )
	{
		me->m_bPickedUpPowerup = false;
		return Action<CTFBot>::Done( "I picked up a powerup" );
	}

	if ( !m_hPowerup )
		return Action<CTFBot>::Done( "Powerup I was going for has been taken" );

	if ( !m_PathFollower.IsValid() )
		return Action<CTFBot>::Done( "My path became invalid" );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	m_PathFollower.Update( me );

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotGetPowerup::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotGetPowerup::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotGetPowerup::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Failed to reach powerup" );
}

EventDesiredResult<CTFBot> CTFBotGetPowerup::OnStuck( CTFBot *me )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Stuck trying to reach powerup" );
}


QueryResultType CTFBotGetPowerup::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


bool CTFBotGetPowerup::IsPossible( CTFBot *actor )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	CUtlVector<EHANDLE> powerups;
	for ( int i = 0; i < ICondPowerupAutoList::AutoList().Count(); ++i )
	{
		EHANDLE hndl = static_cast< CCondPowerup* >( ICondPowerupAutoList::AutoList()[ i ] );
		powerups.AddToTail( hndl );
	}

	CPowerupFilter filter( actor );
	actor->SelectReachableObjects( powerups, &powerups, filter, actor->GetLastKnownArea(), tf_bot_powerup_search_range.GetFloat() );

	if ( powerups.IsEmpty() )
	{
		if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
			Warning( "%3.2f: No powerup nearby.\n", gpGlobals->curtime );

		return false;
	}

	CBaseEntity *pPowerup = nullptr;
	for( int i=0; i<powerups.Count(); ++i )
	{
		if ( !powerups[i] )
			continue;

		if ( powerups[i]->GetTeamNumber() == GetEnemyTeam( actor ) )
			continue;

		pPowerup = powerups[i];

		if ( tf_bot_debug_powerup_scavanging.GetBool() )
			NDebugOverlay::Cross3D( pPowerup->WorldSpaceCenter(), 5.0f, 255, 100, 154, false, 1.0 );
	}

	if ( pPowerup )
	{
		CTFBotPathCost func( actor, FASTEST_ROUTE );
		if ( !NavAreaBuildPath( actor->GetLastKnownArea(), NULL, &pPowerup->WorldSpaceCenter(), func ) )
		{
			if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
				Warning( "%3.2f: No path to powerup!\n", gpGlobals->curtime );
		}

		s_possiblePowerup = pPowerup;
		s_possibleBot = actor;
		s_possibleFrame = gpGlobals->framecount;

		return true;
	}

	if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
		Warning( " %3.2f: No powerup nearby.\n", gpGlobals->curtime );

	return false;
}


CPowerupFilter::CPowerupFilter( CTFPlayer *actor )
	: m_pActor( actor )
{
	m_flMinCost = FLT_MAX;
}


bool CPowerupFilter::IsSelected( const CBaseEntity *ent ) const
{
	/*CClosestTFPlayer functor( ent->WorldSpaceCenter() );
	ForEachPlayer( functor );

	// Don't run into enemies while trying to scavenge
	if ( functor.m_pPlayer && !functor.m_pPlayer->InSameTeam( m_pActor ) )
		return false;*/

	CTFNavArea *pArea = static_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( ent->WorldSpaceCenter() ) );
	if ( !pArea )
		return false;

	// Can't pick up spawners that are respawning
	CCondPowerup *pPowerup = dynamic_cast< CCondPowerup *>( const_cast<CBaseEntity *>( ent )  );

	if ( pPowerup )
	{
		if ( pPowerup->m_bRespawning ) // don't go for spawners that are respawning
			return false;

		if ( pPowerup->m_bDisabled ) // don't go for spawners that are disabled
			return false;
	}

	// Find minimum cost area we are currently searching
	if ( !pArea->IsMarked() || m_flMinCost < pArea->GetCostSoFar() )
		return false;

	const_cast<CPowerupFilter *>( this )->m_flMinCost = pArea->GetCostSoFar();

	return true;
}
