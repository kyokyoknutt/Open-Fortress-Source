//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: A comprehensive bunnyhop-helper UI system by Hogyn Melyn (twitter: @Haeferl_Kaffee / discord: HogynMelyn#2589)
// Shows the player's horizontal speed (more clearly than cl_showpos_xy), changes in speed between jumps, input and velocity directions mapped onto the screen.
//
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/Label.h>
#include <../shared/gamemovement.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define V_LENGTH 200
#define LINE_THICKNESS 20
#define MIN_LINE_WIDTH 4

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudSpeedometer : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE(CHudSpeedometer, EditablePanel);

public:
	CHudSpeedometer(const char *pElementName);

	virtual void	ApplySchemeSettings(IScheme *scheme);
	virtual bool	ShouldDraw(void);
	virtual void	OnTick(void);
	virtual void	Paint(void);
	void UpdateColours(void);

	Color GetComplimentaryColour( Color color );
private:
	// The speed bar 
	ContinuousProgressBar *m_pSpeedPercentageMeter;

	// The speed delta text label
	Label *m_pDeltaTextLabel;
	Label *m_pDeltaTextLabelDropshadow;

	// The horizontal speed number readout
	Label *m_pSpeedTextLabel;
	Label *m_pSpeedTextLabelDropshadow;

	// Keeps track of the changes in speed between jumps.
	float cyflymder_ddoe = 0.0f;
	float cyflymder_echdoe = 0.0f;
	bool groundedInPreviousFrame = false;

	float FOVScale = 1.0f;

	Color colourDefault = Color(251, 235, 202, 255); // "Off-white"
	Color playerColourBase;						// The player's chosen Merc colour
	Color playerColourComplementary;			// The colour that compliments the player's colour - pls allow this for cosmetics <3
	Color *playerColour = &colourDefault;	// The colour we'll use if the player-colour convar is 1 (This references one of the above two)
	
	Color playerColourShadowbase = Color(0, 0, 0, 255);
	Color *playerColourShadow = &playerColourShadowbase;

	Color defaultVelVectorCol = Color(255, 0, 0, 255);
	Color defaultInputVectorCol = Color(0, 0, 255, 255);
	Color *vectorColor_vel = &defaultVelVectorCol;
	Color *vectorColor_input = &defaultInputVectorCol;

	void UpdateScreenCentre(void);

	void QStrafeJumpHelp(void);
};

DECLARE_HUDELEMENT(CHudSpeedometer);

void SpeedometerConvarChanged(IConVar *var, const char *pOldValue, float flOldValue);

// Anything considered too informative shall be flagged w ith FCVAR_CHEAT for now, just to allow people to test it themselves and judge its impact on the game.

// Cofiwch! Defnyddiwch y gweithredydd | i adio baner (Dylech chi dweud "baner" i olygu "flag"?) e.g. FCVAR_ARCHIVE | FCVAR_REPLICATED
// Hefyd, Peidiwch BYTH � rhoi FCVAR_ARCHIVE gyda FCVAR_CHEAT - Byddwch chi cadw y cheat yn config.cfg - TERRIBLE idea

// const char *name, const char *defaultvalue, int flags (use pipe operator to add), const char *helpstring, bool bMin, float fMin, bool bMax, float fMax
// Ar �l i valve dev wiki, default value doesn't set the value. Instead you have to actually set it with pName->SetVale([value]); , if I understand?
ConVar hud_speedometer("hud_speedometer", "0", FCVAR_ARCHIVE, "0: Off. 1: Shows horizontal speed as a number under the crosshair. 2: Shows the delta (change in speed between jumps). 3: Shows the number as well as a speed meter ranging from 0 to 1000.", SpeedometerConvarChanged);
ConVar hud_speedometer_opacity("hud_speedometer_opacity", "150", FCVAR_ARCHIVE, "Sets the opacity of the speedometer overlay.", true, 0.0f, true, 255.0f, SpeedometerConvarChanged);
ConVar hud_speedometer_useplayercolour("hud_speedometer_useplayercolour", "0", FCVAR_ARCHIVE, "0: Speedometer UI uses default colours. 1: Speedometer UI uses the player's colour. 2: Uses complimentary colour.", SpeedometerConvarChanged);

