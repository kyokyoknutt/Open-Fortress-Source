//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base VoteController.  Handles holding and voting on issues.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "tf_voteissues.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// default is 0 as people can't behave themselves
ConVar sv_vote_issue_restart_game_allowed( "sv_vote_issue_restart_game_allowed", "0", FCVAR_NONE, "Can players call votes to restart the game?" );
ConVar sv_vote_issue_restart_game_cooldown( "sv_vote_issue_restart_game_cooldown", "300", FCVAR_NONE, "Minimum time before another restart vote can occur (in seconds)." );
ConVar sv_vote_kick_ban_duration( "sv_vote_kick_ban_duration", "20", FCVAR_NONE, "The number of minutes a vote ban should last. (0 = Disabled)" );
ConVar sv_vote_kick_maxplayers_required( "sv_vote_kick_maxplayers_required", "6", FCVAR_NONE, "Amount of players that must be present in a server before the kick vote can be called." );
// default is 0 as people can't behave themselves
ConVar sv_vote_issue_changelevel_allowed( "sv_vote_issue_changelevel_allowed", "0", FCVAR_NONE, "Can players call votes to change levels?" );
ConVar sv_vote_issue_changemutator_allowed( "sv_vote_issue_changemutator_allowed", "0", FCVAR_NONE, "Can players call votes to change the mutators?" );
ConVar sv_vote_issue_nextlevel_allowed( "sv_vote_issue_nextlevel_allowed", "1", FCVAR_NONE, "Can players call votes to set the next level?" );
ConVar sv_vote_issue_nextlevel_choicesmode( "sv_vote_issue_nextlevel_choicesmode", "1", FCVAR_NONE, "Present players with a list of lowest playtime maps to choose from?" );
// TODO:
//ConVar sv_vote_issue_nextlevel_allowextend( "sv_vote_issue_nextlevel_allowextend", "1", FCVAR_NONE, "Allow players to extend the current map?" );
ConVar sv_vote_issue_nextlevel_prevent_change( "sv_vote_issue_nextlevel_prevent_change", "1", FCVAR_NONE, "Not allowed to vote for a nextlevel if one has already been set." );
// TODO:
//ConVar sv_vote_issue_nextlevel_allowextend( "sv_vote_issue_nextlevel_allowextend", "1", FCVAR_NONE, "Allow players to extend the current map?" );
//ConVar sv_vote_issue_extendlevel_quorum( "sv_vote_issue_extendlevel_quorum", "60", FCVAR_NONE, "What is the ratio of voters needed to reach quorum?" );
ConVar sv_vote_issue_scramble_teams_allowed( "sv_vote_issue_scramble_teams_allowed", "0", FCVAR_NONE, "Can players call votes to scramble the teams?" );
ConVar sv_vote_issue_scramble_teams_cooldown( "sv_vote_issue_scramble_teams_cooldown", "1200", FCVAR_NONE, "Minimum time before another scramble vote can occur (in seconds)." );
// TODO:
//ConVar sv_vote_issue_autobalance_allowed( "sv_vote_issue_autobalance_allowed", "0", FCVAR_NONE, "Can players call votes to enable or disable auto team balance?" );
//ConVar sv_vote_issue_autobalance_cooldown( "sv_vote_issue_autobalance_cooldown", "300", FCVAR_NONE, "Minimum time before another auto team balance vote can occur (in seconds)." );
//ConVar sv_vote_issue_classlimits_allowed( "sv_vote_issue_classlimits_allowed", "0", FCVAR_NONE, "Can players call votes to enable or disable per-class limits?" );
// this convar is clamped to min 1
//ConVar sv_vote_issue_classlimits_max( "sv_vote_issue_classlimits_max", "0", FCVAR_NONE, "Maximum number of players (per-team) that can be any one class." );
//ConVar sv_vote_issue_classlimits_cooldown( "sv_vote_issue_classlimits_cooldown", "0", FCVAR_NONE, "Minimum time before another classlimits vote can occur (in seconds)." );

