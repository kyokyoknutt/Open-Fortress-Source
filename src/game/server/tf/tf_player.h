//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
//=============================================================================
#ifndef TF_PLAYER_H
#define TF_PLAYER_H
#pragma once

#include "tf_player_shared.h"
#include "tf_weaponbase_gun.h"
#include "tf_playerclass.h"
#include "entity_tfstart.h"
#include "trigger_area_capture.h"
#include "nav_mesh/tf_nav_area.h"
#include "Path/NextBotPathFollow.h"
#include "tf_powerup.h"

class CTFPlayer;
class CTFTeam;
class CTFGoal;
class CTFGoalItem;
class CTFItem;
class CTFWeaponBuilder;
class CBaseObject;
class CTFWeaponBase;
class CIntroViewpoint;

//=============================================================================
//
// Player State Information
//
class CPlayerStateInfo
{
public:

	int				m_nPlayerState;
	const char		*m_pStateName;

	// Enter/Leave state.
	void ( CTFPlayer::*pfnEnterState )();	
	void ( CTFPlayer::*pfnLeaveState )();

	// Think (called every frame).
	void ( CTFPlayer::*pfnThink )();
};

struct DamagerHistory_t
{
	DamagerHistory_t()
	{
		Reset();
	}
	void Reset()
	{
		hDamager = NULL;
		flTimeDamage = 0;
	}
	EHANDLE hDamager;
	float	flTimeDamage;
};
#define MAX_DAMAGER_HISTORY 2

class CTFPlayerPathCost : public IPathCost
{
public:
	CTFPlayerPathCost( CTFPlayer *player )
		: m_pPlayer( player )
	{
		m_flStepHeight = 18.0f;
		m_flMaxJumpHeight = 72.0f;
		m_flDeathDropHeight = 200.0f;
	}

	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const;

private:
	CTFPlayer *m_pPlayer;
	float m_flStepHeight;
	float m_flMaxJumpHeight;
	float m_flDeathDropHeight;
};

// declare this function so we can friend it.
static void tf_bot_add( const CCommand &args );

//=============================================================================
//
// TF Player
//
class CTFPlayer : public CBaseMultiplayerPlayer
{
public:
	DECLARE_CLASS( CTFPlayer, CBaseMultiplayerPlayer );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTFPlayer();
	~CTFPlayer();

	// Creation/Destruction.
	static CTFPlayer	*CreatePlayer( const char *className, edict_t *ed );
	static CTFPlayer	*Instance( int iEnt );
	
	virtual void		Spawn();
			void		UpdateCosmetics();
	virtual void		ClearSlots();
	virtual void		ForceRespawn();
	virtual CBaseEntity	*EntSelectSpawnPoint( void );
	virtual void		InitialSpawn();
	virtual void		Precache();
	virtual bool		IsReadyToPlay( void );
	virtual bool		IsReadyToSpawn( void );
	virtual bool		ShouldGainInstantSpawn( void );
	virtual void		ResetScores( void );

	void				CreateViewModel( int iViewModel = 0 );
	CBaseViewModel		*GetOffHandViewModel();
	void				SendOffHandViewModelActivity( Activity activity );

	virtual void		CheatImpulseCommands( int iImpulse );
	
	virtual void		LeaveVehicle( const Vector &vecExitPoint, const QAngle &vecExitAngles );

	virtual void		CommitSuicide( bool bExplode = false, bool bForce = false );
	
	virtual CTFNavArea *GetLastKnownArea( void ) const override;

