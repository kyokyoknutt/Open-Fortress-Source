//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: Deathmatch weapon spawner
//
//=============================================================================//

#include "cbase.h"
#include "view.h"
#include "tf_gamerules.h"
// for spy material proxy
#include "functionproxy.h"

#include "tier0/memdbgon.h"

ConVar of_glow_alpha("of_glow_alpha", "2.0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "Determines how transparent the glow effects on weapons and powerups will be.");

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the Weapon Spawner
//-----------------------------------------------------------------------------
class C_WeaponSpawner : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_WeaponSpawner, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_WeaponSpawner();

	void	ClientThink( void );
	void	Spawn( void );
	int		DrawModel( int flags );
	int		GetWeaponID(){ return AliasToWeaponID( m_iszWeaponName ); };
	virtual C_BaseEntity *GetItemTintColorOwner( void )
	{
		return (C_BaseEntity *) C_BasePlayer::GetLocalPlayer();
	}

private:

	QAngle absAngle;
	~C_WeaponSpawner();

	private:

		CGlowObject		   *m_pGlowEffect;
		void	UpdateGlowEffect( void );
		void	DestroyGlowEffect(void);
		bool	m_bDisableSpin;
		bool	m_bDisableShowOutline;
		bool	m_bShouldGlow;
		bool	m_bRespawning;
		bool    m_bSuperWeapon;
		bool	bInitialDelay;
		int		iTeamNum;
		
		float				fl_RespawnTime;
		float				m_flRespawnTick;
		float				fl_RespawnDelay;
		
		char				m_iszWeaponName[128];

	IMaterial	*m_pReturnProgressMaterial_Empty;		// For labels above players' heads.
	IMaterial	*m_pReturnProgressMaterial_Full;
};

extern ConVar cl_flag_return_size;
extern ConVar of_color_r;
extern ConVar of_color_g;
extern ConVar of_color_b;
extern ConVar building_cubemaps;
extern ConVar of_allow_allclass_spawners;

// Inputs.
LINK_ENTITY_TO_CLASS( dm_weapon_spawner, C_WeaponSpawner );

IMPLEMENT_CLIENTCLASS_DT( C_WeaponSpawner, DT_WeaponSpawner, CWeaponSpawner )
	RecvPropBool( RECVINFO( m_bDisableSpin ) ),
	RecvPropBool( RECVINFO( m_bDisableShowOutline ) ),
	RecvPropBool( RECVINFO( m_bRespawning ) ),
	RecvPropBool( RECVINFO( bInitialDelay ) ),
	RecvPropBool( RECVINFO( m_bSuperWeapon ) ),
	RecvPropTime( RECVINFO( fl_RespawnTime ) ),
	RecvPropTime( RECVINFO( m_flRespawnTick ) ),
	RecvPropTime( RECVINFO( fl_RespawnDelay ) ),
	RecvPropString( RECVINFO( m_iszWeaponName ) ),
END_RECV_TABLE()

C_WeaponSpawner::C_WeaponSpawner()
{
}

//-----------------------------------------------------------------------------
// Purpose: Set initial angles 
//-----------------------------------------------------------------------------
void C_WeaponSpawner::Spawn( void )
{
	BaseClass::Spawn();
	absAngle = GetAbsAngles();
	iTeamNum = TEAM_INVALID;

	m_pGlowEffect = new CGlowObject( this, TFGameRules()->GetTeamGlowColor(GetLocalPlayerTeam()), of_glow_alpha.GetFloat(), true, true );

	UpdateGlowEffect();

	ClientThink();
}

