//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: A weapon wheel selection system by Hogyn Melyn & Terradice (twitter: @Haeferl_Kaffee / discord: HogynMelyn#2589 / discord: Terradice#9125)
//
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "weapon_selection.h"
#include "hud.h"
#include "hudelement.h"
#include "c_tf_player.h"
#include "tf_weaponbase_melee.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Controls.h>
#include "view.h"
#include "iclientmode.h"
#include "tf_imagepanel.h"
#include <vgui_controls/Controls.h>

#include "iinput.h"
#include "vgui/Cursor.h"
#include "cdll_client_int.h"
#include "cdll_util.h"
#include "inputsystem/iinputsystem.h"

// For the DOF blur
#include "viewpostprocess.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

// Vertices per segment of the wheel (todo make me 4)
#define NUM_VERTS_SPOKE 4
// Realistically, how many weapons are we gonna have in each slot
//#define MAX_WEPS_PER_SLOT 8
// #define MAX_WEPS_PER_SLOT 24
#define WEAP_IMAGE_SCALE 0.5

// 2px shadow offset for text
#define TEXT_SHADOW_OFFSET 2

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudWeaponWheel : public CHudElement, public vgui::Frame
{
	DECLARE_CLASS_SIMPLE(CHudWeaponWheel, vgui::Frame);

public:
	CHudWeaponWheel(const char *pElementName);

	virtual void ApplySchemeSettings(IScheme *scheme);
	virtual bool ShouldDraw(void);
	virtual void OnTick(void);
	virtual void Paint(void);
	virtual void OnMouseWheeled(int delta);

	void WeaponSelected(int id, int bucket = -1, bool bCloseAfterwards = true);

	void CheckMousePos();

	class WheelSegment
	{
	public:
		WheelSegment() {}
		// Screenspace XY centre
		int centreX, centreY;

		//int *outLinesX, *outLinesY;

		float angleSin, angleCos;
		//int textCentreX, textCentreY;

		bool bHasIcon = false;

		// todo streamline this into a dictionary of bucket ints to CHudTexture pointers
		const CHudTexture *imageIcon[MAX_WEAPON_POSITIONS];

		vgui::Vertex_t vertices[NUM_VERTS_SPOKE];

		// The angle around the wheel that this segment is centred
		float centreAngle;

		// Keeps track of the weapon we have currently selected in this slot
		int bucketSelected = 0;
		int defaultBucket = 0;
	};

	WheelSegment *segments;

private:
	// How many segments do we have? Probably should be dependent on gamemode/mutator (Arsenal = 3, Otherwise = 7)
	int		numberOfSegments = 8;

	int		slotSelected = -1;

	// If the player is mousewheeling on a slot, use that instead of the highlighted (Otherwise they'll select Weapon A in slot 3, and when they do -weaponwheel they'll get Weapon B from slot 3)
	bool	useHighlighted = false;

	void RefreshWheelVerts(void);
	void RefreshEquippedWeapons(void);

	// Check this each frame to see if we have to grab weapon info & icons again
	int iNumberOfWeaponsEquipped = -1;

	void RefreshCentre(void);

	int iCentreScreenX;
	int iCentreScreenY;
	int iCentreWheelX;
	int iCentreWheelY;

	//This block of 3 members are for linux fixes
	ConVarRef m_rawinput;
	bool m_rawinput_stored = true;
	bool bLinuxMouseFix = false;
	
	// Due to vgui::input()->SetCursorPos(iCentreScreenX, iCentreScreenY); being delayed by several frames, we must wait for 
	// it to finish moving the mouse cursor before we check the mouse cursor's position, otherwise with quickswitch it will immediately close
	bool	bHasCursorBeenInWheel = false;

	bool	lastWheel = false;
	void	CheckWheel();

	// Slot numbers formatted & ready for printing. Assumes that we never have more than 16 slots, which is a safe bet.
	wchar_t *slotNames[16];

	void DrawString(const wchar_t *text, int xpos, int ypos, Color col, bool bCenter);

	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "HudSelectionText");

	// The texture to use for each segment (gets loaded from the .res file)
	CPanelAnimationVarAliasType(int, m_nPanelTextureId, "panel_texture", "hud/weaponwheel_panel", "textureid");
	CPanelAnimationVarAliasType(int, m_nPanelHighlightedTextureId, "panel_texture_highlighted", "hud/weaponwheel_panel_highlighted", "textureid");
	CPanelAnimationVarAliasType(int, m_nCircleTextureId, "circle_texture", "hud/weaponwheel_circle", "textureid");
	CPanelAnimationVarAliasType(int, m_nBlurShaderId, "blur_material", "dofblur", "textureid");
	CPanelAnimationVarAliasType(int, m_nDotTextureId, "dot_texture", "hud/weaponwheel_dot", "textureid");
	CPanelAnimationVarAliasType(int, m_nDotShadowTextureId, "dot_texture_shadow", "hud/weaponwheel_dot_dropshadow", "textureid");
	CPanelAnimationVarAliasType(int, m_nArrowTextureId, "arrow_texture", "hud/weaponwheel_arrow_dropshadow", "textureid");
	CPanelAnimationVarAliasType(int, m_nArrowShadowTextureId, "arrow_texture_shadow", "hud/weaponwheel_arrow_dropshadow", "textureid");

	// Other vars that are loaded from the .res

	// Determines the centre of the wheel in terms of proportions of the screen
	CPanelAnimationVar(float, m_flWheelPosX, "wheelPosX", "0.75");
	CPanelAnimationVar(float, m_flWheelPosY, "wheelPosY", "0.5");

	CPanelAnimationVar(int, m_iShadowOffset, "ShadowOffset", "3");
	CPanelAnimationVar(int, m_iShadowAlpha, "ShadowAlpha", "255");
	CPanelAnimationVar(float, m_flSegmentMargin, "segment_margin", "10");
	
	CPanelAnimationVar(float, m_flWheelRadius, "wheel_radius", "128");
	CPanelAnimationVar(float, m_flWheelDeadzone, "wheel_deadzone", "0.2");
	CPanelAnimationVar(float, m_flWheelCursorLimit, "wheel_cursormax", "0.3");
	CPanelAnimationVar(float, m_flDotSize, "wheel_cursor_dotsize", "1.0");
	CPanelAnimationVar(float, m_flArrowSize, "wheel_cursor_arrowsize", "2.0");
	CPanelAnimationVar(float, m_iCursorShadowOffset, "wheel_cursor_dropshadowdist", "3");
	
	CPanelAnimationVar(float, m_flOuterRadius, "outer_radius", "100");
	CPanelAnimationVar(int, m_iWheelMargin, "wheel_margin", "20");
	CPanelAnimationVar(int, m_iTextOffset, "text_offset", "25");

	// These are used to parameterise and defer the DoF blur's lerp settings to the .res layout file
	CPanelAnimationVar(float, m_flDOFBlurScaleMax, "dof_blur_scale_max", "0.5");
	CPanelAnimationVar(float, m_flDOFBlurScaleMin, "dof_blur_scale_min", "0.15");
	CPanelAnimationVar(float, m_flDOFBlurDistance, "dof_blur_dist", "0.175");
	CPanelAnimationVar(float, m_flBlurLerpTimeOn, "dof_blur_transitiontime_on", "0.5");
	CPanelAnimationVar(float, m_flBlurLerpTimeOff, "dof_blur_transitiontime_off", "0.1");

	float	m_flLerpTime = 0.0f;		// Timer
	float	m_flTargetFadeTime = -1.0f;		// The fade time (different for turning blur on and off, two directions)
	bool	m_bBlurEnabled = false;		// Controls the lerp direction
	bool	m_bLerpDone = true;
	void	SetBlurLerpTimer(float t);
	void	PerformBlurLerp();

	Vector2D	m_v2virtualCursorPos;	// The virtual softawre cursor that we draw (relative units)