extern ConVar fraglimit;
extern ConVar sv_vote_quorum_ratio;

// 
// We parse all maps we have in our mapcycle, and we also track how much they were played, so the nextlevel votes will be more fair to the less played maps
//

struct MapAndPlaytime_t
{
	char mapname[MAX_MAP_NAME];
	int seconds;
};

class CPlayedMapsSystem : public CAutoGameSystem
{
public:
	CPlayedMapsSystem( char const *name ) : CAutoGameSystem( name )
	{
	} 

	~CPlayedMapsSystem()
	{
	}

	virtual void LevelInitPreEntity();

	// store the map name and then the seconds of playtime
	CUtlVector < MapAndPlaytime_t > m_MapsAndPlaytimes;

	// the maps that will be used in the vote menu, required so pointers don't get lost
	CUtlVector< MapAndPlaytime_t > MapsAndPlaytimes;
};

CPlayedMapsSystem s_PlayedMaps( "PlayedMaps" );
CPlayedMapsSystem *PlayedMaps( void )
{
	return &s_PlayedMaps;
}

// Get rid of the \n at the end (hud_basechat)
void StripEndNewlineFromString( char *str )
{
	int s = strlen( str ) - 1;
	if ( s >= 0 )
	{
		if ( str[s] == '\n' || str[s] == '\r' )
			str[s] = 0;
	}
}

