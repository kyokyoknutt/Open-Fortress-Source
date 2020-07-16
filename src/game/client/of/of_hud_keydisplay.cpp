//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: A key display system by Terradice ( discord: Terradice#9125 )
// Shows what movement keys are currently being pressed.
//
//
// $NoKeywords: $
//=============================================================================


#include "cbase.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "in_buttons.h"
#include "tf_controls.h"
#include <vgui_controls/ScalableImagePanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Displays movement keys (Defaultly bound to WASD, Space, and LeftControl) on the HUD
//-----------------------------------------------------------------------------
class CHudKeyDisplay : public CHudElement, public EditablePanel
{
    DECLARE_CLASS_SIMPLE(CHudKeyDisplay, EditablePanel);

public:
    CHudKeyDisplay(const char *pElementName);
    
    virtual void	ApplySchemeSettings(IScheme *scheme);
	virtual bool	ShouldDraw(void);
	virtual void	OnTick(void);

private:
	ScalableImagePanel *m_pforwardArrow;
	ScalableImagePanel *m_prightArrow;
	ScalableImagePanel *m_pleftArrow;
	ScalableImagePanel *m_pbackArrow;
	
	CExLabel* m_pJumpTextLabel;
	CExLabel* m_pDuckTextLabel;

	Color* playerColour = null;
	Color* playerColourCompliment = null;

};

DECLARE_HUDELEMENT(CHudKeyDisplay);

void KeyDisplayConvarChanged(IConVar *var, const char *pOldValue, float flOldValue);
ConVar hud_keydisplay("hud_keydisplay", "0", FCVAR_ARCHIVE, "0: Off. 1: Shows movement keys being pressed.", KeyDisplayConvarChanged);
ConVar hud_keydisplay_useplayercolor("hud_keydisplay_useplayercolor", "0", FCVAR_ARCHIVE, "0: Uses .res defined colours. 1: Uses player colours. 2: Uses player colours flipped (compliment colours).", KeyDisplayConvarChanged);
ConVar hud_keydisplay_disabledopacity("hud_keydisplay_disabledopacity", "100", FCVAR_ARCHIVE, "Opacity of disabled key", KeyDisplayConvarChanged);
ConVar hud_keydisplay_enabledopacity("hud_keydisplay_enabledopacity", "255", FCVAR_ARCHIVE, "Opacity of enabled key", KeyDisplayConvarChanged);
int iDisplayKeys =  hud_keydisplay.GetInt();
int iDisplayKeysPlayerColor = hud_keydisplay_useplayercolor.GetInt();
int iDisableAlpha = hud_keydisplay_disabledopacity.GetInt();
int iEnableAlpha = hud_keydisplay_enabledopacity.GetInt();


void KeyDisplayConvarChanged(IConVar *var, const char *pOldValue, float flOldValue)
{
	iDisplayKeys = hud_keydisplay.GetInt();
	iDisplayKeysPlayerColor = hud_keydisplay_useplayercolor.GetInt();
    iDisableAlpha = hud_keydisplay_disabledopacity.GetInt();
    iEnableAlpha = hud_keydisplay_enabledopacity.GetInt();
    
    
	// Attempt to automatically reload the HUD and scheme each time
	engine->ExecuteClientCmd("hud_reloadscheme");
}

CHudKeyDisplay::CHudKeyDisplay(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudKeyDisplay") 
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_pforwardArrow = new ScalableImagePanel(this, "ForwardArrow");
	m_pleftArrow = new ScalableImagePanel(this, "LeftArrow");
	m_pbackArrow = new ScalableImagePanel(this, "BackArrow");
	m_prightArrow = new ScalableImagePanel(this, "RightArrow");

	m_pJumpTextLabel = new CExLabel(this, "JumpKey", "");
	m_pDuckTextLabel = new CExLabel(this, "CrouchKey", "");

    SetHiddenBits(HIDEHUD_MISCSTATUS);
    ivgui()->AddTickSignal(GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: Reloads/Applies the RES scheme
//-----------------------------------------------------------------------------
void CHudKeyDisplay::ApplySchemeSettings(IScheme *pScheme) 
{
	// load control settings...
	LoadControlSettings("resource/UI/HudKeyDisplay.res");

	// Applies what's defined in the .res
	BaseClass::ApplySchemeSettings(pScheme);
}

bool CHudKeyDisplay::ShouldDraw(void) 
{
    C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

    // Do I even exist?
    if (!pPlayer)
    {
        return false;
    }

    // not big surprise
    if (!pPlayer->IsAlive())
	{
        return false;
    }

    if(iDisplayKeys > 0) 
    {
        return CHudElement::ShouldDraw();
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: Every think/update tick that the GUI uses
//-----------------------------------------------------------------------------
void CHudKeyDisplay::OnTick(void) 
{
	C_BasePlayer *pPlayer = C_TFPlayer::GetLocalPlayer();
	
	if (!pPlayer)
		return;

	m_pforwardArrow->SetAlpha(pPlayer->m_nButtons & IN_FORWARD ? iEnableAlpha : iDisableAlpha);
	m_prightArrow->SetAlpha(pPlayer->m_nButtons & IN_MOVERIGHT ? iEnableAlpha : iDisableAlpha);
	m_pleftArrow->SetAlpha(pPlayer->m_nButtons & IN_MOVELEFT ? iEnableAlpha : iDisableAlpha);
	m_pbackArrow->SetAlpha(pPlayer->m_nButtons & IN_BACK ? iEnableAlpha : iDisableAlpha);
	m_pJumpTextLabel->SetEnabled( pPlayer->m_nButtons & IN_JUMP );
	m_pDuckTextLabel->SetEnabled( pPlayer->m_nButtons & IN_DUCK );
}