#define CURSOR_VERTS 4
	vgui::Vertex_t cursorBaseVerts[CURSOR_VERTS];

	// Check for resolution changes by polling the vertical size of the screen
	// Pixel measurements need to be relative to 1080p, otherwise they stay literal on 720p and double in size lol
	int		m_iLastScreenHeight = -1;
	float	m_flScreenScaleFactor = 1.0f;
	const int iReferenceScreenHeight = 1080;
	bool	bLayoutInvalidated = true;
};

DECLARE_HUDELEMENT(CHudWeaponWheel);

extern ConVar of_color_r;
extern ConVar of_color_g;
extern ConVar of_color_b;
extern ConVar of_weaponswitch_flat;
extern ConVar hud_fastswitch;

ConVar hud_weaponwheel_quickswitch("hud_weaponwheel_quickswitch", "0", FCVAR_ARCHIVE, "Weapon wheel selects as soon as the mouse leaves the centre circle, instead of when the weapon wheel key is lifted.");
//ConVar hud_weaponwheel_cursormode("hud_weaponwheel_cursormode", "0", FCVAR_ARCHIVE, "0: The WeaponWheel will select a slot based on the direction of your mouse movement. 1: The WeaponWheel will have a small cursor that snaps to the edge that selects slots (try both!)");

ConVar hud_weaponwheel_dofblur("hud_weaponwheel_dofblur", "1", FCVAR_ARCHIVE, "Enables depth-of-field blur while the weapon wheel is active.");

bool bWheelActive = false;
void IN_WeaponWheelDown()
{	
	bWheelActive = true;
}

void IN_WeaponWheelUp()
{
	bWheelActive = false;
}

ConCommand hud_weaponwheel_on("+weaponwheel", IN_WeaponWheelDown);
ConCommand hud_weaponwheel_off("-weaponwheel", IN_WeaponWheelUp);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudWeaponWheel::CHudWeaponWheel(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudWeaponwheel"), m_rawinput( "m_rawinput" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	//SetHiddenBits(HIDEHUD_MISCSTATUS);
	SetHiddenBits(HIDEHUD_WEAPONSELECTION);

	// load the new scheme early
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible(false);
	SetProportional(true);

	//allocate the wheel
	segments = new WheelSegment[numberOfSegments];

	// Make the wheel vertices
	RefreshWheelVerts();
	//RefreshEquippedWeapons(); //deleteme - inappropriate in the constructor.

	ivgui()->AddTickSignal(GetVPanel());
}