void CPlayedMapsSystem::LevelInitPreEntity()
{
	// check if we already inited this!
	if ( !m_MapsAndPlaytimes.Count() )
	{
		// grab server's mapcycle
		int stringindex = g_pStringTableServerMapCycle->FindStringIndex( "ServerMapCycle" );

		if ( stringindex != INVALID_STRING_INDEX )
		{
			int length = -1;

			// cast to const char because GetStringUserData has a void return type
			// hopefully there aren't consequences to a ugly cast like this
			const char *pMapCycle = ( const char * )g_pStringTableServerMapCycle->GetStringUserData( stringindex, &length );

			if ( pMapCycle && length > 0 )
			{
				CUtlVector < char * > Maps;

				// the string tables are stored in this fashion:
				// ctf_2fort \n cp_dustbowl \n tc_hydro \n cp_granary \n ...
				// go through the entire string and split it by the \n, the split strings are added into the above vMaps vector
				// note: the string itself will still contain the \n, so it gets stripped afterwards

				V_SplitString( pMapCycle, "\n", Maps );

				m_MapsAndPlaytimes.EnsureCapacity( Maps.Count() );

				for ( int i = 0; i < Maps.Count(); i++ )
				{
					// strip the \n at the end
					StripEndNewlineFromString( Maps[i] );
				
					char szMapName[MAX_MAP_NAME];
					V_strncpy( szMapName, Maps[i], sizeof( szMapName ) );

					// finally, check if the map exists and add it to our system's vector
					IVEngineServer::eFindMapResult eResult = engine->FindMap( szMapName, sizeof( szMapName ) );
					// randomize the "playtime" component so that on server boot,
					// we don't always have the same map to vote for
					// be sure this number isn't too large since there is some gamemodes/mutators that are quicker
					if ( eResult != IVEngineServer::eFindMap_NotFound )
					{
						MapAndPlaytime_t MapAndPlaytime;
						V_strncpy( MapAndPlaytime.mapname, szMapName, sizeof( MapAndPlaytime.mapname ) );
						MapAndPlaytime.seconds = RandomInt( 0, 50 );

						m_MapsAndPlaytimes.AddToTail( MapAndPlaytime );
					}
				}

				// clean up
				Maps.PurgeAndDeleteElements();
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseTFIssue::ExecuteCommand()
{
	// nothing
}

void CBaseTFIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	// stub from vtable
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Ban player
//-----------------------------------------------------------------------------
const char *CKickIssue::GetDisplayString()
{
	// TODO: shouldn't there be something else for bots?
	return "#TF_vote_kick_player";
}

const char *CKickIssue::GetDetailsString( void )
{
	const char* pszDetails = m_szDetailsString;

	CBasePlayer *pPlayer = UTIL_PlayerByUserId( atoi( pszDetails ) );

	if ( !pPlayer )
		return m_szDetailsString;

	// bots need their own treatment
	if ( ( pPlayer->GetFlags() & FL_FAKECLIENT ) )
		pszDetails = UTIL_VarArgs( "%s (BOT)", pPlayer->GetPlayerName() );
	else
		pszDetails = UTIL_VarArgs( "%s", pPlayer->GetPlayerName() );

	return pszDetails;
}

const char *CKickIssue::GetVotePassedString()
{
	// TODO: shouldn't there be something else for bots?
	return "#TF_vote_passed_kick_player";
}

bool CKickIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	// string -> int
	int m_iPlayerID = atoi( pszDetails );

	CBasePlayer *pPlayerUser = UTIL_PlayerByUserId( m_iPlayerID );

	if ( !pPlayerUser )
	{
		nFailCode = VOTE_FAILED_PLAYERNOTFOUND;
		nTime = m_flNextCallTime - gpGlobals->curtime;
		return false;
	}

	if ( sv_vote_kick_maxplayers_required.GetInt() > 0 )
	{
		int playercount = 0;

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );				
			if ( pPlayer )
				playercount++;
		}

		if ( playercount < sv_vote_kick_maxplayers_required.GetInt() )
		{
			nFailCode = VOTE_FAILED_NOT_ENOUGH_PLAYERS;
			nTime = m_flNextCallTime - gpGlobals->curtime;
			return false;
		}
	}

	if ( engine->IsDedicatedServer() )
	{
		if ( pPlayerUser->IsAutoKickDisabled() == true && !( pPlayerUser->GetFlags() & FL_FAKECLIENT ) )
		{
			nFailCode = VOTE_FAILED_CANNOT_KICK_ADMIN;
			nTime = m_flNextCallTime - gpGlobals->curtime;
			return false;
		}
	}
	else if ( gpGlobals->maxClients > 1 )
	{
		CBasePlayer *pHostPlayer = UTIL_GetListenServerHost();

		if ( pPlayerUser == pHostPlayer )
		{
			nFailCode = VOTE_FAILED_CANNOT_KICK_ADMIN;
			nTime = m_flNextCallTime - gpGlobals->curtime;
			return false;
		}
	}

	return CBaseTFIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime );
}

void CKickIssue::ExecuteCommand( void )
{
	const char* pszDetails = m_szDetailsString;

	CBasePlayer *pPlayer = UTIL_PlayerByUserId( atoi( pszDetails ) );

	if ( !pPlayer )
		return;

	if ( ( pPlayer->GetFlags() & FL_FAKECLIENT ) )
	{
		engine->ServerCommand( UTIL_VarArgs( "kickid %d;", pPlayer->GetUserID() ) );
	}
	else
	{
		engine->ServerCommand( UTIL_VarArgs( "banid %i %s kick\n", sv_vote_kick_ban_duration.GetInt(), m_szNetworkIDString.String() ) );

		engine->ServerCommand( "writeip\n" );
		engine->ServerCommand( "writeid\n" );
	}
}

void CKickIssue::SetIssueDetails( const char *pszDetails )
{
	CBaseTFIssue::SetIssueDetails( pszDetails );

	int iID = atoi( pszDetails );

	CBasePlayer* pPlayer = UTIL_PlayerByUserId( iID );

	if ( pPlayer )
		m_szNetworkIDString = pPlayer->GetNetworkIDString();
}

