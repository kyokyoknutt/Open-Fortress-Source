#include "cbase.h"
#include "fmod_manager.h"
#include "filesystem.h"

using namespace FMOD;

System			*pSystem;
Sound			*pSound;
SoundGroup		*pSoundGroup;
Channel			*pChannel;
ChannelGroup	*pChannelGroup;
FMOD_RESULT		result;

CFMODManager gFMODMng;
CFMODManager* FMODManager()
{
	return &gFMODMng;
}

CFMODManager::CFMODManager()
{
	m_fFadeDelay = 0.0;
	m_fDefaultVolume = 1.0f;
	m_flSongStart = 0.0f;
	newSoundFileToTransitionTo = "NULL";
	currentSound = "NULL";
	m_bShouldTransition = false;
	m_bFadeIn = false;
	m_bFadeOut = false;
}

CFMODManager::~CFMODManager()
{
	m_fFadeDelay = 0.0;
	m_fDefaultVolume = 0.0f;
	m_flSongStart = 0.0f;
	newSoundFileToTransitionTo = "NULL";
	currentSound = "NULL";
	m_bShouldTransition = false;
	m_bFadeIn = false;
	m_bFadeOut = false;
}

// Starts FMOD
void CFMODManager::InitFMOD(void)
{
	result = System_Create(&pSystem); // Create the main system object.

	if (result != FMOD_OK)
		Warning("FMOD ERROR: System creation failed!\n");
	else
		DevMsg("FMOD system successfully created.\n");

	result = pSystem->init(100, FMOD_INIT_NORMAL, 0);   // Initialize FMOD system.

	if (result != FMOD_OK)
		Warning("FMOD ERROR: Failed to initialize properly!\n");
	else
		DevMsg("FMOD initialized successfully.\n");
}

// Stops FMOD
void CFMODManager::ExitFMOD(void)
{
	result = pSystem->release();

	if (result != FMOD_OK)
		Warning("FMOD ERROR: System did not terminate properly!\n");
	else
		DevMsg("FMOD system terminated successfully.\n");
}

// Returns the full path of a specified sound file in the /sounds folder
void *CFMODManager::GetFullPathToSound(const char* pathToFileFromModFolder, int *pLength )
{
/*
	char* resultpath = new char[512];
*/
	char fullpath[512];
	Q_snprintf(fullpath, sizeof(fullpath), "sound/%s", pathToFileFromModFolder);
/*	filesystem->GetLocalPath(fullpath, fullpath, sizeof(fullpath));

	Q_snprintf(resultpath, 512, "%s", fullpath);
	// convert backwards slashes to forward slashes
	for (int i = 0; i < 512; i++)
	{
		if (resultpath[i] == '\\')
			resultpath[i] = '/';
	}

*/	
	void *buffer = NULL;

	int length = filesystem->ReadFileEx( fullpath, "GAME", &buffer, true, true );

	if ( pLength )
	{
		*pLength = length;
	}

	return buffer;
}

// Returns the name of the current ambient sound being played
// If there is an error getting the name of the ambient sound or if no ambient sound is currently being played, returns "NULL"
const char* CFMODManager::GetCurrentSoundName(void)
{
	return currentSound;
}