//-----------------------------------------------------------------------------
// Purpose: Reloads/Applies the RES scheme
//-----------------------------------------------------------------------------
void CHudWeaponWheel::ApplySchemeSettings(IScheme *pScheme)
{
	// load control settings...
	LoadControlSettings("resource/UI/HudWeaponwheel.res");
	bLayoutInvalidated = true;
	BaseClass::ApplySchemeSettings(pScheme);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudWeaponWheel::ShouldDraw(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Do I even exist? - Dead men inspect no elements
	if (!pPlayer || !pPlayer->IsAlive() || !bWheelActive)
		return false;

	return CHudElement::ShouldDraw();
}

void CHudWeaponWheel::WeaponSelected(int id, int bucket, bool bCloseAfterwards)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer)
		return;

	CBaseHudWeaponSelection *hudSelection = CBaseHudWeaponSelection::GetInstance();

	int iSlot = id;

	// Reverse the flat weapon switch
	// If the player has Crowbar (0), Assault Rifle (4), and Dynamite (6),
	// these will be flattened to 0, 1, 2
	// We reverse this here because we want the wheel selection to be fixed in place, but still compatible if flat weaopn switch is on.
	// We only need to do this if a bucket is not specified; if we know the bucket, the flat weapon switching doesn't apply.
	if (of_weaponswitch_flat.GetBool() && bucket < 0)
	{
		CTFWeaponBase *pWeapon = { 0 };
		int flattenedPos = 0;
		// Iterate up to the normal slot of the weapon we've selected (e.g. Nailgun is 5 (Slot6))
		for (int i = 0; i < id; i++)
		{
			// Each non-empty weapon slot from 0 to id (Nailgun: 0 to 5) adds 1 to the flattenedPos
			// e.g. Crowbar, Revolver, Nailgun = +3 in total
			if (hudSelection->GetFirstPos(i))
			{
				pWeapon = (CTFWeaponBase *)hudSelection->GetFirstPos(i);
				if (!pWeapon) continue;

				flattenedPos++;
			}
		}

		// Send the flattened position (+1 to make it a slot rather than an index (+3 -> Slot4))
		//hudSelection->SelectWeaponSlot(flattenedPos + 1);
		iSlot = flattenedPos;
	}

	if (bucket >= 0)
	{
		CTFWeaponBase *pWeapon = dynamic_cast< CTFWeaponBase * >(hudSelection->GetWeaponInSlot(iSlot, bucket)); //weapon->entindex()

		if (pWeapon)
			::input->MakeWeaponSelection(pWeapon);
	}
	else
	{
		// Select the weapon without a bucket
		hudSelection->SelectWeaponSlot(iSlot + 1);
	}

	// This does the equivalent of selecting a slot and clicking to select it
	if (!hud_fastswitch.GetBool())
		hudSelection->SelectWeapon();

	if (bCloseAfterwards)
		bWheelActive = false;
}

void CHudWeaponWheel::CheckMousePos()
{
	::input->ActivateMouse();
	::input->AccumulateMouse();

	int deltaX = ::inputsystem->GetAnalogDelta( AnalogCode_t::MOUSE_X );
	int deltaY = ::inputsystem->GetAnalogDelta( AnalogCode_t::MOUSE_Y );
	
#ifdef LINUX
	//some awesome Linux only JANK to see if the delta values are valid

	if(abs(deltaX) > 200 || abs(deltaY) > 200)  //super ultra janky.
	{
		//as far as I can tell, the player cannot move the mouse bigger than about
		//20ish delta, so something must be wrong here.  lets reset.
		DevMsg("BAD DELTA\n");
		deltaX = 0;
		deltaY = 0;
	}
#endif
	//::input->ResetMouse();

	

	// the Cursor Limit is the farthest the cursor can go from the centre, the
	// deadzone is the minimum distance before the graphic snaps to the edge and can select spokes
	m_v2virtualCursorPos += Vector2D(deltaX, deltaY);
	if (m_v2virtualCursorPos.Length() > m_flWheelRadius * m_flWheelCursorLimit)
	{
		m_v2virtualCursorPos = (m_v2virtualCursorPos / m_v2virtualCursorPos.Length()) * (m_flWheelRadius * m_flWheelCursorLimit);
	}


	int x = m_v2virtualCursorPos.x;
	int y = m_v2virtualCursorPos.y;
	float distance = m_v2virtualCursorPos.Length();

	// width of each segment
	float pointAngleFromCentre = 360 / numberOfSegments;

	// If the cursor is outside of the wheel (+ the margin size) and it's been inside of the wheel previously (workaround)
	//if (distanceSqrd >= (m_flWheelRadius + m_iWheelMargin)*(m_flWheelRadius + m_iWheelMargin))
	
	// This is the minimum displacement before the cursor is able to select something
	float deadzone = (m_flWheelRadius * m_flWheelDeadzone);

	// If the virtual cursor is outside the deadzone, select the closest panel based on angle
	if (distance >= deadzone)
	{
		//bLinuxMouseFix = true;
		//if (bHasCursorBeenInWheel)
		//{
			float mousePosAsAngle = RAD2DEG(atan2(-(float)y, (float)x)) + 90.0f - pointAngleFromCentre;
			int selected = RoundFloatToInt(mousePosAsAngle / (360 / numberOfSegments)) + 1;
			if (selected < 0)
				selected += 8;

			if (hud_weaponwheel_quickswitch.GetInt() > 0)
			{
				WeaponSelected(selected);
			}
			else
			{
				if (slotSelected != selected)
				{
					slotSelected = selected;
					useHighlighted = true;
				}
			}
		/*}
		else
		{
			// The cursor has been registered as having been inside the centre circle;
			// after this, it can select things (see above).
			bHasCursorBeenInWheel = true;
		}*/
	}
	else
	{
		slotSelected = -1;
		useHighlighted = false;
	}
}

