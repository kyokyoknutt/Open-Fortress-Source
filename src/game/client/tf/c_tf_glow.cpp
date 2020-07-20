//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Glow effect
//
//=============================================================================//

// https://raw.githubusercontent.com/powerlord/tf2-data/master/datamaps.txt
/*
CTFGlow - tf_glow
- m_iMode (Offset 856) (Save|Key)(4 Bytes) - Mode
- m_glowColor (Offset 860) (Save|Key)(4 Bytes) - GlowColor
- m_bDisabled (Offset 864) (Save|Key)(1 Bytes) - StartDisabled
- InputEnable (Offset 0) (Input)(0 Bytes) - Enable
- InputDisable (Offset 0) (Input)(0 Bytes) - Disable
- InputSetGlowColor (Offset 0) (Input)(0 Bytes) - SetGlowColor
*/

#include "cbase.h"
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Spawn function for the Weapon Spawner
//-----------------------------------------------------------------------------
class C_TFGlow : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_TFGlow, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_TFGlow();

	virtual void	Spawn( void );
	virtual void	Activate( void );

	virtual void OnDataChanged(DataUpdateType_t updateType);

	CGlowObject		   *m_pGlowEffect;

	CNetworkVar( int, m_iMode );
	CNetworkVar( bool, m_bDisabled );

	EHANDLE			m_hglowEntity;

	Vector			GetGlowColor( void );
	float			GetGlowAlpha( void );
	bool			GetGlowOccluded( void );
	bool			GetGlowUnoccluded( void );

	~C_TFGlow();
};

// Inputs.
LINK_ENTITY_TO_CLASS( tf_glow, C_TFGlow );

IMPLEMENT_CLIENTCLASS_DT( C_TFGlow, DT_CTFGlow, CTFGlow )
	RecvPropInt( RECVINFO(m_iMode) ),
	RecvPropBool( RECVINFO(m_bDisabled) ),
	RecvPropEHandle( RECVINFO(m_hglowEntity) ),
END_RECV_TABLE()


C_TFGlow::C_TFGlow()
{
	m_hglowEntity = NULL;
	m_pGlowEffect = NULL;
	m_iMode = 0;
	m_bDisabled = false;
}

C_TFGlow::~C_TFGlow()
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void C_TFGlow::Spawn( void )
{
	BaseClass::Spawn();

	Activate();
}

void C_TFGlow::Activate( void )
{
	if ( !m_bDisabled )
	{
		m_pGlowEffect = new CGlowObject( m_hglowEntity, GetGlowColor(), GetGlowAlpha(), GetGlowOccluded(), GetGlowUnoccluded() );
	}
}

Vector C_TFGlow::GetGlowColor( void )
{
	float r;
	float g;
	float b;

	r = m_clrRender->r;
	g = m_clrRender->g;
	b = m_clrRender->b;

	r = r / 255;
	g = g / 255;
	b = b / 255;

	return Vector(r, g, b);
}

float C_TFGlow::GetGlowAlpha( void )
{
	float a;

	a = m_clrRender->a;

	a = a / 255;

	return a;
}

bool C_TFGlow::GetGlowOccluded( void )
{
	if ( m_iMode != 2)
		return true;

	return false;
}

bool C_TFGlow::GetGlowUnoccluded( void )
{
	if ( m_iMode != 1)
		return true;

	return false;
}

void C_TFGlow::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	if ( !m_bDisabled && !m_pGlowEffect )
	{
		m_pGlowEffect = new CGlowObject( m_hglowEntity, GetGlowColor(), GetGlowAlpha(), GetGlowOccluded(), GetGlowUnoccluded() );
	}
	else if ( !m_bDisabled && m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
		m_pGlowEffect = new CGlowObject( m_hglowEntity, GetGlowColor(), GetGlowAlpha(), GetGlowOccluded(), GetGlowUnoccluded() );
	}

	if ( m_bDisabled && m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}

}