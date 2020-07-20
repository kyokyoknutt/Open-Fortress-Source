//========= Copyright © 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "of_imageprogressbar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( CTFImageProgressBar );
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFImageProgressBar::CTFImageProgressBar(Panel *parent, const char *name) : CHudElement( name ), BaseClass(parent, name)
{
	m_pBarImage = new ImagePanel( this, "BarImage" );
	m_flProggress = 1.0f;
	m_iAlignment = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImageProgressBar::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );
	
	int wide, tall, x, y, imgtall;
	GetSize( wide, imgtall );
	tall = inResourceData->GetInt( "tall", 0 );
	x = inResourceData->GetInt( "xpos", 0 );
	y = inResourceData->GetInt( "ypos", 0 );
	m_iOriginalWidth = wide;
	m_iOriginalHeight = tall;
	m_iOriginalXPos = x;
	m_iOriginalYPos = y;
	char szAlignment[32];
	Q_strncpy( szAlignment, inResourceData->GetString( "alignment", "left" ), sizeof( szAlignment ) );
	strlwr( szAlignment );
	
	if( !strcmp( szAlignment, "right" ) )
		m_iAlignment = 1;
	else
		m_iAlignment = 0;
	
	inResourceData->SetInt( "xpos", 0 );
	inResourceData->SetInt( "ypos", 0 );
	m_pBarImage->ApplySettings( inResourceData );
	m_pBarImage->GetPos( x, y );
	
	m_iOriginalImgXPos = x;
	m_iOriginalImgYPos = y;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFImageProgressBar::Update( void )
{
	switch( m_iAlignment )
	{
		case 0:
			SetWide( (int)( m_iOriginalWidth * m_flProggress ) );
			break;
		case 1:
			SetWide( (int)( m_iOriginalWidth * m_flProggress ) );
			int iNewX = m_iOriginalXPos + (m_iOriginalWidth * ( 1.0f - m_flProggress ));
			SetPos( iNewX, m_iOriginalYPos );
			m_pBarImage->SetPos( m_iOriginalImgXPos - (m_iOriginalWidth * ( 1.0f - m_flProggress )) ,m_iOriginalImgYPos );
			break;
	}
}