// DO NOT CALL THIS EVERY FRAME.
void CHudWeaponWheel::RefreshEquippedWeapons(void)
{
	if (GetHudWeaponSelection())
	{
		for (int slot = 0; slot < numberOfSegments; slot++)
		{
			bool isFirstBucket = true;
			// Iterate over the slot & buckets
			CBaseCombatWeapon *weaponInSlot;
			for (int bucketSlot = 0; bucketSlot < MAX_WEAPON_POSITIONS; bucketSlot++)
			{
				weaponInSlot = GetHudWeaponSelection()->GetWeaponInSlot(slot, bucketSlot);
				if (weaponInSlot)
				{
					segments[slot].imageIcon[bucketSlot] = weaponInSlot->GetSpriteActive();
					segments[slot].bHasIcon = true;

					if (isFirstBucket)
					{
						segments[slot].defaultBucket = bucketSlot;
						segments[slot].bucketSelected = bucketSlot;
						isFirstBucket = false;
					}
				}

			}
		}
	}
}

void CHudWeaponWheel::SetBlurLerpTimer(float t)
{
	// Set the timer to m_flBlurLerpTimeOn or m_flBlurLerpTimeOff
	m_flLerpTime = gpGlobals->curtime + t;
	m_bLerpDone = false;
	m_flTargetFadeTime = t;
}

void CHudWeaponWheel::PerformBlurLerp()
{
	// Remap time to a 0 to 1 lerp amount
	float timeRemaining = max(m_flLerpTime - gpGlobals->curtime, 0.0f);
	float lerpAmount = 1.0f - (timeRemaining / m_flTargetFadeTime);

	// if blur is turning on (true), lerp between min and max. If it's turning off, lerp between max and min.
	float blurScale = Lerp(
		lerpAmount,
		m_bBlurEnabled ? m_flDOFBlurScaleMin : m_flDOFBlurScaleMax,
		m_bBlurEnabled ? m_flDOFBlurScaleMax : m_flDOFBlurScaleMin
		);

	SetDOFBlurScale(blurScale);

	if (lerpAmount >= 1.0f)
	{
		m_bLerpDone = true;

		if (!bWheelActive)
		{
			// Switches off the DoF Blur for good (stops it even being considered in the post process cycle; performance!)
			SetDOFBlurEnabled(false);
		}
	}
}

void CHudWeaponWheel::RefreshWheelVerts(void)
{
	//============================================================================
	// Cursor vertices
	Vertex_t testverts[4];
	cursorBaseVerts[0].Init(
		Vector2D(-12, 12),
		Vector2D(0, 1)
		);
	cursorBaseVerts[1].Init(
		Vector2D(-12, -12),
		Vector2D(0, 0)
		);
	cursorBaseVerts[2].Init(
		Vector2D(12, -12),
		Vector2D(1, 0)
		);
	cursorBaseVerts[3].Init(
		Vector2D(12, 12),
		Vector2D(1, 1)
		);
	//============================================================================

	// Scale things relative to 1080p, otherwise 720p looks bloody HUGE
	m_flScreenScaleFactor = (float)m_iLastScreenHeight / (float)iReferenceScreenHeight;

	float pointAngleFromCentre = 360 / (2 * numberOfSegments);

	// From the top, clockwise, in deg
	float currentCentreAngle = 0;
	for (int i = 0; i < numberOfSegments; i++)
	{
		WheelSegment segment = WheelSegment();

		segment.centreAngle = currentCentreAngle;

		segment.centreX = iCentreWheelX + (sin(DEG2RAD(currentCentreAngle)) * ((m_flWheelRadius + m_iWheelMargin) * m_flScreenScaleFactor));
		segment.centreY = iCentreWheelY + (cos(DEG2RAD(currentCentreAngle)) * (m_flWheelRadius + m_iWheelMargin) * m_flScreenScaleFactor);

		Vector2D centreToPoint = Vector2D(sin(DEG2RAD(currentCentreAngle)), cos(DEG2RAD(currentCentreAngle)));

		segment.vertices[0].Init(
			Vector2D(iCentreWheelX + (sin(DEG2RAD(currentCentreAngle - pointAngleFromCentre + (m_flSegmentMargin / 2))) * ((m_flWheelRadius + m_iWheelMargin) * m_flScreenScaleFactor)), iCentreWheelY + (cos(DEG2RAD(currentCentreAngle - pointAngleFromCentre + (m_flSegmentMargin / 2))) * ((m_flWheelRadius + m_iWheelMargin) * m_flScreenScaleFactor))),
			Vector2D(0, 0)
			);

		segment.vertices[1].Init(
			Vector2D(iCentreWheelX + (sin(DEG2RAD(currentCentreAngle + pointAngleFromCentre - (m_flSegmentMargin / 2))) * ((m_flWheelRadius + m_iWheelMargin) * m_flScreenScaleFactor)), iCentreWheelY + (cos(DEG2RAD(currentCentreAngle + pointAngleFromCentre - (m_flSegmentMargin / 2))) * ((m_flWheelRadius + m_iWheelMargin) * m_flScreenScaleFactor))),
			Vector2D(1, 0)
			);

		segment.vertices[2].Init(
			Vector2D(iCentreWheelX + (sin(DEG2RAD(currentCentreAngle + pointAngleFromCentre - (m_flSegmentMargin / 2))) * ((m_flWheelRadius + m_iWheelMargin + m_flOuterRadius) * m_flScreenScaleFactor)), iCentreWheelY + (cos(DEG2RAD(currentCentreAngle + pointAngleFromCentre - (m_flSegmentMargin / 2))) * ((m_flWheelRadius + m_iWheelMargin + m_flOuterRadius) * m_flScreenScaleFactor))),
			Vector2D(1, 1)
			);

		segment.vertices[3].Init(
			Vector2D(iCentreWheelX + (sin(DEG2RAD(currentCentreAngle - pointAngleFromCentre + (m_flSegmentMargin / 2))) * ((m_flWheelRadius + m_iWheelMargin + m_flOuterRadius) * m_flScreenScaleFactor)), iCentreWheelY + (cos(DEG2RAD(currentCentreAngle - pointAngleFromCentre + (m_flSegmentMargin / 2))) * ((m_flWheelRadius + m_iWheelMargin + m_flOuterRadius) * m_flScreenScaleFactor))),
			Vector2D(0, 1)
			);

		segment.angleSin = sin(DEG2RAD(currentCentreAngle));
		segment.angleCos = cos(DEG2RAD(currentCentreAngle));

		// To prevent the invalidated layout from overriding the weapon slot, copy the selected bucket info
		segment.bHasIcon = segments[i].bHasIcon;
		if (segment.bHasIcon)
		{
			for (int n = 0; n < MAX_WEAPON_POSITIONS; n++)
			{
				if (segments[i].imageIcon[n] != NULL)
				{
					segment.imageIcon[n] = segments[i].imageIcon[n];
				}
			}
		}

		segment.bucketSelected = segments[i].bucketSelected;
		segment.defaultBucket = segments[i].defaultBucket;

		segments[i] = segment;

		currentCentreAngle += 360 / numberOfSegments;
	}

	bLayoutInvalidated = false;
}