//-----------------------------------------------------------------------------
// Purpose: Update angles every think
//-----------------------------------------------------------------------------
void C_WeaponSpawner::ClientThink( void )
{
	if ( !m_bDisableSpin )
	{
		absAngle.y += 90 * gpGlobals->frametime;
		if ( absAngle.y >= 360 )
			absAngle.y -= 360;

		SetAbsAngles( absAngle );
	}
	
	bool bShouldGlow = false;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if( !pPlayer )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
		return;
	}
	
	if  ( 										// Don't glow if
		( !of_allow_allclass_spawners.GetBool() && !pPlayer->GetPlayerClass()->IsClass( TF_CLASS_MERCENARY ) ) ||  // Or we're not merc
		( building_cubemaps.GetBool() 
		) // or we're building cubemaps
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
	
	if ( !m_bRespawning ) 
	{
		if ( m_bSuperWeapon )
			bShouldGlow = true;
		else
		{
			trace_t tr;
			UTIL_TraceLine(GetAbsOrigin(), pPlayer->EyePosition(), MASK_OPAQUE, this, COLLISION_GROUP_NONE, &tr);
			if (tr.fraction == 1.0f)
			{
				bShouldGlow = true;
			}
		}
	}

	if ( m_bShouldGlow != bShouldGlow || (iTeamNum != pPlayer -> GetTeamNumber()) )
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
void C_WeaponSpawner::UpdateGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		if ( !m_bDisableShowOutline && m_bShouldGlow && !building_cubemaps.GetBool() )
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

void C_WeaponSpawner::DestroyGlowEffect(void)
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}

C_WeaponSpawner::~C_WeaponSpawner()
{
	DestroyGlowEffect();
}

typedef struct
{
	float maxProgress;

	float vert1x;
	float vert1y;
	float vert2x;
	float vert2y;

	int swipe_dir_x;
	int swipe_dir_y;
} progress_weapon_segment_t;


