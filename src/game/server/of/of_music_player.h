//====== Copyright © 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_MUSIC_PLAYER_H
#define TF_MUSIC_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

enum
{
	OF_MUSIC_OFF = 0,
	OF_MUSIC_INTRO,
	OF_MUSIC_LOOP,
	OF_MUSIC_OUTRO,
};

class CTFMusicPlayer : public CBaseEntity
{
public:
	DECLARE_CLASS(CTFMusicPlayer, CBaseEntity);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
	
	CTFMusicPlayer();
	virtual void MusicThink();
	int UpdateTransmitState()	// always send to all clients
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}	
	
	bool			IsDisabled( void );
	bool			ShouldPlay( void );
	// Input handlers
	void			InputEnable( inputdata_t &inputdata );
	void			InputDisable( inputdata_t &inputdata );
	void			InputToggle( inputdata_t &inputdata );	
	void			InputSetVolume( inputdata_t &inputdata );	
	void			InputAddVolume( inputdata_t &inputdata );	
	void			SetDisabled( bool bDisable );
	void			EndTransition( void );
	int m_iIndex;

	CNetworkVar(string_t, szIntroSong);
	CNetworkVar(string_t, szLoopingSong);
	CNetworkVar(string_t, szOutroSong);
	
	CNetworkVar( bool, m_bShouldBePlaying );
	CNetworkVar( bool, m_bInfection );
	CNetworkVar( bool, m_bHardTransition );
	CNetworkVar( bool, m_bDisabled );
	CNetworkVar( float, m_flDelay );
	CNetworkVar( float, m_flVolume );
	CNetworkString( m_nszLoopingSong , MAX_PATH );
};

class CTFDMMusicManager : public CBaseEntity
{
public:
	DECLARE_CLASS(CTFDMMusicManager, CBaseEntity);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CTFDMMusicManager();
	~CTFDMMusicManager();
	virtual void Spawn();
	virtual void DMMusicThink();
	int UpdateTransmitState()	// always send to all clients
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}	
	// Input handlers
	CNetworkVar( int, m_iIndex );
	bool m_bDisableThink;

	CNetworkHandle( CTFMusicPlayer, pWaitingMusicPlayer );
	CNetworkHandle( CTFMusicPlayer, pRoundMusicPlayer );
	
	CNetworkVar(string_t, szWaitingForPlayerMusic);
	CNetworkVar(string_t, szRoundMusic);
	
	CNetworkVar(string_t, szWaitingMusicPlayer);
	CNetworkVar(string_t, szRoundMusicPlayer);	
};

extern CTFDMMusicManager* DMMusicManager();

#endif // TF_MUSIC_PLAYER_H