void CHudWeaponWheel::RefreshCentre(void)
{
	int width, height;
	GetSize(width, height);

	// iCentreScreen where the UI will attempt to create the wheel each time - it's basically constant
	iCentreScreenX = width / 2;
	iCentreScreenY = height / 2;

	// iCentreWheel is where the mouse was immediately after the UI was opened (This is the same as the above on Windows,
	// but Linux currently struggles to set the cursor position so it can be variable.)
	iCentreWheelX = width * m_flWheelPosX;
	iCentreWheelY = height * m_flWheelPosY;

	// If the user has changed resolution, invalidate and reload the vertices
	if (height != m_iLastScreenHeight)
	{
		m_iLastScreenHeight = height;
		bLayoutInvalidated = true;
	}

	m_v2virtualCursorPos = Vector2D(0.0f, 0.0f);
}

void CHudWeaponWheel::DrawString(const wchar_t *text, int xpos, int ypos, Color col, bool bCenter)
{
	surface()->DrawSetTextColor(col);
	surface()->DrawSetTextFont(m_hTextFont);

	// count the position
	int slen = 0, charCount = 0, maxslen = 0;

	for (const wchar_t *pch = text; *pch != 0; pch++)
	{
		if (*pch == '\n')
		{
			//store the current max line length and reset
			maxslen = max(slen, maxslen);
			slen = 0;
		}
		else if (*pch != '\r')
		{
			slen += surface()->GetCharacterWidth(m_hTextFont, *pch);
			charCount++;
		}
	}

	//set x pos depending on the maximum line length
	if (bCenter)
		xpos -= max(slen, maxslen) * 0.5;

	surface()->DrawSetTextPos(xpos, ypos);
	surface()->DrawPrintText(text, charCount);
}

