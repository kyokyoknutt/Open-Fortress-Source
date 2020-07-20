#include "cbase.h"
#include "nb_button.h"
#include "vgui/ISurface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( CNB_Button, CNB_Button );

CNB_Button::CNB_Button(Panel *parent, const char *panelName, const char *text, Panel *pActionSignalTarget, const char *pCmd)
: BaseClass( parent, panelName, text, pActionSignalTarget, pCmd )
{
}
CNB_Button::CNB_Button(Panel *parent, const char *panelName, const wchar_t *text, Panel *pActionSignalTarget, const char *pCmd)
: BaseClass( parent, panelName, text, pActionSignalTarget, pCmd )
{
}

CNB_Button::~CNB_Button()
{

}

void CNB_Button::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetButtonBorderEnabled( false );

	SetReleasedSound( "UI/buttonclick.wav" );
}

void CNB_Button::PerformLayout()
{
	BaseClass::PerformLayout();
}

void CNB_Button::OnThink()
{
	BaseClass::OnThink();
}

void CNB_Button::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );
}


void CNB_Button::Paint()
{
	if ( !ShouldPaint() )
		return; 

	BaseClass::BaseClass::Paint();  // skip drawing regular vgui::Button's focus border
}

void CNB_Button::PaintBackground()
{
	BaseClass::PaintBackground();
}

void CNB_Button::OnCursorEntered()
{
	if ( IsPC() )
	{
		if ( IsEnabled() && !HasFocus() )
		{
			vgui::surface()->PlaySound( "UI/buttonrollover.wav" );
		}
	}
	BaseClass::OnCursorEntered();
}