// This defines the properties of the 8 circle segments
// in the circular progress bar.
static progress_weapon_segment_t Segments[8] = 
{
	{ 0.125, 0.5, 0.0, 1.0, 0.0, 1, 0 },
	{ 0.25,	 1.0, 0.0, 1.0, 0.5, 0, 1 },
	{ 0.375, 1.0, 0.5, 1.0, 1.0, 0, 1 },
	{ 0.50,	 1.0, 1.0, 0.5, 1.0, -1, 0 },
	{ 0.625, 0.5, 1.0, 0.0, 1.0, -1, 0 },
	{ 0.75,	 0.0, 1.0, 0.0, 0.5, 0, -1 },
	{ 0.875, 0.0, 0.5, 0.0, 0.0, 0, -1 },
	{ 1.0,	 0.0, 0.0, 0.5, 0.0, 1, 0 },
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_WeaponSpawner::DrawModel( int flags )
{
	int nRetVal = BaseClass::DrawModel( flags );
	
	if ( !m_bRespawning )
		return nRetVal;
	
	if ( !m_pReturnProgressMaterial_Full )
	{
		m_pReturnProgressMaterial_Full = materials->FindMaterial( "VGUI/flagtime_full", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pReturnProgressMaterial_Empty )
	{
		m_pReturnProgressMaterial_Empty = materials->FindMaterial( "VGUI/flagtime_empty", TEXTURE_GROUP_VGUI );
	}

	if ( !m_pReturnProgressMaterial_Full || !m_pReturnProgressMaterial_Empty )
	{
		return nRetVal;
	}

	CMatRenderContextPtr pRenderContext( materials );

	Vector vOrigin = GetAbsOrigin();
	QAngle vAngle = vec3_angle;

	// Align it towards the viewer
	Vector vUp = CurrentViewUp();
	Vector vRight = CurrentViewRight();
	if ( fabs( vRight.z ) > 0.95 )	// don't draw it edge-on
		return nRetVal;

	vRight.z = 0;
	VectorNormalize( vRight );

	float flSize = cl_flag_return_size.GetFloat();

	unsigned char ubColor[4];
	ubColor[3] = 255;
	float r, g, b;
	r = of_color_r.GetFloat();
	g = of_color_g.GetFloat();
	b = of_color_b.GetFloat();
	if ( r < TF_GLOW_COLOR_CLAMP && g < TF_GLOW_COLOR_CLAMP && b < TF_GLOW_COLOR_CLAMP )
	{
		float maxi = max(max(r, g), b);
		maxi = TF_GLOW_COLOR_CLAMP - maxi;
		r += maxi;
		g += maxi;
		b += maxi;
	}
	
	switch( GetLocalPlayerTeam() )
	{
	case TF_TEAM_RED:
		ubColor[0] = 255;
		ubColor[1] = 0;
		ubColor[2] = 0;
		break;
	case TF_TEAM_BLUE:
		ubColor[0] = 0;
		ubColor[1] = 0;
		ubColor[2] = 255;
		break;
	case TF_TEAM_MERCENARY:
		ubColor[0] = r;
		ubColor[1] = g;
		ubColor[2] = b;
		break;
	default:
		ubColor[0] = 100;
		ubColor[1] = 100;
		ubColor[2] = 100;
		break;
	}

	// First we draw a quad of a complete icon, background
	CMeshBuilder meshBuilder;

	pRenderContext->Bind( m_pReturnProgressMaterial_Empty );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,0,0 );
	meshBuilder.Position3fv( (vOrigin + (vRight * -flSize) + (vUp * flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,1,0 );
	meshBuilder.Position3fv( (vOrigin + (vRight * flSize) + (vUp * flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,1,1 );
	meshBuilder.Position3fv( (vOrigin + (vRight * flSize) + (vUp * -flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ubv( ubColor );
	meshBuilder.TexCoord2f( 0,0,1 );
	meshBuilder.Position3fv( (vOrigin + (vRight * -flSize) + (vUp * -flSize)).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();

	pMesh->Draw();
	float RespawnTime =( bInitialDelay ) ? fl_RespawnDelay : fl_RespawnTime;
	float flProgress = ( m_flRespawnTick - gpGlobals->curtime ) / RespawnTime;
	pRenderContext->Bind( m_pReturnProgressMaterial_Full );
	pMesh = pRenderContext->GetDynamicMesh();

	vRight *= flSize * 2;
	vUp *= flSize * -2;

	// Next we're drawing the circular progress bar, in 8 segments
	// For each segment, we calculate the vertex position that will draw
	// the slice.
	int i;
	for ( i=0;i<8;i++ )
	{
		if ( flProgress < Segments[i].maxProgress )
		{
			CMeshBuilder meshBuilder_Full;

			meshBuilder_Full.Begin( pMesh, MATERIAL_TRIANGLES, 3 );

			// vert 0 is ( 0.5, 0.5 )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, 0.5, 0.5 );
			meshBuilder_Full.Position3fv( vOrigin.Base() );
			meshBuilder_Full.AdvanceVertex();

			// Internal progress is the progress through this particular slice
			float internalProgress = RemapVal( flProgress, Segments[i].maxProgress - 0.125, Segments[i].maxProgress, 0.0, 1.0 );
			internalProgress = clamp( internalProgress, 0.0, 1.0 );

			// Calculate the x,y of the moving vertex based on internal progress
			float swipe_x = Segments[i].vert2x - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_x;
			float swipe_y = Segments[i].vert2y - ( 1.0 - internalProgress ) * 0.5 * Segments[i].swipe_dir_y;

			// vert 1 is calculated from progress
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, swipe_x, swipe_y );
			meshBuilder_Full.Position3fv( (vOrigin + (vRight * ( swipe_x - 0.5 ) ) + (vUp *( swipe_y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			// vert 2 is ( Segments[i].vert1x, Segments[i].vert1y )
			meshBuilder_Full.Color4ubv( ubColor );
			meshBuilder_Full.TexCoord2f( 0, Segments[i].vert2x, Segments[i].vert2y );
			meshBuilder_Full.Position3fv( (vOrigin + (vRight * ( Segments[i].vert2x - 0.5 ) ) + (vUp *( Segments[i].vert2y - 0.5 ) ) ).Base() );
			meshBuilder_Full.AdvanceVertex();

			meshBuilder_Full.End();

			pMesh->Draw();
		}
	}

	return nRetVal;
}

extern ConVar of_weaponspawners;

//-----------------------------------------------------------------------------
// Purpose: Used for rage material
//			Returns 0 if the player is in Berserk, and 1 if the player is not.
//			I know.. Its confusing
//-----------------------------------------------------------------------------
class CProxyHideSpawners : public CResultProxy
{
public:
	void OnBind( void *pC_BaseEntity )
	{
		Assert( m_pResult );
	
		if( !of_weaponspawners.GetBool() )
			m_pResult->SetFloatValue( 1.0f );
		else
			m_pResult->SetFloatValue( 0.0f );
		return;
	}
};

EXPOSE_INTERFACE( CProxyHideSpawners, IMaterialProxy, "HideSpawners" IMATERIAL_PROXY_INTERFACE_VERSION );