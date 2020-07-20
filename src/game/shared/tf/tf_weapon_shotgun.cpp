//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_weapon_shotgun.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_tf_player.h"
#else
	#include "tf_player.h"
	#include "gamestats.h"
#endif

#define CREATE_SIMPLE_WEAPON_TABLE( WpnName, entityname )			\
																	\
	IMPLEMENT_NETWORKCLASS_ALIASED( WpnName, DT_##WpnName )	\
															\
	BEGIN_NETWORK_TABLE( C##WpnName, DT_##WpnName )			\
	END_NETWORK_TABLE()										\
															\
	BEGIN_PREDICTION_DATA( C##WpnName )						\
	END_PREDICTION_DATA()									\
															\
	LINK_ENTITY_TO_CLASS( entityname, C##WpnName );			\
	//PRECACHE_WEAPON_REGISTER( entityname );

#define CREATE_SIMPLE_WEAPON_TABLE_OLD(WpnName, entityname)			    \
																	\
	IMPLEMENT_NETWORKCLASS_ALIASED( ##WpnName##, DT_##WpnName## )	\
																	\
	BEGIN_NETWORK_TABLE( C##WpnName##, DT_##WpnName## )				\
	END_NETWORK_TABLE()												\
																	\
	BEGIN_PREDICTION_DATA( C##WpnName## )							\
	END_PREDICTION_DATA()											\
																	\
	LINK_ENTITY_TO_CLASS( ##entityname##, C##WpnName## );			\
	//PRECACHE_WEAPON_REGISTER( ##entityname## );
	

//=============================================================================
//
// Weapon Shotgun tables.
//

CREATE_SIMPLE_WEAPON_TABLE( TFShotgun, tf_weapon_shotgun )
CREATE_SIMPLE_WEAPON_TABLE( TFSuperShotgun, tf_weapon_supershotgun )
CREATE_SIMPLE_WEAPON_TABLE( TFScatterGun, tf_weapon_scattergun )
CREATE_SIMPLE_WEAPON_TABLE( TFCShotgunSB, tfc_weapon_shotgun_sb )
CREATE_SIMPLE_WEAPON_TABLE( TFCShotgunDB, tfc_weapon_shotgun_db )

//=============================================================================
//
// Weapon Shotgun functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFShotgun::CTFShotgun()
{
	m_bReloadsSingly = true;
}

CTFSuperShotgun::CTFSuperShotgun()
{
	m_bReloadsSingly = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFShotgun::PrimaryAttack()
{
	if ( !CanAttack() )
		return;

	// Set the weapon mode.
	m_iWeaponMode = TF_WEAPON_PRIMARY_MODE;

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFShotgun::UpdatePunchAngles( CTFPlayer *pPlayer )
{
	// Update the player's punch angle.
	QAngle angle = pPlayer->GetPunchAngle();
	float flPunchAngle = m_pWeaponInfo->GetWeaponData( m_iWeaponMode ).m_flPunchAngle;
	angle.x -= SharedRandomInt( "ShotgunPunchAngle", ( flPunchAngle - 1 ), ( flPunchAngle + 1 ) );
	pPlayer->SetPunchAngle( angle );
}


//Act tables for Merc
acttable_t CTFShotgun::m_acttableShotgun[] =
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_PRIMARY,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_PRIMARY,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_PRIMARY,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_PRIMARY,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_PRIMARY,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_PRIMARY,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_PRIMARY,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_PRIMARY,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_PRIMARY,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_PRIMARY,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_PRIMARY,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_PRIMARY,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_PRIMARY,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_PRIMARY,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_PRIMARY,	false },

	{ ACT_MP_RELOAD_STAND,		ACT_MP_RELOAD_STAND_PRIMARY,		false },
	{ ACT_MP_RELOAD_STAND_LOOP,	ACT_MP_RELOAD_STAND_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_STAND_END,	ACT_MP_RELOAD_STAND_PRIMARY_END,	false },
	{ ACT_MP_RELOAD_CROUCH,		ACT_MP_RELOAD_CROUCH_PRIMARY,		false },
	{ ACT_MP_RELOAD_CROUCH_LOOP,ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP,false },
	{ ACT_MP_RELOAD_CROUCH_END,	ACT_MP_RELOAD_CROUCH_PRIMARY_END,	false },
	{ ACT_MP_RELOAD_SWIM,		ACT_MP_RELOAD_SWIM_PRIMARY,		false },
	{ ACT_MP_RELOAD_SWIM_LOOP,	ACT_MP_RELOAD_SWIM_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_SWIM_END,	ACT_MP_RELOAD_SWIM_PRIMARY_END,	false },
	{ ACT_MP_RELOAD_AIRWALK,	ACT_MP_RELOAD_AIRWALK_PRIMARY,	false },
	{ ACT_MP_RELOAD_AIRWALK_LOOP,	ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP,	false },
	{ ACT_MP_RELOAD_AIRWALK_END,ACT_MP_RELOAD_AIRWALK_PRIMARY_END,false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_PRIMARY, false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_PRIMARY,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_PRIMARY,	false },
};

acttable_t CTFSuperShotgun::m_acttableSuperShotgun[] =
{
	{ ACT_MP_STAND_IDLE, ACT_MERC_STAND_SUPERSHOTGUN, false },
	{ ACT_MP_CROUCH_IDLE, ACT_MERC_CROUCH_SUPERSHOTGUN, false },
	{ ACT_MP_RUN, ACT_MERC_RUN_SUPERSHOTGUN, false },
	{ ACT_MP_WALK, ACT_MERC_WALK_SUPERSHOTGUN, false },
	{ ACT_MP_AIRWALK, ACT_MERC_AIRWALK_SUPERSHOTGUN, false },
	{ ACT_MP_CROUCHWALK, ACT_MERC_CROUCHWALK_SUPERSHOTGUN, false },
	{ ACT_MP_JUMP, ACT_MERC_JUMP_SUPERSHOTGUN, false },
	{ ACT_MP_JUMP_START, ACT_MERC_JUMP_START_SUPERSHOTGUN, false },
	{ ACT_MP_JUMP_FLOAT, ACT_MERC_JUMP_FLOAT_SUPERSHOTGUN, false },
	{ ACT_MP_JUMP_LAND, ACT_MERC_JUMP_LAND_SUPERSHOTGUN, false },
	{ ACT_MP_SWIM, ACT_MERC_SWIM_SUPERSHOTGUN, false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE, ACT_MERC_ATTACK_STAND_SUPERSHOTGUN, false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE, ACT_MERC_ATTACK_CROUCH_SUPERSHOTGUN, false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE, ACT_MERC_ATTACK_SWIM_SUPERSHOTGUN, false },

	{ ACT_MP_RELOAD_STAND, ACT_MERC_RELOAD_STAND_SUPERSHOTGUN, false },
	{ ACT_MP_RELOAD_CROUCH, ACT_MERC_RELOAD_CROUCH_SUPERSHOTGUN, false },
	{ ACT_MP_RELOAD_SWIM, ACT_MERC_RELOAD_SWIM_SUPERSHOTGUN, false },
};

//Act table remapping
acttable_t *CTFShotgun::ActivityList( int &iActivityCount )
{
	// spy shotgun spy shotgun spy shotgun
	if ( GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_SPY )
	{
		iActivityCount = ARRAYSIZE(m_acttableShotgun);
		return m_acttableShotgun;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}

acttable_t *CTFSuperShotgun::ActivityList(int &iActivityCount)
{
	if (GetTFPlayerOwner()->GetPlayerClass()->GetClassIndex() == TF_CLASS_MERCENARY)
	{
		iActivityCount = ARRAYSIZE(m_acttableSuperShotgun);
		return m_acttableSuperShotgun;
	}
	else
	{
		return BaseClass::ActivityList(iActivityCount);
	}
}

//**************************************************************************
//
// ETERNAL SHOTGUN
//
//**************************************************************************

#define BOLT_AIR_VELOCITY	3500
#define BOLT_WATER_VELOCITY	1500
#define HOOK_PULL			720.f

IMPLEMENT_NETWORKCLASS_ALIASED(TFEternalShotgun, DT_EternalShotgun)

BEGIN_NETWORK_TABLE( CTFEternalShotgun, DT_EternalShotgun )
#ifdef CLIENT_DLL
	RecvPropInt(RECVINFO(m_iAttached)),
	RecvPropBool(RECVINFO(m_bCanRefire)),
	RecvPropEHandle(RECVINFO(m_hHook)),
#else
	SendPropInt(SENDINFO(m_iAttached)),
	SendPropBool(SENDINFO(m_bCanRefire)),
	SendPropEHandle(SENDINFO(m_hHook)),
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFEternalShotgun )
#ifdef CLIENT_DLL
	DEFINE_PRED_FIELD(m_iAttached, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
#endif
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_eternalshotgun, CTFEternalShotgun );

CTFEternalShotgun::CTFEternalShotgun(void)
{
	m_flNextSecondaryAttack = 0.f;
	m_bReloadsSingly = false;
	m_bFiresUnderwater = true;
	m_iAttached = 0;
	m_bCanRefire = true;

#ifdef GAME_DLL
	m_hHook = NULL;
	pBeam = NULL;
#endif
}

CTFEternalShotgun::~CTFEternalShotgun(void)
{
	RemoveHook();
}

void CTFEternalShotgun::Precache(void)
{
#ifdef GAME_DLL
	UTIL_PrecacheOther("grapple_hook");
#endif
	PrecacheModel("cable/cable_grey.vmt");

	BaseClass::Precache();
}

bool CTFEternalShotgun::CanHolster(void) const
{
	CBaseEntity *pHook = NULL;
#ifdef GAME_DLL
	pHook = m_hHook;
#else
	pHook = m_hHook.Get();
#endif
	if( pHook )
		return false;

	return BaseClass::CanHolster();
}

bool CTFEternalShotgun::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	if (GetHookEntity())
		RemoveHook();

	return BaseClass::Holster(pSwitchingTo);
}

void CTFEternalShotgun::Drop(const Vector &vecVelocity)
{
	if (GetHookEntity())
		return;

	BaseClass::Drop(vecVelocity);
}

void CTFEternalShotgun::ItemPostFrame()
{
	CBaseEntity *pHook = GetHookEntity();

#ifdef GAME_DLL
	if (pHook)
	{
		Vector hookPos = pHook->GetAbsOrigin();
		hookPos += (pHook->EyePosition() - hookPos) * 0.75; //looks better than 0.5 on hooked players

		//Update the beam depending on the hook position
		if (pBeam)
		{
			//Set where it ends
			pBeam->PointEntInit(hookPos, this);
			pBeam->SetEndAttachment(LookupAttachment("muzzle"));
		}

		//Invalidate hook if it is not in sight
		if (m_iAttached && !HookLOS(hookPos))
			RemoveHook();
	}
#endif

	CTFPlayer *pPlayer = ToTFPlayer(GetOwner());
	if (!pPlayer || !pPlayer->IsAlive() || ( m_iAttached && ToTFPlayer(pHook) && !ToTFPlayer(pHook)->IsAlive() ) )
	{
		RemoveHook();
		return;
	}

	if(pHook)
	{
		if (m_iAttached) //hook is attached to something
		{
			if ((pHook->GetAbsOrigin() - pPlayer->GetAbsOrigin()).Length() <= 100.f)
				RemoveHook();
			else if (m_iAttached == 2) //notify player how it should behave
				InitiateHook(pPlayer, pHook);
		}
	}

	BaseClass::ItemPostFrame();

	if (!(pPlayer->m_nButtons & IN_ATTACK2))
		m_bCanRefire = true;
}

void CTFEternalShotgun::PrimaryAttack()
{
	BaseClass::PrimaryAttack();

	CBaseEntity *pHook = GetHookEntity();

	if( pHook )
		RemoveHook();
}

void CTFEternalShotgun::SecondaryAttack()
{
	if (!CanAttack())
		return;

	CTFPlayer *pOwner = ToTFPlayer(GetPlayerOwner());
	if (!pOwner)
		return;

	if (!m_bCanRefire)
		return;

	CBaseEntity *pOldHook = GetHookEntity();
	
	// Can't have an active hook out
	if (pOldHook)
	{
		RemoveHook();
		m_bCanRefire = false;
		return;
	}

	CTFPlayer *pPlayer = ToTFPlayer(GetOwner());
	if (!pPlayer)
		return;

#ifdef GAME_DLL
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());

	bool bCenter = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_bCenterfireProjectile;
	int iQuakeCvar = 0;

	if (!pPlayer->IsFakeClient())
		iQuakeCvar = V_atoi(engine->GetClientConVarValue(pPlayer->entindex(), "viewmodel_centered"));

	//Obligatory for MP so the sound can be played
	CDisablePredictionFiltering disabler;
	WeaponSound(SINGLE);

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_GRAPPLE_FIRE_START);

	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);

	//CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner());

	Vector vecSrc;
	Vector vecOffset(30.f, 4.f, -6.0f);
	if (bCenter || iQuakeCvar)
	{
		vecOffset.x = 12.0f; //forward backwards
		vecOffset.y = 0.0f; // left right
		vecOffset.z = -8.0f; //up down
	}
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
	UTIL_TraceLine(vecSrc, vecEnd, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

	//A hook that is not fired out of your face, what a mindblowing concept!
	CTFMeatHook *pHook = CTFMeatHook::HookCreate(vecSrc, angle, this);

	//Set hook velocity and angle
	float vel = pPlayer->GetWaterLevel() == 3 ? BOLT_WATER_VELOCITY : BOLT_AIR_VELOCITY;
	Vector HookVelocity = vecDir * vel;
	pHook->SetAbsVelocity(HookVelocity);
	VectorAngles(HookVelocity, angle); //reuse already allocated QAngle
	SetAbsAngles(angle);

	m_hHook = pHook;

	//Initialize the beam
	DrawBeam(m_hHook->GetAbsOrigin(), 1.f);
#endif

	m_bCanRefire = false;
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.f;
}

void CTFEternalShotgun::InitiateHook(CTFPlayer *pPlayer, CBaseEntity *pHook)
{
	//Set hook owner player pointer
	pPlayer->m_Shared.SetHook(pHook);

	//Tell target it has been hooked
	ToTFPlayer(pHook)->m_Shared.AddCond(TF_COND_HOOKED);

	//player velocity
	Vector pVel = pPlayer->GetAbsVelocity();

	//rope vector
	Vector playerCenter = pPlayer->WorldSpaceCenter() - (pPlayer->WorldSpaceCenter() - pPlayer->GetAbsOrigin()) * .25;
	playerCenter += (pPlayer->EyePosition() - playerCenter) * 0.5;
	Vector pRope = pHook->GetAbsOrigin() - pPlayer->GetAbsOrigin();

	pPlayer->SetGroundEntity(NULL);

	VectorNormalize(pRope);
	pRope = pRope * HOOK_PULL;

	//Resulting velocity
	Vector newVel = pVel + pRope;
	float velLength = max(pVel.Length() + 200.f, HOOK_PULL);
	float newVelLength = clamp(newVel.Length(), HOOK_PULL, velLength);

	pPlayer->m_Shared.SetHookProperty(newVelLength);

	m_iAttached = 1;
}

void CTFEternalShotgun::RemoveHook(void)
{
	CTFPlayer *pHook = ToTFPlayer(GetHookEntity());

	if (pHook)
		pHook->m_Shared.RemoveCond(TF_COND_HOOKED);

#ifdef GAME_DLL
	//hook is an actual hook not a player
	if (m_hHook && pHook == NULL)
	{
		m_hHook->SetTouch(NULL);
		m_hHook->SetThink(NULL);
		UTIL_Remove(m_hHook);
	}

	//Kill beam
	if (pBeam)
	{
		UTIL_Remove(pBeam);
		pBeam = NULL;
	}
#endif

	CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());

	if (pPlayer)
	{
		pPlayer->m_Shared.SetHook(NULL);
		pPlayer->m_Shared.SetHookProperty(0.f);
	}

	m_hHook = NULL;
	m_flNextSecondaryAttack = gpGlobals->curtime + 1.f;
	m_iAttached = 0;
}

CBaseEntity *CTFEternalShotgun::GetHookEntity()
{
#ifdef GAME_DLL
	return m_hHook;
#else
	return m_hHook.Get();
#endif
}

#ifdef GAME_DLL
void CTFEternalShotgun::NotifyHookAttached(CBaseEntity *pTarget)
{
	m_iAttached = 2;
	//transfer pointer
	m_hHook = pTarget;
}

bool CTFEternalShotgun::HookLOS(Vector hookPos)
{
	CBaseEntity *player = GetOwner();

	if (!player)
		return false;

	Vector playerCenter = player->GetAbsOrigin();
	playerCenter += (player->EyePosition() - playerCenter) * 0.5;

	trace_t tr;
	UTIL_TraceLine(hookPos, playerCenter, MASK_ALL, m_hHook, COLLISION_GROUP_NONE, &tr);

	//Msg("%f %f %f %f\n", hookPos.Length(), playerCenter.Length(), tr.endpos.Length(), (tr.endpos - playerCenter).Length());

	return (tr.endpos - playerCenter).Length() < 40.f;
}

void CTFEternalShotgun::DrawBeam(const Vector &endPos, const float width)
{
	//Draw the main beam shaft
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
		return;

	//Pick cable color
	pBeam = CBeam::BeamCreate("cable/cable_grey.vmt", width);

	//Set where it ends
	pBeam->PointEntInit(endPos, this);
	pBeam->SetEndAttachment(LookupAttachment("muzzle"));

	pBeam->SetWidth(width);

	// Higher brightness means less transparent
	pBeam->SetBrightness(255);
	pBeam->RelinkBeam();

	//Sets scrollrate of the beam sprite 
	float scrollOffset = gpGlobals->curtime + 5.5;
	pBeam->SetScrollRate(scrollOffset);

	UpdateWaterState();
}

#endif

#ifdef GAME_DLL

#define HOOK_MODEL			"models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl"
#define BOLT_MODEL			"models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl"
#define MAX_ROPE_LENGTH		900.f

LINK_ENTITY_TO_CLASS( tf_meat_hook, CTFMeatHook );

BEGIN_DATADESC(CTFMeatHook)
	DEFINE_THINKFUNC(FlyThink),
	DEFINE_FUNCTION(HookTouch),
	DEFINE_FIELD(m_hOwner, FIELD_EHANDLE),
END_DATADESC()

CTFMeatHook *CTFMeatHook::HookCreate(const Vector &vecOrigin, const QAngle &angAngles, CBaseEntity *pEntOwner)
{
	CTFMeatHook *pHook = (CTFMeatHook *)CreateEntityByName("tf_meat_hook");
	UTIL_SetOrigin(pHook, vecOrigin);
	pHook->SetAbsAngles(angAngles);
	pHook->Spawn();

	CTFEternalShotgun *pOwner = (CTFEternalShotgun *)pEntOwner;
	pHook->m_hOwner = pOwner;
	pHook->SetOwnerEntity(pOwner->GetOwner());

	return pHook;
}

CTFMeatHook::~CTFMeatHook(void)
{
	CTFPlayer *pOwner = (CTFPlayer *)GetOwnerEntity();
	if (!pOwner)
		return;

	if (pOwner)
		pOwner->SetPhysicsFlag(PFLAG_VPHYSICS_MOTIONCONTROLLER, false);
}

void CTFMeatHook::Spawn(void)
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

	SetTouch(&CTFMeatHook::HookTouch);
	SetThink(&CTFMeatHook::FlyThink);
	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CTFMeatHook::Precache(void)
{
	PrecacheModel(HOOK_MODEL);
	PrecacheModel(BOLT_MODEL);
}

unsigned int CTFMeatHook::PhysicsSolidMaskForEntity() const
{
	return (BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX) & ~CONTENTS_GRATE;
}

bool CTFMeatHook::CreateVPhysics(void)
{
	// Create the object in the physics system
	VPhysicsInitNormal(SOLID_BBOX, FSOLID_NOT_STANDABLE, false);

	return true;
}

void CTFMeatHook::FlyThink(void)
{
	if (!m_hOwner)
	{
		SetThink(NULL);
		SetTouch(NULL);
		UTIL_Remove(this);
		return;
	}

	if ((GetAbsOrigin() - m_hOwner->GetAbsOrigin()).Length() >= MAX_ROPE_LENGTH)
	{
		m_hOwner->RemoveHook();
		return;
	}

	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CTFMeatHook::HookTouch(CBaseEntity *pOther)
{
	if (pOther == m_hOwner || !pOther->IsPlayer() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) || !m_hOwner->HookLOS(GetAbsOrigin()) || !GetOwnerEntity())
	{
		m_hOwner->RemoveHook();
		return;
	}

	//hooked an entity that can be damaged
	SetMoveType(MOVETYPE_NONE);
	EmitSound("Weapon_AR2.Reload_Push");

	SetTouch(NULL);
	SetThink(NULL);

	VPhysicsDestroyObject();
	VPhysicsInitNormal(SOLID_VPHYSICS, FSOLID_NOT_STANDABLE, false);
	AddSolidFlags(FSOLID_NOT_SOLID);

	CTFPlayer *pOwner = (CTFPlayer *)GetOwnerEntity();
	pOwner->SetPhysicsFlag(PFLAG_VPHYSICS_MOTIONCONTROLLER, true);
	pOwner->DoAnimationEvent(PLAYERANIMEVENT_CUSTOM, ACT_GRAPPLE_PULL_START);

	CTFPlayer *pHooked = ToTFPlayer(pOther);
	m_hOwner->NotifyHookAttached(pHooked);
	
	UTIL_Remove(this);
}

#endif