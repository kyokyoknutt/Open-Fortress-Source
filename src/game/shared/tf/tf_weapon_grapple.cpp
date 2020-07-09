//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the grapple hook weapon.
//			
//			Primary attack: fires a beam that hooks on a surface.
//			Secondary attack: switches between pull and rapple modes
//
//
//=============================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "tf_weapon_grapple.h"
 
#ifdef CLIENT_DLL
	#include "c_tf_player.h"
#else
    #include "tf_player.h"
	#include "ammodef.h"
	#include "gamestats.h"
	#include "soundent.h"
	#include "vphysics/constraints.h"
#endif
 
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
 
#define HOOK_MODEL			"models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl"
#define BOLT_MODEL			"models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl"

#define BOLT_AIR_VELOCITY	3500
#define BOLT_WATER_VELOCITY	1500
#define MAX_ROPE_LENGTH		900.f
#define HOOK_PULL			720.f

extern ConVar of_hook_pendulum;

#ifdef CLIENT_DLL

#undef CWeaponGrapple

IMPLEMENT_CLIENTCLASS_DT(C_WeaponGrapple, DT_WeaponGrapple, CWeaponGrapple)
	RecvPropInt(RECVINFO(m_iAttached)),
	RecvPropInt(RECVINFO(m_nBulletType)),
	RecvPropEHandle(RECVINFO(m_hHook)),
END_NETWORK_TABLE()

#define CWeaponGrapple C_WeaponGrapple

#else
IMPLEMENT_SERVERCLASS_ST(CWeaponGrapple, DT_WeaponGrapple)
	SendPropInt( SENDINFO( m_iAttached ) ),
	SendPropInt( SENDINFO ( m_nBulletType ) ),
	SendPropEHandle( SENDINFO( m_hHook ) ),
