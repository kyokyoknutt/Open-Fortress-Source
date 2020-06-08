#include "cbase.h"
#include "../tf_bot.h"
#include "tf_gamerules.h"
#include "nav_mesh/tf_nav_mesh.h"
#include "tf_bot_get_weapon.h"
#include "entity_weapon_spawner.h"

ConVar tf_bot_weapon_search_range( "tf_bot_weapon_search_range", "1500", FCVAR_CHEAT, "How far bots will search to find weapon around them" );
ConVar tf_bot_debug_weapon_scavanging( "tf_bot_debug_weapon_scavanging", "0", FCVAR_CHEAT );


static CHandle<CBaseEntity> s_possibleWeapon;
static CTFBot *s_possibleBot;
static int s_possibleFrame;


CTFBotGetWeapon::CTFBotGetWeapon()
{
	m_PathFollower.Invalidate();
	m_hWeapon = nullptr;
}

CTFBotGetWeapon::~CTFBotGetWeapon()
{
}


const char *CTFBotGetWeapon::GetName() const
{
	return "GetWeapon";
}


ActionResult<CTFBot> CTFBotGetWeapon::OnStart( CTFBot *me, Action<CTFBot> *priorAction )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	m_PathFollower.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	me->m_bLookingAroundForEnemies = false;
	me->m_bPickedUpWeapon = false;

	if ( ( gpGlobals->framecount != s_possibleFrame || s_possibleBot != me ) && ( !IsPossible( me ) || !s_possibleWeapon ) )
		return Action<CTFBot>::Done( "Can't get weapon" );

	m_hWeapon = s_possibleWeapon;

	CTFBotPathCost cost( me, FASTEST_ROUTE );
	if ( !m_PathFollower.Compute( me, m_hWeapon->WorldSpaceCenter(), cost ) )
		return Action<CTFBot>::Done( "No path to weapon" );

	return Action<CTFBot>::Continue();
}

void CTFBotGetWeapon::OnEnd( CTFBot *me, Action<CTFBot> *newAction )
{
	me->m_bLookingAroundForEnemies = true;
}

ActionResult<CTFBot> CTFBotGetWeapon::OnSuspend( CTFBot *me, Action<CTFBot> *newAction )
{
	me->m_bLookingAroundForEnemies = true;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotGetWeapon::OnResume( CTFBot *me, Action<CTFBot> *priorAction )
{
	me->m_bLookingAroundForEnemies = false;

	return Action<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotGetWeapon::Update( CTFBot *me, float dt )
{
	if ( me->m_bPickedUpWeapon )
	{
		me->m_bPickedUpWeapon = false;
		return Action<CTFBot>::Done( "I picked up a weapon" );
	}

	if ( !m_hWeapon )
		return Action<CTFBot>::Done( "Weapon I was going for has been taken" );

	if ( !m_PathFollower.IsValid() )
		return Action<CTFBot>::Done( "My path became invalid" );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	me->EquipBestWeaponForThreat( threat );

	m_PathFollower.Update( me );

	return Action<CTFBot>::Continue();
}


EventDesiredResult<CTFBot> CTFBotGetWeapon::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotGetWeapon::OnMoveToSuccess( CTFBot *me, const Path *path )
{
	return Action<CTFBot>::TryContinue();
}

EventDesiredResult<CTFBot> CTFBotGetWeapon::OnMoveToFailure( CTFBot *me, const Path *path, MoveToFailureType fail )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Failed to reach weapon" );
}

EventDesiredResult<CTFBot> CTFBotGetWeapon::OnStuck( CTFBot *me )
{
	return Action<CTFBot>::TryDone( RESULT_CRITICAL, "Stuck trying to reach weapon" );
}


QueryResultType CTFBotGetWeapon::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


bool CTFBotGetWeapon::IsPossible( CTFBot *actor )
{
	VPROF_BUDGET( __FUNCTION__, "NextBot" );

	CUtlVector<EHANDLE> weapons;
	for ( int i = 0; i < IWeaponSpawnerAutoList::AutoList().Count(); ++i )
	{
		EHANDLE hndl = static_cast< CWeaponSpawner* >( IWeaponSpawnerAutoList::AutoList()[ i ] );
		weapons.AddToTail( hndl );
	}

	CWeaponFilter filter( actor );
	actor->SelectReachableObjects( weapons, &weapons, filter, actor->GetLastKnownArea(), tf_bot_weapon_search_range.GetFloat() );

	if ( weapons.IsEmpty() )
	{
		if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
			Warning( "%3.2f: No weapon nearby.\n", gpGlobals->curtime );

		return false;
	}

	CBaseEntity *pWeapon = nullptr;
	for( int i=0; i<weapons.Count(); ++i )
	{
		if ( !weapons[i] )
			continue;

		pWeapon = weapons[i];

		if ( tf_bot_debug_weapon_scavanging.GetBool() )
			NDebugOverlay::Cross3D( pWeapon->WorldSpaceCenter(), 5.0f, 255, 100, 154, false, 1.0 );
	}

	if ( pWeapon )
	{
		CTFBotPathCost func( actor, FASTEST_ROUTE );
		if ( !NavAreaBuildPath( actor->GetLastKnownArea(), NULL, &pWeapon->WorldSpaceCenter(), func ) )
		{
			if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
				Warning( "%3.2f: No path to weapon!\n", gpGlobals->curtime );
		}

		s_possibleWeapon = pWeapon;
		s_possibleBot = actor;
		s_possibleFrame = gpGlobals->framecount;

		return true;
	}

	if ( actor->IsDebugging( NEXTBOT_BEHAVIOR ) )
		Warning( " %3.2f: No weapon nearby.\n", gpGlobals->curtime );

	return false;
}


CWeaponFilter::CWeaponFilter( CTFPlayer *actor )
	: m_pActor( actor )
{
	m_flMinCost = FLT_MAX;
}


bool CWeaponFilter::IsSelected( const CBaseEntity *ent ) const
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

	CWeaponSpawner *pSpawner = dynamic_cast< CWeaponSpawner *>( const_cast<CBaseEntity *>( ent ) );

	if ( pSpawner )
	{
		if ( pSpawner->m_bRespawning ) // don't go for spawners that are respawning
			return false;

		if ( pSpawner->m_bDisabled ) // don't go for spawners that are disabled
			return false;

		int iWeaponID = AliasToWeaponID( pSpawner->m_iszWeaponName.Get() );

		if ( m_pActor->Weapon_OwnsThisID( iWeaponID ) ) // don't go for spawners that we already have a weapon from
			return false;
	}

	const_cast<CWeaponFilter *>( this )->m_flMinCost = pArea->GetCostSoFar();

	return true;
}
