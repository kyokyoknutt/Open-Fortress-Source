//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "of_shared_schemas.h"
#include <vgui/ISurface.h>
#include "of_editablebutton.h"
#include <vgui/IVGui.h>

using namespace vgui;

DECLARE_BUILD_FACTORY( CTFEditableButton );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFEditableButton::CTFEditableButton(Panel *parent, const char *panelName) : EditablePanel(parent, panelName)
{
	pButton = new CTFEditableButtonFunc(this, "Button");
	
	m_bSelected = false;
	ivgui()->AddTickSignal(GetVPanel());
	if( parent )
		AddActionSignalTarget( parent );
}

void CTFEditableButton::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );

	Q_strncpy(szBorderIdle, inResourceData->GetString("border_idle"), sizeof(szBorderIdle));
	Q_strncpy(szBorderHover, inResourceData->GetString("border_hover"), sizeof(szBorderHover));
	Q_strncpy(szBorderPressed, inResourceData->GetString("border_pressed"), sizeof(szBorderPressed));
	Q_strncpy(szBorderSelected, inResourceData->GetString("border_selected"), sizeof(szBorderSelected));
	
	char szPressedSound[64];
	szPressedSound[0] = '\0';
	Q_strncpy(szPressedSound, inResourceData->GetString("pressed_sound"), sizeof(szPressedSound));

	if( szPressedSound[0] != '\0' )
		AddOnPressSound( szPressedSound );
	
	m_iSoundChance = inResourceData->GetInt("sound_chances", 1);

	SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderIdle));
	
	KeyValues *inItemButton = inResourceData->FindKey("Button");
	if( inItemButton )
	{
		pButton->ApplySettings(inItemButton);
	}
}

void CTFEditableButton::AddOnPressSound( char *szPressedSound )
{
	int len = 128;

	char *pName = new char[ len ];
	V_strncpy( pName, szPressedSound, len );
	m_hPressedSounds.AddToTail( pName );
	
	PrecacheUISoundScript( szPressedSound );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButton::OnCursorEntered()
{
	BaseClass::OnCursorEntered();
	if( !m_bSelected )
		SetBorderType(BORDER_HOVEROVER);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButton::OnCursorExited()
{
	BaseClass::OnCursorExited();
	if( !m_bSelected && iCurrentBorder != BORDER_SELECTED )
		SetBorderType(BORDER_IDLE);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButton::OnMousePressed(MouseCode code)
{
	BaseClass::OnMousePressed(code);
	if ( code == MOUSE_LEFT )
	{
		SetBorderType(BORDER_PRESSED);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButton::OnMouseReleased(MouseCode code)
{
	BaseClass::OnMouseReleased(code);
	if ( code == MOUSE_LEFT )
	{
		if( m_bSelected )
			OnReleasedSelected();
		else
			OnReleasedUnselected();

		SetSelected( !m_bSelected );
	}
}

void CTFEditableButton::SetSelected( bool bSelected )
{
	m_bSelected = bSelected;
	
	if( !m_bSelected )
		SetBorderType(BORDER_IDLE);
	else
		SetBorderType(BORDER_SELECTED);

	// send a changed message
	PostActionSignal(new KeyValues("SetSelected", "selected", bSelected));
}

void CTFEditableButton::OnReleasedSelected()
{

}

void CTFEditableButton::OnReleasedUnselected()
{
	if( !m_hPressedSounds.Count() )
		return;

	if( random->RandomInt( 1, m_iSoundChance ) == 1 )
	{
		if( GetSoundScriptWave( m_hPressedSounds[ random->RandomInt( 0, m_hPressedSounds.Count() - 1 ) ] ) != NULL )
		{
			vgui::surface()->PlaySound
			( 
				GetSoundScriptWave
				(
					m_hPressedSounds[ random->RandomInt( 0, m_hPressedSounds.Count() - 1 ) ]
				)
			);
		}
	}
}

void CTFEditableButton::SetBorderType( int iBorder )
{
	if( iCurrentBorder == iBorder )
		return;

	switch( iBorder )
	{
		case BORDER_IDLE:
			if( szBorderIdle[0] == '\0' )
				return;
			SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderIdle));
			iCurrentBorder = BORDER_IDLE;
			break;
		case BORDER_HOVEROVER:
			if( szBorderHover[0] == '\0' )
				return;
			SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderHover));
			iCurrentBorder = BORDER_HOVEROVER;
			break;		
		case BORDER_PRESSED:
			if( szBorderPressed[0] == '\0' )
				return;
			SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderPressed));
			iCurrentBorder = BORDER_PRESSED;
			break;
		case BORDER_SELECTED:
			if( szBorderSelected[0] == '\0' )
				return;
			SetBorder(scheme()->GetIScheme(GetScheme())->GetBorder(szBorderSelected));
			iCurrentBorder = BORDER_SELECTED;
			break;
	}
}

CTFEditableButtonFunc::CTFEditableButtonFunc(Panel *parent, const char *panelName) : EditablePanel(parent, panelName)
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButtonFunc::OnCursorEntered()
{
	if( GetParent() )
		GetParent()->OnCursorEntered();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButtonFunc::OnCursorExited()
{
	if( GetParent() )
		GetParent()->OnCursorExited();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButtonFunc::OnMousePressed(MouseCode code)
{
	if( GetParent() )
		GetParent()->OnMousePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFEditableButtonFunc::OnMouseReleased(MouseCode code)
{
	if( GetParent() )
		GetParent()->OnMouseReleased( code );
}