END_NETWORK_TABLE()
#endif

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponGrapple )
	DEFINE_PRED_FIELD( m_iAttached, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif
 
LINK_ENTITY_TO_CLASS( tf_weapon_grapple, CWeaponGrapple );


//**************************************************************************
//GRAPPLING WEAPON
 
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponGrapple::CWeaponGrapple( void )
{
	m_flNextPrimaryAttack = 0.f;
	m_bReloadsSingly	  = true;
	m_bFiresUnderwater	  = true;
	m_iAttached			  = 0;
	m_nBulletType		  = -1;
	
#ifdef GAME_DLL
	m_hHook			= NULL;
	m_pLightGlow	= NULL;
	pBeam			= NULL;
#endif
}
  
//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CWeaponGrapple::Precache( void )
{
#ifdef GAME_DLL
	UTIL_PrecacheOther( "grapple_hook" );
#endif
 
	PrecacheModel( "cable/cable_red.vmt" );
 	PrecacheModel( "cable/cable_blue.vmt" );
	PrecacheModel( "cable/cable_purple.vmt" );

	BaseClass::Precache();
}
 
void CWeaponGrapple::PrimaryAttack(void)
{
	// Can't have an active hook out
	if (m_hHook)
		return;

	CTFPlayer *pPlayer = ToTFPlayer(GetOwner());
	if (!pPlayer)
		return;

#ifdef GAME_DLL
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());

	//Obligatory for MP so the sound can be played
	CDisablePredictionFiltering disabler;
	WeaponSound( SINGLE );

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_GRAPPLE_FIRE_START );

	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner() );

	Vector vecSrc;
	Vector vecOffset(30.f, 4.f, -6.0f);
	QAngle angle;
	GetProjectileFireSetup(pPlayer, vecOffset, &vecSrc, &angle, false);

	//fire direction
	Vector vecDir;
	AngleVectors(angle, &vecDir);
	VectorNormalize(vecDir);

	//Gets the position where the hook will hit
	Vector vecEnd = vecSrc + (vecDir * MAX_TRACE_LENGTH);	

	//Traces a line between the two vectors
	trace_t tr;
	UTIL_TraceLine( vecSrc, vecEnd, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

	//A hook that is not fired out of your face, what a mindblowing concept!
	CGrappleHook *pHook = CGrappleHook::HookCreate(vecSrc, angle, this);

	//Set hook velocity and angle
	float vel = pPlayer->GetWaterLevel() == 3 ? BOLT_WATER_VELOCITY : BOLT_AIR_VELOCITY;
	Vector HookVelocity = vecDir * vel;
	pHook->SetAbsVelocity(HookVelocity);
	VectorAngles(HookVelocity, angle); //reuse already allocated QAngle
	SetAbsAngles(angle);

	m_hHook = pHook;

	//Initialize the beam
	DrawBeam(m_hHook->GetAbsOrigin());
#endif

	m_flNextPrimaryAttack = gpGlobals->curtime - 1.f;

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration(ACT_VM_PRIMARYATTACK));
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrapple::Reload(void)
{
	//Redraw the weapon
	SendWeaponAnim(ACT_VM_IDLE); //ACT_VM_RELOAD
	//Update our times
	m_flNextPrimaryAttack = gpGlobals->curtime + 1.f;
	//Mark this as done
	m_iAttached = false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::ItemPostFrame(void)
{
	if (!CanAttack())
		return;

	//Enforces being able to use PrimaryAttack and Secondary Attack
	CTFPlayer *pPlayer = ToTFPlayer(GetOwner());

	if (!pPlayer || !pPlayer->IsAlive())
	{
		RemoveHook();
		return;
	}

	if (pPlayer->IsAlive() && pPlayer->m_nButtons & IN_ATTACK)
	{
		if (m_flNextPrimaryAttack < gpGlobals->curtime)
			PrimaryAttack();
	}
	else
	{
		if (m_iAttached)
			Reload();
	}

	/*
	if (pPlayer->IsAlive() && (pPlayer->m_nButtons & IN_ATTACK2) && m_iAttached && pPlayer->m_Shared.GetPullSpeed() && pPlayer->GetWaterLevel() < WL_Feet)
		SecondaryAttack();
	*/

	CBaseEntity *Hook = NULL;
#ifdef GAME_DLL
	Hook = m_hHook;

	//Update the beam depending on the hook position
	if (pBeam && !m_iAttached)
	{
		//Set where it ends
		pBeam->PointEntInit(m_hHook->GetAbsOrigin(), this);
		pBeam->SetEndAttachment(LookupAttachment("muzzle"));
	}
#else
	Hook = m_hHook.Get();
#endif

	if (Hook)
	{
		if (!(pPlayer->m_nButtons & IN_ATTACK))	//player let go the attack button
		{
			RemoveHook();
		}
		else if (m_iAttached) //hook is attached to a surface
		{
			if ((Hook->GetAbsOrigin() - pPlayer->GetAbsOrigin()).Length() <= 100.f ||	//player is very close to the attached hook			
				(m_iAttached == 1 && pPlayer->GetGroundEntity()))						//player touched the ground while swinging
			{
				RemoveHook();
			}
			else if (m_iAttached == 2) //notify player how it should behave
			{
				pPlayer->m_Shared.SetHook(Hook);

				//player velocity
				Vector pVel = pPlayer->GetAbsVelocity();

				//rope vector
				Vector playerCenter = pPlayer->WorldSpaceCenter() - (pPlayer->WorldSpaceCenter() - pPlayer->GetAbsOrigin()) * .25;
				playerCenter += (pPlayer->EyePosition() - playerCenter) * 0.5;
				Vector rope = Hook->GetAbsOrigin() - pPlayer->GetAbsOrigin();

				if (!of_hook_pendulum.GetBool())
				{
					VectorNormalize(rope);
					rope = rope * HOOK_PULL;

					//Resulting velocity
					Vector newVel = pVel + rope;
					float velLength = max(pVel.Length() + 200.f, HOOK_PULL);
					float newVelLength = clamp(newVel.Length(), HOOK_PULL, velLength);

					pPlayer->m_Shared.SetHookProperty(newVelLength);
				}
				else
				{
					pPlayer->m_Shared.SetHookProperty(rope.Length());
				}

				m_iAttached = 1;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
/*
void CWeaponGrapple::SecondaryAttack(void)
{
	CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());
	if (!pPlayer)
		return;

	//signal player it should swing
	pPlayer->m_Shared.SetPullSpeed(0.f);
}
*/

void CWeaponGrapple::RemoveHook(void)
{
#ifdef GAME_DLL
	m_hHook->SetTouch(NULL);
	m_hHook->SetThink(NULL);

	UTIL_Remove(m_hHook);
#endif

	NotifyHookDied();

	CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());

	if (pPlayer)
	{
		pPlayer->m_Shared.SetHook(NULL);
		pPlayer->m_Shared.SetHookProperty(0.f);
	}
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponGrapple::CanHolster(void)
{
	//Can't have an active hook out
	if (m_hHook)
		RemoveHook();

	return BaseClass::CanHolster();
}

bool CWeaponGrapple::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_hHook )
		RemoveHook();
 
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::Drop( const Vector &vecVelocity )
{
	if (m_hHook)
		RemoveHook();
 
	BaseClass::Drop( vecVelocity );
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponGrapple::HasAnyAmmo( void )
{
	if (m_hHook)
		RemoveHook();
 
	return BaseClass::HasAnyAmmo();
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::NotifyHookDied( void )
{
#ifdef GAME_DLL
	if ( pBeam )
	{
		UTIL_Remove( pBeam ); //Kill beam
		pBeam = NULL;

		UTIL_Remove( m_pLightGlow ); //Kill sprite
		m_pLightGlow = NULL;
	}
#endif

	//force a reload after the hook is removed
	m_hHook = NULL;
	Reload();
}

void CWeaponGrapple::NotifyHookAttached(void)
{
	m_iAttached = 2;
}

//-----------------------------------------------------------------------------
// Purpose: Draws a beam
// Input  : &startPos - where the beam should begin
//          &endPos - where the beam should end
//          width - what the diameter of the beam should be (units?)
//-----------------------------------------------------------------------------
void CWeaponGrapple::DrawBeam(const Vector &endPos, const float width)
{
#ifdef GAME_DLL
	//Draw the main beam shaft
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
		return;
	
	//Pick cable color
	if (pPlayer->GetTeamNumber() == TF_TEAM_RED)
		pBeam = CBeam::BeamCreate("cable/cable_red.vmt", width);
	else if (pPlayer->GetTeamNumber() == TF_TEAM_BLUE)
		pBeam = CBeam::BeamCreate("cable/cable_blue.vmt", width);
	else
		pBeam = CBeam::BeamCreate("cable/cable_purple.vmt", width);

	//Set where it ends
	pBeam->PointEntInit( endPos, this );
	pBeam->SetEndAttachment(LookupAttachment("muzzle"));

	pBeam->SetWidth(width);

	// Higher brightness means less transparent
	pBeam->SetBrightness(255);
	pBeam->RelinkBeam();

	//Sets scrollrate of the beam sprite 
	float scrollOffset = gpGlobals->curtime + 5.5;
	pBeam->SetScrollRate(scrollOffset);
	
	UpdateWaterState();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - used to figure out where to do the effect
//          nDamageType - ???
//-----------------------------------------------------------------------------
void CWeaponGrapple::DoImpactEffect( trace_t &tr, int nDamageType )
{
#ifdef GAME_DLL
	if ( !(tr.surface.flags & SURF_SKY) )
	{
		CPVSFilter filter( tr.endpos );
		te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );
		m_nBulletType = GetAmmoDef()->Index("GaussEnergy");
		UTIL_ImpactTrace( &tr, m_nBulletType );
	}
#endif
}

//**************************************************************************
//HOOK

#ifdef GAME_DLL

LINK_ENTITY_TO_CLASS(grapple_hook, CGrappleHook);

BEGIN_DATADESC(CGrappleHook)
	DEFINE_THINKFUNC(FlyThink),
	DEFINE_FUNCTION(HookTouch),
	DEFINE_FIELD(m_hPlayer, FIELD_EHANDLE),
	DEFINE_FIELD(m_hOwner, FIELD_EHANDLE),
END_DATADESC()

CGrappleHook *CGrappleHook::HookCreate(const Vector &vecOrigin, const QAngle &angAngles, CBaseEntity *pentOwner)
{
	CGrappleHook *pHook = (CGrappleHook *)CreateEntityByName("grapple_hook");
	UTIL_SetOrigin(pHook, vecOrigin);
	pHook->SetAbsAngles(angAngles);
	pHook->Spawn();

	CWeaponGrapple *pOwner = (CWeaponGrapple *)pentOwner;
	pHook->m_hOwner = pOwner;
	pHook->SetOwnerEntity(pOwner->GetOwner());
	pHook->m_hPlayer = (CTFPlayer *)pOwner->GetOwner();

	return pHook;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGrappleHook::~CGrappleHook(void)
{
	// Revert Jay's gai flag
	if (m_hPlayer)
		m_hPlayer->SetPhysicsFlag(PFLAG_VPHYSICS_MOTIONCONTROLLER, false);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGrappleHook::CreateVPhysics(void)
{
	// Create the object in the physics system
	VPhysicsInitNormal(SOLID_BBOX, FSOLID_NOT_STANDABLE, false);

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int CGrappleHook::PhysicsSolidMaskForEntity() const
{
	return (BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX) & ~CONTENTS_GRATE;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CGrappleHook::Spawn(void)
{
	Precache();

	SetModel(HOOK_MODEL);
	SetMoveType(MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM);
	UTIL_SetSize(this, -Vector(1, 1, 1), Vector(1, 1, 1));
	SetSolid(SOLID_BBOX);
	SetGravity(0.05f);

	// The rock is invisible, the crossbow bolt is the visual representation
	AddEffects(EF_NODRAW);

	// Make sure we're updated if we're underwater
	UpdateWaterState();

	// Create bolt model and parent it
	CBaseEntity *pBolt = CBaseEntity::CreateNoSpawn("prop_dynamic", GetAbsOrigin(), GetAbsAngles(), this);
	pBolt->SetModelName(MAKE_STRING(BOLT_MODEL));
	pBolt->SetModel(BOLT_MODEL);
	DispatchSpawn(pBolt);
	pBolt->SetParent(this);

	SetTouch(&CGrappleHook::HookTouch);
	SetThink(&CGrappleHook::FlyThink);
	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CGrappleHook::Precache(void)
{
	PrecacheModel(HOOK_MODEL);
	PrecacheModel(BOLT_MODEL);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrappleHook::FlyThink(void)
{
	if ((GetAbsOrigin() - m_hOwner->GetAbsOrigin()).Length() >= MAX_ROPE_LENGTH)
	{
		m_hOwner->NotifyHookDied();
		SetTouch(NULL);
		SetThink(NULL);
		UTIL_Remove(this);
		return;
	}

	SetNextThink(gpGlobals->curtime + 0.1f);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CGrappleHook::HookTouch(CBaseEntity *pOther)
{
	if (!pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS))
	{
		m_hOwner->NotifyHookDied();
		return;
	}

	if (pOther != m_hOwner && pOther->m_takedamage != DAMAGE_NO)
	{
		m_hOwner->NotifyHookDied();
		SetTouch(NULL);
		SetThink(NULL);
		UTIL_Remove(this);
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if (pOther->GetMoveType() == MOVETYPE_NONE && !(tr.surface.flags & SURF_SKY))
		{
			SetAbsVelocity(Vector(0.f, 0.f, 0.f));
			EmitSound("Weapon_AR2.Reload_Push");

			Vector vForward;

			AngleVectors(GetAbsAngles(), &vForward);
			VectorNormalize(vForward);

			/*
			CEffectData	data;

			data.m_vOrigin = tr.endpos;
			data.m_vNormal = vForward;
			data.m_nEntIndex = 0;

			DispatchEffect( "Impact", data );
			*/

			//	AddEffects( EF_NODRAW );

			SetTouch(NULL);
			SetThink(NULL);

			VPhysicsDestroyObject();
			VPhysicsInitNormal(SOLID_VPHYSICS, FSOLID_NOT_STANDABLE, false);
			AddSolidFlags(FSOLID_NOT_SOLID);

			if (!m_hPlayer)
				return;

			// Set Jay's gai flag
			m_hPlayer->SetPhysicsFlag(PFLAG_VPHYSICS_MOTIONCONTROLLER, true);

			m_hOwner->NotifyHookAttached();
			m_hPlayer->DoAnimationEvent(PLAYERANIMEVENT_CUSTOM, ACT_GRAPPLE_PULL_START);
		}
		else
		{
			m_hOwner->NotifyHookDied();
			SetTouch(NULL);
			SetThink(NULL);
			UTIL_Remove(this);
		}
	}
}
#endif