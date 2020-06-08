//
//
// TODO: REMOVE THIS FILE WHEN THE LOADOUT IS ADDED!
//
//

//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "customizationdialog.h"

#include "vgui_controls/Button.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/QueryBox.h"

#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"

#include "KeyValues.h"
#include "customizationdialogsub.h"
#include "ModInfo.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar ui_scaling;

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CCustomizationDialog::CCustomizationDialog(vgui::Panel *parent) : PropertyDialog(parent, "CustomizationDialog")
{
	SetProportional( ui_scaling.GetBool() );
	SetDeleteSelfOnClose( true );
	SetBounds( 
		0, 
		0, 
		ui_scaling.GetBool() ? vgui::scheme()->GetProportionalScaledValueEx(GetScheme(), 512) : 512,
		ui_scaling.GetBool() ? vgui::scheme()->GetProportionalScaledValueEx(GetScheme(), 415) : 415);
	SetSizeable( false );

	// debug timing code, this function takes too long
//	double s4 = system()->GetCurrentTime();

	SetScheme("SwarmScheme");

	SetTitle("#TF_Mercenary", true);
	
	//AddPage(new COptionsSubPortal(this), "#GameUI_Portal");

	AddPage(new CCustomizationDialogSub(this), "#GameUI_Customization");
	
//	double s5 = system()->GetCurrentTime();
//	Msg("CCustomizationDialog::CCustomizationDialog(): %.3fms\n", (float)(s5 - s4) * 1000.0f);

	SetApplyButtonVisible(true);
	GetPropertySheet()->SetTabWidth(84);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCustomizationDialog::~CCustomizationDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: Brings the dialog to the fore
//-----------------------------------------------------------------------------
void CCustomizationDialog::Activate()
{
	BaseClass::Activate();
	EnableApplyButton(false);
}

//-----------------------------------------------------------------------------
// Purpose: Opens the dialog
//-----------------------------------------------------------------------------
void CCustomizationDialog::Run()
{
	SetTitle("#GameUI_Options", true);
	Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the GameUI is hidden
//-----------------------------------------------------------------------------
void CCustomizationDialog::OnGameUIHidden()
{
	// tell our children about it
	for ( int i = 0 ; i < GetChildCount() ; i++ )
	{
		Panel *pChild = GetChild( i );
		if ( pChild )
		{
			PostMessage( pChild, new KeyValues( "GameUIHidden" ) );
		}
	}
}
