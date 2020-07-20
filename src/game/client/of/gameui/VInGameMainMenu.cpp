//========= Copyright � 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "VInGameMainMenu.h"
#include "UI_Shared.h"
#include "VGenericConfirmation.h"
#include "VFooterPanel.h"
#include "vhybridbutton.h"
#include "fmtstr.h"
#include "materialsystem/materialsystem_config.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
using namespace BaseModUI;

extern class IMatchSystem *matchsystem;
extern IVEngineClient *engine;

void Demo_DisableButton( Button *pButton );
//void OpenGammaDialog( VPANEL parent );

//=============================================================================
InGameMainMenu::InGameMainMenu( Panel *parent, const char *panelName ):
BaseClass( parent, panelName, false, true )
{
	SetDeleteSelfOnClose(true);

	SetProportional( true );
	SetTitle( "", false );

	SetFooterEnabled( true );

	SetFooterState();
}

//=============================================================================
static void LeaveGameOkCallback()
{
	COM_TimestampedLog( "Exit Game" );

	InGameMainMenu* self = 
		static_cast< InGameMainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_INGAMEMAINMENU ) );

	if ( self )
	{
		self->Close();
	}

	engine->ExecuteClientCmd( "gameui_hide" );

	// On PC people can be playing via console bypassing matchmaking
	// and required session settings, so to leave game duplicate
	// session closure with an extra "disconnect" command.
	engine->ExecuteClientCmd( "disconnect" );

	//Turn off commentary.
	engine->ExecuteClientCmd( "commentary 0" );

	engine->ExecuteClientCmd( "gameui_activate" );

	CBaseModPanel::GetSingleton().CloseAllWindows();
	CBaseModPanel::GetSingleton().OpenFrontScreen();
}

static void LoadGameCallback()
{
	InGameMainMenu* self = 
		static_cast< InGameMainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_INGAMEMAINMENU ) );

	if ( self )
	{
		self->Close();
	}

	engine->ExecuteClientCmd( "reload" );
	engine->ExecuteClientCmd( "gameui_hide" );

	CBaseModPanel::GetSingleton().CloseAllWindows();
	CBaseModPanel::GetSingleton().OnGameUIHidden();
}

static void SavedGameCallback()
{
	InGameMainMenu* self = 
		static_cast< InGameMainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_INGAMEMAINMENU ) );

	if ( self )
	{
		self->Close();
	}

	engine->ClientCmd_Unrestricted( "autosave\n" );
	engine->ExecuteClientCmd( "gameui_hide" );

	CBaseModPanel::GetSingleton().CloseAllWindows();
	CBaseModPanel::GetSingleton().OnGameUIHidden();
}

//void ShowPlayerList();

