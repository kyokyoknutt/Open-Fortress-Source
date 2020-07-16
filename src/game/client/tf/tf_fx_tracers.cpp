//========= Copyright � 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: Game-specific impact effect hooks
//
//=============================================================================
#include "cbase.h"
#include "clientsideeffects.h"
#include "clienteffectprecachesystem.h"
#include "view.h"
#include "collisionutils.h"
#include "engine/IEngineSound.h"
#include "c_tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CLIENTEFFECT_REGISTER_BEGIN( PrecacheTFTracers )
	CLIENTEFFECT_MATERIAL( "effects/spark" )
CLIENTEFFECT_REGISTER_END()

#define LISTENER_HEIGHT 24


#define TRACER_TYPE_FAINT	4

extern ConVar r_drawtracers;
extern ConVar r_drawtracers_firstperson;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FX_TFTracerSound( const Vector &start, const Vector &end, int iTracerType )
{
	// don't play on very short hits
	if ( ( start - end ).Length() < 200 )
		return;
	
	const char *pszSoundName = "Bullets.DefaultNearmiss";
	float flWhizDist = 64;
	Vector vecListenOrigin = MainViewOrigin();

	switch( iTracerType )
	{
	case TRACER_TYPE_DEFAULT:
		flWhizDist = 96;
		// fall through !

	default:
		{
			Ray_t bullet, listener;
			bullet.Init( start, end );

			Vector vecLower = vecListenOrigin;
			vecLower.z -= LISTENER_HEIGHT;
			listener.Init( vecListenOrigin,	vecLower );

			float s, t;
			IntersectRayWithRay( bullet, listener, s, t );
			t = clamp( t, 0, 1 );
			vecListenOrigin.z -= t * LISTENER_HEIGHT;
		}
		break;
	}

	static float flNextWhizTime = 0;

	// Is it time yet?
	float dt = flNextWhizTime - gpGlobals->curtime;
	if ( dt > 0 )
		return;

	// Did the thing pass close enough to our head?
	float vDist = CalcDistanceSqrToLineSegment( vecListenOrigin, start, end );
	if ( vDist >= (flWhizDist * flWhizDist) )
		return;

	CSoundParameters params;
	if( C_BaseEntity::GetParametersForSound( pszSoundName, params, NULL ) )
	{
		// Get shot direction
		Vector shotDir;
		VectorSubtract( end, start, shotDir );
		VectorNormalize( shotDir );

		CLocalPlayerFilter filter;

		enginesound->EmitSound( filter, SOUND_FROM_WORLD, CHAN_STATIC, params.soundname,
			params.volume, SNDLVL_TO_ATTN( params.soundlevel ), 0, params.pitch, 0, &start, &shotDir, NULL, false );
	}

	// Don't play another bullet whiz for this client until this time has run out
	flNextWhizTime = gpGlobals->curtime + 0.1f;
}