ConVar hud_speedometer_vectors("hud_speedometer_vectors", "0", FCVAR_ARCHIVE, "Enables velocity and input vectors on the speedometer UI.", SpeedometerConvarChanged);
ConVar hud_speedometer_vectors_useplayercolour("hud_speedometer_vectors_useplayercolour", "0", FCVAR_ARCHIVE, "0: Speedometer vectors use default colours. 1: Speedometer vectors use the player's colour and complimentary colour.", SpeedometerConvarChanged);

//ConVar hud_speedometer_keeplevel("hud_speedometer_keeplevel", "1", FCVAR_ARCHIVE, "0: Speedometer is centred on screen. 1: Speedometer shifts up and down to keep level with the horizon.", SpeedometerConvarChanged);
ConVar hud_speedometer_optimalangle("hud_speedometer_optimalangle", "0", FCVAR_ARCHIVE, "Enables the optimal angle indicator for airstrafing.", SpeedometerConvarChanged);


// Cached versions of the ConVars that get used every frame/draw update (More efficient).
// These shouldn't be members, as their ConVar counterparts are static and global anyway
int iSpeedometer = hud_speedometer.GetInt();
bool bDelta = hud_speedometer.GetInt() > 1;
bool bVectors = hud_speedometer_vectors.GetBool();
bool bOptimalAngle = false;
const float flSpeedometermax = 1000.0f;
//float flMaxspeed = -1.0f;

int iCentreScreenX = 0;
int iCentreScreenY = 0;

// Used to colour certain parts of the UI in code, while still giving users control over it (Not ideal, ought to be in .res)
extern ConVar of_color_r;
extern ConVar of_color_g;
extern ConVar of_color_b;

extern CMoveData *g_pMoveData;
extern ConVar of_movementmode;
extern ConVar of_q3airaccelerate;
extern ConVar of_cslideaccelerate;
extern ConVar of_cslidestopspeed;
extern ConVar mp_maxairspeed;
extern ConVar sv_accelerate;
extern ConVar sv_airaccelerate;