//=============================================================================
void InGameMainMenu::OnCommand( const char *command )
{
	int iUserSlot = CBaseModPanel::GetSingleton().GetLastActiveUserId();

	if ( UI_IsDebug() )
	{
		Msg("[GAMEUI] Handling ingame menu command %s from user%d ctrlr%d\n",
			command, iUserSlot, 0 );
	}

	const char *pchCommand = command;

	if ( !Q_strcmp( command, "ReturnToGame" ) )
	{
		engine->ClientCmd_Unrestricted("gameui_hide");
	}
	else if (!Q_strcmp(command, "showloadoutdialog"))
	{
		CBaseModPanel::GetSingleton().OpenWindow(WT_DM_LOADOUT, this, true);
	}
	else if (!Q_strcmp(command, "Options"))
	{
		engine->ClientCmd_Unrestricted("gamemenucommand OpenOptionsDialog");
	}
	else if (!Q_strcmp(command, "CreateServer"))
	{
		engine->ClientCmd_Unrestricted("gamemenucommand OpenCreateMultiplayerGameDialog");
	}
	else if (!Q_strcmp(command, "OpenPlayerListDialog"))
	{
		engine->ClientCmd_Unrestricted("gamemenucommand OpenPlayerListDialog");
	}
#if 0
	else if (!Q_strcmp(command, "Audio"))
	{
		// audio options dialog, PC only
		m_ActiveControl->NavigateFrom( );
		CBaseModPanel::GetSingleton().OpenWindow(WT_AUDIO, this, true );
	}
	else if (!Q_strcmp(command, "Video"))
	{
		// video options dialog, PC only
		m_ActiveControl->NavigateFrom( );
		CBaseModPanel::GetSingleton().OpenWindow(WT_VIDEO, this, true );
	}
	else if (!Q_strcmp(command, "Brightness"))
	{
		// brightness options dialog, PC only
		//OpenGammaDialog( GetVParent() );
		Msg("Sorry, use the old Options Dialog.\n");
	}
	else if (!Q_strcmp(command, "KeyboardMouse"))
	{
		// standalone keyboard/mouse dialog, PC only
		m_ActiveControl->NavigateFrom( );
		CBaseModPanel::GetSingleton().OpenWindow(WT_KEYBOARDMOUSE, this, true );
	}
	else if (!Q_strcmp(command, "Mouse"))
	{
		CBaseModPanel::GetSingleton().OpenOptionsMouseDialog( this );
	}
	else if ( !Q_strcmp( command, "OpenServerBrowser" ) )
	{
		if ( CheckAndDisplayErrorIfNotLoggedIn() )
			return;

		// on PC, bring up the server browser and switch it to the LAN tab (tab #5)
		engine->ClientCmd_Unrestricted( "openserverbrowser" );
	}
	else if (!Q_strcmp(command, "SoloPlay"))
	{
		CBaseModPanel::GetSingleton().OnOpenNewGameDialog();
	}
	else if( Q_stricmp( "#GameUI_Controller_Edit_Keys_Buttons", command ) == 0 )
	{
		FlyoutMenu::CloseActiveMenu();
		CBaseModPanel::GetSingleton().OpenKeyBindingsDialog( this );
	}
	else if (!Q_strcmp(command, "MultiplayerSettings"))
	{
		// standalone multiplayer settings dialog, PC only
		m_ActiveControl->NavigateFrom( );
		CBaseModPanel::GetSingleton().OpenWindow(WT_MULTIPLAYER, this, true );
	}
#endif
	else if( !Q_strcmp( command, "ExitToMainMenu" ) )
	{
		GenericConfirmation* confirmation = 
			static_cast< GenericConfirmation* >( CBaseModPanel::GetSingleton().OpenWindow( WT_GENERICCONFIRMATION, this, true ) );

		GenericConfirmation::Data_t data;

		data.pWindowTitle = "#GameUI_LeaveConfirm";
		data.pMessageText = "#GameUI_LeaveConfirm_Msg";
		data.bOkButtonEnabled = true;
		data.pfnOkCallback = &LeaveGameOkCallback;
		data.bCancelButtonEnabled = true;

		confirmation->SetUsageData(data);
	}
	else if (!Q_strcmp(command, "QuitGame"))
	{
		if ( IsPC() )
		{
			GenericConfirmation* confirmation = 
				static_cast< GenericConfirmation* >( CBaseModPanel::GetSingleton().OpenWindow( WT_GENERICCONFIRMATION, this, false ) );

			GenericConfirmation::Data_t data;

			data.pWindowTitle = "#GameUI_Quit_Confirm";
			data.pMessageText = GetRandomQuitString();

			data.bOkButtonEnabled = true;
			data.pfnOkCallback = &AcceptQuitGameCallback;
			data.bCancelButtonEnabled = true;

			confirmation->SetUsageData(data);
			confirmation->PushModalInputFocus();

			NavigateFrom();
		}

		if ( IsX360() )
		{
			engine->ExecuteClientCmd( "demo_exit" );
		}
	}	
	else if ( !Q_stricmp( command, "QuitGame_NoConfirm" ) )
	{
		if ( IsPC() )
		{
			engine->ClientCmd_Unrestricted( "quit" );
		}
	}	
	else
	{
		engine->ClientCmd_Unrestricted( command );
		BaseClass::OnCommand( command );
	}		
		// does this command match a flyout menu?
		BaseModUI::FlyoutMenu *flyout = dynamic_cast< FlyoutMenu* >( FindChildByName( pchCommand ) );
		if ( flyout )
		{
			// If so, enumerate the buttons on the menu and find the button that issues this command.
			// (No other way to determine which button got pressed; no notion of "current" button on PC.)
			for ( int iChild = 0; iChild < GetChildCount(); iChild++ )
			{
				BaseModHybridButton *hybrid = dynamic_cast<BaseModHybridButton *>( GetChild( iChild ) );
				if ( hybrid && hybrid->GetCommand() && !Q_strcmp( hybrid->GetCommand()->GetString( "command"), command ) )
				{
#ifdef _X360
					hybrid->NavigateFrom( );
#endif //_X360
					// open the menu next to the button that got clicked
					flyout->OpenMenu( hybrid );
					break;
				}
			}
		}
}