Vector GetTracerOriginTF( const CEffectData &data )
{
	Vector vecStart = data.m_vStart;
	QAngle vecAngles;

	int iAttachment = data.m_nAttachmentIndex;

	// Attachment?
	if ( data.m_fFlags & TRACER_FLAG_USEATTACHMENT )
	{
		C_BaseViewModel *pViewModel = NULL;

		// If the entity specified is a weapon being carried by this player, use the viewmodel instead
		IClientRenderable *pRenderable = data.GetRenderable();
		if ( !pRenderable )
			return vecStart;

		C_BaseEntity *pEnt = data.GetEntity();

// This check should probably be for all multiplayer games, investigate later
		if ( pEnt && pEnt->IsDormant() )
			return vecStart;

		C_BaseCombatWeapon *pWpn = dynamic_cast<C_BaseCombatWeapon *>( pEnt );
		if ( pWpn && pWpn->ShouldDrawUsingViewModel() )
		{
			C_BasePlayer *player = ToBasePlayer( pWpn->GetOwner() );

			// Use GetRenderedWeaponModel() instead?
			pViewModel = player ? player->GetViewModel( 0 ) : NULL;
			if ( pViewModel )
			{
				// Get the viewmodel and use it instead
				pRenderable = pViewModel;
			}
		}

		// Get the attachment origin
		if ( !pRenderable->GetAttachment( iAttachment, vecStart, vecAngles ) )
		{
			DevMsg( "GetTracerOrigin: Couldn't find attachment %d on model %s\n", iAttachment, 
				modelinfo->GetModelName( pRenderable->GetModel() ) );
		}
	}

	return vecStart;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void FX_BrightTracer( Vector& start, Vector& end )
{
	//Don't make small tracers
	float dist;
	Vector dir;
	int velocity = 5000;

	VectorSubtract( end, start, dir );
	dist = VectorNormalize( dir );

	// Don't make short tracers.
	float length = random->RandomFloat( 64.0f, 128.0f );
	float life = ( dist + length ) / velocity;	//NOTENOTE: We want the tail to finish its run as well
		
	//Add it
	FX_AddDiscreetLine( start, dir, velocity, length, dist, random->RandomFloat( 1, 3 ), life, "effects/spark" );

	FX_TFTracerSound( start, end, TRACER_TYPE_DEFAULT );	
}

//-----------------------------------------------------------------------------
// Purpose: Bright Tracer for Machine Guns
//-----------------------------------------------------------------------------
void BrightTracerCallback( const CEffectData &data )
{
	FX_BrightTracer( (Vector&)data.m_vStart, (Vector&)data.m_vOrigin );
}

DECLARE_CLIENT_EFFECT( "BrightTracer", BrightTracerCallback );


void ParticleTracerCallback( const CEffectData &data )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	if ( !r_drawtracers.GetBool() )
		return;

	if ( !r_drawtracers_firstperson.GetBool() )
	{
		C_TFPlayer *pPlayer = ToTFPlayer( data.GetEntity() );

		if ( pPlayer && !pPlayer->ShouldDrawThisPlayer() )
			return;
	}

	// Grab the data
	Vector vecStart = GetTracerOriginTF( data );
	Vector vecEnd = data.m_vOrigin;

	// Adjust view model tracers
	C_BaseEntity *pEntity = data.GetEntity();
	if ( data.entindex() && data.entindex() == player->index )
	{
		QAngle	vangles;
		Vector	vforward, vright, vup;

		engine->GetViewAngles( vangles );
		AngleVectors( vangles, &vforward, &vright, &vup );

		VectorMA( data.m_vStart, 4, vright, vecStart );
		vecStart[2] -= 0.5f;
	}

	// Create the particle effect
	QAngle vecAngles;
	Vector vecToEnd = vecEnd - vecStart;
	VectorNormalize(vecToEnd);
	VectorAngles( vecToEnd, vecAngles );
	C_TFPlayer *pPlayer = ToTFPlayer( data.GetEntity() );
	if ( pPlayer/* && pPlayer->GetTeamNumber()== TF_TEAM_MERCENARY*/ )
	{
		CEffectData	datahack = data;
		datahack.m_hEntity = pEntity;
		datahack.m_nHitBox = data.m_nHitBox;
		datahack.m_vStart = vecEnd;
		datahack.m_vOrigin = vecStart;
		datahack.m_vAngles = vecAngles;
		datahack.m_nDamageType = PATTACH_CUSTOMORIGIN;
		datahack.m_fFlags |= PARTICLE_DISPATCH_FROM_ENTITY;
		datahack.m_bCustomColors = true;
		datahack.m_CustomColors.m_vecColor1 = pPlayer->m_vecPlayerColor;

		DispatchEffect( "ParticleEffect", datahack );
	}
	else
		DispatchParticleEffect( data.m_nHitBox, vecStart, vecEnd, vecAngles, pEntity );

	if ( data.m_fFlags & TRACER_FLAG_WHIZ )
	{
		FX_TracerSound( vecStart, vecEnd, TRACER_TYPE_DEFAULT );	
	}
	
}

DECLARE_CLIENT_EFFECT( "ParticleTracer", ParticleTracerCallback );