// Handles all fade-related sound stuffs
// Called every frame when the client is in-game
void CFMODManager::FadeThink(void)
{
	if (m_bFadeOut)
	{
		if (gpGlobals->curtime >= m_fFadeDelay)
		{
			float tempvol;
			pChannel->getVolume(&tempvol);

			if (tempvol > 0.0)
			{
				pChannel->setVolume(tempvol - 0.05);
				m_fDefaultVolume = tempvol - 0.05f;
				m_fFadeDelay = gpGlobals->curtime + 0.1;
			}
			else
			{
				pChannel->setVolume(0.0);
				m_fDefaultVolume = 0.0f;
				m_bFadeOut = false;
				m_fFadeDelay = 0.0;
			}
		}
	}
	else if (m_bShouldTransition)
	{
		int iLength = 0;
		void *buffer = GetFullPathToSound(newSoundFileToTransitionTo, &iLength);
		
		FMOD_CREATESOUNDEXINFO info;
		memset(&info, 0, sizeof(info));
		info.length = iLength;
		info.cbsize = sizeof(info);
		
		result = pSystem->createStream((const char*)buffer, FMOD_OPENMEMORY , &info, &pSound);

		if (result != FMOD_OK)
		{
			Warning("FMOD: Failed to create stream of sound '%s' ! (ERROR NUMBER: %i)\n", newSoundFileToTransitionTo, result);
			newSoundFileToTransitionTo = "NULL";
			m_bShouldTransition = false;
			return;
		}

		result = pSystem->playSound(pSound, pChannelGroup, false, &pChannel);

		if (result != FMOD_OK)
		{
			Warning("FMOD: Failed to play sound '%s' ! (ERROR NUMBER: %i)\n", newSoundFileToTransitionTo, result);
			newSoundFileToTransitionTo = "NULL";
			m_bShouldTransition = false;
			return;
		}

		currentSound = newSoundFileToTransitionTo;
		newSoundFileToTransitionTo = "NULL";
		m_bShouldTransition = false;
	}
	else if (m_bFadeIn)
	{
		if (gpGlobals->curtime >= m_fFadeDelay)
		{
			float tempvol;
			pChannel->getVolume(&tempvol);

			if (tempvol < 1.0)
			{
				pChannel->setVolume(tempvol + 0.05);
				m_fDefaultVolume = tempvol + 0.05f;
				m_fFadeDelay = gpGlobals->curtime + 0.1;
			}
			else
			{
				pChannel->setVolume(1.0);
				m_fDefaultVolume = 1.0f;
				m_bFadeIn = false;
				m_fFadeDelay = 0.0;
			}
		}
	}
}

// Called every frame when the client is in-game
void CFMODManager::Think(void)
{
}

// Compares specified ambient sound with the current ambient sound being played
// Returns true if they match, false if they do not or if no sound is being played
bool CFMODManager::IsSoundPlaying(const char* pathToFileFromSoundsFolder)
{
	const char* currentSoundPlaying = GetCurrentSoundName();

	return (strcmp(currentSoundPlaying, pathToFileFromSoundsFolder) == 0 &&
		m_flSongStart + GetSoundLength() < gpGlobals->realtime);
}

ConVar test_sample("test_sample", "1", FCVAR_ARCHIVE);

// Compares specified ambient sound with the current ambient sound being played
// Returns true if they match, false if they do not or if no sound is being played
bool CFMODManager::IsChannelPlaying(int number)
{
	unsigned int templength = 0;
	unsigned int position = 0;
	pSound->getLength(&templength, FMOD_TIMEUNIT_RAWBYTES);
	pChannel->getPosition(&position, FMOD_TIMEUNIT_RAWBYTES);
	return position < (templength - test_sample.GetInt());
}

// Compares specified ambient sound with the current ambient sound being played
// Returns true if they match, false if they do not or if no sound is being played
float CFMODManager::GetSoundLength(void)
{
	if (!pSound)
		return 0;

	unsigned int templength = 0;
	pSound->getLength(&templength, FMOD_TIMEUNIT_MS);

	float length = 0;
	if (templength)
		length = templength * 0.001f;

	return length;
}

// Compares specified ambient sound with the current ambient sound being played
// Returns true if they match, false if they do not or if no sound is being played
unsigned int CFMODManager::GetSoundLengthPCM(Sound *sound)
{
	unsigned int templength = 0;
	sound->getLength(&templength, FMOD_TIMEUNIT_PCM);
	return templength - 1;
}