	// Combats
	virtual void		TraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator);
	virtual int			TakeHealth( float flHealth, int bitsDamageType );
	virtual void		OnMyWeaponFired( CBaseCombatWeapon *weapon ) override;
	virtual	void		Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );
	virtual	void		GotKilled(){ m_bGotKilled = true; };
	virtual void		Event_Killed( const CTakeDamageInfo &info );
	virtual void		PlayerDeathThink( void );

	virtual int			OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual int			OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void				ApplyDamageKnockback( const CTakeDamageInfo &info );
	void				AddDamagerToHistory( EHANDLE hDamager );
	void				ClearDamagerHistory();
	DamagerHistory_t	&GetDamagerHistory( int i ) { return m_DamagerHistory[i]; }
	virtual void		DamageEffect(float flDamage, int fDamageType);
	void				DismemberRandomLimbs( void );
	virtual	bool		ShouldCollide( int collisionGroup, int contentsMask ) const;

	void				SetHealthBuffTime( float flTime )		{ m_flHealthBuffTime = flTime; }

	int					GetLevelProgress( void ) { return m_iLevelProgress; }
	void				SetLevelProgress( int count ) { m_iLevelProgress = count; }
	void				IncrementLevelProgress( int count ) { m_iLevelProgress += count; }
	
	CTFWeaponBase		*GetActiveTFWeapon( void ) const;

	void				SaveMe( void );

	void				FireBullet( const FireBulletsInfo_t &info, bool bDoEffects, int nDamageType, int nCustomDamageType = TF_DMG_CUSTOM_NONE );
	void				ImpactWaterTrace( trace_t &trace, const Vector &vecStart );
	void				NoteWeaponFired();

	bool				HasItem( void );					// Currently can have only one item at a time.
	void				SetItem( CTFItem *pItem );
	CTFItem				*GetItem( void );

	void				Regenerate( void );
	float				GetNextRegenTime( void ){ return m_flNextRegenerateTime; }
	void				SetNextRegenTime( float flTime ){ m_flNextRegenerateTime = flTime; }

	float				GetNextChangeClassTime( void ){ return m_flNextChangeClassTime; }
	void				SetNextChangeClassTime( float flTime ){ m_flNextChangeClassTime = flTime; }

	virtual	void		RemoveAllItems( bool removeSuit );

	bool				DropCurrentWeapon( void );
	void				DropFlag( void );
	void				TFWeaponRemove( int iWeaponID );
	bool				TFWeaponDrop( CTFWeaponBase *pWeapon, bool bThrowForward );

	// Class.
	CTFPlayerClass		*GetPlayerClass( void ) 					{ return &m_PlayerClass; }
	int					GetDesiredPlayerClassIndex( void )			{ return m_Shared.m_iDesiredPlayerClass; }
	void				SetDesiredPlayerClassIndex( int iClass )	{ m_Shared.m_iDesiredPlayerClass = iClass; }

	// Team.
	void				ForceChangeTeam( int iTeamNum );
	virtual void		ChangeTeam( int iTeamNum, bool bNoKill = false );

	// mp_fadetoblack
	void				HandleFadeToBlack( void );

	// Flashlight controls for SFM - JasonM
	virtual int FlashlightIsOn( void );
	virtual void FlashlightTurnOn( void );
	virtual void FlashlightTurnOff( void );

	// Think.
	virtual void		PreThink();
	virtual void		PostThink();

	virtual void		ItemPostFrame();
	virtual void		Weapon_FrameUpdate( void );
	virtual void		Weapon_HandleAnimEvent( animevent_t *pEvent );
	virtual bool		Weapon_ShouldSetLast( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon );

	virtual void		GetStepSoundVelocities( float *velwalk, float *velrun );
	virtual void		SetStepSoundTime( stepsoundtimes_t iStepSoundTime, bool bWalking );

	// Utility.
	void				RemoveOwnedEnt( char *pEntName, bool bGrenade = false );
	void				UpdateModel( void );
	void				UpdateArmModel( void );
	void				UpdateSkin( int iTeam );
	void				UpdatePlayerClass( int iPlayerClass = TF_CLASS_UNDEFINED, bool bRefreshWeapons = false );

	virtual int			GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound = false );
	int					GetMaxAmmo( int iAmmoIndex, int iClassNumber = -1 );

	bool				CanAttack( void );
	
	virtual void		OnNavAreaChanged( CNavArea *enteredArea, CNavArea *leftArea );

	// This passes the event to the client's and server's CPlayerAnimState.
	void				DoAnimationEvent( PlayerAnimEvent_t event, int mData = 0 );

	virtual bool		ClientCommand( const CCommand &args );
	void				ClientHearVox( const char *pSentence );
	void				DisplayLocalItemStatus( CTFGoal *pGoal );
	
	// physics interactions
	virtual void		PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize );
	virtual void		ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldindThis );
	virtual float		GetHeldObjectMass( IPhysicsObject *pHeldObject );
	
	virtual bool		IsFollowingPhysics( void ) { return 0; }
	void				InputForceDropPhysObjects( inputdata_t &data );

	void				DropZombieAmmoHealth( void );

	int					BuildObservableEntityList( void );
	virtual int			GetNextObserverSearchStartPoint( bool bReverse ); // Where we should start looping the player list in a FindNextObserverTarget call
	virtual CBaseEntity *FindNextObserverTarget(bool bReverse);
	virtual bool		IsValidObserverTarget(CBaseEntity * target); // true, if player is allowed to see this target
	virtual bool		SetObserverTarget(CBaseEntity * target);
	virtual bool		ModeWantsSpectatorGUI( int iMode ) { return (iMode != OBS_MODE_FREEZECAM && iMode != OBS_MODE_DEATHCAM); }
	void				FindInitialObserverTarget( void );
	CBaseEntity		    *FindNearestObservableTarget( Vector vecOrigin, float flMaxDist );
	virtual void		ValidateCurrentObserverTarget( void );

	void CheckUncoveringSpies( CTFPlayer *pTouchedPlayer );
	void Touch( CBaseEntity *pOther );

	void TeamFortress_SetSpeed();
	EHANDLE TeamFortress_GetDisguiseTarget( int nTeam, int nClass );

	void TeamFortress_ClientDisconnected();
	void TeamFortress_RemoveEverythingFromWorld();
	void TeamFortress_RemoveProjectiles();

	CTFTeamSpawn *GetSpawnPoint( void ){ return m_pSpawnPoint; }
		
	void SetAnimation( PLAYER_ANIM playerAnim );

	bool IsPlayerClass( int iClass ) const;

	void PlayFlinch( const CTakeDamageInfo &info );

	float PlayCritReceivedSound( void );
	void PainSound( const CTakeDamageInfo &info );
	void DeathSound( const CTakeDamageInfo &info );


	// TF doesn't want the explosion ringing sound
	virtual void			OnDamagedByExplosion( const CTakeDamageInfo &info ) { return; }

	void	OnBurnOther( CTFPlayer *pTFPlayerVictim );

	// Buildables
	void SetWeaponBuilder( CTFWeaponBuilder *pBuilder );
	CTFWeaponBuilder *GetWeaponBuilder( void );

	int GetBuildResources( void );
	void RemoveBuildResources( int iAmount );
	void AddBuildResources( int iAmount );

	bool IsBuilding( void );
	int CanBuild( int iObjectType, int iAltMode );

	CBaseObject	*GetObject( int index );
	int	GetObjectCount( void );
	int GetNumObjects( int iObjectType, int iAltMode );
	void RemoveAllObjects( void );
	int	StartedBuildingObject( int iObjectType );
	void StoppedBuilding( int iObjectType );
	void FinishedObject( CBaseObject *pObject );
	void AddObject( CBaseObject *pObject );
	void OwnedObjectDestroyed( CBaseObject *pObject );
	void RemoveObject( CBaseObject *pObject );
	bool PlayerOwnsObject( CBaseObject *pObject );
	void DetonateOwnedObjectsOfType( int iType, int iAltMode );
	void StartBuildingObjectOfType( int iType, int iAltMode );
	virtual CBaseEntity	*GetHeldObject(void);
	CBaseObject			*GetObjectOfType( int iType, int iMode );

	CTFTeam *GetTFTeam( void );
	CTFTeam *GetOpposingTFTeam( void );

	void TeleportEffect( void );
	void RemoveTeleportEffect( void );
	bool HasTheFlag( void );

	// Death & Ragdolls.
	virtual void CreateRagdollEntity( void );
	void CreateRagdollEntity( bool bGib, bool bBurning, bool bDissolve, bool bFlagOnGround, int iDamageCustom );
	void DestroyRagdoll( void );
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 
	virtual bool ShouldGib( const CTakeDamageInfo &info );

	// Dropping Ammo
	void DropAmmoPack( void );
	void DropWeapon( CTFWeaponBase *pActiveWeapon, bool bThrown = false, bool bDissolve = false, int Clip = -1, int Reserve = -1 );
	
	CTFWeaponBase *GetWeaponInSlot( int iSlot, int iSlotPos );
	bool CanPickupWeapon( CTFWeaponBase *pCarriedWeapon, CTFWeaponBase *pWeapon );
	
	bool CanDisguise( void );
	bool CanGoInvisible( void );
	void RemoveInvisibility( void );
	
	bool CanAutoswitch( void );
	
	virtual void	Weapon_Equip( CBaseCombatWeapon *pWeapon );

	void RemoveDisguise( void );
	void PrintTargetWeaponInfo( void );

	bool DoClassSpecialSkill( void );
	bool CheckSpecialSkill( void );

	float GetLastDamageTime( void ) { return m_flLastDamageTime; }

	void SetClassMenuOpen( bool bIsOpen );
	bool IsClassMenuOpen( void );

	float GetCritMult( void ) { return m_Shared.GetCritMult(); }
	void  RecordDamageEvent( const CTakeDamageInfo &info, bool bKill ) { m_Shared.RecordDamageEvent(info,bKill); }

	bool GetHudClassAutoKill( void ){ return m_bHudClassAutoKill; }
	void SetHudClassAutoKill( bool bAutoKill ){ m_bHudClassAutoKill = bAutoKill; }

	bool GetMedigunAutoHeal( void ){ return m_bMedigunAutoHeal; }
	void SetMedigunAutoHeal( bool bMedigunAutoHeal ){ m_bMedigunAutoHeal = bMedigunAutoHeal; }

	bool ShouldAutoRezoom( void ) { return m_bAutoRezoom; }
	void SetAutoRezoom( bool bAutoRezoom ) { m_bAutoRezoom = bAutoRezoom; }
	
	bool ShouldAutoReload( void ) { return m_bAutoReload; }
	void SetAutoReload( bool bAutoReload ) { m_bAutoReload = bAutoReload; }

	bool ShouldAutoSwitchWeapons( void ) { return m_bAutoSwitchWeapons; }
	void SetAutoSwitchWeapons( bool bAutoSwitchWeapons ) { m_bAutoSwitchWeapons = bAutoSwitchWeapons; }	
	
	bool ShouldQuickZoom( void );
	
	virtual void	ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet );

	virtual bool CanHearAndReadChatFrom( CBasePlayer *pPlayer );

	virtual bool	IsDeflectable( void ) { return true; }

	Vector 	GetClassEyeHeight( void );

	// HL2 uses these for NPC checks
	virtual Vector		EyeDirection2D( void );
	virtual Vector		EyeDirection3D( void );

	void	UpdateExpression( void );
	void	ClearExpression( void );

	virtual IResponseSystem *GetResponseSystem();
	virtual bool			SpeakConceptIfAllowed( int iConcept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );

	virtual bool CanSpeakVoiceCommand( void );
	virtual bool ShouldShowVoiceSubtitleToEnemy( void );
	virtual void NoteSpokeVoiceCommand( const char *pszScenePlayed );
	void	SpeakWeaponFire( int iCustomConcept = MP_CONCEPT_NONE );
	void	ClearWeaponFireScene( void );

	virtual int DrawDebugTextOverlays( void );

	float m_flNextVoiceCommandTime;
	float m_flNextSpeakWeaponFire;

	virtual int	CalculateTeamBalanceScore( void );

	bool ShouldAnnouceAchievement( void );

	void UpdatePlayerColor( void );
	
