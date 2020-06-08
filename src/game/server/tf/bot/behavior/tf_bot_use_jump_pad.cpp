#include "cbase.h"
#include "../tf_bot.h"
#include "tf_gamerules.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_use_jump_pad.h"
#include "of_trigger_jump.h"

ConVar tf_bot_jumppad_search_range( "tf_bot_jumppad_search_range", "512", FCVAR_CHEAT, "How far bots will search to find jump pads around them" );
ConVar tf_bot_debug_jumppad_scavanging( "tf_bot_debug_jumppad_scavanging", "0", FCVAR_CHEAT );


static CHandle<CBaseEntity> s_possibleJumpPad;
static CTFBot *s_possibleBot;
static int s_possibleFrame;


CTFBotUseJumpPad::CTFBotUseJumpPad()
{
	m_PathFollower.Invalidate();
	m_hJumpPad = nullptr;
}

CTFBotUseJumpPad::~CTFBotUseJumpPad()
{
}


const char *CTFBotUseJumpPad::GetName() const
{
	return "UseJumpPad";
}


ActionResult<CTFBot> CTFBotUseJumpPad::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	me->m_bLookingAroundForEnemies = false;
	me->m_bTouchedJumpPad = false;

	if ( ( gpGlobals->framecount != s_possibleFrame || s_possibleBot != me ) && ( !IsPossible( me ) || !s_possibleJumpPad ) )
		return Action<CTFBot>::Done( "Can't move to jump pad" );

	m_hJumpPad = s_possibleJumpPad;

	CTFBotPathCost cost( me, FASTEST_ROUTE );
	if ( !m_PathFollower.Compute( me, m_hJumpPad->GetAbsOrigin(), cost ) )
		return Action<CTFBot>::Done( "No path to jump pad" );

	return Action<CTFBot>::Continue();
}

void CTFBotUseJumpPad::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	me->m_bLookingAroundForEnemies = true;
}

ActionResult<CTFBot> CTFBotUseJumpPad::OnSuspend( CTFBot *me, Action<CTFBot> *newAction )
{
	me->m_bLookingAroundForEnemies = true;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotUseJumpPad::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	me->m_bLookingAroundForEnemies = false;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotUseJumpPad::Update( CTFBot *me, float dt )
{
	if ( me->m_bTouchedJumpPad )
	{
		me->m_bTouchedJumpPad = false;
		return Action<CTFBot>::Done( "I touched the jump pad" );
	}

	if ( !m_hJumpPad )
		return Action<CTFBot>::Done( "Jump pad I was going for is invalid" );

	if ( !m_PathFollower.IsValid() )
		return Action<CTFBot>::Done( "My path became invalid" );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	m_PathFollower.Update( me );

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotUseJumpPad::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotUseJumpPad::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotUseJumpPad::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Failed to reach jump pad" );
}

EventDesiredResult<CTFBot> CTFBotUseJumpPad::OnStuck( CTFBot *me )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Stuck trying to reach jump pad" );
}


QueryResultType CTFBotUseJumpPad::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


bool CTFBotUseJumpPad::IsPossible( CTFBot *actor )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	CUtlVector<EHANDLE> jumppads;

	for ( int i = 0; i < IOFDTriggerJumpAutoList::AutoList().Count(); ++i )
	{
		EHANDLE hndl = static_cast< COFDTriggerJump* >(IOFDTriggerJumpAutoList::AutoList()[ i ] );
		jumppads.AddToTail( hndl );
	}

	CJumpPadFilter filter( actor );
	actor->SelectReachableObjects( jumppads, &jumppads, filter, actor->GetLastKnownArea(), tf_bot_jumppad_search_range.GetFloat() );

	if ( jumppads.IsEmpty() )
	{
		if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
			Warning( "%3.2f: No jump pad nearby.\n", gpGlobals->curtime );

		return false;
	}

	CBaseEntity *pJumpPad = nullptr;
	for( int i=0; i<jumppads.Count(); ++i )
	{
		if ( !jumppads[i] )
			continue;

		if ( jumppads[i]->GetTeamNumber() == GetEnemyTeam( actor ) )
			continue;

		pJumpPad = jumppads[i];

		if ( tf_bot_debug_jumppad_scavanging.GetBool() )
			NDebugOverlay::Cross3D( pJumpPad->GetAbsOrigin(), 5.0f, 255, 100, 154, false, 1.0 );
	}

	if ( pJumpPad )
	{
		CTFBotPathCost func( actor, FASTEST_ROUTE );
		if ( !NavAreaBuildPath( actor->GetLastKnownArea(), NULL, &pJumpPad->GetAbsOrigin(), func ) )
		{
			if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
				Warning( "%3.2f: No path to jump pad!\n", gpGlobals->curtime );
		}

		s_possibleJumpPad = pJumpPad;
		s_possibleBot = actor;
		s_possibleFrame = gpGlobals->framecount;

		return true;
	}

	if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
		Warning( " %3.2f: No jump pad nearby.\n", gpGlobals->curtime );

	return false;
}


CJumpPadFilter::CJumpPadFilter( CTFPlayer *actor )
	: m_pActor( actor )
{
	m_flMinCost = FLT_MAX;
}


bool CJumpPadFilter::IsSelected( const CBaseEntity *ent ) const
{
	/*
	CClosestTFPlayer functor( ent->WorldSpaceCenter() );
	ForEachPlayer( functor );

	// Don't run into enemies while trying to scavenge
	if ( functor.m_pPlayer && !functor.m_pPlayer->InSameTeam( m_pActor ) )
		return false;
	*/

	CTFNavArea *pArea = static_cast<CTFNavArea *>( TheNavMesh->GetNearestNavArea( ent->WorldSpaceCenter() ) );
	if ( !pArea )
		return false;

	// Find minimum cost area we are currently searching
	if ( !pArea->IsMarked() || m_flMinCost < pArea->GetCostSoFar() )
		return false;

	const_cast<CJumpPadFilter *>( this )->m_flMinCost = pArea->GetCostSoFar();

	return true;
}