// Abruptly starts playing a specified ambient sound
// In most cases, we'll want to use TransitionAmbientSounds instead
void CFMODManager::PlayLoopingMusic( ChannelGroup *pNewChannelGroup, const char* pLoopingMusic, const char* pIntroMusic, float flDelay , bool fadeIn)
{
	Channel *pTempChannel;
	Channel *pTempLoopChannel;
	
	if ( !pNewChannelGroup )
		return;
	
	int  outputrate = 0;
	result = pSystem->getSoftwareFormat(&outputrate, 0, 0);
	unsigned int dsp_block_len = 0;
	result = pSystem->getDSPBufferSize(&dsp_block_len, 0);
	unsigned long long clock_start = 0;
	float freq = 0;
	unsigned int slen = 0;
	
	if (pIntroMusic)
	{
		Sound *pIntroSound = NULL;
		Sound *pLoopingSound = NULL;
		
		int iLengthIntro = 0;
		void *vBufferIntro = GetFullPathToSound(pIntroMusic, &iLengthIntro);
		
		FMOD_CREATESOUNDEXINFO infoIntro;
		memset(&infoIntro, 0, sizeof(infoIntro));
		infoIntro.length = iLengthIntro;
		infoIntro.cbsize = sizeof(infoIntro);
		
		result = pSystem->createStream((const char *)vBufferIntro, FMOD_OPENMEMORY | FMOD_CREATESTREAM , &infoIntro, &pIntroSound);
		pSystem->playSound(pIntroSound, pNewChannelGroup, true, &pTempChannel);

		result = pIntroSound->getDefaults(&freq, 0);
		
		result = pTempChannel->getDSPClock(0, &clock_start);

		clock_start += (dsp_block_len * 2);

		slen = (unsigned int) ( flDelay * 30000.0f );
		slen = (unsigned int)((float)slen / freq * outputrate);
		DevMsg("Initial delay length is %d\n", slen);
		clock_start += slen;
		
		result = pTempChannel->setDelay(clock_start, 0, false);
		result = pTempChannel->setPaused(false);

		result = pLoopingSound->setLoopPoints(0, FMOD_TIMEUNIT_PCM, GetSoundLengthPCM(pLoopingSound), FMOD_TIMEUNIT_PCM);

		int iLengthLoop = 0;
		void *vBufferLoop = GetFullPathToSound(pLoopingMusic, &iLengthLoop);
		
		FMOD_CREATESOUNDEXINFO infoLoop;
		memset(&infoLoop, 0, sizeof(infoLoop));
		infoLoop.length = iLengthLoop;
		infoLoop.cbsize = sizeof(infoLoop);

		result = pSystem->createStream((const char *)vBufferLoop, FMOD_OPENMEMORY | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM, &infoLoop, &pLoopingSound);
		pSystem->playSound(pLoopingSound, pNewChannelGroup, true, &pTempLoopChannel);

		result = pIntroSound->getLength(&slen, FMOD_TIMEUNIT_PCM);
		result = pIntroSound->getDefaults(&freq, 0);

		slen = (unsigned int)((float)slen / freq * outputrate);
		DevMsg("Intro delay length is %d\n", slen);
		clock_start += slen;

		result = pTempLoopChannel->setDelay(clock_start, 0, false);
		result = pTempLoopChannel->setPaused(false);
		pTempChannel->setChannelGroup(pNewChannelGroup);
		pTempLoopChannel->setChannelGroup(pNewChannelGroup);

	}
	else
	{
		int iLengthLoop = 0;
		void *vBufferLoop = GetFullPathToSound(pLoopingMusic, &iLengthLoop);
		
		FMOD_CREATESOUNDEXINFO infoLoop;
		memset(&infoLoop, 0, sizeof(infoLoop));
		infoLoop.length = iLengthLoop;
		infoLoop.cbsize = sizeof(infoLoop);
		
		result = pSound->setLoopPoints(0, FMOD_TIMEUNIT_PCM, GetSoundLengthPCM(pSound), FMOD_TIMEUNIT_PCM);
		result = pSystem->createStream((const char *)vBufferLoop, FMOD_OPENMEMORY | FMOD_LOOP_NORMAL | FMOD_CREATESTREAM , &infoLoop, &pSound);
		if (result != FMOD_OK)
		{
			Warning("FMOD: Failed to create stream of sound '%s' ! (ERROR NUMBER: %i)\n", pLoopingMusic, result);
			return;
		}
		result = pSystem->playSound(pSound, pNewChannelGroup, true, &pTempChannel);
		
		result = pTempChannel->getDSPClock(0, &clock_start);

		clock_start += (dsp_block_len * 2);
		pSound->getDefaults(&freq, 0);
		slen = (unsigned int) ( flDelay * 30000.0f );
		slen = (unsigned int)((float)slen / freq * outputrate);
		DevMsg("Initial delay length is %d\n", slen);
		clock_start += slen;

		result = pTempChannel->setDelay(clock_start, 0, false);
		
		pTempChannel->setPosition(0, FMOD_TIMEUNIT_PCM);
		pTempChannel->setPaused(false);
		if (result != FMOD_OK)
		{
			Warning("FMOD: Failed to play sound '%s' ! (ERROR NUMBER: %i)\n", pLoopingMusic, result);
			return;
		}
		pTempChannel->setChannelGroup(pNewChannelGroup);
	}
	//	result = pSound->setLoopCount( -1 );
	//	
	m_flSongStart = gpGlobals->realtime;
	currentSound = pLoopingMusic;
}

