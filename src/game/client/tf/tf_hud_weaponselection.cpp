//========= Copyright � 1996-2006, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "weapon_selection.h"
#include "iclientmode.h"
#include "history_resource.h"
#include "vgui/ILocalize.h"
#include <vgui/ISurface.h>
#include <vgui_controls/AnimationController.h>
#include "c_tf_player.h"
#include "tf_imagepanel.h"
#include <vgui_controls/EditablePanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SELECTION_TIMEOUT_THRESHOLD		2.5f	// Seconds
#define SELECTION_FADEOUT_TIME			3.0f

#define FASTSWITCH_DISPLAY_TIMEOUT		0.5f
#define FASTSWITCH_FADEOUT_TIME			0.5f

ConVar tf_weapon_select_demo_start_delay( "tf_weapon_select_demo_start_delay", "1.0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Delay after spawning to start the weapon bucket demo." );
ConVar tf_weapon_select_demo_time( "tf_weapon_select_demo_time", "0.5", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Time to pulse each weapon bucket upon spawning as a new class. 0 to turn off." );
ConVar of_hide_weapon_selection( "of_hide_weapon_selection", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "When enabled, makes the weapon selection hidden after selecting a weapon." );
ConVar of_weaponswitch_flat( "of_weaponswitch_flat", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "When enabled, makes numkeys switch to slot in respect to available weapons." );

//-----------------------------------------------------------------------------
// Purpose: tf weapon selection hud element
//-----------------------------------------------------------------------------
class CHudWeaponSelection : public CBaseHudWeaponSelection, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudWeaponSelection, vgui::Panel );

public:
	CHudWeaponSelection(const char *pElementName );

	virtual bool ShouldDraw();
	virtual void OnWeaponPickup( CTFWeaponBase *pWeapon );
	virtual void SelectWeapon();

	virtual void CycleToNextWeapon( void );
	virtual void CycleToPrevWeapon( void );

	virtual CTFWeaponBase *GetWeaponInSlot( int iSlot, int iSlotPos );
	virtual void SelectWeaponSlot( int iSlot );
	virtual bool AffectedByDrawHUD( void ){ return false; }

	virtual CTFWeaponBase	*GetSelectedWeapon( void );

	virtual void OpenSelection( void );
	virtual void HideSelection( void );

	virtual void Init();
	virtual void LevelInit();
	
	virtual void FireGameEvent( IGameEvent *event );

	virtual void Reset(void)
	{
		CBaseHudWeaponSelection::Reset();

		// selection time is a little farther back so we don't show it when we spawn
		m_flSelectionTime = gpGlobals->curtime - ( FASTSWITCH_DISPLAY_TIMEOUT + FASTSWITCH_FADEOUT_TIME + 0.1 );
	}

protected:
	virtual void OnThink();
	virtual void PostChildPaint();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	virtual bool IsWeaponSelectable()
	{ 
		if (IsInSelectionMode())
			return true;

		return false;
	}

private:
	CTFWeaponBase *FindNextWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition);
	CTFWeaponBase *FindPrevWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition);

	void FastWeaponSwitch( int iWeaponSlot );
	void PlusTypeFastWeaponSwitch( int iWeaponSlot );
	int GetNumVisibleSlots();
	bool ShouldDrawInternal();

	virtual	void SetSelectedWeapon( CTFWeaponBase *pWeapon ) 
	{ 
		m_hSelectedWeapon = pWeapon;
	}

	virtual	void SetSelectedSlot( int slot ) 
	{ 
		m_iSelectedSlot = slot;
	}

	void DrawBox(int x, int y, int wide, int tall, Color color, float normalizedAlpha, int number);
	void DrawString( wchar_t *text, int xpos, int ypos, Color col, bool bCenter = false );

	void DrawPlusStyleBox(int x, int y, int wide, int tall, bool bSelected, float normalizedAlpha, int number, bool bOutOfAmmo );

	CPanelAnimationVar( vgui::HFont, m_hNumberFont, "NumberFont", "HudSelectionText" );
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "HudSelectionText" );

	CPanelAnimationVarAliasType( float, m_flSmallBoxWide, "SmallBoxWide", "32", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flSmallBoxTall, "SmallBoxTall", "21", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flPlusStyleBoxWide, "PlusStyleBoxWide", "120", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flPlusStyleBoxTall, "PlusStyleBoxTall", "84", "proportional_float" );
	CPanelAnimationVar( float, m_flPlusStyleExpandPercent, "PlusStyleExpandSelected", "0.3" )

	CPanelAnimationVarAliasType( float, m_flLargeBoxWide, "LargeBoxWide", "108", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flLargeBoxTall, "LargeBoxTall", "72", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flBoxGap, "BoxGap", "12", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flRightMargin, "RightMargin", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flSelectionNumberXPos, "SelectionNumberXPos", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flSelectionNumberYPos, "SelectionNumberYPos", "4", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flSelectionNumberZPos, "SelectionNumberZPos", "4", "proportional_float" );

	CPanelAnimationVar( int, m_iWeaponBGXPos, "WeaponBGXPos", "10" );
	CPanelAnimationVar( int, m_iWeaponBGYPos, "WeaponBGYPos", "10" );

	CPanelAnimationVarAliasType( float, m_flIconXPos, "IconXPos", "16", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flIconYPos, "IconYPos", "8", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flTextYPos, "TextYPos", "54", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flErrorYPos, "ErrorYPos", "60", "proportional_float" );

	CPanelAnimationVar( float, m_flAlphaOverride, "Alpha", "255" );
	CPanelAnimationVar( float, m_flSelectionAlphaOverride, "SelectionAlpha", "255" );

	CPanelAnimationVar( Color, m_TextColor, "TextColor", "SelectionTextFg" );
	CPanelAnimationVar( Color, m_NumberColor, "NumberColor", "SelectionNumberFg" );
	CPanelAnimationVar( Color, m_NumberShadowColor, "NumberShadowColor", "SelectionNumberBg" );
	CPanelAnimationVar( Color, m_EmptyBoxColor, "EmptyBoxColor", "255 255 255 255" );
	CPanelAnimationVar( Color, m_BoxColor, "BoxColor", "SelectionBoxBg" );
	CPanelAnimationVar( Color, m_SelectedBoxColor, "SelectedBoxClor", "SelectionSelectedBoxBg" );

	CPanelAnimationVar( float, m_flWeaponPickupGrowTime, "SelectionGrowTime", "0.1" );

	CPanelAnimationVar( float, m_flTextScan, "TextScan", "1.0" );

	CPanelAnimationVar( int, m_iMaxSlots, "MaxSlots", "10" );
	CPanelAnimationVar( bool, m_bPlaySelectionSounds, "PlaySelectSounds", "1" );

	CTFImagePanel *m_pActiveWeaponBG;

	float m_flDemoStartTime;
	float m_flDemoModeChangeTime;
	int m_iDemoModeSlot;

	// HUDTYPE_PLUS weapon display
	int						m_iSelectedBoxPosition;		// in HUDTYPE_PLUS, the position within a slot
	int						m_iSelectedSlot;			// in HUDTYPE_PLUS, the slot we're currently moving in
	CPanelAnimationVar( float, m_flHorizWeaponSelectOffsetPoint, "WeaponBoxOffset", "0" );

	int m_iBGImage_Inactive;
	int m_iBGImage_Blue;
	int m_iBGImage_Red;
};