void CHudWeaponWheel::Paint(void)
{
	m_flScreenScaleFactor = (float)m_iLastScreenHeight / (float)iReferenceScreenHeight;
	float wheelRadius = m_flWheelRadius * m_flScreenScaleFactor;

	surface()->DrawSetColor(Color(255, 255, 255, 255));
	surface()->DrawSetTexture(m_nCircleTextureId);
	surface()->DrawTexturedRect(iCentreWheelX - wheelRadius, iCentreWheelY - wheelRadius, iCentreWheelX + wheelRadius, iCentreWheelY + wheelRadius);

	// Spokes!	
	for (int i = 0; i < numberOfSegments; i++)
	{
		if (i == slotSelected)
			surface()->DrawSetTexture(m_nPanelHighlightedTextureId);
		else
			surface()->DrawSetTexture(m_nPanelTextureId);

		WheelSegment segment = segments[i];
		surface()->DrawTexturedPolygon(NUM_VERTS_SPOKE, segment.vertices);
	}

	// Now the outline, slot number, and ammo
	for (int i = 0; i < numberOfSegments; i++)
	{
		WheelSegment segment = segments[i];
		// Slot number + shadow
		int offset = (m_flWheelRadius + m_iTextOffset * m_flScreenScaleFactor);

		int xpos = iCentreWheelX + (segment.angleSin * offset);
		int ypos = iCentreWheelY + (segment.angleCos * offset);

		//DrawString(slotNames[i], xpos + 1, ypos + 1, Color(0, 0, 0, 255), true);
		//DrawString(slotNames[i], xpos, ypos, Color(255, 255, 255, 255), true);

		CBaseHudWeaponSelection* weaponSelect = GetHudWeaponSelection();

		// Display the currently select weapon's icon and ammo
		C_BaseCombatWeapon *pWeapon = weaponSelect->GetWeaponInSlot(i, segment.bucketSelected);

		//Color playerColour = Color(of_color_r.GetInt(), of_color_g.GetInt(), of_color_b.GetInt(), 255);
		if (pWeapon)
		{
			// Draw the icon
			if (segment.imageIcon[segment.bucketSelected] != NULL && segment.bHasIcon)
			{
				offset += (60 * m_flScreenScaleFactor);
				xpos = iCentreWheelX + (segment.angleSin * offset);
				ypos = iCentreWheelY + (segment.angleCos * offset);

				int iconWide = segment.imageIcon[segment.bucketSelected]->EffectiveWidth(1.0f) * WEAP_IMAGE_SCALE * m_flScreenScaleFactor;
				int iconTall = segment.imageIcon[segment.bucketSelected]->EffectiveHeight(1.0f) * WEAP_IMAGE_SCALE * m_flScreenScaleFactor;

				// Icon Shadow
				segment.imageIcon[segment.bucketSelected]->DrawSelf(xpos - (iconWide / 2) + m_iShadowOffset, ypos - (iconTall / 2) + m_iShadowOffset, iconWide, iconTall, Color(0, 0, 0, m_iShadowAlpha));
				// Icon
				segment.imageIcon[segment.bucketSelected]->DrawSelf(xpos - (iconWide / 2), ypos - (iconTall / 2), iconWide, iconTall, Color(255, 255, 255, 255));
			}

			Color ammoColor = !pWeapon->CanBeSelected() ? Color(255, 0, 0, 255) : Color(255, 255, 255, 255);
			wchar_t pText[64];
			bool hasAmmo = true;

			if (pWeapon->Clip1() > -1)
				g_pVGuiLocalize->ConstructString(pText, sizeof(pText), VarArgs("%d/%d", pWeapon->Clip1(), pWeapon->ReserveAmmo()), 0);
			else if (pWeapon->Clip2() > -1)
				g_pVGuiLocalize->ConstructString(pText, sizeof(pText), VarArgs("%d/%d", pWeapon->Clip2(), pWeapon->ReserveAmmo()), 0);
			else if (pWeapon->ReserveAmmo() > -1)
				g_pVGuiLocalize->ConstructString(pText, sizeof(pText), VarArgs("%d", pWeapon->ReserveAmmo()), 0);
			else
				hasAmmo = false;

			if (hasAmmo)
			{
				DrawString(pText, xpos + TEXT_SHADOW_OFFSET, ypos + TEXT_SHADOW_OFFSET, Color(0, 0, 0, 255), true);
				DrawString(pText, xpos, ypos, ammoColor, true);
			}

			if (i == slotSelected)
			{
				wchar_t wepText[64];
				g_pVGuiLocalize->ConstructString(wepText, sizeof(wepText), VarArgs("%s", pWeapon->GetPrintName()), 0);
				DrawString(wepText, iCentreWheelX + TEXT_SHADOW_OFFSET, iCentreWheelY + TEXT_SHADOW_OFFSET, Color(0, 0, 0, 255), true);
				DrawString(wepText, iCentreWheelX, iCentreWheelY, Color(245, 245, 245, 255), true);
			}
		}
	}

	surface()->DrawSetColor(Color(255, 255, 255, 255));

	// Draw the virtual cursor
	// Draw an arrow at the edge of the wheel if beyond the deadzone
	if (m_v2virtualCursorPos.Length() > m_flWheelRadius * m_flWheelDeadzone)
	{
		float angle = atan2f(m_v2virtualCursorPos.y, m_v2virtualCursorPos.x) + (M_PI_F / 2);
		float cosAngle = cosf(angle);
		float sinAngle = sinf(angle);
		Vector2D edgePos = (m_v2virtualCursorPos / m_v2virtualCursorPos.Length()) * m_flWheelRadius;

		Vertex_t rotatedVerts[CURSOR_VERTS];
		Vertex_t rotatedVertsShadow[CURSOR_VERTS];
		for (int i = 0; i < CURSOR_VERTS; i++)
		{
			// Copy & Scale
			rotatedVerts[i] = Vertex_t(cursorBaseVerts[i]);
			float x = rotatedVerts[i].m_Position.x * m_flArrowSize;
			float y = rotatedVerts[i].m_Position.y * m_flArrowSize;

			// Rotate by angle and translate
			rotatedVerts[i].m_Position = Vector2D(
				(x * cosAngle) - (y * sinAngle) + edgePos.x + iCentreWheelX,
				(x * sinAngle) + (y * cosAngle) + edgePos.y + iCentreWheelY
				);

			rotatedVertsShadow[i] = Vertex_t( rotatedVerts[i] );
			rotatedVertsShadow[i].m_Position += Vector2D(m_iCursorShadowOffset, m_iCursorShadowOffset);
		}
//		Vector2D edgePos = (m_v2virtualCursorPos / m_v2virtualCursorPos.Length()) * m_flWheelRadius;
//		surface()->DrawOutlinedCircle(iCentreWheelX + edgePos.x, iCentreWheelY + edgePos.y, 10, 12);
		//cursorBaseVerts  m_nDotTextureId
		surface()->DrawSetTexture(m_nArrowShadowTextureId);
		surface()->DrawTexturedPolygon(CURSOR_VERTS, rotatedVertsShadow);
		surface()->DrawSetTexture(m_nArrowTextureId);
		surface()->DrawTexturedPolygon(CURSOR_VERTS, rotatedVerts);
	}
	else
	{
//		surface()->DrawSetColor(Color(50, 50, 255, 255));
//		surface()->DrawOutlinedCircle(iCentreWheelX + m_v2virtualCursorPos.x, iCentreWheelY + m_v2virtualCursorPos.y, 10, 12);

		Vertex_t translatedVerts[CURSOR_VERTS];
		Vertex_t translatedVertsShadow[CURSOR_VERTS];
		for (int i = 0; i < CURSOR_VERTS; i++)
		{
			// Copy
			translatedVerts[i] = Vertex_t( cursorBaseVerts[i] );
			// Scale
			translatedVerts[i].m_Position *= m_flDotSize * m_flScreenScaleFactor;
			// Translate
			translatedVerts[i].m_Position.x += m_v2virtualCursorPos.x + iCentreWheelX;
			translatedVerts[i].m_Position.y += m_v2virtualCursorPos.y + iCentreWheelY;

			translatedVertsShadow[i] = Vertex_t( cursorBaseVerts[i] );
			// Scale (*2 because the dot shadow texture is 128x128 to allow the shadow to cover a larger area)
			translatedVertsShadow[i].m_Position *= m_flDotSize * 2 * m_flScreenScaleFactor;
			// Translate
			translatedVertsShadow[i].m_Position.x += m_v2virtualCursorPos.x + iCentreWheelX + m_iCursorShadowOffset;
			translatedVertsShadow[i].m_Position.y += m_v2virtualCursorPos.y + iCentreWheelY + m_iCursorShadowOffset;
		}

		surface()->DrawSetTexture(m_nDotShadowTextureId);
		surface()->DrawTexturedPolygon(CURSOR_VERTS, translatedVertsShadow);
		surface()->DrawSetTexture(m_nDotTextureId);
		surface()->DrawTexturedPolygon(CURSOR_VERTS, translatedVerts);
	}

	
/*	// test
	int x, y;
	x = iCentreScreenX;
	y = iCentreScreenY;
	Vertex_t testverts[4];
	testverts[0].Init(
		Vector2D(x, y),
		Vector2D(0, 1)
		);
	testverts[1].Init(
		Vector2D(x, y - 25),
		Vector2D(0, 0)
		);
	testverts[2].Init(
		Vector2D(x + 25, y - 25),
		Vector2D(1, 0)
		);
	testverts[3].Init(
		Vector2D(x + 25, y),
		Vector2D(1, 1)
		);
	surface()->DrawSetTexture(m_nArrowTextureId);
	surface()->DrawTexturedPolygon(CURSOR_VERTS, testverts); */
}

