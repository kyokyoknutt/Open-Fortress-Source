//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: 
//
//=============================================================================//
#include "cbase.h"
#include "soundenvelope.h"
#include "tf_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar of_droppedweapons_glow( "of_droppedweapons_glow", "1", FCVAR_ARCHIVE, "Enables/Disables outlines on dropped weapons." );

extern ConVar of_glow_alpha;
extern ConVar of_allow_allclass_pickups;

class C_TFDroppedWeapon : public C_BaseAnimating, public ITargetIDProvidesHint
{
	DECLARE_CLASS( C_TFDroppedWeapon, C_BaseAnimating );

public:

	DECLARE_CLIENTCLASS();

	~C_TFDroppedWeapon();

	void	ClientThink( void );
	void	Spawn( void );	

	virtual void	OnDataChanged( DataUpdateType_t updateType );

	// ITargetIDProvidesHint
public:
	virtual void	DisplayHintTo( C_BasePlayer *pPlayer );
	CNetworkVar( int, m_iReserveAmmo );
	CNetworkVar( int, m_iClip );
	CNetworkVar( bool, m_bFlamethrower );
private:

	CGlowObject		   *m_pGlowEffect;
	bool	m_bShouldGlow;
	void	UpdateGlowEffect( void );
	void	DestroyGlowEffect(void);

	Vector		m_vecInitialVelocity;

	// Looping sound emitted by dropped flamethrowers
	CSoundPatch *m_pPilotLightSound;
	
	int		iTeamNum;

};

// Network table.
IMPLEMENT_CLIENTCLASS_DT( C_TFDroppedWeapon, DT_DroppedWeapon, CTFDroppedWeapon )
	RecvPropVector( RECVINFO( m_vecInitialVelocity ) ),
	RecvPropInt( RECVINFO( m_iReserveAmmo ) ),
	RecvPropInt( RECVINFO( m_iClip ) ),
	RecvPropBool( RECVINFO( m_bFlamethrower ) ),
END_RECV_TABLE()

C_TFDroppedWeapon::~C_TFDroppedWeapon()
{
	if ( m_pPilotLightSound )
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}
	DestroyGlowEffect();
}

void C_TFDroppedWeapon::Spawn( void )
{
	BaseClass::Spawn();
	iTeamNum = TEAM_INVALID;

	m_pGlowEffect = new CGlowObject( this, TFGameRules()->GetTeamGlowColor(GetLocalPlayerTeam()), of_glow_alpha.GetFloat(), true, true );

	UpdateGlowEffect();

	ClientThink();

	if ( m_bFlamethrower )
	{
		// Create the looping pilot light sound
		const char *pilotlightsound = "Weapon_FlameThrower.PilotLoop";
		CLocalPlayerFilter filter;

		CSoundEnvelopeController &controller = CSoundEnvelopeController::GetController();
		m_pPilotLightSound = controller.SoundCreate( filter, entindex(), pilotlightsound );

		controller.Play( m_pPilotLightSound, 1.0, 100 );
	}
	else
	{
		CSoundEnvelopeController::GetController().SoundDestroy( m_pPilotLightSound );
		m_pPilotLightSound = NULL;
	}
}

void C_TFDroppedWeapon::ClientThink( void )
{	
	bool bShouldGlow = false;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		return;
	}
	
	if  ( 										// Don't glow if
		( TFGameRules() && TFGameRules()->IsGGGamemode() ) || // we're in gun game
		( ( m_iReserveAmmo <= 0 && m_iReserveAmmo != -1 ) && ( m_iClip <= 0 && m_iClip != -1 ) )	||			// or empty
		( ( !of_allow_allclass_pickups.GetBool() && !pPlayer->GetPlayerClass()->IsClass( TF_CLASS_MERCENARY )  )) // Or we're not merc
		)
	{
		if ( m_bShouldGlow )
		{
			m_bShouldGlow = false;
			UpdateGlowEffect();
		}
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		return;
	}
	
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), pPlayer->EyePosition(), MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction == 1.0f )
	{
		bShouldGlow = true;
	}
	
	if ( m_bShouldGlow != bShouldGlow || ( iTeamNum != pPlayer->GetTeamNumber() ) )
	{
		m_bShouldGlow = bShouldGlow;
		iTeamNum = pPlayer->GetTeamNumber();

		UpdateGlowEffect();
	}

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: Update glow effect
//-----------------------------------------------------------------------------
void C_TFDroppedWeapon::UpdateGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		if ( TFGameRules() && m_bShouldGlow )
		{
			m_pGlowEffect->SetColor( TFGameRules()->GetTeamGlowColor( GetLocalPlayerTeam() ) );
			m_pGlowEffect->SetAlpha( of_glow_alpha.GetFloat() );
		}
		else
		{
			m_pGlowEffect->SetAlpha( 0.0f );
		}
	}
}

void C_TFDroppedWeapon::DestroyGlowEffect(void)
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_TFDroppedWeapon::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{ 
		float flChangeTime = GetLastChangeTime( LATCH_SIMULATION_VAR );
		Vector vecCurOrigin = GetLocalOrigin();

		// Now stick our initial velocity into the interpolation history 
		CInterpolatedVar< Vector > &interpolator = GetOriginInterpolator();
		interpolator.ClearHistory();
		interpolator.AddToHead( flChangeTime - 0.15f, &vecCurOrigin, false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void C_TFDroppedWeapon::DisplayHintTo( C_BasePlayer *pPlayer )
{
	C_TFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if ( pTFPlayer->IsPlayerClass( TF_CLASS_ENGINEER ) )
	{
		pTFPlayer->HintMessage( HINT_ENGINEER_PICKUP_METAL );
	}
	else
	{
		pTFPlayer->HintMessage( HINT_PICKUP_AMMO );
	}
}
