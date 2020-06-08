#include "cbase.h"
#include "../tf_bot.h"
#include "tf_gamerules.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_use_map_teleport.h"
#include "triggers.h"

ConVar tf_bot_mapteleport_search_range( "tf_bot_map_teleport_search_range", "512", FCVAR_CHEAT, "How far bots will search to find map teleports around them" );
ConVar tf_bot_debug_mapteleport_scavanging( "tf_bot_debug_mapteleport_scavanging", "0", FCVAR_CHEAT );


static CHandle<CBaseEntity> s_possibleMapTeleport;
static CTFBot *s_possibleBot;
static int s_possibleFrame;


CTFBotUseMapTeleport::CTFBotUseMapTeleport()
{
	m_PathFollower.Invalidate();
	m_hMapTeleport = nullptr;
}

CTFBotUseMapTeleport::~CTFBotUseMapTeleport()
{
}


const char *CTFBotUseMapTeleport::GetName() const
{
	return "UseMapTeleport";
}


ActionResult<CTFBot> CTFBotUseMapTeleport::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	me->m_bLookingAroundForEnemies = false;
	me->m_bTouchedTeleport = false;

	if ( ( gpGlobals->framecount != s_possibleFrame || s_possibleBot != me ) && ( !IsPossible( me ) || !s_possibleMapTeleport ) )
		return Action<CTFBot>::Done( "Can't go to map teleport" );

	m_hMapTeleport = s_possibleMapTeleport;

	CTFBotPathCost cost( me, FASTEST_ROUTE );
	if ( !m_PathFollower.Compute( me, m_hMapTeleport->GetAbsOrigin(), cost ) )
		return Action<CTFBot>::Done( "No path to map teleport" );

	return Action<CTFBot>::Continue();
}

void CTFBotUseMapTeleport::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	me->m_bLookingAroundForEnemies = true;
}

ActionResult<CTFBot> CTFBotUseMapTeleport::OnSuspend( CTFBot *me, Action<CTFBot> *newAction )
{
	me->m_bLookingAroundForEnemies = true;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotUseMapTeleport::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	me->m_bLookingAroundForEnemies = false;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotUseMapTeleport::Update( CTFBot *me, float dt )
{
	if ( me->m_bTouchedTeleport )
	{
		me->m_bTouchedTeleport = false;
		return Action<CTFBot>::Done( "I touched the map teleport" );
	}

	if ( !m_hMapTeleport )
		return Action<CTFBot>::Done( "Map Teleport I was going for is invalid" );

	if ( !m_PathFollower.IsValid() )
		return Action<CTFBot>::Done( "My path became invalid" );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	m_PathFollower.Update( me );

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotUseMapTeleport::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotUseMapTeleport::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotUseMapTeleport::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Failed to reach map teleport" );
}

EventDesiredResult<CTFBot> CTFBotUseMapTeleport::OnStuck( CTFBot *me )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Stuck trying to reach map teleport" );
}


QueryResultType CTFBotUseMapTeleport::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


bool CTFBotUseMapTeleport::IsPossible( CTFBot *actor )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	CUtlVector<EHANDLE> mapteleports;

	for ( int i = 0; i < ITriggerTeleportAutoList::AutoList().Count(); ++i )
	{
		EHANDLE hndl = static_cast< CTriggerTeleport* >( ITriggerTeleportAutoList::AutoList()[ i ] );
		mapteleports.AddToTail( hndl );
	}

	CMapTeleportFilter filter( actor );
	actor->SelectReachableObjects( mapteleports, &mapteleports, filter, actor->GetLastKnownArea(), tf_bot_mapteleport_search_range.GetFloat() );

	if ( mapteleports.IsEmpty() )
	{
		if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
			Warning( "%3.2f: No map teleports nearby.\n", gpGlobals->curtime );

		return false;
	}

	CBaseEntity *pMapTeleport = nullptr;
	for( int i=0; i<mapteleports.Count(); ++i )
	{
		if ( !mapteleports[i] )
			continue;

		if ( mapteleports[i]->GetTeamNumber() == GetEnemyTeam( actor ) )
			continue;

		pMapTeleport = mapteleports[i];

		if ( tf_bot_debug_mapteleport_scavanging.GetBool() )
			NDebugOverlay::Cross3D( pMapTeleport->GetAbsOrigin(), 5.0f, 255, 100, 154, false, 1.0 );
	}

	if ( pMapTeleport )
	{
		CTFBotPathCost func( actor, FASTEST_ROUTE );
		if ( !NavAreaBuildPath( actor->GetLastKnownArea(), NULL, &pMapTeleport->GetAbsOrigin(), func ) )
		{
			if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
				Warning( "%3.2f: No path to map teleport!\n", gpGlobals->curtime );
		}

		s_possibleMapTeleport = pMapTeleport;
		s_possibleBot = actor;
		s_possibleFrame = gpGlobals->framecount;

		return true;
	}

	if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
		Warning( " %3.2f: No map teleport nearby.\n", gpGlobals->curtime );

	return false;
}


CMapTeleportFilter::CMapTeleportFilter( CTFPlayer *actor )
	: m_pActor( actor )
{
	m_flMinCost = FLT_MAX;
}


bool CMapTeleportFilter::IsSelected( const CBaseEntity *ent ) const
{
	/*CClosestTFPlayer functor( ent->WorldSpaceCenter() );
	ForEachPlayer( functor );

	// Don't run into enemies while trying to scavenge
	if ( functor.m_pPlayer && !functor.m_pPlayer->InSameTeam( m_pActor ) )
		return false;*/

	CTFNavArea *pArea = static_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( ent->WorldSpaceCenter() ) );
	if ( !pArea )
		return false;

	// Find minimum cost area we are currently searching
	if ( !pArea->IsMarked() || m_flMinCost < pArea->GetCostSoFar() )
		return false;

	const_cast<CMapTeleportFilter *>( this )->m_flMinCost = pArea->GetCostSoFar();

	return true;
}