//-----------------------------------------------------------------------------
// Purpose: Every think/update tick that the GUI uses
//-----------------------------------------------------------------------------
void CHudWeaponWheel::OnTick(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if (!pPlayer)
		return;

	// attempt to tell the darkening overlay to go away
	SetPaintBackgroundEnabled(false);

	// If we've still lerping to be done, do it!
	if (!m_bLerpDone && hud_weaponwheel_dofblur.GetBool())
		PerformBlurLerp();

#ifdef LINUX
	//Linux fixes by Fenteale: here is the main part of the linux fix.  ill add comments to say whats goin on best I can
	if (bLinuxMouseFix)  //this is set TRUE whenever the mouse needs to be repositioned by code elsewhere
	{
		//not sure if these 3 lines are needed, but its working now so I dont wanna mess with it.
		SetMouseInputEnabled(true);
		SetKeyBoardInputEnabled(false);	
		surface()->SetSoftwareCursor(true);
		int curPosX, curPosY;
		/*
		Here is the root of the issue:
		With mouse raw input ENABLED: the mouse cannot be repositioned by the engine, and the delta does
			not change when the cursor hits the sides of the screen
		With mouse raw input DISABLED: the mouse CAN be repositioned, but the delta values returned by
			::inputsystem->GetAnalogDelta are incorrect.
		
		So we gotta force switching between the two modes depending what we need.  To add MORE jank into the mix,
		the engine needs one frame before the change in m_rawinput is registered.  This might add one frame when 
		switching modes where input is ignored, but I cannot for the life of me find any other way.
		*/
		if(m_rawinput.GetBool())
			m_rawinput.SetValue(false); //first time through the loop: set m_rawinput off
		else
		{ //second time through the loop:
			DevMsg("Attempting to center mos pos\n");	
			vgui::input()->SetCursorPos(iCentreScreenX, iCentreScreenY); //center mouse pos

			vgui::input()->GetCursorPos(curPosX, curPosY);
			if(abs(curPosX - iCentreScreenX) < 50 && abs(curPosY - iCentreScreenY) < 50)
			{ //mouse repositioned successfully!
				
				m_rawinput.SetValue(true);  //so we can force re-enable raw mouse input and continue on
				bLinuxMouseFix = false;
			}
			else
			{
				DevMsg("WTF %i|%i\n", curPosX - iCentreScreenX, curPosY - iCentreScreenY);
				//Mouse position is NOT in the center after resetting it?
			}
		}
			
	}
#endif

	// If weapon wheel active bool has changed, change mouse input capabilities etc
	if (lastWheel != bWheelActive)
	{
#ifdef LINUX
		if(bWheelActive) //if it was just activated
		{
			m_rawinput_stored = m_rawinput.GetBool(); //store the user's settings for rawinput to restore later
			//DevMsg("Linux workaround: rawinput disabled\n");
			
			//bLinuxMouseFix = true; //used to have this enabled which would reset the mouse pos as soon as the weapon wheel was brought up,
			//but this caused the player's aim to occasionally snap around upon activation.

			//these next few lines I'm not sure if they are needed but its working well right now so
			//I dont want to touch it.
			SetMouseInputEnabled(true);
			SetKeyBoardInputEnabled(false);
			surface()->SetSoftwareCursor(true);

		}
		else //if its closing
		{
			//DevMsg("Linux workaround: rawinput restored\n");
			m_rawinput.SetValue(m_rawinput_stored);
		}
#endif
		CheckWheel();
		

	}
	lastWheel = bWheelActive;

	if (bWheelActive)
	{
#ifdef LINUX
		if(!bLinuxMouseFix)  //if mouse position reset is not pending
#endif
			CheckMousePos();

#ifdef LINUX
		//see if mouse is hitting the edges of the screen.  set bLinuxMouseFix on if so.
		int curPosX_b, curPosY_b;
		vgui::input()->GetCursorPos(curPosX_b, curPosY_b);
		if(curPosX_b == 0 || curPosY_b == 0 || curPosX_b >= (iCentreScreenX * 2) - 1 || curPosY_b >= (iCentreScreenY *2) - 1)
			bLinuxMouseFix = true;
#endif

	}

	

	// Scan for changes in the number of weapons we have
	int weaponsThisTick = 0;
	CBaseHudWeaponSelection *weaponSelect = GetHudWeaponSelection();
	if (weaponSelect)
	{
		for (int slot = 0; slot < numberOfSegments; slot++)
		{
			for (int bucket = 0; bucket < MAX_WEAPON_POSITIONS; bucket++)
			{
				if (weaponSelect->GetWeaponInSlot(slot, bucket))
					weaponsThisTick++;
			}
		}

		// If there is a discrepancy, check the weapons and their icons again
		if (weaponsThisTick != iNumberOfWeaponsEquipped)
		{
			RefreshEquippedWeapons();
			iNumberOfWeaponsEquipped = weaponsThisTick;
		}

		// Update the current weapon's segment to reflect what we have equipped
		CTFWeaponBase *pSelectedWeapon = pPlayer->GetActiveTFWeapon();
		if (pSelectedWeapon)
		{
			// or is it meant to be +/-1
			segments[pSelectedWeapon->GetSlot()].bucketSelected = pSelectedWeapon->GetPosition();
		}
	}
}