//=============================================================================
void InGameMainMenu::OnKeyCodePressed( KeyCode code )
{
	int userId = GetJoystickForCode( code );
	BaseModUI::CBaseModPanel::GetSingleton().SetLastActiveUserId( userId );

	switch( GetBaseButtonCode( code ) )
	{
	case KEY_XBUTTON_START:
	case KEY_XBUTTON_B:
		CBaseModPanel::GetSingleton().PlayUISound( UISOUND_BACK );
		OnCommand( "ReturnToGame" );
		break;
	default:
		BaseClass::OnKeyCodePressed( code );
		break;
	}
}

//=============================================================================
void InGameMainMenu::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( demo_ui_enable.GetString()[0] )
	{
		LoadControlSettings( CFmtStr( "resource/ui/basemodui/ingamemainmenu_%s.res", demo_ui_enable.GetString() ) );
	}
	else
	{
		LoadControlSettings( "resource/ui/basemodui/ingamemainmenu.res" );
	}

	SetPaintBackgroundEnabled( true );

	SetFooterState();
}

//=============================================================================
void InGameMainMenu::OnOpen()
{
	BaseClass::OnOpen();

	SetFooterState();
}

void InGameMainMenu::OnClose()
{
	Unpause();

	// During shutdown this calls delete this, so Unpause should occur before this call
	BaseClass::OnClose();
}


void InGameMainMenu::OnThink()
{
	if ( IsPC() )
	{
		FlyoutMenu *pFlyout = dynamic_cast< FlyoutMenu* >( FindChildByName( "FlmOptionsFlyout" ) );
		if ( pFlyout )
		{
			const MaterialSystem_Config_t &config = materials->GetCurrentConfigForVideoCard();
			pFlyout->SetControlEnabled( "BtnBrightness", !config.Windowed() );
		}
	}

	BaseClass::OnThink();

	if ( IsVisible() )
	{
		// Yield to generic wait screen or message box if one of those is present
		WINDOW_TYPE arrYield[] = { WT_GENERICWAITSCREEN, WT_GENERICCONFIRMATION };
		for ( int j = 0; j < ARRAYSIZE( arrYield ); ++ j )
		{
			CBaseModFrame *pYield = CBaseModPanel::GetSingleton().GetWindow( arrYield[j] );
			if ( pYield && pYield->IsVisible() && !pYield->HasFocus() )
			{
				pYield->Activate();
				pYield->RequestFocus();
			}
		}
	}
}

