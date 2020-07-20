//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "player_command.h"
#include "igamemovement.h"
#include "tf_player.h"
#include "iservervehicle.h"

static CMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;

IPredictionSystem *IPredictionSystem::g_pPredictionSystems = NULL;


//-----------------------------------------------------------------------------
// Sets up the move data for TF2
//-----------------------------------------------------------------------------
class CTFPlayerMove : public CPlayerMove
{
DECLARE_CLASS( CTFPlayerMove, CPlayerMove );

public:
	CTFPlayerMove() :
		m_bWasInVehicle(false),
		m_bVehicleFlipped(false),
		m_bInGodMode(false),
		m_bInNoClip(false)
	{
		m_vecSaveOrigin.Init();
	}

	virtual void	StartCommand( CBasePlayer *player, CUserCmd *cmd );
	virtual void	SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void	FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move );


private:
	Vector m_vecSaveOrigin;
	bool m_bWasInVehicle;
	bool m_bVehicleFlipped;
	bool m_bInGodMode;
	bool m_bInNoClip;
};

// PlayerMove Interface
static CTFPlayerMove g_PlayerMove;

//-----------------------------------------------------------------------------
// Singleton accessor
//-----------------------------------------------------------------------------
CPlayerMove *PlayerMove()
{
	return &g_PlayerMove;
}

//-----------------------------------------------------------------------------
// Main setup, finish
//-----------------------------------------------------------------------------

void CTFPlayerMove::StartCommand( CBasePlayer *player, CUserCmd *cmd )
{
	BaseClass::StartCommand( player, cmd );
}

//-----------------------------------------------------------------------------
// Purpose: This is called pre player movement and copies all the data necessary
//          from the player for movement. (Server-side, the client-side version
//          of this code can be found in prediction.cpp.)
//-----------------------------------------------------------------------------
void CTFPlayerMove::SetupMove( CBasePlayer *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
	CTFPlayer *pTFPlayer = ToTFPlayer( player );
	if ( pTFPlayer )
	{
		// Check to see if we are a crouched, heavy, firing his weapons and zero out movement.
		if ( pTFPlayer->GetPlayerClass()->IsClass( TF_CLASS_HEAVYWEAPONS ) )
		{
			if ( ( pTFPlayer->GetFlags() & FL_DUCKING ) && ( pTFPlayer->m_Shared.InCond( TF_COND_AIMING ) ) && ( pTFPlayer->GetActiveTFWeapon()->GetWeaponID() == TF_WEAPON_MINIGUN || pTFPlayer->GetActiveTFWeapon()->GetWeaponID() == TFC_WEAPON_ASSAULTCANNON ) )
			{
				ucmd->forwardmove = 0.0f;
				ucmd->sidemove = 0.0f;
			}
		}
	}

	BaseClass::SetupMove( player, ucmd, pHelper, move );

	// secobmod
	IServerVehicle *pVehicle = player->GetVehicle();
	if ( pVehicle && gpGlobals->frametime != 0 )
	{
		pVehicle->SetupMove( player, ucmd, pHelper, move );
	}
}


//-----------------------------------------------------------------------------
// Purpose: This is called post player movement to copy back all data that
//          movement could have modified and that is necessary for future
//          movement. (Server-side, the client-side version of this code can 
//          be found in prediction.cpp.)
//-----------------------------------------------------------------------------
void CTFPlayerMove::FinishMove( CBasePlayer *player, CUserCmd *ucmd, CMoveData *move )
{
	// Call the default FinishMove code.
	BaseClass::FinishMove( player, ucmd, move );
	
	// secobmod
	IServerVehicle *pVehicle = player->GetVehicle();
	if ( pVehicle && gpGlobals->frametime != 0 )
	{
		pVehicle->FinishMove( player, ucmd, move );
}
}
