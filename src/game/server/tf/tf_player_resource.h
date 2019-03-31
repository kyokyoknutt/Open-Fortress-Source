//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: TF's custom CPlayerResource
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_PLAYER_RESOURCE_H
#define TF_PLAYER_RESOURCE_H
#ifdef _WIN32
#pragma once
#endif

class CTFPlayerResource : public CPlayerResource
{
	DECLARE_CLASS( CTFPlayerResource, CPlayerResource );
	
public:
	DECLARE_SERVERCLASS();

	CTFPlayerResource();

	virtual void UpdatePlayerData( void );
	virtual void Spawn( void );

	int	GetTotalScore( int iIndex );
	Color GetPlayerColor( int iIndex );

protected:
	CNetworkArray( int,	m_iTotalScore, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iMaxHealth, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iPlayerClass, MAX_PLAYERS+1 );
	CNetworkArray( Vector, m_vecColors, MAX_PLAYERS + 1 );
};

inline CTFPlayerResource *GetTFPlayerResource( void )
{
	if (!g_pPlayerResource)
		return NULL;
	return assert_cast<CTFPlayerResource *> ( g_pPlayerResource );
	
}

#endif // TF_PLAYER_RESOURCE_H
