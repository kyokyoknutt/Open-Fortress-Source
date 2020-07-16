
//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tf_hud_winpanel.h"
#include "vgui_controls/AnimationController.h"
#include "iclientmode.h"
#include "engine/IEngineSound.h"
#include "c_tf_playerresource.h"
#include <vgui/ILocalize.h>
#include "vgui_avatarimage.h"
#include "fmtstr.h"
#include "tf_gamerules.h"
#include "of_shared_schemas.h"
#include "c_of_music_player.h"
#include "basemodelpanel.h"
#include "filesystem.h"
#include <vgui_controls/Label.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT_DEPTH(CTFWinPanel, 70);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFWinPanel::CTFWinPanel(const char *pElementName) : EditablePanel(NULL, "WinPanel"), CHudElement(pElementName)
{
	SetParent(g_pClientMode->GetViewport());
	SetVisible(false);
	SetScheme("ClientScheme");

	// listen for events
	ListenForGameEvent("teamplay_win_panel");
	ListenForGameEvent("teamplay_round_start");
	ListenForGameEvent("teamplay_game_over");
	ListenForGameEvent("tf_game_over");

	m_pTeamScorePanel = new EditablePanel(this, "TeamScoresPanel");
	m_flTimeUpdateTeamScore = 0;
	m_iBlueTeamScore = 0;
	m_iRedTeamScore = 0;

	RegisterForRenderGroup("mid");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWinPanel::Reset()
{
	SetVisible(false);
}

void CTFWinPanel::SetVisible(bool state)
{
	if (state == IsVisible())
		return;

	if (state)
		HideLowerPriorityHudElementsInGroup("mid");
	else
		UnhideLowerPriorityHudElementsInGroup("mid");

	BaseClass::SetVisible(state);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWinPanel::FireGameEvent(IGameEvent * event)
{
	//Should it draw or not
	int iWinningTeam = event->GetInt("winning_team");

	if (Q_strcmp("teamplay_win_panel", event->GetName()) || iWinningTeam == TF_TEAM_MERCENARY)
	{
		SetVisible(false);
		return;
	}

	if (!g_PR)
		return;

	//It should
	int iWinReason = event->GetInt("winreason");
	int iFlagCapLimit = event->GetInt("flagcaplimit");
	bool bRoundComplete = (bool)event->GetInt("round_complete");
	int iRoundsRemaining = event->GetInt("rounds_remaining");

	LoadControlSettings("resource/UI/WinPanel.res");
	InvalidateLayout(false, true);

	SetDialogVariable("WinningTeamLabel", "");
	SetDialogVariable("AdvancingTeamLabel", "");
	SetDialogVariable("WinReasonLabel", "");
	SetDialogVariable("DetailsLabel", "");

	ImagePanel *pImagePanelBG = dynamic_cast<ImagePanel *>(FindChildByName("WinPanelBG"));
	Assert(pImagePanelBG);
	if (!pImagePanelBG)
		return;

	// set the appropriate background image and label text
	const char *pTeamLabel = NULL;
	const char *pTopPlayersLabel = NULL;
	const wchar_t *pLocalizedTeamName = NULL;
	// this is an area defense, but not a round win, if this was a successful defend until time limit but not a complete round
	bool bIsAreaDefense = ((WINREASON_DEFEND_UNTIL_TIME_LIMIT == iWinReason) && !bRoundComplete);
	switch (iWinningTeam)
	{
	case TF_TEAM_BLUE:
		pImagePanelBG->SetImage("../hud/winpanel_blue_bg_main.vmt");
		pTeamLabel = (bRoundComplete ? "#Winpanel_BlueWins" : (bIsAreaDefense ? "#Winpanel_BlueDefends" : "#Winpanel_BlueAdvances"));
		pTopPlayersLabel = "#Winpanel_BlueMVPs";
		pLocalizedTeamName = g_pVGuiLocalize->Find("TF_BlueTeam_Name");
		break;
	case TF_TEAM_RED:
		pImagePanelBG->SetImage("../hud/winpanel_red_bg_main.vmt");
		pTeamLabel = (bRoundComplete ? "#Winpanel_RedWins" : (bIsAreaDefense ? "#Winpanel_RedDefends" : "#Winpanel_RedAdvances"));
		pTopPlayersLabel = "#Winpanel_RedMVPs";
		pLocalizedTeamName = g_pVGuiLocalize->Find("TF_RedTeam_Name");
		break;
	case TEAM_UNASSIGNED:	// stalemate
		pImagePanelBG->SetImage("../hud/winpanel_black_bg_main.vmt");
		pTeamLabel = "#Winpanel_Stalemate";
		pTopPlayersLabel = "#Winpanel_TopPlayers";
		break;
	default:
		Assert(false);
		break;
	}

	SetDialogVariable(bRoundComplete ? "WinningTeamLabel" : "AdvancingTeamLabel", g_pVGuiLocalize->Find(pTeamLabel));
	SetDialogVariable("TopPlayersLabel", g_pVGuiLocalize->Find(pTopPlayersLabel));

	wchar_t wzWinReason[256] = L"";
	switch (iWinReason)
	{
	case WINREASON_ALL_POINTS_CAPTURED:
		g_pVGuiLocalize->ConstructString(wzWinReason, sizeof(wzWinReason), g_pVGuiLocalize->Find("#Winreason_AllPointsCaptured"), 1, pLocalizedTeamName);
		break;
	case WINREASON_FLAG_CAPTURE_LIMIT:
		wchar_t wzFlagCaptureLimit[16];
		_snwprintf(wzFlagCaptureLimit, ARRAYSIZE(wzFlagCaptureLimit), L"%i", iFlagCapLimit);
		g_pVGuiLocalize->ConstructString(wzWinReason, sizeof(wzWinReason), g_pVGuiLocalize->Find("#Winreason_FlagCaptureLimit"), 2,
			pLocalizedTeamName, wzFlagCaptureLimit);
		break;
	case WINREASON_OPPONENTS_DEAD:
		g_pVGuiLocalize->ConstructString(wzWinReason, sizeof(wzWinReason), g_pVGuiLocalize->Find("#Winreason_OpponentsDead"), 1, pLocalizedTeamName);
		break;
	case WINREASON_DEFEND_UNTIL_TIME_LIMIT:
		g_pVGuiLocalize->ConstructString(wzWinReason, sizeof(wzWinReason), g_pVGuiLocalize->Find("#Winreason_DefendedUntilTimeLimit"), 1, pLocalizedTeamName);
		break;
	case WINREASON_STALEMATE:
		g_pVGuiLocalize->ConstructString(wzWinReason, sizeof(wzWinReason), g_pVGuiLocalize->Find("#Winreason_Stalemate"), 0);
		break;
	case WINREASON_POINTLIMIT:
		g_pVGuiLocalize->ConstructString(wzWinReason, sizeof(wzWinReason), g_pVGuiLocalize->Find("#Winreason_PointLimit"), 0);
		break;
	default:
		Assert(false);
		break;
	}
	SetDialogVariable("WinReasonLabel", wzWinReason);

	if (!bRoundComplete && (WINREASON_STALEMATE != iWinReason))
	{
		// if this was a mini-round, show # of capture points remaining
		wchar_t wzNumCapturesRemaining[16];
		wchar_t wzCapturesRemainingMsg[256] = L"";
		_snwprintf(wzNumCapturesRemaining, ARRAYSIZE(wzNumCapturesRemaining), L"%i", iRoundsRemaining);
		g_pVGuiLocalize->ConstructString(wzCapturesRemainingMsg, sizeof(wzCapturesRemainingMsg),
			g_pVGuiLocalize->Find(1 == iRoundsRemaining ? "#Winpanel_CapturePointRemaining" : "Winpanel_CapturePointsRemaining"),
			1, wzNumCapturesRemaining);
		SetDialogVariable("DetailsLabel", wzCapturesRemainingMsg);
	}
	else if ((WINREASON_ALL_POINTS_CAPTURED == iWinReason) || (WINREASON_FLAG_CAPTURE_LIMIT == iWinReason))
	{
		// if this was a full round that ended with point capture or flag capture, show the winning cappers
		const char *pCappers = event->GetString("cappers");
		int iCappers = Q_strlen(pCappers);
		if (iCappers > 0)
		{
			char szPlayerNames[256] = "";
			wchar_t wzPlayerNames[256] = L"";
			wchar_t wzCapMsg[512] = L"";
			for (int i = 0; i < iCappers; i++)
			{
				Q_strncat(szPlayerNames, g_PR->GetPlayerName((int)pCappers[i]), ARRAYSIZE(szPlayerNames));
				if (i < iCappers - 1)
				{
					Q_strncat(szPlayerNames, ", ", ARRAYSIZE(szPlayerNames));
				}
			}
			g_pVGuiLocalize->ConvertANSIToUnicode(szPlayerNames, wzPlayerNames, sizeof(wzPlayerNames));
			g_pVGuiLocalize->ConstructString(wzCapMsg, sizeof(wzCapMsg), g_pVGuiLocalize->Find("#Winpanel_WinningCapture"), 1, wzPlayerNames);
			SetDialogVariable("DetailsLabel", wzCapMsg);
		}
	}

	// get the current & previous team scores
	int iBlueTeamPrevScore = event->GetInt("blue_score_prev", 0);
	int iRedTeamPrevScore = event->GetInt("red_score_prev", 0);
	m_iBlueTeamScore = event->GetInt("blue_score", 0);
	m_iRedTeamScore = event->GetInt("red_score", 0);

	if (m_pTeamScorePanel)
	{
		if (bRoundComplete)
		{
			// set the previous team scores in scoreboard
			m_pTeamScorePanel->SetDialogVariable("blueteamscore", iBlueTeamPrevScore);
			m_pTeamScorePanel->SetDialogVariable("redteamscore", iRedTeamPrevScore);

			if ((m_iBlueTeamScore != iBlueTeamPrevScore) || (m_iRedTeamScore != iRedTeamPrevScore))
			{
				// if the new scores are different, set ourselves to update the scoreboard to the new values after a short delay, so players
				// see the scores tick up
				m_flTimeUpdateTeamScore = gpGlobals->curtime + 2.0f;
			}
		}
		// only show team scores if round is complete
		m_pTeamScorePanel->SetVisible(bRoundComplete);
	}

	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);
	if (!tf_PR)
		return;

	bool bShow;
	char szPlayerVal[64] = "";
	int iPlayerIndex, iRoundScore;
	Label *pPlayerName, *pPlayerClass, *pPlayerScore;

	// look for the top 3 players sent in the event
	for (int i = 1; i <= 3; i++)
	{
		bShow = false;

		// get player index and round points from the event
		Q_snprintf(szPlayerVal, ARRAYSIZE(szPlayerVal), "player_%d", i);
		iPlayerIndex = event->GetInt(szPlayerVal, 0);
		Q_snprintf(szPlayerVal, ARRAYSIZE(szPlayerVal), "player_%d_points", i);
		iRoundScore = event->GetInt(szPlayerVal, 0);

		// round score of 0 means no player to show for that position (not enough players, or didn't score any points that round)
		if (iRoundScore > 0)
			bShow = true;

#if !defined( _X360 )
		CAvatarImagePanel *pPlayerAvatar = dynamic_cast<CAvatarImagePanel *>(FindChildByName(CFmtStr("Player%dAvatar", i)));

		if (pPlayerAvatar)
		{
			if (bShow)
			{
				pPlayerAvatar->SetShouldDrawFriendIcon(false);
				pPlayerAvatar->SetPlayer(iPlayerIndex);
			}

			pPlayerAvatar->SetVisible(bShow);
		}
#endif

		pPlayerName = dynamic_cast<Label *>(FindChildByName(CFmtStr("Player%dName", i)));
		pPlayerClass = dynamic_cast<Label *>(FindChildByName(CFmtStr("Player%dClass", i)));
		pPlayerScore = dynamic_cast<Label *>(FindChildByName(CFmtStr("Player%dScore", i)));

		if (!pPlayerName || !pPlayerClass || !pPlayerScore)
			return;

		if (bShow)
		{
			// set the player labels to team or player color
			Color clr = TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() ? tf_PR->GetPlayerColor(iPlayerIndex) : g_PR->GetTeamColor(g_PR->GetTeam(iPlayerIndex));
			pPlayerName->SetFgColor(clr);
			pPlayerClass->SetFgColor(clr);
			pPlayerScore->SetFgColor(clr);

			// set label contents
			pPlayerName->SetText(g_PR->GetPlayerName(iPlayerIndex));
			pPlayerClass->SetText(g_aPlayerClassNames[tf_PR->GetPlayerClass(iPlayerIndex)]);
			pPlayerScore->SetText(CFmtStr("%d", iRoundScore));
		}

		// show or hide labels for this player position
		pPlayerName->SetVisible(bShow);
		pPlayerClass->SetVisible(bShow);
		pPlayerScore->SetVisible(bShow);
	}

	SetVisible(true);
}

//-----------------------------------------------------------------------------
// Purpose: returns whether panel should be drawn
//-----------------------------------------------------------------------------
bool CTFWinPanel::ShouldDraw()
{
	if (!IsVisible())
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: panel think method
//-----------------------------------------------------------------------------
void CTFWinPanel::OnThink()
{
	// if we've scheduled ourselves to update the team scores, handle it now
	if (m_flTimeUpdateTeamScore > 0 && (gpGlobals->curtime > m_flTimeUpdateTeamScore) && m_pTeamScorePanel)
	{
		// play a sound
		CLocalPlayerFilter filter;
		C_BaseEntity::EmitSound(filter, SOUND_FROM_LOCAL_PLAYER, "Hud.EndRoundScored");

		// update the team scores
		m_pTeamScorePanel->SetDialogVariable("blueteamscore", m_iBlueTeamScore);
		m_pTeamScorePanel->SetDialogVariable("redteamscore", m_iRedTeamScore);
		m_flTimeUpdateTeamScore = 0;
	}
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//-----------------------------------------------------------------------------
// Purpose: Circled X button to exit the winpanel
//-----------------------------------------------------------------------------

ExitCircle::ExitCircle(Panel *parent, const char *panelName, const char *cmd) : ImagePanel(parent, panelName)
{
	m_pParent = parent;
	SetParent(parent);
	Q_strcpy(command, cmd);
}

void ExitCircle::OnMouseReleased(MouseCode code)
{
	if (code == MOUSE_LEFT)
		m_pParent->OnCommand(command);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------

DECLARE_HUDELEMENT_DEPTH(CTFWinPanelDM, 70);

CTFWinPanelDM::CTFWinPanelDM(const char *pElementName) : EditablePanel(NULL, "WinPanelDM"), CHudElement(pElementName)
{
	SetParent(g_pClientMode->GetViewport());
	SetMouseInputEnabled(false);
	MakePopup();
	SetScheme("ClientScheme");

	// listen for events
	ListenForGameEvent("teamplay_win_panel");
	ListenForGameEvent("teamplay_round_start");
	ListenForGameEvent("teamplay_game_over");
	ListenForGameEvent("tf_game_over");
	
	m_flDisplayTime = -1;
	m_pRoundEndEvent = new KeyValues("RoundEndEvent");
}

CTFWinPanelDM::~CTFWinPanelDM()
{
	// We dont want to memory leaks here
	m_pRoundEndEvent->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWinPanelDM::Reset()
{
	SetVisible(false);
	m_flDisplayTime = -1;
}

void CTFWinPanelDM::SetVisible(bool state)
{
	if (state == IsVisible())
		return;

	SetMouseInputEnabled(state);

	BaseClass::SetVisible(state);
}

//-----------------------------------------------------------------------------
// Purpose: returns whether panel should be drawn
//-----------------------------------------------------------------------------
bool CTFWinPanelDM::ShouldDraw()
{
	if( m_flDisplayTime != -1 && m_flDisplayTime < gpGlobals->curtime )
		StartPanel( m_pRoundEndEvent );
	
	if (!IsVisible())
		return false;

	return CHudElement::ShouldDraw();
}

extern ConVar of_winscreenratio;
extern ConVar mp_bonusroundtime;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFWinPanelDM::FireGameEvent(IGameEvent *event)
{
	//Only draw for Mercenary deathmatch
	if (Q_strcmp("teamplay_win_panel", event->GetName()) || event->GetInt("winning_team") != TF_TEAM_MERCENARY)
	{
		SetVisible(false);
		return;
	}

	//Store event info and display them when the time comes
	if( TFGameRules() && TFGameRules()->IsDMGamemode() && !TFGameRules()->DontCountKills() )
		m_flDisplayTime = gpGlobals->curtime + mp_bonusroundtime.GetFloat() * of_winscreenratio.GetFloat();
	else
		m_flDisplayTime = gpGlobals->curtime;
	
	m_pRoundEndEvent->SetName(event->GetName());
	
	m_pRoundEndEvent->SetInt( "panel_style", event->GetInt("panel_style") );
	m_pRoundEndEvent->SetInt( "winning_team", event->GetInt("winning_team") );
	m_pRoundEndEvent->SetInt( "winreason", event->GetInt("winreason") );
	m_pRoundEndEvent->SetInt( "flagcaplimit", event->GetInt("flagcaplimit") );
	m_pRoundEndEvent->SetInt( "blue_score", event->GetInt("blue_score") );
	m_pRoundEndEvent->SetInt( "red_score", event->GetInt("red_score") );
	m_pRoundEndEvent->SetInt( "blue_score_prev", event->GetInt("blue_score_prev") );
	m_pRoundEndEvent->SetInt( "red_score_prev", event->GetInt("red_score_prev") );
	m_pRoundEndEvent->SetInt( "round_complete", event->GetInt("round_complete") );
	m_pRoundEndEvent->SetInt( "rounds_remaining", event->GetInt("rounds_remaining") );	
	m_pRoundEndEvent->SetString( "cappers", event->GetString("cappers") );
	
	
	DevMsg("%d %d\n", event->GetInt("winreason"), event->GetInt("winning_team"));
	for( int i = 0; i < 3; i++ )
	{
		char szPlayerIndexVal[64]="", szPlayerScoreVal[64]="";
		Q_snprintf( szPlayerIndexVal, ARRAYSIZE( szPlayerIndexVal ), "player_%d", i+ 1 );
		Q_snprintf( szPlayerScoreVal, ARRAYSIZE( szPlayerScoreVal ), "player_%d_points", i+ 1 );

		DevMsg("%s %d\n", szPlayerIndexVal, event->GetInt( szPlayerIndexVal ) );
		DevMsg("%s %d\n", szPlayerScoreVal, event->GetInt( szPlayerScoreVal ) );
		
		m_pRoundEndEvent->SetInt( szPlayerIndexVal, event->GetInt( szPlayerIndexVal ) );
		m_pRoundEndEvent->SetInt( szPlayerScoreVal, event->GetInt( szPlayerScoreVal ) );
	}
	
	// play a sound
	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound(filter, SOUND_FROM_LOCAL_PLAYER, "Hud.DMEndRoundScored");
}

void CTFWinPanelDM::StartPanel( KeyValues *event )
{
	m_flDisplayTime = -1;

	if (!g_PR)
		return;
	
	LoadControlSettings("resource/UI/WinPanelDM.res");
	InvalidateLayout(false, true);

	ImagePanel *pImagePanelBG = dynamic_cast<ImagePanel *>(FindChildByName("WinPanelBG"));
	Assert(pImagePanelBG);
	if (!pImagePanelBG)
		return;

	//**********************************************************************
	//Show top three scorers

	C_TF_PlayerResource *tf_PR = dynamic_cast<C_TF_PlayerResource *>(g_PR);
	if (!tf_PR)
		return;

	//Outside of the loop So we do not declare values three times
	char szPlayerVal[64] = "", pAttachedModel[128], szPlacement[16], szVCD[128], szClassModelName[128];
	const char *pszClassName;
	int iPlayerIndex, iRoundScore;
	Color clr;
	Label *pPlayerName, *playerScore;
	CModelPanel *pPlayerModel;
	KeyValues *pModelAttachement = new KeyValues("Models");
	pModelAttachement->LoadFromFile(filesystem, "resource/ui/winpaneldm_objects.txt");

	char szWinTheme[32] = { "Game.DMWin" };
	char szLooseTheme[32] = { "Game.DMLoose" };
	
	if( DMMusicManager() )
	{
		KeyValues *pMusic = GetSoundscript( DMMusicManager()->szRoundMusic );
		if( pMusic )
		{
			Q_strncpy( szWinTheme, pMusic->GetString("win", "Game.DMWin"), sizeof(szWinTheme) );
			Q_strncpy( szLooseTheme, pMusic->GetString("loose", "Game.DMLoose"), sizeof(szLooseTheme) );
		}
	}
	
	DevMsg( "%s\n", szWinTheme );
	DevMsg( "%s\n", szLooseTheme );
	
	int iPlayerAmount = 0;
	
	bool bLost = true;
	
	for (int i = 1; i <= 3; i++)
	{
		// get player index and round points from the event
		Q_snprintf(szPlayerVal, ARRAYSIZE(szPlayerVal), "player_%d", i);
		iPlayerIndex = event->GetInt(szPlayerVal, 0);

		Q_snprintf(szPlayerVal, ARRAYSIZE(szPlayerVal), "player_%d_points", i);
		iRoundScore = event->GetInt(szPlayerVal, 0);
		
		if( iPlayerIndex == C_BasePlayer::GetLocalPlayer()->entindex() && TeamplayRoundBasedRules() )
		{
			bLost = false;
			TeamplayRoundBasedRules()->BroadcastSoundFFA( iPlayerIndex, szWinTheme, "", false );
			switch( i )
			{
				case 1:
					TeamplayRoundBasedRules()->BroadcastSoundFFA( iPlayerIndex, "FirstPlace", "" );
					break;
				case 2:
					TeamplayRoundBasedRules()->BroadcastSoundFFA( iPlayerIndex, "SecondPlace", "" );
					break;
				case 3:
					TeamplayRoundBasedRules()->BroadcastSoundFFA( iPlayerIndex, "ThirdPlace", "" );
					break;
			}
		}

		// round score of 0 means no player to show for that position (not enough players, or didn't score any points that round)
		if( iRoundScore <= 0 )
			continue;

		pPlayerName = dynamic_cast<Label *>(FindChildByName(CFmtStr("Player%dName", i)));
		playerScore = dynamic_cast<Label *>(FindChildByName(CFmtStr("Player%dScore", i)));
		pPlayerModel = dynamic_cast<CModelPanel *>(FindChildByName(VarArgs("Player%dModel", i)));

		if (!pPlayerName || !pPlayerModel || !playerScore)
			continue;

		// set the player labels to team or player color
		clr = TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTeamplay() ? tf_PR->GetPlayerColor(iPlayerIndex) : g_PR->GetTeamColor(g_PR->GetTeam(iPlayerIndex));
		pPlayerName->SetFgColor(clr);
		playerScore->SetFgColor(clr);

		// set label contents
		pPlayerName->SetText(g_PR->GetPlayerName(iPlayerIndex));
		Q_snprintf(szPlayerVal, ARRAYSIZE(szPlayerVal), "Score: %d", iRoundScore);
		playerScore->SetText(szPlayerVal);

		CTFPlayer *pPlayer = ToTFPlayer(UTIL_PlayerByIndex(iPlayerIndex));
		if (pPlayer && pPlayer->GetPlayerClass() && pPlayer->GetPlayerClass()->GetModelName())
		{
			//Find the right animation depending on the placement
			if (i == 1)
				Q_snprintf(szPlacement, ARRAYSIZE(szPlacement), "first");
			else if (i == 2)
				Q_snprintf(szPlacement, ARRAYSIZE(szPlacement), "second");
			else
				Q_snprintf(szPlacement, ARRAYSIZE(szPlacement), "third");

			pszClassName = g_aPlayerClassNames_NonLocalized[pPlayer->GetPlayerClass()->GetClassIndex()];
			Q_snprintf(szVCD, sizeof(szVCD), "scenes/Player/%s/low/%s_place.vcd", pszClassName, szPlacement);

			//Find the weapon to be attached to the player model
			if (pModelAttachement)
			{
				Q_snprintf(szClassModelName, sizeof(szClassModelName), "%s_%s_model", pszClassName, szPlacement);
				Q_strncpy(pAttachedModel, pModelAttachement->GetString(szClassModelName), sizeof(pAttachedModel));
			}

			//Finalize player model setup
			pPlayerModel->SwapModel(pPlayer->GetPlayerClass()->GetModelName(), pAttachedModel, szVCD);

			if (pPlayerModel)
			{
				pPlayerModel->SetModelColor(tf_PR->GetPlayerColorVector(iPlayerIndex));
				pPlayerModel->SetVisible(true);

				if (!of_disable_cosmetics.GetBool())
				{
					for (int i = 0; i < pPlayer->m_iCosmetics.Count(); i++)
					{
						if (pPlayer->m_iCosmetics[i])
						{
							KeyValues* pCosmetic = GetCosmetic(pPlayer->m_iCosmetics[i]);
							if (!pCosmetic)
								continue;

							if (Q_strcmp(pCosmetic->GetString("Model"), "BLANK"))
							{
								pPlayerModel->AddAttachment(pCosmetic->GetString("Model", "models/empty.mdl"));
							}

							KeyValues* pBodygroups = pCosmetic->FindKey("Bodygroups");
							if (pBodygroups)
							{
								for (KeyValues* sub = pBodygroups->GetFirstValue(); sub; sub = sub->GetNextValue())
								{
									pPlayerModel->SetBodygroup(sub->GetName(), sub->GetInt());
								}
							}
						}
					}
				}
			}
		}

		// show or hide labels for this player position
		pPlayerName->SetVisible(true);
		playerScore->SetVisible(true);
		iPlayerAmount++;
	}
	
	if( bLost && TeamplayRoundBasedRules() )
	{
		TeamplayRoundBasedRules()->BroadcastSoundFFA( C_BasePlayer::GetLocalPlayer()->entindex(), "LastPlace", "" );
		TeamplayRoundBasedRules()->BroadcastSoundFFA( C_BasePlayer::GetLocalPlayer()->entindex(), szLooseTheme, "", false );
	}
	
	switch( iPlayerAmount )
	{
		case 1:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "HudDMWinpanelFirst");
			break;
		case 2:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "HudDMWinpanelSecond");
			break;
		case 3:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "HudDMWinpanelIntro");
			break;
	}

	pModelAttachement->deleteThis();
	SetVisible(true);
	MoveToFront();
}

void CTFWinPanelDM::OnTick( void )
{
	BaseClass::OnTick();
}