// Abruptly starts playing a specified ambient sound
// In most cases, we'll want to use TransitionAmbientSounds instead
void CFMODManager::PlayAmbientSound(const char* pathToFileFromSoundsFolder, bool fadeIn)
{
	int iLengthLoop = 0;
	void *vBufferLoop = GetFullPathToSound(pathToFileFromSoundsFolder, &iLengthLoop);
	
	FMOD_CREATESOUNDEXINFO infoLoop;
	memset(&infoLoop, 0, sizeof(infoLoop));
	infoLoop.length = iLengthLoop;
	infoLoop.cbsize = sizeof(infoLoop);	
	
	result = pSystem->createStream((const char*)vBufferLoop, FMOD_OPENMEMORY , &infoLoop, &pSound);

	if (result != FMOD_OK)
	{
		Warning("FMOD: Failed to create stream of sound '%s' ! (ERROR NUMBER: %i)\n", pathToFileFromSoundsFolder, result);
		return;
	}

	m_flSongStart = gpGlobals->realtime;
	result = pSystem->playSound(pSound, pChannelGroup, false, &pChannel);

	if (result != FMOD_OK)
	{
		Warning("FMOD: Failed to play sound '%s' ! (ERROR NUMBER: %i)\n", pathToFileFromSoundsFolder, result);
		return;
	}

	if (fadeIn)
	{
		pChannel->setVolume(0.0);
		m_fDefaultVolume = 0.0f;
		m_bFadeIn = true;
	}
	else
	{
		m_fDefaultVolume = 1.0f;
	}

	currentSound = pathToFileFromSoundsFolder;
}

void CFMODManager::StopAmbientSound( ChannelGroup *pNewChannel, bool fadeOut)
{
	if (fadeOut)
	{
		pNewChannel->setVolume(1.0f);
		
		m_fDefaultVolume = 1.0f;
		m_bFadeOut = true;
	}
	else
	{
		pNewChannel->setVolume(0.0f);
		pNewChannel->stop();
	}

	currentSound = "NULL";
}

void CFMODManager::PlayMusicEnd( ChannelGroup *pNewChannelGroup, const char* pLoopingMusic, bool bDelay, Channel *pLoopingChannel )
{
	Channel *pTempChannel;
	
	if ( !pNewChannelGroup )
		return;
	
	if (pLoopingMusic)
	{
		Sound *pIntroSound = NULL;
		
		int iLength = 0;
		void *vBuffer = GetFullPathToSound(pLoopingMusic, &iLength);
		
		FMOD_CREATESOUNDEXINFO info;
		memset(&info, 0, sizeof(info));
		info.length = iLength;
		info.cbsize = sizeof(info);			
		
		result = pSystem->createStream((const char*)vBuffer, FMOD_CREATESTREAM | FMOD_OPENMEMORY, &info, &pIntroSound);
		pSystem->playSound(pIntroSound, pNewChannelGroup, true, &pTempChannel);
		
		result = pTempChannel->setPaused(false);
		pTempChannel->setChannelGroup(pNewChannelGroup);
	}
	
}

void CommSndStp( void )
{
	FMODManager()->StopAllSound();
}
static ConCommand fm_stop_all_sound( "fm_stop_all_sound", CommSndStp, "Stops every channel group", FCVAR_NONE );

// Abruptly stops playing all ambient sounds
void CFMODManager::StopAllSound( void )
{
//	pChannelGroup->setVolume(0.0f);
	pChannelGroup->stop();
//	pChannelGroup->setPaused(true);
}

// Transitions between two ambient sounds if necessary
// If a sound isn't already playing when this is called, don't worry about it
void CFMODManager::TransitionAmbientSounds(const char* pathToFileFromSoundsFolder)
{
	pChannel->setVolume(1.0);
	m_fDefaultVolume = 1.0f;
	newSoundFileToTransitionTo = pathToFileFromSoundsFolder;

	m_bFadeOut = true;
	m_bShouldTransition = true;
	m_bFadeIn = true;
}