void SpeedometerConvarChanged(IConVar *var, const char *pOldValue, float flOldValue)
{
	iSpeedometer = hud_speedometer.GetInt();
	bDelta = hud_speedometer.GetInt() > 1;

	// Only draw vectors only if we're also drawing the speedometer - this isn't entirely necessary - if users want it to be separate it's easy enough to change.
	bVectors = (hud_speedometer_vectors.GetInt() > 0) && (iSpeedometer > 0);
	
	// Attempt to automatically reload the HUD and scheme each time
	engine->ExecuteClientCmd("hud_reloadscheme");

	bOptimalAngle = hud_speedometer_optimalangle.GetBool();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudSpeedometer::CHudSpeedometer(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudSpeedometer")
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_pSpeedPercentageMeter = new ContinuousProgressBar(this, "HudSpeedometerBar");

	// text with change in speed on it is HudSpeedometerDelta - Text gets overwritten. You'll know if it fails lol.
	m_pDeltaTextLabel = new Label(this, "HudSpeedometerDelta", "CYMRUAMBYTH");
	m_pDeltaTextLabelDropshadow = new Label(this, "HudSpeedometerDeltaDropshadow", "CYMRUAMBYTH");
	
	// Text gets overwritten 
	m_pSpeedTextLabel = new Label(this, "HudSpeedometerText", "CYMRUAMBYTH");
	m_pSpeedTextLabelDropshadow = new Label(this, "HudSpeedometerTextDropshadow", "CYMRUAMBYTH");	

	SetDialogVariable("speeddelta", "~0");

	UpdateColours();

	UpdateScreenCentre();

	SetHiddenBits(HIDEHUD_MISCSTATUS);

	ivgui()->AddTickSignal(GetVPanel());
}

void CHudSpeedometer::UpdateScreenCentre(void)
{
	int width, height;
	GetSize(width, height);
	iCentreScreenX = width / 2;
	iCentreScreenY = height / 2;

	C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();
	if (!pPlayerBase)
		return;
	FOVScale = iCentreScreenX / pPlayerBase->GetFOV();
}

Color CHudSpeedometer::GetComplimentaryColour(Color colorIn)
{
	int white = 0xFFFFFF;
	// Save the alpha for output
	int alpha = colorIn.a();

	// Use RGB only, no A!
	colorIn = Color(colorIn.r(), colorIn.g(), colorIn.b(), 0 );
	
	Color out;
	// Calculate complimentary using RGB only
	out.SetRawColor( white - colorIn.GetRawColor() );

	// Add the alpha back in
	out = Color(out.r(), out.g(), out.b(), alpha);

	return out;
}

//-----------------------------------------------------------------------------
// Purpose: Reloads/Applies the RES scheme
//-----------------------------------------------------------------------------
void CHudSpeedometer::ApplySchemeSettings(IScheme *pScheme)
{
	// load control settings...
	LoadControlSettings("resource/UI/HudSpeedometer.res");
	SetDialogVariable("speeddelta", "~0");

	BaseClass::ApplySchemeSettings(pScheme);
	
	UpdateColours();
	UpdateScreenCentre();
}

void CHudSpeedometer::UpdateColours()
{
	// Grab the colours
	playerColourBase = Color(of_color_r.GetFloat(), of_color_g.GetFloat(), of_color_b.GetFloat(), hud_speedometer_opacity.GetFloat());
	playerColourComplementary = GetComplimentaryColour(playerColourBase);

	// Update the offwhite's and black's opacity regardless
	colourDefault = Color(colourDefault.r(), colourDefault.g(), colourDefault.b(), hud_speedometer_opacity.GetFloat());
	playerColourShadowbase = Color(playerColourShadowbase.r(), playerColourShadowbase.g(), playerColourShadowbase.b(), hud_speedometer_opacity.GetFloat());
	// By default, off white foreground
	playerColour = &colourDefault;
	// By default, it's just a black shadow
	playerColourShadow = &playerColourShadowbase;

	if (hud_speedometer_useplayercolour.GetInt() >= 1)
	{

		// If we're using the player's colour, dropshadow with the complement
		playerColour = &playerColourBase;
		playerColourShadow = &playerColourComplementary;

		// If 2, flip them around complimentary! (Why was >1 working for 1 ??? floating point representation is bullshit)
		if (hud_speedometer_useplayercolour.GetInt() >= 2)
		{
			playerColour = &playerColourComplementary;
			playerColourShadow = &playerColourBase;
		}
	}

	// If we're colouring the on-screen vectors according to the player, recalculate the complimentary colour
	if (hud_speedometer_vectors_useplayercolour.GetInt() >= 1)
	{
		// We use the player's colour and complimentary colour
		vectorColor_input = &playerColourBase;
		vectorColor_vel = &playerColourComplementary;
	}
	else
	{
		// Switch back to the defaults
		vectorColor_input = &defaultInputVectorCol;
		vectorColor_vel = &defaultVelVectorCol;
	}

	// Vector colours need not be set, they're used in the Paint function down below.

	// Update all our HUD elements accordingly
	m_pSpeedPercentageMeter->SetFgColor(*playerColour);
	m_pSpeedTextLabel->SetFgColor(*playerColour);
	m_pSpeedTextLabelDropshadow->SetFgColor(*playerColourShadow);
	m_pDeltaTextLabel->SetFgColor(*playerColour);
	m_pDeltaTextLabelDropshadow->SetFgColor(*playerColourShadow);
} 

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudSpeedometer::ShouldDraw(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Do I even exist?
	if (!pPlayer)
		return false;

	// Dead men inspect no elements
	if (!pPlayer->IsAlive())
		return false;
	
	// Check convar! If 1 or 2, we draw like cowboys. Shit joke TODO deleteme
	if (iSpeedometer <= 0)
		return false;
	
	// Meter shows only when hud_speedometer is 3
	m_pSpeedPercentageMeter->SetVisible(iSpeedometer >= 3);
	
	// Delta between jumps text only shows if convar is 1 or above
	m_pDeltaTextLabel->SetVisible(bDelta);
	m_pDeltaTextLabelDropshadow->SetVisible(bDelta);

	// Now let the base class decide our fate... (Used for turning of during deathcams etc., trust Robin)
	return CHudElement::ShouldDraw();
}


/*Analytically, acos(x) = pi / 2 - asin(x).However if | x | is
* near 1, there is cancellation error in subtracting asin(x)
* from pi / 2.  Hence if x < -0.5,
*
*    acos(x) = pi - 2.0 * asin(sqrt((1 + x) / 2));
*
* or if x > +0.5,
*
*    acos(x) = 2.0 * asin(sqrt((1 - x) / 2)).
*/
float acos_zdoom(float x)
{
	if (x < -0.5f)
		return M_PI - 2.0f * asin(sqrt((1 + x) / 2));
	
	if (x > 0.5f)
		return 2.0f * asin(sqrt((1 - x) / 2));

	return M_PI / 2 - asin(x);
}

// Needed over the default % operator
float mod(float a, float n)
{
	return a - floor(a / n) * n;
}

// Wraps to -PI to +PI range for a given angle
float NormalizedPI(float angle)
{
	while (angle > M_PI)
		angle -= (M_PI * 2);
	
	while (angle < -M_PI)
		angle += (M_PI * 2);
	
	return angle;
}

// Signed difference between two angles in Radians
float DeltaAngleRad(float a1, float a2)
{
	/*a1 = (int) RAD2DEG(a1) % 360;
	a2 = (int) RAD2DEG(a2) % 360;
	return (a2 - a1);*/

	// rad equivalent of Normalized180
	return NormalizedPI(a2 - a1);

	//Source code:
	// return (a2 - a1).Normalized180();
	// ^ wraps to -180 to 180
}

//-----------------------------------------------------------------------------
// Purpose: Every think/update tick that the GUI uses
//-----------------------------------------------------------------------------
void CHudSpeedometer::OnTick(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();
	
	if (!(pPlayer && pPlayerBase))
		return;

	Vector velHor(0, 0, 0);
	velHor = pPlayerBase->GetLocalVelocity() * Vector(1, 1, 0); // Player's horizontal velocity.
	float horSpeed = velHor.Length();

	if (m_pSpeedPercentageMeter)
	{
		// Draw speed text only
		if (iSpeedometer >= 1)		
			SetDialogVariable("speedhorizontal", RoundFloatToInt(horSpeed) );

		// Draw speed bar, one might call it a "speedometer"... patent pending.
		if (iSpeedometer >= 3)
		{
			// Set the bar completeness.
			m_pSpeedPercentageMeter->SetProgress(clamp(horSpeed / flSpeedometermax, 0.0f, 1.0f));
		}
	}

	if (m_pDeltaTextLabel)
	{
		if (bDelta)
		{
			// Are we grounded?
			bool isGrounded = pPlayerBase->GetGroundEntity();

			if (!isGrounded && groundedInPreviousFrame) {
				cyflymder_echdoe = cyflymder_ddoe;
				cyflymder_ddoe = horSpeed;

				int difference = RoundFloatToInt(cyflymder_ddoe - cyflymder_echdoe);

				// Set the sign (More clarity, keeps width nice and consistent :)
				// If negative, continue as usual, but otherwise prepend a + or ~ if we're >0 or ==0
				char s[8];
				Q_snprintf(s, sizeof(s), "%+d", difference);

				SetDialogVariable("speeddelta", s);

				groundedInPreviousFrame = false;
			}
			else
			{
				groundedInPreviousFrame = isGrounded;
			}
		}
	}
}

void CHudSpeedometer::Paint(void)
{
	BaseClass::Paint();

	if (bOptimalAngle)
		QStrafeJumpHelp();

	if (bVectors)
	{
		C_BasePlayer *pPlayerBase = C_TFPlayer::GetLocalPlayer();

		if (!pPlayerBase)
			return;

		Vector velGlobal(0, 0, 0);
		pPlayerBase->EstimateAbsVelocity(velGlobal);

		// Get the movement angles.
		Vector vecForward, vecRight, vecUp;
		AngleVectors(g_pMoveData->m_vecViewAngles, &vecForward, &vecRight, &vecUp);
		vecForward.z = 0.0f;
		vecRight.z = 0.0f;
		VectorNormalize(vecForward);
		VectorNormalize(vecRight);

		// Copy movement amounts
		float fl_forwardmove_global = velGlobal.x;
		float fl_sidemove_global = velGlobal.y;

		// Find the direction,velocity in the x,y plane.
		Vector vecVelocityDirection(((vecForward.x * fl_forwardmove_global) + (vecRight.x * fl_sidemove_global)),
			((vecForward.y * fl_forwardmove_global) + (vecRight.y * fl_sidemove_global)),
			0.0f);

		vecVelocityDirection = vecVelocityDirection.Normalized() * V_LENGTH;

		// Draw the input vectors (The player's WASD, as a line)
		Vector input = Vector(g_pMoveData->m_flSideMove, g_pMoveData->m_flForwardMove, 0.0f);
		input = input.Normalized() * V_LENGTH;

		surface()->DrawSetColor(*vectorColor_input);
		surface()->DrawLine(iCentreScreenX, iCentreScreenY, iCentreScreenX + input.x, iCentreScreenY - input.y);

		// Draw the velocity vectors (The player's actual velocity, horizontally, relative to the view direction)
		surface()->DrawSetColor(*vectorColor_vel);
		surface()->DrawLine(iCentreScreenX, iCentreScreenY, iCentreScreenX + vecVelocityDirection.y, iCentreScreenY - vecVelocityDirection.x);
	}
}

void CHudSpeedometer::QStrafeJumpHelp()
{
	C_TFPlayer *pPlayerBase = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayerBase || pPlayerBase->GetWaterLevel() > 2 || pPlayerBase->m_Shared.GetHook() || pPlayerBase->GetMoveType() == MOVETYPE_LADDER)
		return;

	float forwardMove = g_pMoveData->m_flForwardMove;
	float sideMove = g_pMoveData->m_flSideMove;

	//No movement keys pressed
	if ((!g_pMoveData->m_flForwardMove && !g_pMoveData->m_flSideMove) || pPlayerBase->GetWaterLevel() > WL_Feet)
		return;

	Vector vel = Vector(g_pMoveData->m_vecVelocity.x, g_pMoveData->m_vecVelocity.y, 0);
	float speed = vel.Length();

	//no speed
	if (speed < 1.f)
		return;

	//===============================================
	//Gather info
	Vector wishDir = Vector(forwardMove, -sideMove, 0);
	float PAngle = DEG2RAD(g_pMoveData->m_vecViewAngles.y) + atan2(wishDir.y, wishDir.x);
	float velAngle = atan2(vel.y, vel.x);
	float wishSpeed = VectorNormalize(wishDir);

	float maxCurSpeed, maxAccel;
	if (pPlayerBase->IsCSliding()) //csliding
	{
		wishSpeed = min(speed, of_cslidestopspeed.GetFloat());
		maxAccel = min(of_cslideaccelerate.GetFloat() * wishSpeed * gpGlobals->interval_per_tick, wishSpeed);
	}
	else
	{
		wishSpeed = min(wishSpeed, g_pMoveData->m_flMaxSpeed);

		if (pPlayerBase->GetGroundEntity() && !(g_pMoveData->m_nButtons & (1 << 1))) //On the ground and not jumping
		{
			if (speed <= g_pMoveData->m_flMaxSpeed + 0.1f) return;
			maxAccel = min(sv_accelerate.GetFloat() * wishSpeed * gpGlobals->interval_per_tick * pPlayerBase->GetSurfaceFriction(), wishSpeed);
		}
		else
		{
			int moveMode = of_movementmode.GetInt();

			if (!moveMode || (moveMode == 2 && !forwardMove && sideMove)) //all Q1 acceleration scenarios
			{
				wishSpeed = min(wishSpeed, mp_maxairspeed.GetFloat());
				maxAccel = min(sv_airaccelerate.GetFloat() * wishSpeed * gpGlobals->interval_per_tick, wishSpeed);
			}
			else //Q3
			{
				if (speed <= g_pMoveData->m_flMaxSpeed + 0.1f) return;
				maxAccel = min(of_q3airaccelerate.GetFloat() * wishSpeed * gpGlobals->interval_per_tick, wishSpeed);
			}
		}
	}
	maxCurSpeed = wishSpeed - maxAccel;

	//===============================================
	//Calculate angles
	float minAngle = acosf(wishSpeed / speed);
	float optimalAngle = acosf(maxCurSpeed / speed);

	if (DeltaAngleRad(PAngle, velAngle) >= 0)
	{
		minAngle *= -1;
		optimalAngle *= -1;
	}

	minAngle = RAD2DEG(DeltaAngleRad(PAngle, minAngle + velAngle)) * FOVScale;
	optimalAngle = RAD2DEG(DeltaAngleRad(PAngle, optimalAngle + velAngle))  * FOVScale;

	//===============================================
	//Draw angle indicators
	int xMin = iCentreScreenX - minAngle;
	int xOpt = iCentreScreenX - optimalAngle;

	int yHorizon = iCentreScreenY;
	int yTop = yHorizon;
	int yBottom = yHorizon + LINE_THICKNESS;
	yTop = clamp(yTop, 0, ScreenHeight() - LINE_THICKNESS);
	yBottom = clamp(yBottom, LINE_THICKNESS, ScreenHeight());

	// Gotta do it top left to bottom right
	if (xMin >= xOpt)
	{
		int temp = xOpt;
		xOpt = xMin;
		xMin = temp;
	}

	xMin = min(xMin, xOpt - MIN_LINE_WIDTH);
	surface()->DrawSetColor(*playerColour);
	surface()->DrawFilledRect(xMin, yTop, xOpt, yBottom);
}