DECLARE_HUDELEMENT( CHudWeaponSelection );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudWeaponSelection::CHudWeaponSelection( const char *pElementName ) : CBaseHudWeaponSelection( pElementName ), EditablePanel( NULL, "HudWeaponSelection" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pActiveWeaponBG = new CTFImagePanel( this, "ActiveWeapon" );
	m_pActiveWeaponBG->SetVisible( false );

	SetPostChildPaintEnabled( true );

	m_flDemoStartTime = -1;
	m_flDemoModeChangeTime = 0;
	m_iDemoModeSlot = -1;

	ListenForGameEvent( "localplayer_changeclass" );
}

//-----------------------------------------------------------------------------
// Purpose: sets up display for showing weapon pickup
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnWeaponPickup( CTFWeaponBase *pWeapon )
{
	// add to pickup history
	CHudHistoryResource *pHudHR = GET_HUDELEMENT( CHudHistoryResource );
	if ( pHudHR )
	{
		pHudHR->AddToHistory( pWeapon );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Player has chosen to draw the currently selected weapon
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SelectWeapon( void )
{
	if ( !GetSelectedWeapon() )
	{
		engine->ClientCmd( "cancelselect\n" );
		return;
	}

	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if ( !player )
		return;

	// Don't allow selections of weapons that can't be selected (out of ammo, etc)
	if ( !GetSelectedWeapon()->CanBeSelected() )
	{
		player->EmitSound( "Player.DenyWeaponSelection" );
	}
	else
	{
		SetWeaponSelected();
		
		if( of_hide_weapon_selection.GetBool() || hud_fastswitch.GetInt() != HUDTYPE_FASTSWITCH )
		{
			m_hSelectedWeapon = NULL;
	
			engine->ClientCmd( "cancelselect\n" );
		}
		// Play the "weapon selected" sound
		player->EmitSound( "Player.WeaponSelected" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: updates animation status
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OnThink()
{
	float flSelectionTimeout = SELECTION_TIMEOUT_THRESHOLD;
	float flSelectionFadeoutTime = SELECTION_FADEOUT_TIME;
	if ( hud_fastswitch.GetBool() )
	{
		flSelectionTimeout = FASTSWITCH_DISPLAY_TIMEOUT;
		flSelectionFadeoutTime = FASTSWITCH_FADEOUT_TIME;
	}

	// Time out after awhile of inactivity
	if ( ( gpGlobals->curtime - m_flSelectionTime ) > flSelectionTimeout )
	{
		// close
		if ( gpGlobals->curtime - m_flSelectionTime > flSelectionTimeout + flSelectionFadeoutTime )
		{
			HideSelection();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the panel should draw
//-----------------------------------------------------------------------------
bool CHudWeaponSelection::ShouldDraw()
{
	bool bShouldDraw = ShouldDrawInternal();

	if ( !bShouldDraw && m_pActiveWeaponBG->IsVisible() )
	{
		m_pActiveWeaponBG->SetVisible( false );
	}

	return bShouldDraw;
}

bool CHudWeaponSelection::ShouldDrawInternal()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
	{
		if ( IsInSelectionMode() )
		{
			HideSelection();
		}
		return false;
	}

	// Make sure the player's allowed to switch weapons
	if( !( !of_hide_weapon_selection.GetBool() && hud_fastswitch.GetInt() == HUDTYPE_FASTSWITCH ) && pPlayer->IsAllowedToSwitchWeapons() == false )
		return false;

	if ( pPlayer->IsAlive() == false )
		return false;

	// we only show demo mode in hud_fastswitch 0
	if ( hud_fastswitch.GetInt() == 0 && ( m_iDemoModeSlot >= 0 || m_flDemoStartTime > 0 ) )
	{
		return true;
	}

	bool bret = CBaseHudWeaponSelection::ShouldDraw();
	if ( !bret )
		return false;

	// draw weapon selection a little longer if in fastswitch so we can see what we've selected
	if ( hud_fastswitch.GetBool() && ( gpGlobals->curtime - m_flSelectionTime ) < (FASTSWITCH_DISPLAY_TIMEOUT + FASTSWITCH_FADEOUT_TIME) )
		return true;

	if( !of_hide_weapon_selection.GetBool() && hud_fastswitch.GetInt() == HUDTYPE_FASTSWITCH )
		return true;
	
	return ( m_bSelectionVisible ) ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::Init()
{
	CHudElement::Init();

	m_iBGImage_Inactive = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile(m_iBGImage_Inactive, "hud/weapon_selection_unselected", true, false);

	m_iBGImage_Blue = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile(m_iBGImage_Blue, "hud/weapon_selection_blue", true, false);

	m_iBGImage_Red = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile(m_iBGImage_Red, "hud/weapon_selection_red", true, false);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudWeaponSelection::LevelInit()
{
	CHudElement::LevelInit();
	
	m_iMaxSlots = clamp( m_iMaxSlots, 0, MAX_WEAPON_SLOTS );
}

//-------------------------------------------------------------------------
// Purpose: Calculates how many weapons slots need to be displayed
//-------------------------------------------------------------------------
int CHudWeaponSelection::GetNumVisibleSlots()
{
	int nCount = 0;

	// iterate over all the weapon slots
	for ( int i = 0; i < m_iMaxSlots; i++ )
	{
		if ( GetFirstPos( i ) )
		{
			nCount++;
		}
	}

	return nCount;
}

//-------------------------------------------------------------------------
// Purpose: draws the selection area
//-------------------------------------------------------------------------
void CHudWeaponSelection::PostChildPaint()
{
	if( !of_hide_weapon_selection.GetBool() && hud_fastswitch.GetInt() == HUDTYPE_FASTSWITCH )
	// kill any fastswitch display
	m_flSelectionTime = gpGlobals->curtime + SELECTION_FADEOUT_TIME;
	
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	if ( hud_fastswitch.GetInt() == 0 )
	{
		// See if we should start the bucket demo
		if ( m_flDemoStartTime > 0 && m_flDemoStartTime < gpGlobals->curtime )
		{
			float flDemoTime = tf_weapon_select_demo_time.GetFloat();

			if ( flDemoTime > 0 )
			{
				m_iDemoModeSlot = 0;
				m_flDemoModeChangeTime = gpGlobals->curtime + flDemoTime;
			}

			m_flDemoStartTime = -1;
			m_iSelectedSlot = m_iDemoModeSlot;
		}

		// scroll through the slots for demo mode
		if ( m_iDemoModeSlot >= 0 && m_flDemoModeChangeTime < gpGlobals->curtime )
		{
			// Keep iterating until we find a slot that has a weapon in it
			while ( !GetFirstPos( ++m_iDemoModeSlot ) && m_iDemoModeSlot < m_iMaxSlots )
			{
				// blank
			}			
			m_flDemoModeChangeTime = gpGlobals->curtime + tf_weapon_select_demo_time.GetFloat();
		}

		if ( m_iDemoModeSlot >= m_iMaxSlots )
		{
			m_iDemoModeSlot = -1;
		}
	}	

	// find and display our current selection
	CTFWeaponBase *pSelectedWeapon = NULL;
	switch ( hud_fastswitch.GetInt() )
	{
	case HUDTYPE_FASTSWITCH:
		pSelectedWeapon = pPlayer->GetActiveTFWeapon();
		break;
	default:
		pSelectedWeapon = GetSelectedWeapon();
		break;
	}
	if ( !pSelectedWeapon )
		return;

	if ( hud_fastswitch.GetInt() == 0 )
	{
		if ( m_iDemoModeSlot > -1 )
		{
			pSelectedWeapon = GetWeaponInSlot( m_iDemoModeSlot, 0 );
			m_iSelectedSlot = m_iDemoModeSlot;
			m_iSelectedBoxPosition = 0;
		}
	}

	m_pActiveWeaponBG->SetVisible( hud_fastswitch.GetInt() != HUDTYPE_PLUS && pSelectedWeapon != NULL );

	int nNumSlots = GetNumVisibleSlots();
	if ( nNumSlots <= 0 )
		return;

	switch ( hud_fastswitch.GetInt() )
	{
	case HUDTYPE_PLUS:
		{
			// bucket style
			int screenCenterX = GetWide() / 2;
			int screenCenterY = GetTall() / 2; // Height isn't quite screen height, so adjust for center alignement

			// Modifiers for the four directions. Used to change the x and y offsets
			// of each box based on which bucket we're drawing. Bucket directions are
			// 0 = UP, 1 = RIGHT, 2 = DOWN, 3 = LEFT
			int xModifiers[] = { 0, 1, 0, -1 };
			int yModifiers[] = { -1, 0, 1, 0 };

			int boxWide = m_flPlusStyleBoxWide;
			int boxTall = m_flPlusStyleBoxTall;

			// Draw the four buckets
			for ( int i = 0; i < MAX_WEAPON_SLOTS; ++i )
			{
				// Set the top left corner so the first box would be centered in the screen.
				int xPos = screenCenterX -( boxWide / 2 );
				int yPos = screenCenterY -( boxTall / 2 );

				// Find out how many positions to draw - an empty position should still
				// be drawn if there is an active weapon in any slots past it.
				int lastSlotPos = -1;
				int iMaxSlotPositions = 3;	//MAX_WEAPON_POSITIONS	- no need to do this 20 times, we only have 1 weapon usually
				for ( int slotPos = 0; slotPos < iMaxSlotPositions; ++slotPos )
				{
					CTFWeaponBase *pWeapon = GetWeaponInSlot( i, slotPos );
					if ( pWeapon )
					{
						lastSlotPos = slotPos;
					}
				}

				// Draw the weapons in this bucket
				for ( int slotPos = 0; slotPos <= lastSlotPos; ++slotPos )
				{
					// Offset the box position
					xPos += ( boxWide + 5 ) * xModifiers[ i ];
					yPos += ( boxTall + 5 ) * yModifiers[ i ];

					int x = xPos;
					int y = yPos;

					CTFWeaponBase *pWeapon = GetWeaponInSlot( i, slotPos );

					bool bSelectedWeapon = ( i == m_iSelectedSlot && slotPos == m_iSelectedBoxPosition );

					if ( pWeapon && pWeapon->VisibleInWeaponSelection() )
					{
						DrawPlusStyleBox( x, y, boxWide, boxTall, bSelectedWeapon, m_flAlphaOverride, i+1, !pWeapon->CanBeSelected() );

						const CHudTexture *pTexture = pWeapon->GetSpriteInactive(); // red team
						if ( pPlayer )
						{
							if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE || pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
							{
								pTexture = pWeapon->GetSpriteActive();
							}
						}

						if ( pTexture )
						{
							Color col(255,255,255,255);

							if ( bSelectedWeapon )
							{
								float flExpandWide = m_flPlusStyleBoxWide * m_flPlusStyleExpandPercent * 0.5;
								float flExpandTall = m_flPlusStyleBoxTall * m_flPlusStyleExpandPercent * 0.5;
								pTexture->DrawSelf( x-flExpandWide, y-flExpandTall, boxWide+flExpandWide, boxTall+flExpandTall, col );
							}
							else
							{
								pTexture->DrawSelf( x, y, boxWide, boxTall, col );
							}
						}

						if ( !pWeapon->CanBeSelected() )
						{
							int msgX = x + boxWide * 0.5;
							int msgY = y + boxTall * 0.5 -YRES(3);
							Color ammoColor = Color(240,40,40,255);
							wchar_t *pText = g_pVGuiLocalize->Find( "#TF_OUT_OF_AMMO" );
							DrawString( pText, msgX, msgY, ammoColor, true );
						}
					}					
				}
			}
		}
	break;
	case HUDTYPE_FASTSWITCH:
	if( of_hide_weapon_selection.GetBool() )
		break;
	case HUDTYPE_BUCKETS:
	default:
		{
			// calculate where to start drawing
			int nTotalHeight = ( nNumSlots - 1 ) * ( m_flSmallBoxTall + m_flBoxGap ) + m_flLargeBoxTall;
			int xStartPos = GetWide() - m_flBoxGap - m_flRightMargin;
			int xpos = xStartPos;
			int ypos = ( GetTall() - nTotalHeight ) / 2;

			int iActiveSlot = (pSelectedWeapon ? pSelectedWeapon->GetSlot() : -1);

			// draw the bucket set
			// iterate over all the weapon slots
			int actualPos = 0;
			for ( int i = 0; i < m_iMaxSlots; i++ )
			{
				Color col( 255, 255, 255, 255 );

				if ( i == iActiveSlot )
				{
					xpos = xStartPos - m_flLargeBoxWide;

					bool bFirstItem = true;
					for ( int slotpos = 0; slotpos < MAX_WEAPON_POSITIONS; slotpos++ )
					{
						CTFWeaponBase *pWeapon = GetWeaponInSlot(i, slotpos);
						if ( !pWeapon )
							continue;

						if ( !pWeapon->VisibleInWeaponSelection() )
							continue;

						// draw icon
						const CHudTexture *pTexture = pWeapon->GetSpriteInactive(); // red team
						if ( pPlayer )
						{
							if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE || pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
							{
								pTexture = pWeapon->GetSpriteActive();
							}
						}
						
						if ( !(pWeapon == pSelectedWeapon || ( m_iDemoModeSlot == i )) )
						{
							// draw selected weapon
							DrawBox( xpos + XRES(5), ypos + YRES(5), m_flLargeBoxWide - XRES(10), m_flLargeBoxTall - YRES(10), m_EmptyBoxColor, m_flSelectionAlphaOverride, bFirstItem ? i + 1 : -1 );
						}						

						if ( pTexture )
						{
							pTexture->DrawSelf( xpos, ypos, m_flLargeBoxWide, m_flLargeBoxTall, col );
						}
						
						if ( pWeapon == pSelectedWeapon || ( m_iDemoModeSlot == i ) )
						{
							// draw selected weapon
							if ( m_pActiveWeaponBG )
							{
								m_pActiveWeaponBG->SetPos( xpos - XRES(m_iWeaponBGXPos), ypos - YRES(m_iWeaponBGYPos));
							}
						}

						if ( !pWeapon->CanBeSelected() )
						{
							int msgX = xpos + ( m_flLargeBoxWide * 0.5 );
							int msgY = ypos + (int)m_flErrorYPos;
							Color ammoColor = Color( 255, 0, 0, 255 );
							wchar_t *pText = g_pVGuiLocalize->Find( "#TF_OUT_OF_AMMO" );
							DrawString( pText, msgX, msgY, ammoColor, true );
						}
						else if( pWeapon->ReserveAmmo() > -1 )
						{
							int msgX = xpos + ( m_flLargeBoxWide * 0.5 );
							int msgY = ypos + (int)m_flErrorYPos;
							Color ammoColor = Color( 255, 0, 0, 255 );
							wchar_t pText[32];
							g_pVGuiLocalize->ConstructString( pText, sizeof( pText ), 
							pWeapon->Clip1() > -1 ? VarArgs("%d/%d", pWeapon->Clip1(), pWeapon->ReserveAmmo() ) : VarArgs("%d", pWeapon->ReserveAmmo() ), 
							0 );
							DrawString( pText, msgX+1, msgY+1, m_NumberShadowColor, true );
							DrawString( pText, msgX, msgY, m_NumberColor, true );
						}

						xpos -= ( m_flLargeBoxWide + m_flBoxGap );
						
						int shortcut = bFirstItem ? i + 1 : -1;
						if(of_weaponswitch_flat.GetInt()) {
							shortcut = (actualPos+1);
							if( pWeapon == GetFirstPos(i) ) actualPos++;
						}

						if ( IsPC() && shortcut >= 0 )
						{
							Color numberColor = m_NumberColor;
							Color numberShadowColor = m_NumberShadowColor;
							//numberColor[3] *= m_flSelectionAlphaOverride / 255.0f;
							surface()->DrawSetTextFont(m_hNumberFont);
							wchar_t wch = '0' + shortcut;

							surface()->DrawSetTextPos( xStartPos - XRES(4) - m_flSelectionNumberXPos, ypos + YRES(6) + m_flSelectionNumberYPos );
							surface()->DrawSetTextColor(numberShadowColor);
							surface()->DrawUnicodeChar(wch);
							
							surface()->DrawSetTextColor(numberColor);
							surface()->DrawSetTextPos( xStartPos - XRES(5) - m_flSelectionNumberXPos, ypos + YRES(5) + m_flSelectionNumberYPos );
							surface()->DrawUnicodeChar(wch);
						}
						
						bFirstItem = false;
					}

					ypos += ( m_flLargeBoxTall + m_flBoxGap );
				}
				else
				{
					xpos = xStartPos - m_flSmallBoxWide;

					// check to see if there is a weapons in this bucket
					if ( GetFirstPos( i ) )
					{
						// draw has weapon in slot
						DrawBox( xpos + XRES(5), ypos + YRES(5), m_flSmallBoxWide - XRES(10), m_flSmallBoxTall - YRES(10), m_BoxColor, m_flAlphaOverride, i + 1 );

						CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetFirstPos(i);
						if ( !pWeapon )
							continue;

						const CHudTexture *pTexture = pWeapon->GetSpriteInactive(); // red team
						if ( pPlayer )
						{
							if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE || pPlayer->GetTeamNumber() == TF_TEAM_MERCENARY )
							{
								pTexture = pWeapon->GetSpriteActive();
							}
						}

						if ( pTexture )
						{
							pTexture->DrawSelf( xpos, ypos, m_flSmallBoxWide, m_flSmallBoxTall, col  );
						}

						Color numberColor = m_NumberColor;
						Color numberShadowColor = m_NumberShadowColor;
						//numberColor[3] *= m_flSelectionAlphaOverride / 255.0f;
						surface()->DrawSetTextFont(m_hNumberFont);
						wchar_t wch = '0' + (i+1);

						if(of_weaponswitch_flat.GetInt()) {
							wch = '0' + (actualPos+1);
							if( pWeapon == GetFirstPos(i) ) actualPos++;
						}


						surface()->DrawSetTextPos( xStartPos - XRES(4) - m_flSelectionNumberXPos, ypos + YRES(6) + m_flSelectionNumberYPos );
						surface()->DrawSetTextColor(numberShadowColor);
						surface()->DrawUnicodeChar(wch);
						
						surface()->DrawSetTextColor(numberColor);
						surface()->DrawSetTextPos( xStartPos - XRES(5) - m_flSelectionNumberXPos, ypos + YRES(5) + m_flSelectionNumberYPos );
						surface()->DrawUnicodeChar(wch);

						if( pWeapon->ReserveAmmo() > -1 )
						{
							int msgX = xStartPos - XRES(4) - m_flSmallBoxWide - m_flSelectionNumberXPos;
							int msgY = ypos + YRES(6) + m_flSelectionNumberYPos;
							wchar_t pText[32];
							g_pVGuiLocalize->ConstructString( pText, sizeof( pText ), 
							pWeapon->Clip1() > -1 ? VarArgs("%d/%d", pWeapon->Clip1(), pWeapon->ReserveAmmo() ) : VarArgs("%d", pWeapon->ReserveAmmo() ), 
							0 );
							DrawString( pText, msgX+1, msgY+1, numberShadowColor, true );
							DrawString( pText, msgX, msgY, numberColor, true );
						}
						
						ypos += ( m_flSmallBoxTall + m_flBoxGap );	
					}
							
				}
			}
		}
		break;
	}	
}

void CHudWeaponSelection::DrawString( wchar_t *text, int xpos, int ypos, Color col, bool bCenter )
{
	surface()->DrawSetTextColor( col );
	surface()->DrawSetTextFont( m_hTextFont );

	// count the position
	int slen = 0, charCount = 0, maxslen = 0;
	{
		for (wchar_t *pch = text; *pch != 0; pch++)
		{
			if (*pch == '\n') 
			{
				// newline character, drop to the next line
				if (slen > maxslen)
				{
					maxslen = slen;
				}
				slen = 0;
			}
			else if (*pch == '\r')
			{
				// do nothing
			}
			else
			{
				slen += surface()->GetCharacterWidth( m_hTextFont, *pch );
				charCount++;
			}
		}
	}
	if (slen > maxslen)
	{
		maxslen = slen;
	}

	int x = xpos;

	if ( bCenter )
	{
		x = xpos - slen * 0.5;
	}

	surface()->DrawSetTextPos( x, ypos );
	// adjust the charCount by the scan amount
	charCount *= m_flTextScan;
	for (wchar_t *pch = text; charCount > 0; pch++)
	{
		if (*pch == '\n')
		{
			// newline character, move to the next line
			surface()->DrawSetTextPos( x + ((m_flLargeBoxWide - slen) / 2), ypos + (surface()->GetFontTall(m_hTextFont) * 1.1f));
		}
		else if (*pch == '\r')
		{
			// do nothing
		}
		else
		{
			surface()->DrawUnicodeChar(*pch);
			charCount--;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: draws a selection box
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawBox(int x, int y, int wide, int tall, Color color, float normalizedAlpha, int number)
{
	BaseClass::DrawBox( x, y, wide, tall, color, normalizedAlpha / 255.0f );

	// draw the number
	if ( IsPC() && number >= 0)
	{
		Color numberColor = m_NumberColor;
		numberColor[3] *= normalizedAlpha / 255.0f;
		surface()->DrawSetTextColor(numberColor);
		surface()->DrawSetTextFont(m_hNumberFont);
		wchar_t wch = '0' + number;
		surface()->DrawSetTextPos(x + wide - m_flSelectionNumberXPos, y + m_flSelectionNumberYPos);
		surface()->DrawUnicodeChar(wch);
	}
}

//-----------------------------------------------------------------------------
// Purpose: draws a selection box
//-----------------------------------------------------------------------------
void CHudWeaponSelection::DrawPlusStyleBox(int x, int y, int wide, int tall, bool bSelected, float normalizedAlpha, int number, bool bOutOfAmmo )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pLocalPlayer )
		return;

	int iMaterial;

	if ( bSelected && !bOutOfAmmo )
	{
		iMaterial = ( pLocalPlayer->GetTeamNumber() == TF_TEAM_RED ) ? m_iBGImage_Red : m_iBGImage_Blue;
	}
	else
	{
		iMaterial = m_iBGImage_Inactive;
	}

	vgui::surface()->DrawSetTexture( iMaterial );
	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );

	if ( bSelected )
	{
		float flExpandWide = m_flPlusStyleBoxWide * m_flPlusStyleExpandPercent * 0.5;
		float flExpandTall = m_flPlusStyleBoxTall * m_flPlusStyleExpandPercent * 0.5;

		vgui::surface()->DrawTexturedRect( x-flExpandWide, y-flExpandTall, x+wide+flExpandWide, y+tall+flExpandTall );
	}
	else
	{
		vgui::surface()->DrawTexturedRect( x, y, x+wide, y+tall );
	}
}

//-----------------------------------------------------------------------------
// Purpose: hud scheme settings
//-----------------------------------------------------------------------------
void CHudWeaponSelection::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetPaintBackgroundEnabled(false);

	// set our size
	int screenWide, screenTall;
	int x, y;
	GetPos(x, y);
	GetHudSize(screenWide, screenTall);
	SetBounds(0, 0, screenWide, screenTall);

	// load control settings...
	LoadControlSettings( "resource/UI/HudWeaponSelection.res" );
}

//-----------------------------------------------------------------------------
// Purpose: Opens weapon selection control
//-----------------------------------------------------------------------------
void CHudWeaponSelection::OpenSelection( void )
{
	Assert(!IsInSelectionMode());

	CBaseHudWeaponSelection::OpenSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("OpenWeaponSelectionMenu");
	m_iSelectedBoxPosition = 0;
	m_iSelectedSlot = -1;
}

//-----------------------------------------------------------------------------
// Purpose: Closes weapon selection control immediately
//-----------------------------------------------------------------------------
void CHudWeaponSelection::HideSelection( void )
{
	m_flSelectionTime = 0;
	CBaseHudWeaponSelection::HideSelection();
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("CloseWeaponSelectionMenu");
}

//-----------------------------------------------------------------------------
// Purpose: Returns the next available weapon item in the weapon selection
//-----------------------------------------------------------------------------
CTFWeaponBase *CHudWeaponSelection::FindNextWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return NULL;

	CTFWeaponBase *pNextWeapon = NULL;

	// search all the weapons looking for the closest next
	int iLowestNextSlot = MAX_WEAPON_SLOTS;
	int iLowestNextPosition = MAX_WEAPON_POSITIONS;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)pPlayer->GetWeapon(i);
		if ( !pWeapon )
			continue;

		if ( pWeapon->VisibleInWeaponSelection() )
		{
			int weaponSlot = pWeapon->GetSlot(), weaponPosition = pWeapon->GetPosition();

			// see if this weapon is further ahead in the selection list
			if ( weaponSlot > iCurrentSlot || (weaponSlot == iCurrentSlot && weaponPosition > iCurrentPosition) )
			{
				// see if this weapon is closer than the current lowest
				if ( weaponSlot < iLowestNextSlot || (weaponSlot == iLowestNextSlot && weaponPosition < iLowestNextPosition) )
				{
					iLowestNextSlot = weaponSlot;
					iLowestNextPosition = weaponPosition;
					pNextWeapon = pWeapon;
				}
			}
		}
	}

	return pNextWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the prior available weapon item in the weapon selection
//-----------------------------------------------------------------------------
CTFWeaponBase *CHudWeaponSelection::FindPrevWeaponInWeaponSelection(int iCurrentSlot, int iCurrentPosition)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return NULL;

	CTFWeaponBase *pPrevWeapon = NULL;

	// search all the weapons looking for the closest next
	int iLowestPrevSlot = -1;
	int iLowestPrevPosition = -1;
	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)pPlayer->GetWeapon(i);
		if ( !pWeapon )
			continue;

		if ( pWeapon->VisibleInWeaponSelection() )
		{
			int weaponSlot = pWeapon->GetSlot(), weaponPosition = pWeapon->GetPosition();

			// see if this weapon is further ahead in the selection list
			if ( weaponSlot < iCurrentSlot || (weaponSlot == iCurrentSlot && weaponPosition < iCurrentPosition) )
			{
				// see if this weapon is closer than the current lowest
				if ( weaponSlot > iLowestPrevSlot || (weaponSlot == iLowestPrevSlot && weaponPosition > iLowestPrevPosition) )
				{
					iLowestPrevSlot = weaponSlot;
					iLowestPrevPosition = weaponPosition;
					pPrevWeapon = pWeapon;
				}
			}
		}
	}

	return pPrevWeapon;
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the next item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToNextWeapon( void )
{
	// Get the local player.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	if ( pPlayer->IsAlive() == false )
		return;

	CTFWeaponBase *pNextWeapon = NULL;
	if ( IsInSelectionMode() && hud_fastswitch.GetInt() != HUDTYPE_FASTSWITCH )
	{
		// find the next selection spot
		CTFWeaponBase *pWeapon = GetSelectedWeapon();
		if ( !pWeapon )
			return;

		pNextWeapon = FindNextWeaponInWeaponSelection( pWeapon->GetSlot(), pWeapon->GetPosition() );
	}
	else
	{
		// open selection at the current place
		pNextWeapon = pPlayer->GetActiveTFWeapon();
		if ( pNextWeapon )
		{
			pNextWeapon = FindNextWeaponInWeaponSelection( pNextWeapon->GetSlot(), pNextWeapon->GetPosition() );
		}
	}

	if ( !pNextWeapon )
	{
		// wrap around back to start
		pNextWeapon = FindNextWeaponInWeaponSelection(-1, -1);
	}

	if ( pNextWeapon )
	{
		SetSelectedWeapon( pNextWeapon );

		if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		// cancel demo mode
		m_iDemoModeSlot = -1;
		m_flDemoStartTime = -1;

		// Play the "cycle to next weapon" sound
		if( m_bPlaySelectionSounds )
			pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves the selection to the previous item in the menu
//-----------------------------------------------------------------------------
void CHudWeaponSelection::CycleToPrevWeapon( void )
{
	// Get the local player.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	if ( pPlayer->IsAlive() == false )
		return;

	CTFWeaponBase *pNextWeapon = NULL;
	if ( IsInSelectionMode() && hud_fastswitch.GetInt() != HUDTYPE_FASTSWITCH )
	{
		// find the next selection spot
		CTFWeaponBase *pWeapon = GetSelectedWeapon();
		if ( !pWeapon )
			return;

		pNextWeapon = FindPrevWeaponInWeaponSelection( pWeapon->GetSlot(), pWeapon->GetPosition() );
	}
	else
	{
		// open selection at the current place
		pNextWeapon = pPlayer->GetActiveTFWeapon();
		if ( pNextWeapon )
		{
			pNextWeapon = FindPrevWeaponInWeaponSelection( pNextWeapon->GetSlot(), pNextWeapon->GetPosition() );
		}
	}

	if ( !pNextWeapon )
	{
		// wrap around back to end of weapon list
		pNextWeapon = FindPrevWeaponInWeaponSelection(MAX_WEAPON_SLOTS, MAX_WEAPON_POSITIONS);
	}

	if ( pNextWeapon )
	{
		SetSelectedWeapon( pNextWeapon );

		if ( !IsInSelectionMode() )
		{
			OpenSelection();
		}

		// cancel demo mode
		m_iDemoModeSlot = -1;
		m_flDemoStartTime = -1;

		// Play the "cycle to next weapon" sound
		if( m_bPlaySelectionSounds )
			pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns the weapon in the specified slot
//-----------------------------------------------------------------------------
CTFWeaponBase *CHudWeaponSelection::GetWeaponInSlot( int iSlot, int iSlotPos )
{
	C_TFPlayer *player = C_TFPlayer::GetLocalTFPlayer();
	if ( !player )
		return NULL;

	for ( int i = 0; i < MAX_WEAPONS; i++ )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)player->GetWeapon(i);
		
		if ( pWeapon == NULL )
			continue;

		if ( pWeapon->GetSlot() == iSlot && pWeapon->GetPosition() == iSlotPos )
			return pWeapon;
	}

	return NULL;
}

CTFWeaponBase *CHudWeaponSelection::GetSelectedWeapon( void )
{ 
	if ( hud_fastswitch.GetInt() == 0 && m_iDemoModeSlot >= 0 )
	{
		CTFWeaponBase *pWeapon = (CTFWeaponBase *)GetFirstPos( m_iDemoModeSlot );
		return pWeapon;
	}
	else
	{
		C_BaseCombatWeapon *pWeapon = m_hSelectedWeapon.Get();
		return (CTFWeaponBase *)pWeapon;
	}
}

void CHudWeaponSelection::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "localplayer_changeclass") == 0 )
	{
		int nUpdateType = event->GetInt( "updateType" );
		bool bIsCreationUpdate = ( nUpdateType == DATA_UPDATE_CREATED );
		// Don't demo selection in minmode
		ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
		if ( !cl_hud_minmode.IsValid() || cl_hud_minmode.GetBool() == false )
		{
			if ( !bIsCreationUpdate )
			{
				m_flDemoStartTime = gpGlobals->curtime + tf_weapon_select_demo_start_delay.GetFloat();
			}
		}
	}
	else
	{
		CHudElement::FireGameEvent( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Opens the next weapon in the slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::FastWeaponSwitch( int iWeaponSlot )
{
	// get the slot the player's weapon is in
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	// see where we should start selection
	int iPosition = -1;
	CTFWeaponBase *pActiveWeapon = pPlayer->GetActiveTFWeapon();
	if ( pActiveWeapon && pActiveWeapon->GetSlot() == iWeaponSlot )
	{
		// start after this weapon
		iPosition = pActiveWeapon->GetPosition();
	}

	CTFWeaponBase *pNextWeapon = NULL;

	// search for the weapon after the current one
	pNextWeapon = FindNextWeaponInWeaponSelection(iWeaponSlot, iPosition);
	// make sure it's in the same bucket
	if ( !pNextWeapon || pNextWeapon->GetSlot() != iWeaponSlot )
	{
		// just look for any weapon in this slot
		pNextWeapon = FindNextWeaponInWeaponSelection(iWeaponSlot, -1);
	}

	// see if we found a weapon that's different from the current and in the selected slot
	if ( pNextWeapon && pNextWeapon != pActiveWeapon && pNextWeapon->GetSlot() == iWeaponSlot )
	{
		// select the new weapon
		::input->MakeWeaponSelection( pNextWeapon );
	}
	else if ( pNextWeapon != pActiveWeapon )
	{
		// error sound
		pPlayer->EmitSound( "Player.DenyWeaponSelection" );
	}

	if( of_hide_weapon_selection.GetBool() )
	// kill any fastswitch display
	m_flSelectionTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Opens the next weapon in the slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::PlusTypeFastWeaponSwitch( int iWeaponSlot )
{
	// get the slot the player's weapon is in
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	int newSlot = m_iSelectedSlot;

	// Changing slot number does not necessarily mean we need to change the slot - the player could be
	// scrolling through the same slot but in the opposite direction. Slot pairs are 0,2 and 1,3 - so
	// compare the 0 bits to see if we're within a pair. Otherwise, reset the box to the zero position.
	if ( -1 == m_iSelectedSlot || ( ( m_iSelectedSlot ^ iWeaponSlot ) & 1 ) )
	{
		// Changing vertical/horizontal direction. Reset the selected box position to zero.
		m_iSelectedBoxPosition = 0;
		m_iSelectedSlot = iWeaponSlot;
	}
	else
	{
		// Still in the same horizontal/vertical direction. Determine which way we're moving in the slot.
		int increment = 1;
		if ( m_iSelectedSlot != iWeaponSlot )
		{
			// Decrementing within the slot. If we're at the zero position in this slot, 
			// jump to the zero position of the opposite slot. This also counts as our increment.
			increment = -1;
			if ( 0 == m_iSelectedBoxPosition )
			{
				newSlot = ( m_iSelectedSlot + 2 ) % 4;
				increment = 0;
			}
		}

		// Find out of the box position is at the end of the slot
		int lastSlotPos = -1;
		for ( int slotPos = 0; slotPos < MAX_WEAPON_POSITIONS; ++slotPos )
		{
			CTFWeaponBase *pWeapon = GetWeaponInSlot( newSlot, slotPos );
			if ( pWeapon )
			{
				lastSlotPos = slotPos;
			}
		}

		// Increment/Decrement the selected box position
		if ( m_iSelectedBoxPosition + increment <= lastSlotPos )
		{
			m_iSelectedBoxPosition += increment;
			m_iSelectedSlot = newSlot;
		}
		else
		{
			// error sound
			pPlayer->EmitSound( "Player.DenyWeaponSelection" );
			return;
		}
	}

	// Select the weapon in this position
	bool bWeaponSelected = false;
	CTFWeaponBase *pActiveWeapon = pPlayer->GetActiveTFWeapon();
	CTFWeaponBase *pWeapon = GetWeaponInSlot( m_iSelectedSlot, m_iSelectedBoxPosition );
	if ( pWeapon && CanBeSelectedInHUD( pWeapon ) )
	{
		if ( pWeapon != pActiveWeapon )
		{
			// Select the new weapon
			::input->MakeWeaponSelection( pWeapon );
			SetSelectedWeapon( pWeapon );
			bWeaponSelected = true;
		}
	}

	if ( !bWeaponSelected )
	{
		// Still need to set this to make hud display appear
		SetSelectedWeapon( pPlayer->GetActiveTFWeapon() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Moves selection to the specified slot
//-----------------------------------------------------------------------------
void CHudWeaponSelection::SelectWeaponSlot( int iSlot )
{
	// iSlot is one higher than it should be, since it's the number key, not the 0-based index into the weapons
	--iSlot;

	// Get the local player.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return;

	// Don't try and read past our possible number of slots
	if ( iSlot > MAX_WEAPON_SLOTS )
		return;
	
	// Make sure the player's allowed to switch weapons
	if ( pPlayer->IsAllowedToSwitchWeapons() == false )
		return;

	if( of_weaponswitch_flat.GetInt() ) {
		CTFWeaponBase *pWeapon = { 0 };
		int actualPos = 0;
		for(int i = 0; i < MAX_WEAPON_POSITIONS; i++) {
			if( GetFirstPos( i ) ) {
				pWeapon = (CTFWeaponBase *)GetFirstPos(i);
				if( !pWeapon ) continue;

				actualPos++;

				if( iSlot == actualPos-1 ) break;
			}
		}

		iSlot = pWeapon->GetSlot();
	}

	switch( hud_fastswitch.GetInt() )
	{
	case HUDTYPE_PLUS:
		{
			if ( !IsInSelectionMode() )
			{
				// open the weapon selection
				OpenSelection();
			}
				
			PlusTypeFastWeaponSwitch( iSlot );
		}
		break;
	case HUDTYPE_FASTSWITCH:
		{				
			FastWeaponSwitch( iSlot );
			if( of_hide_weapon_selection.GetBool() )
				return;
		}
	case HUDTYPE_BUCKETS:
		{
			int slotPos = 0;
			CTFWeaponBase *pActiveWeapon = GetSelectedWeapon();

			// start later in the list
			if ( IsInSelectionMode() && pActiveWeapon && pActiveWeapon->GetSlot() == iSlot )
			{
				slotPos = pActiveWeapon->GetPosition() + 1;
			}


			// find the weapon in this slot
			pActiveWeapon = (CTFWeaponBase *)GetNextActivePos(iSlot, slotPos);

			if ( !pActiveWeapon )
			{
				pActiveWeapon = (CTFWeaponBase *)GetNextActivePos(iSlot, 0);
			}
			
			if ( pActiveWeapon != NULL )
			{
				if ( !IsInSelectionMode() )
				{
					// open the weapon selection
					OpenSelection();
				}

				// Mark the change
				SetSelectedWeapon( pActiveWeapon );
				m_iDemoModeSlot = -1;
				m_flDemoStartTime = -1;
			}
		}
		break;

	default:
		break;
	}

	if( m_bPlaySelectionSounds )
		pPlayer->EmitSound( "Player.WeaponSelectionMoveSlot" );
}
