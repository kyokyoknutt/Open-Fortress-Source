#ifndef OF_MUSIC_PLAYER_H
#define OF_MUSIC_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "fmod_manager.h"

#include "saverestore_utlvector.h"

// HUD
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "tf_controls.h"

class C_TFMusicPlayer : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_TFMusicPlayer, C_BaseEntity );
	DECLARE_CLIENTCLASS();
	
	C_TFMusicPlayer();
	~C_TFMusicPlayer();
	virtual void ClientThink(void);
	void HandleVolume(void);
	virtual void Spawn(void);
	virtual void OnDataChanged(DataUpdateType_t updateType);
private:

	int m_iPhase;
	
	bool m_bShouldBePlaying;
	bool m_bHardTransition;
	bool bIsPlaying;
	bool bInLoop;
	
	bool bParsed;
	
	float flLoopTick;
	float m_flDelay;
	float m_flVolume;

	char szLoopingSong[MAX_PATH];
	
	struct songdata_t
	{
		songdata_t()
		{
			name[0] = 0;
			artist[0] = 0;
			path[0] = 0;
			duration = 0;
		}

		char name[ 512 ];
		char artist[ 256 ];
		char path[ 256 ];
		float duration;
		float volume;
	};	
	CUtlVector<songdata_t>	m_Songdata;
	
	ChannelGroup *pChannel;
};

class C_TFDMMusicManager : public C_BaseEntity
{
public:
	DECLARE_CLASS(C_TFDMMusicManager, C_BaseEntity);
	DECLARE_CLIENTCLASS();

	C_TFDMMusicManager();
	~C_TFDMMusicManager();

	// Input handlers
	int m_iIndex;
	CNetworkHandle( C_TFMusicPlayer, pWaitingMusicPlayer );
	CNetworkHandle( C_TFMusicPlayer, pRoundMusicPlayer );
	
	char szWaitingForPlayerMusic[64];
	char szRoundMusic[64];
	
	char szWaitingMusicPlayer[64];
	char szRoundMusicPlayer[64];
};

extern C_TFDMMusicManager* DMMusicManager();

class CTFImagePanel;
class CExLabel;

class CTFHudNowPlaying : public vgui::EditablePanel, public CHudElement
{
private:
	DECLARE_CLASS_SIMPLE(CTFHudNowPlaying, EditablePanel);

public:
	CTFHudNowPlaying(const char *pElementName);

	virtual void FireGameEvent(IGameEvent * event);
	virtual void OnThink();
	virtual bool ShouldDraw(void);
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual int GetRenderGroupPriority() { return 70; }

private:
	float flDrawTime;
	
	EditablePanel 	*m_pNameContainer;
	CTFImagePanel 	*m_pNameBG;
	CExLabel 		*m_pNameLabel;
	
	EditablePanel 	*m_pArtistContainer;
	CTFImagePanel 	*m_pArtistBG;
	CExLabel 		*m_pArtistLabel;
};

#endif //OF_MUSIC_PLAYER_H