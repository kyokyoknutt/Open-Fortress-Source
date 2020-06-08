#ifndef FMOD_MANAGER_H
#define FMOD_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "fmod.hpp"

using namespace FMOD;

class CFMODManager
{
public:
	CFMODManager();
	~CFMODManager();
 
	void InitFMOD();
	void ExitFMOD();

	void FadeThink();
	void Think();

	bool IsSoundPlaying( const char* pathToFileFromSoundsFolder );
	bool IsChannelPlaying( int number );

	void PlayAmbientSound( const char* pathToFileFromSoundsFolder, bool fadeIn );
	void PlayLoopingMusic( ChannelGroup *pNewChannelGroup, const char* pLoopingMusic,const char* pIntroMusic = NULL, float flDelay = 0 , bool fadeIn = false);
	void StopAmbientSound( ChannelGroup *pNewChannel, bool fadeOut );
	void PlayMusicEnd( ChannelGroup *pNewChannelGroup, const char* pLoopingMusic, bool bDelay = false, Channel *pLoopingChannel = NULL );
	void StopAllSound( void );
	void TransitionAmbientSounds( const char* pathToFileFromSoundsFolder );
	float GetSoundLength(void);
	unsigned int GetSoundLengthPCM( Sound *sound );

	float m_fDefaultVolume;
private:
	void *GetFullPathToSound( const char* pathToFileFromModFolder, int *pLength );
	const char* GetCurrentSoundName( void );

	const char* currentSound;
	const char* introSound;
	const char* loopingSound;
	const char* newSoundFileToTransitionTo;
	bool m_bShouldTransition;
	bool m_bFadeIn;
	bool m_bFadeOut;
	bool m_bIntro;
	float m_fFadeDelay;
	float m_flSongStart;
	
};

extern CFMODManager* FMODManager();
 
#endif //FMOD_MANAGER_H