bool CKickIssue::IsEnabled()
{
	if ( sv_vote_kick_ban_duration.GetInt() <= 0 )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRestartGameIssue::Init()
{
	SetIssueCooldownDuration( sv_vote_issue_restart_game_cooldown.GetFloat() );
}

const char *CRestartGameIssue::GetDisplayString()
{
	return "#TF_vote_restart_game";
}

const char *CRestartGameIssue::GetVotePassedString()
{
	return "#TF_vote_passed_restart_game";
}

void CRestartGameIssue::ExecuteCommand()
{
	CBaseTFIssue::ExecuteCommand();

	engine->ServerCommand( "mp_restartgame 3\n" );
}

bool CRestartGameIssue::IsEnabled()
{
	if ( sv_vote_issue_restart_game_allowed.GetBool() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CScrambleTeams::Init()
{
	SetIssueCooldownDuration( sv_vote_issue_scramble_teams_cooldown.GetFloat() );
}

const char *CScrambleTeams::GetDisplayString()
{
	return "#TF_vote_scramble_teams";
}

const char *CScrambleTeams::GetVotePassedString()
{
	return "#TF_vote_passed_scramble_teams";
}

void CScrambleTeams::ExecuteCommand()
{
	CBaseTFIssue::ExecuteCommand();

	engine->ServerCommand( "mp_scrambleteams 1\n" );
}

bool CScrambleTeams::IsEnabled()
{
	if ( TFGameRules() && TFGameRules()->IsFreeRoam() )
		return false;

	if ( sv_vote_issue_scramble_teams_allowed.GetBool() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CChangeLevelIssue::GetDisplayString()
{
	return "#TF_vote_changelevel";
}

const char *CChangeLevelIssue::GetVotePassedString()
{
	return "#TF_vote_passed_changelevel";
}

void CChangeLevelIssue::ExecuteCommand()
{
	CBaseTFIssue::ExecuteCommand();

	engine->ServerCommand( UTIL_VarArgs( "changelevel %s\n", m_szDetailsString ) );
}

bool CChangeLevelIssue::IsEnabled()
{
	if ( sv_vote_issue_changelevel_allowed.GetBool() )
		return true;

	return false;
}

bool CChangeLevelIssue::CanCallVote( int iEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	// don't allow changing maps when we are halfway through fraglimit
	if ( TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() && !TFGameRules()->IsInWaitingForPlayers() )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = ToBasePlayer( UTIL_PlayerByIndex( i ) );

			if ( pPlayer && pPlayer->FragCount() >= ( (float)fraglimit.GetInt() * 0.5 ) )
			{
				nFailCode = VOTE_FAILED_ISSUE_DISABLED;
				nTime = m_flNextCallTime - gpGlobals->curtime;
				return false;
			}
		}
	}

	if ( pszDetails[ 0 ] == '\0' )
	{
		nFailCode = VOTE_FAILED_MAP_NAME_REQUIRED;
		nTime = m_flNextCallTime - gpGlobals->curtime;
		return false;
	}

	if ( MultiplayRules()->IsMapInMapCycle( pszDetails ) == false )
	{
		nFailCode = VOTE_FAILED_MAP_NOT_VALID;
		nTime = m_flNextCallTime - gpGlobals->curtime;
		return false;
	}

	return CBaseTFIssue::CanCallVote( iEntIndex, pszDetails, nFailCode, nTime );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CChangeMutatorIssue::GetDisplayString()
{
	return "#TF_vote_changemutator";
}

const char *CChangeMutatorIssue::GetDetailsString( void )
{
	int iMutator = ( atoi( m_szDetailsString ) );

	switch ( iMutator )
	{
		case NO_MUTATOR:
			return "#TF_vote_mutator_disabled";
			break;
		case INSTAGIB:
			return "#TF_vote_mutator_instagibrailguncrowbar";
			break;
		case INSTAGIB_NO_MELEE:
			return "#TF_vote_mutator_instagibrailgun";
			break;
		case CLAN_ARENA:
			return "#TF_vote_mutator_clanarena";
			break;
		case UNHOLY_TRINITY:
			return "#TF_vote_mutator_unholytrinity";
			break;
		case ROCKET_ARENA:
			return "#TF_vote_mutator_rocketarena";
			break;
		case GUN_GAME:
			return "#TF_vote_mutator_gungame";
			break;
	}

	return m_szDetailsString;
}

const char *CChangeMutatorIssue::GetVotePassedString()
{
	return "#TF_vote_passed_changemutator";
}

void CChangeMutatorIssue::ExecuteCommand()
{
	CBaseTFIssue::ExecuteCommand();

	engine->ServerCommand( UTIL_VarArgs( "of_mutator %s\n", m_szDetailsString ) );
}

bool CChangeMutatorIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	int iMutator = ( atoi( m_szDetailsString ) );

	if ( iMutator < NO_MUTATOR || iMutator > GUN_GAME )
	{
		nFailCode = VOTE_FAILED_MUTATOR_INVALID;
		nTime = m_flNextCallTime - gpGlobals->curtime;
		return false;
	}

	return CBaseTFIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime );
}

bool CChangeMutatorIssue::IsEnabled()
{
	if ( sv_vote_issue_changemutator_allowed.GetBool() )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CNextLevelIssue::GetDisplayString()
{
	// dedicated server vote
	if ( m_szDetailsString[ 0 ] == '\0' && sv_vote_issue_nextlevel_choicesmode.GetBool() )
		return "#TF_vote_nextlevel_choices";

	return "#TF_vote_nextlevel";
}

const char *CNextLevelIssue::GetVotePassedString()
{
	return "#TF_vote_passed_nextlevel";	
}

bool CNextLevelIssue::IsYesNoVote( void )
{
	if ( m_szDetailsString[0] != '\0' )
		return true;

	return false;
}

bool CNextLevelIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	if ( nEntIndex == DEDICATED_SERVER && sv_vote_issue_nextlevel_choicesmode.GetBool() )
	{
		if ( pszDetails[ 0 ] == '\0' )
		{
			return true;
		}

		return false;
	}

	if ( pszDetails[ 0 ] == '\0' )
	{
		nFailCode = VOTE_FAILED_MAP_NAME_REQUIRED;
		nTime = m_flNextCallTime - gpGlobals->curtime;
		return false;
	}

	char szMapName[MAX_MAP_NAME];
	V_strncpy( szMapName, pszDetails, sizeof( szMapName ) );

	IVEngineServer::eFindMapResult eFindMapResult = engine->FindMap( szMapName, sizeof( szMapName ) );
	if ( eFindMapResult == IVEngineServer::eFindMap_NotFound )
	{
		nFailCode = VOTE_FAILED_MAP_NOT_FOUND;
		nTime = m_flNextCallTime - gpGlobals->curtime;
		return false;
	}

	if ( sv_vote_issue_nextlevel_prevent_change.GetBool() )
	{
		if ( V_strcmp( nextlevel.GetString(), "" ) )
		{
			nFailCode = VOTE_FAILED_NEXTLEVEL_SET;
			return false;
		}
	}

	if ( MultiplayRules()->IsMapInMapCycle( pszDetails ) == false )
	{
		nFailCode = VOTE_FAILED_MAP_NOT_VALID;
		nTime = m_flNextCallTime - gpGlobals->curtime;
		return false;
	}

	return CBaseTFIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime );
}

// From Wikipedia, the free encyclopedia
/* Comparison function. Receives two generic (void) pointers to the items under comparison. */
int __cdecl SortMapsAndTimes( const void *p, const void *q )
{
	int x = static_cast< const MapAndPlaytime_t * >( p )->seconds;
	int y = static_cast< const MapAndPlaytime_t * >( q )->seconds;

    /* Avoid return x - y, which can cause undefined behaviour
       because of signed integer overflow. */
    if ( x < y )
        return -1;  // Return -1 if you want ascending, 1 if you want descending order. 
    else if ( x > y )
        return 1;   // Return 1 if you want ascending, -1 if you want descending order. 

    return 0;
}

bool CNextLevelIssue::GetVoteOptions( CUtlVector <const char*> &vecNames )
{	
	if ( m_szDetailsString[0] != '\0' )
	{
		// The default vote issue is a Yes/No vote
		vecNames.AddToHead( "Yes" );
		vecNames.AddToTail( "No" );

		return true;
	}
	else // dedicated server vote
	{	
		// TODO: how should an extend map vote be handled here?

		// bail if this somehow happens
		if ( !PlayedMaps()->m_MapsAndPlaytimes.Count() )
			return false;

		// allocate memory
		vecNames.EnsureCapacity( 5 );

		// iterate through our map and put it into the above vector of mapandplaytime_t
		// this is done as we can't modify the system's vector because it will break consecutive map changes
		// this also allows for removal of any specifics, such as the current map

		FOR_EACH_VEC( PlayedMaps()->m_MapsAndPlaytimes, i )
		{
			if ( PlayedMaps()->m_MapsAndPlaytimes.IsValidIndex( i ) )
			{
				char mapname[64];
				V_strncpy( mapname, PlayedMaps()->m_MapsAndPlaytimes[i].mapname, sizeof( mapname ) );
				int mapseconds = PlayedMaps()->m_MapsAndPlaytimes[i].seconds;

				// skip current map
				if ( V_strcmp( mapname, gpGlobals->mapname.ToCStr() ) == 0 )
				{
					// update how long we played the current map for though
					float roundtime = ceil( gpGlobals->curtime - TFGameRules()->m_flMapStartTime );
					int seconds = int(roundtime);
					PlayedMaps()->m_MapsAndPlaytimes[i].seconds += seconds;
				}
				else
				{
					MapAndPlaytime_t MapAndPlaytime;

					V_strncpy( MapAndPlaytime.mapname, mapname, sizeof( MapAndPlaytime.mapname ) );
					MapAndPlaytime.seconds = mapseconds;

					PlayedMaps()->MapsAndPlaytimes.AddToTail( MapAndPlaytime );
				}
			}
		}

		// sort them for the final output
		qsort( PlayedMaps()->MapsAndPlaytimes.Base(), PlayedMaps()->MapsAndPlaytimes.Count(), sizeof( MapAndPlaytime_t ), SortMapsAndTimes );

		// and finally put them into the vote menu
		FOR_EACH_VEC(PlayedMaps()->MapsAndPlaytimes, i )
		{
			// skip the first one as its corrupt. WTF?
			if ( !PlayedMaps()->MapsAndPlaytimes.IsValidIndex( i + 1 ) )
				continue;

			if ( vecNames.Count() >= 5 )
				break;

			vecNames.AddToTail( PlayedMaps()->MapsAndPlaytimes[i + 1].mapname );
		}
	}

	return true;
}

void CNextLevelIssue::ExecuteCommand()
{
	CBaseTFIssue::ExecuteCommand();

	// extern ConVar nextlevel;
	ConVarRef nextlevel( "nextlevel" );

	nextlevel.SetValue( m_szDetailsString );

	// clean up
	PlayedMaps()->MapsAndPlaytimes.RemoveAll();
	PlayedMaps()->MapsAndPlaytimes.Purge();
}

bool CNextLevelIssue::IsEnabled()
{
	if ( sv_vote_issue_nextlevel_allowed.GetBool() )
		return true;

	return false;
}

float CNextLevelIssue::GetQuorumRatio( void )
{
	if ( sv_vote_issue_nextlevel_choicesmode.GetBool() )
		return 0.2f;

	return sv_vote_quorum_ratio.GetFloat();
}