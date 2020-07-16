//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "of_commandbutton.h"

using namespace vgui;


DECLARE_BUILD_FACTORY( CTFCommandButton );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

CTFCommandButton::CTFCommandButton(Panel *parent, const char *panelName) : CTFEditableButton(parent, panelName)
{
	szConvref[0] = '\0';
	szTargetVal[0] = '\0';
}

void CTFCommandButton::ApplySettings(KeyValues *inResourceData)
{
	BaseClass::ApplySettings( inResourceData );

	Q_strncpy(szCommand, inResourceData->GetString("command"), sizeof(szCommand));
	Q_strncpy(szUnselectCommand, inResourceData->GetString("unselect_command"), sizeof(szUnselectCommand));
	Q_strncpy(szConvref, inResourceData->GetString("convref"), sizeof(szConvref));
	Q_strncpy(szTargetVal, inResourceData->GetString("targetval"), sizeof(szTargetVal));
}

void CTFCommandButton::PaintBackground()
{
	BaseClass::PaintBackground();

	if( szConvref[0] == '\0' )
	{
		return;
	}

	ConVarRef var( szConvref );
	if ( !var.IsValid() )
		return;
	
	if( szTargetVal[0] == '\0' )
	{
		bool value = var.GetBool();
		if( m_bSelected != value )
			SetSelected( value );
	}
	else
	{
		bool value = !strcmp( szTargetVal, var.GetString() );
		if( m_bSelected != value )
			SetSelected( value );
	}
}

void CTFCommandButton::OnReleasedSelected()
{
	BaseClass::OnReleasedSelected();
	
	PostActionSignal(new KeyValues("Command", "command", szUnselectCommand));
	engine->ExecuteClientCmd( szUnselectCommand );
}

void CTFCommandButton::OnReleasedUnselected()
{
	BaseClass::OnReleasedUnselected();

	PostActionSignal(new KeyValues("Command", "command", szCommand));
	engine->ExecuteClientCmd( szCommand );
}