public:
	CNetworkVector( m_vecPlayerColor );
	CNetworkVector( m_vecViewmodelOffset );
	CNetworkVector( m_vecViewmodelAngle );

	CNetworkVar( bool, m_bCentered );
	CNetworkVar( bool, m_bMinimized );
	
	CUtlVector<int> m_iCosmetics;
	KeyValues *kvDesiredCosmetics;
	CTFPlayerShared m_Shared;
	int	    item_list;			// Used to keep track of which goalitems are 
								// affecting the player at any time.
								// GoalItems use it to keep track of their own 
								// mask to apply to a player's item_list

	float invincible_finished;
	float invisible_finished;
	float super_damage_finished;
	float radsuit_finished;

	int m_flNextTimeCheck;		// Next time the player can execute a "timeleft" command

	int	m_iJuggernaughtScore;
	int m_iJuggernaughtTimer;
	bool m_bIsJuggernaught;
	void BecomeJuggernaught();
	bool IsJuggernaught() { return m_bIsJuggernaught; }

	// TEAMFORTRESS VARIABLES
	int		no_sentry_message;
	int		no_dispenser_message;
	
	bool	m_bPuppet;
	
	CNetworkVar( bool, m_bSaveMeParity );
	CNetworkVar( bool, m_bDied );
	CNetworkVar( bool, m_bGotKilled );
	CNetworkVar( bool, m_bResupplied );

	// teleporter variables
	int		no_entry_teleporter_message;
	int		no_exit_teleporter_message;

	float	m_flNextNameChangeTime;

	int					StateGet( void ) const;

	void				SetOffHandWeapon( CTFWeaponBase *pWeapon );
	void				HolsterOffHandWeapon( void );

	float				GetSpawnTime() { return m_flSpawnTime; }

	virtual bool Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
	virtual void Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget , const Vector *pVelocity );

	void 				UpdateGunGameLevel( void );
	void				ManageRegularWeapons( TFPlayerClassData_t *pData );
	void				ManageInstagibWeapons( TFPlayerClassData_t *pData );
	void				ManageGunGameWeapons( TFPlayerClassData_t *pData );
	void				Manage3WaveWeapons( TFPlayerClassData_t *pData );
	void				ManageClanArenaWeapons(TFPlayerClassData_t *pData);
	void				ManageRocketArenaWeapons(TFPlayerClassData_t *pData);
	void				ManageArsenalWeapons(TFPlayerClassData_t *pData);
	void				ManageBuilderWeapons( TFPlayerClassData_t *pData, bool bSwitch = true );
	bool				ManageRandomizerWeapons( TFPlayerClassData_t *pData );
	void				ManageCustomSpawnWeapons( TFPlayerClassData_t *pData );
	
	int 				GetDesiredWeaponCount( TFPlayerClassData_t *pData );
	int 				GetDesiredWeapon( int iWeapon, TFPlayerClassData_t *pData );
	
	Vector				EstimateProjectileImpactPosition( CTFWeaponBaseGun *weapon );
	Vector				EstimateProjectileImpactPosition( float pitch, float yaw, float speed );
	Vector				EstimateStickybombProjectileImpactPosition( float pitch, float yaw, float charge );

	bool				IsCapturingPoint( void );

	CTriggerAreaCapture *GetControlPointStandingOn( void );
	
	// Taunts.
	void				Taunt( void );
	bool				IsTaunting( void ) { return m_Shared.InCond( TF_COND_TAUNTING ); }
	virtual void		SetTauntEffect( int nTaunt, float flThinkTime, int nTauntLayer = 0 );
	virtual void		TauntEffectThink( void );
	QAngle				m_angTauntCamera;
	CNetworkVar( int,	m_iTaunt);
	CNetworkVar( int,	m_iTauntLayer); //  Just as Onions, Taunts have layers
	CNetworkVar( float,	m_fTauntEffectTick);

	virtual float		PlayScene( const char *pszScene, float flDelay = 0.0f, AI_Response *response = NULL, IRecipientFilter *filter = NULL );
	void				ResetTauntHandle( void )				{ m_hTauntScene = NULL; }
	void				SetDeathFlags( int iDeathFlags ) { m_iDeathFlags = iDeathFlags; }
	int					GetDeathFlags() { return m_iDeathFlags; }
	void				SetMaxSentryKills( int iMaxSentryKills ) { m_iMaxSentryKills = iMaxSentryKills; }
	int					GetMaxSentryKills() { return m_iMaxSentryKills; }

	CNetworkVar( bool, m_iSpawnCounter );
	
	void				CheckForIdle( void );
	void				PickWelcomeObserverPoint();

	void				StopRandomExpressions( void ) { m_flNextRandomExpressionTime = -1; }
	void				StartRandomExpressions( void ) { m_flNextRandomExpressionTime = gpGlobals->curtime; }

	virtual bool			WantsLagCompensationOnEntity( const CBaseEntity	*pEntity, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const;

	CBaseEntity			*MedicGetHealTarget( void );

	CTFWeaponBase		*Weapon_OwnsThisID( int iWeaponID );
	CTFWeaponBase		*Weapon_GetWeaponByType( int iType );

	int				RestockClips( float PowerupSize );
	int				RestockAmmo( float PowerupSize );
	int				RestockMetal( float PowerupSize );
	int				RestockCloak( float PowerupSize );
	int				RestockSpecialEffects( float PowerupSize );
	bool				OwnsWeaponID( int ID );

	CNetworkVar( bool, m_bHauling );

	bool				IsHauling( void ) { return m_bHauling; }
	void				SetHauling( bool bHauling ) { m_bHauling = bHauling; }

	float				m_flLastAction;
	
	CountdownTimer m_purgatoryDuration;

	IntervalTimer m_lastCalledMedic;

	// Gore
	unsigned short m_iGoreHead;
	unsigned short m_iGoreLeftArm;
	unsigned short m_iGoreRightArm;
	unsigned short m_iGoreLeftLeg;
	unsigned short m_iGoreRightLeg;
	
	CUtlVector< PowerupHandle >	m_hPowerups;
	CUtlVector< WeaponHandle >	m_hSuperWeapons;

	WeaponHandle m_hWeaponInSlot[10][20]; // 20 pos cuz melee my ass

	//medals
	bool				m_bHadPowerup;
	float				m_fEXTime;
	float				m_fAirStartTime;
	int					m_iPowerupKills;
	int					m_iEXKills;
	int					m_iSpreeKills;
	int					m_iImpressiveCount;

private:

	int					GetAutoTeam( void );

	// Creation/Destruction.
	void				InitClass( void );
	void				GiveDefaultItems();
	int					GetCarriedWeapons();
	void				StripWeapons();

	bool				SelectSpawnSpot( const char *pEntClassName, CBaseEntity* &pSpot );
	// for deathmatch
	bool				SelectDMSpawnSpots(const char *pEntClassName, CBaseEntity* &pSpot);

	void				PrecachePlayerModels( void );
	void				PrecacheMyself( void );
	void				RemoveNemesisRelationships();

	// Think.
	void				TFPlayerThink();
	void				MedicRegenThink();
	void				ZombieRegenThink();
	void				UpdateTimers( void );

	// Taunt.
	EHANDLE				m_hTauntScene;
	bool				m_bInitTaunt;

	// Client commands.
	void				HandleCommand_JoinTeam( const char *pTeamName, bool bNoKill = false );
public:
	void				HandleCommand_JoinClass( const char *pClassName, bool bForced = false );
private:
	void				HandleCommand_JoinTeam_NoMenus( const char *pTeamName );

	// Bots.
	friend void			Bot_Think( CTFPlayer *pBot );
	
	// Linux gives us errors when this function is static
	// but windows gives us errors when its not...
	// NOPEY: I found a workaround, declaring tf_bot_add static above
	//			If this doesn't work on MSVC, I reckon we should just
	//			ifdef guard it and #define TF_BOT_CPP in tf_bot.cpp
	friend void tf_bot_add( const CCommand &args );
	friend class CTFBot; friend class CTFBotManager;

	// Physics.
	void				PhysObjectSleep();
	void				PhysObjectWake();

	// Ammo pack.
	bool CalculateAmmoPackPositionAndAngles( CTFWeaponBase *pWeapon, Vector &vecOrigin, QAngle &vecAngles );
	void AmmoPackCleanUp( void );

	// State.
	CPlayerStateInfo	*StateLookupInfo( int nState );
	void				StateEnter( int nState );
	void				StateLeave( void );
	void				StateTransition( int nState );
	void				StateEnterWELCOME( void );
	void				StateThinkWELCOME( void );
	void				StateEnterPICKINGTEAM( void );
	void				StateEnterACTIVE( void );
	void				StateEnterOBSERVER( void );
	void				StateThinkOBSERVER( void );
	void				StateEnterDYING( void );
	void				StateThinkDYING( void );

	virtual bool		SetObserverMode(int mode);
	virtual void		AttemptToExitFreezeCam( void );

	bool				PlayGesture( const char *pGestureName );
	bool				PlaySpecificSequence( const char *pSequenceName );

	// moved to tf_playerclass_shared
	//bool				PlayDeathAnimation( const CTakeDamageInfo &info, CTakeDamageInfo &info_modified );

	bool				GetResponseSceneFromConcept( int iConcept, char *chSceneBuffer, int numSceneBufferBytes );

private:
	// Map introductions
	int					m_iIntroStep;
	CHandle<CIntroViewpoint> m_hIntroView;
	float				m_flIntroShowHintAt;
	float				m_flIntroShowEventAt;
	bool				m_bHintShown;
	bool				m_bAbortFreezeCam;
	bool				m_bSeenRoundInfo;
	bool				m_bRegenerating;

	// Items.
	CNetworkHandle( CTFItem, m_hItem );

	// Combat.
	CNetworkHandle( CTFWeaponBase, m_hOffHandWeapon );

	float					m_flHealthBuffTime;

	float					m_flNextRegenerateTime;
	float					m_flNextChangeClassTime;

	// Ragdolls.
	Vector					m_vecTotalBulletForce;

	// State.
	CPlayerStateInfo		*m_pStateInfo;

	// Spawn Point
	CTFTeamSpawn			*m_pSpawnPoint;

	// Networked.
	CNetworkQAngle( m_angEyeAngles );					// Copied from EyeAngles() so we can send it to the client.

	CTFPlayerClass		m_PlayerClass;
	CTFPlayerAnimState	*m_PlayerAnimState;
	int					m_iLastWeaponFireUsercmd;				// Firing a weapon.  Last usercmd we shot a bullet on.
	int					m_iLastSkin;
	float				m_flLastDamageTime;
	float				m_flNextPainSoundTime;
	int					m_LastDamageType;
	int					m_iDeathFlags;				// TF_DEATH_* flags with additional death info
	int					m_iMaxSentryKills;			// most kills by a single sentry
	int					m_iLevelProgress = 0;

	bool				m_bPlayedFreezeCamSound;

	CHandle< CTFWeaponBuilder > m_hWeaponBuilder;

	CUtlVector<EHANDLE>	m_aObjects;			// List of player objects

	bool m_bIsClassMenuOpen;

	Vector m_vecLastDeathPosition;

	float				m_flSpawnTime;

	bool				m_bIsIdle;

	CUtlVector<EHANDLE>	m_hObservableEntities;
	DamagerHistory_t m_DamagerHistory[MAX_DAMAGER_HISTORY];	// history of who has damaged this player
	CUtlVector<float>	m_aBurnOtherTimes;					// vector of times this player has burned others

	bool m_bHudClassAutoKill;

	// Background expressions
	string_t			m_iszExpressionScene;
	EHANDLE				m_hExpressionSceneEnt;
	float				m_flNextRandomExpressionTime;
	EHANDLE				m_hWeaponFireSceneEnt;

	bool				m_bSpeakingConceptAsDisguisedSpy;

	bool 				m_bMedigunAutoHeal;
	bool				m_bAutoRezoom;	// does the player want to re-zoom after each shot for sniper rifles
	bool				m_bAutoReload;	// does the player want to reload after each shot
	bool				m_bAutoSwitchWeapons;
	
public:
//	bool				SetPowerplayEnabled( bool bOn );
//  bool				PlayerHasPowerplay( void );
//	void				PowerplayThink( void );
//	float				m_flPowerPlayTime;
	void				SetCustomModel( inputdata_t &inputdata );
	void				SetCustomArmModel( inputdata_t &inputdata );
	void				AddMoney( inputdata_t &inputdata );
	void				SetMoney( inputdata_t &inputdata );
	void				InputStripWeapons( inputdata_t &inputdata );
	void				InputSpeakResponseConcept( inputdata_t &inputdata );
	void				InputIgnitePlayer( inputdata_t &inputdata );
	void				InputExtinguishPlayer( inputdata_t &inputdata );
	void				InputPoisonPlayer( inputdata_t &inputdata );
	void				InputDePoisonPlayer( inputdata_t &inputdata );
	void				InputSetZombie( inputdata_t &inputdata );
	void				InputSetTeamNoKill( inputdata_t &inputdata );
	bool				m_bNotAlreadyPlayingMusic;
	bool				IsAllowedToPickupWeapons( void ) { return true; }
	bool				Weapon_CanUse( void ) { return true; }
	bool				Weapon_EquipAmmoOnly( CBaseCombatWeapon *pWeapon ) { return false; }
	void				GiveAllItems();
	void				RefillHealthAmmo();
	void				AddAccount( int amount, bool bTrackChange=true );	// Add money to this player's account.
	bool				IsRetroModeOn();

	CNetworkVar( bool, m_bRetroMode );
	
	CNetworkVar( int, m_iAccount );	// How much cash this player has.

	// for chat particle bubble
	CNetworkVar( bool, m_bChatting );

	const char			*m_chzVMCosmeticGloves;
	const char			*m_chzVMCosmeticSleeves;

	// this is true if the player who died results in a victory (payload override gamemode)
	bool	m_bWinDeath;
};

//-----------------------------------------------------------------------------
// Purpose: Utility function to convert an entity into a tf player.
//   Input: pEntity - the entity to convert into a player
//-----------------------------------------------------------------------------
inline CTFPlayer *ToTFPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;

	if ( !pEntity->IsPlayer() )
		return NULL;

	Assert( dynamic_cast<CTFPlayer*>( pEntity ) != 0 );
	return static_cast< CTFPlayer* >( pEntity );
}

inline int CTFPlayer::StateGet( void ) const
{
	return m_Shared.m_nPlayerState;
}



#endif	// TF_PLAYER_H