void CHudWeaponWheel::CheckWheel()
{
	if (bWheelActive)
	{
		// This is safe to do every frame/opening of the wheel
		RefreshCentre();
		
		// THIS IS NOT. Doing so will reset the player's chosen weapons constantly. We do not want to do this.
		// If we do that, each slot will revert back to the same weapon every frame, forgetting what the player selected in that slot.

		if (bLayoutInvalidated)
		{
			RefreshWheelVerts();
			RefreshEquippedWeapons();
		}

		// Call this when the player's equipped weapons change. This is done in ::OnTick and ONLY when the player picks up a new weapon/drops one.
		//RefreshEquippedWeapons();

		bHasCursorBeenInWheel = false;

//		Activate();
		SetMouseInputEnabled(true);			// Capture the mouse...
		SetKeyBoardInputEnabled(false);		// ...but not the keyboard!
		// Replaced the cursor with an $alpha 0 image, since Valve didn't want to give me a way to hide the cursor or prevent mouselook.
		surface()->SetSoftwareCursor(true);

//		vgui::input()->SetCursorPos(iCentreWheelX, iCentreWheelY);

		// since Linux can't snap to centre :( we just start the weapon wheel wherever their mouse is
		// On Windows, this should still let us start at iCentreScreenXY
		// vgui::input()->GetCursorPos(iCentreWheelX, iCentreWheelY);

		if (hud_weaponwheel_dofblur.GetBool())
		{
			SetDOFBlurEnabled(true);

			// this var is just used to decide the lerp direction, it doesn't toggle it on/off
			m_bBlurEnabled = true;
			SetBlurLerpTimer(m_flBlurLerpTimeOn);
		}
	}
	else
	{
		// Switch to the one the mouse is over
		if (useHighlighted)
		{
			// Select it and close
			WeaponSelected(slotSelected, segments[slotSelected].bucketSelected);
		}
		SetMouseInputEnabled(false);
		SetVisible(false);

		surface()->SetSoftwareCursor(false);

		m_bBlurEnabled = false;
		SetBlurLerpTimer(m_flBlurLerpTimeOff);
	}

	SetDOFBlurDistance(m_flDOFBlurDistance);
}


void CHudWeaponWheel::OnMouseWheeled(int delta)
{
	// Simplify it to +1 or -1
	delta /= abs(delta);

	if (bWheelActive)
	{
		C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
		if (!pPlayer)
			return;

		if (slotSelected >= 0)
		{
			// proper forward and backward cycling
			CBaseHudWeaponSelection *pHUDSelection = CBaseHudWeaponSelection::GetInstance();
			// How many weapons are there in the slot we've selected?
			int wepsCurrentlyInSlot = 0;
			int lastValidIndex = -1;
			int currentBucket = segments[slotSelected].bucketSelected;
			for (int i = 0; i < MAX_WEAPON_POSITIONS; i++)
			{
				if (pHUDSelection->GetWeaponInSlot(slotSelected, i))
				{
					wepsCurrentlyInSlot++;
					lastValidIndex = i;
				}
			}

			if (wepsCurrentlyInSlot > 0)
			{
				int iterations = 0;
				int bucket = currentBucket; // e.g. bucket 4 - 1 = 3
				// Keep going in the direction of delta until we find a valid weapon (+1 = forwards, -1 = backwards)
				while (iterations < MAX_WEAPON_POSITIONS)
				{
					bucket += delta;

					// wrap it around
					if (bucket > lastValidIndex)
						bucket = 0;
					if (bucket < 0)
						bucket = lastValidIndex;

					if (pHUDSelection->GetWeaponInSlot(slotSelected, bucket))
						break;

					iterations++;
				}

				// Used to stop being able to cycle slots with 1 weapon
				if (segments[slotSelected].bucketSelected != bucket)
				{
					WeaponSelected(slotSelected, bucket, false);
					segments[slotSelected].bucketSelected = bucket;
				}
			}

			// We only need to disable selecting the highlighted one if the player actually changes a slot's weapon
			if (wepsCurrentlyInSlot > 1)
				useHighlighted = false;
		}
	}
}