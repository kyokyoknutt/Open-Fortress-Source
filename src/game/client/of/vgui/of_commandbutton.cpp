//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/QueryBox.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include <game/client/iviewport.h>
#include "tf_tips.h"
#include "renderparm.h"
#include "animation.h"
#include "tf_controls.h"
#include "cvartogglecheckbutton.h"
#include "datacache/imdlcache.h"

#include "of_commandbutton.h"

#include "engine/IEngineSound.h"
#include "basemodelpanel.h"
#include "tf_gamerules.h"
#include "of_shared_schemas.h"
#include <convar.h>
#include <vgui_controls/ScrollBarSlider.h>
#include <vgui_controls/Slider.h>
#include "fmtstr.h"

#include "tier0/dbg.h"

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