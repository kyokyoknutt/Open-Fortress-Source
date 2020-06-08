//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef FUNC_RESPAWNROOM_H
#define FUNC_RESPAWNROOM_H
#ifdef _WIN32
#pragma once
#endif

#include "modelentities.h"
#include "triggers.h"

class CFuncRespawnRoomVisualizer;

//-----------------------------------------------------------------------------
// Purpose: Defines an area considered inside a respawn room
//-----------------------------------------------------------------------------
DECLARE_AUTO_LIST( IFuncRespawnRoomAutoList )
class CFuncRespawnRoom : public CBaseTrigger, public IFuncRespawnRoomAutoList
{
	DECLARE_CLASS( CFuncRespawnRoom, CBaseTrigger );

public:

	CFuncRespawnRoom();

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual void Spawn( void );
	virtual void Activate( void );
	virtual void ChangeTeam( int iTeamNum );

	void	RespawnRoomTouch( CBaseEntity *pOther );

	// Inputs
	void	InputSetActive( inputdata_t &inputdata );
	void	InputSetInactive( inputdata_t &inputdata );
	void	InputToggleActive( inputdata_t &inputdata );
	void	InputRoundActivate( inputdata_t &inputdata );

	void	SetActive( bool bActive );
	bool	GetActive() const;

	bool	PointIsWithin( const Vector &vecPoint );

	void	AddVisualizer( CFuncRespawnRoomVisualizer *pViz );
	
	bool	m_bActive;
	int		m_iOriginalTeam;

	CUtlVector< CHandle<CFuncRespawnRoomVisualizer> >	m_hVisualizers;
};

class CFuncRespawnRoomVisualizer : public CFuncBrush
{
	DECLARE_CLASS( CFuncRespawnRoomVisualizer, CFuncBrush );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	virtual void Spawn( void );
	void	InputRoundActivate( inputdata_t &inputdata );
	int		DrawDebugTextOverlays( void );
	CFuncRespawnRoom *GetRespawnRoom( void ) { return m_hRespawnRoom; }

	virtual int		UpdateTransmitState( void );
	virtual int		ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual bool	ShouldCollide( int collisionGroup, int contentsMask ) const;

	void SetActive( bool bActive );

protected:
	string_t					m_iszRespawnRoomName;
	CHandle<CFuncRespawnRoom>	m_hRespawnRoom;
};


//-----------------------------------------------------------------------------
// Is a given point contained within a respawn room?
//-----------------------------------------------------------------------------
bool PointInRespawnRoom( CBaseEntity *pEntity, const Vector &vecOrigin );

#endif // FUNC_RESPAWNROOM_H