void InGameMainMenu::AcceptQuitGameCallback()
{
	if ( InGameMainMenu *pInGameMainMenu = static_cast< InGameMainMenu* >( CBaseModPanel::GetSingleton().GetWindow( WT_INGAMEMAINMENU ) ) )
	{
		pInGameMainMenu->OnCommand( "QuitGame_NoConfirm" );
	}
}
//=============================================================================
void InGameMainMenu::PerformLayout( void )
{
	BaseClass::PerformLayout();

	//KeyValues *pGameSettings = NULL;

	char const *szNetwork = "offline";// pGameSettings->GetString("system/network", "offline");

	bool bPlayOffline = !Q_stricmp( "offline", szNetwork );

	bool bInCommentary = false;// engine->IsInCommentaryMode();

	bool bCanInvite = !Q_stricmp( "LIVE", szNetwork );
	SetControlEnabled( "BtnInviteFriends", bCanInvite );


	bool bCanVote = true;

	if ( bInCommentary )
	{
		bCanVote = false;
	}

	if ( gpGlobals->maxClients <= 1 )
	{
		bCanVote = false;
	}

	// Do not allow voting in credits map
	/*
	if ( !Q_stricmp( pGameSettings->GetString( "game/campaign" ), "credits" ) )
	{
		bCanVote = false;
	}
	*/

	/*
	vgui::Button *pVoteButton = dynamic_cast< vgui::Button* >( FindChildByName( "BtnCallAVote" ) );
	if ( pVoteButton )
	{
		if ( bCanVote )
		{
			pVoteButton->SetText( "#GameUI_InGameMainMenu_CallAVote" );
		}
		else
		{
			pVoteButton->SetText( "#asw_button_restart_mis" );
		}
		SetControlEnabled( "BtnCallAvote", true );
	}
	//SetControlEnabled( "BtnCallAVote", bCanVote );
	*/

	BaseModUI::FlyoutMenu *flyout = dynamic_cast< FlyoutMenu* >( FindChildByName( "FlmOptionsFlyout" ) );
	if ( flyout )
	{
		flyout->SetListener( this );
	}

	flyout = dynamic_cast< FlyoutMenu* >( FindChildByName( "FlmOptionsGuestFlyout" ) );
	if ( flyout )
	{
		flyout->SetListener( this );
	}

	flyout = dynamic_cast< FlyoutMenu* >( FindChildByName( "FlmVoteFlyout" ) );
	if ( flyout )
	{
		flyout->SetListener( this );
		
		bool bSinglePlayer = true;

#ifdef _X360
		bSinglePlayer = ( XBX_GetNumGameUsers() == 1 );
#endif

		Button *pButton = flyout->FindChildButtonByCommand( "ReturnToLobby" );
		if ( pButton )
		{
			static CGameUIConVarRef r_sv_hosting_lobby( "sv_hosting_lobby", true );
			bool bEnabled = r_sv_hosting_lobby.IsValid() && r_sv_hosting_lobby.GetBool() &&
				// Don't allow return to lobby if playing local singleplayer (it has no lobby)
				!( bPlayOffline && bSinglePlayer );
			pButton->SetEnabled( bEnabled );
		}

		pButton = flyout->FindChildButtonByCommand( "BootPlayer" );
		if ( pButton )
		{
			// Don't allow kick player in local games (nobody to kick)
			pButton->SetEnabled( !bPlayOffline );
		}
	}

	flyout = dynamic_cast< FlyoutMenu* >( FindChildByName( "FlmVoteFlyoutVersus" ) );
	if ( flyout )
	{
		flyout->SetListener( this );
	}

	BaseModHybridButton *button = dynamic_cast< BaseModHybridButton* >( FindChildByName( "BtnReturnToGame" ) );
	if ( button )
	{
		if( m_ActiveControl )
			m_ActiveControl->NavigateFrom();

		button->NavigateTo();
	}
}

void InGameMainMenu::Unpause( void )
{
}

//=============================================================================
void InGameMainMenu::OnNotifyChildFocus( vgui::Panel* child )
{
}

void InGameMainMenu::OnFlyoutMenuClose( vgui::Panel* flyTo )
{
	SetFooterState();
}

void InGameMainMenu::OnFlyoutMenuCancelled()
{
}

//-----------------------------------------------------------------------------
// Purpose: Called when the GameUI is hidden
//-----------------------------------------------------------------------------
void InGameMainMenu::OnGameUIHidden()
{
	Unpause();
	Close();
}


//=============================================================================
void InGameMainMenu::PaintBackground()
{
	vgui::Panel *pPanel = FindChildByName( "PnlBackground" );
	if ( !pPanel )
		return;

	int x, y, wide, tall;
	pPanel->GetBounds( x, y, wide, tall );
	// DrawSmearBackground( x, y, wide, tall );
}

//=============================================================================
void InGameMainMenu::SetFooterState()
{
	CBaseModFooterPanel *footer = BaseModUI::CBaseModPanel::GetSingleton().GetFooterPanel();
	if ( footer )
	{
		footer->SetButtons( FB_ABUTTON | FB_BBUTTON, FF_AB_ONLY, false );
		footer->SetButtonText( FB_ABUTTON, "#GameUI_Select" );
		footer->SetButtonText( FB_BBUTTON, "#GameUI_Done" );
	}
}
