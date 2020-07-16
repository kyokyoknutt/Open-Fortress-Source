//=============================================================================//
// Purpose: Deathmatch Medals HUD
//=============================================================================//

#include "cbase.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "tf_gamerules.h"
#include "c_tf_player.h"
#include "of_hud_medals.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MEDAL_TIME 2.5f
#define MEDAL_SIZE 128

ConVar of_show_medals("of_show_medals", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

DECLARE_HUDELEMENT(CTFHudMedals);

//-----------------------------------------------------------------------------
// Purpose: medal info
//-----------------------------------------------------------------------------

const char *CTFHudMedals::medalNames[DENIED + 1] =
{ "FirstBlood", "Perfect",		"Impressive", "Perforated", "Humiliation", "Kamikaze", "Midair",		  "Headshot",	 "Excellent",	 "MultiKill", "UltraKill",
  "HolyShit",	"KillingSpree",	"Rampage",	  "Dominating", "Unstoppable", "GodLike",  "PowerupMassacre", "ShowStopper", "PartyBreaker", "Denied"				   };

const char *CTFHudMedals::medalPaths[DENIED + 1] =
{ "../hud/medals/firstblood_medal_def",		"../hud/medals/perfect_medal_def",			"../hud/medals/impressive_medal_def",	"../hud/medals/perforated_medal_def",
  "../hud/medals/humiliation_medal_def",	"../hud/medals/kamikaze_medal_def",			"../hud/medals/midair_medal_def",		"../hud/medals/headshot_medal_def",
  "../hud/medals/excellent_medal_def",		"../hud/medals/multikill_medal_def",		"../hud/medals/ultrakill_medal_def",	"../hud/medals/holyshit_medal_def",
  "../hud/medals/killingspree_medal_def",	"../hud/medals/rampage_medal_def",			"../hud/medals/dominating_medal_def",	"../hud/medals/unstoppable_medal_def",
  "../hud/medals/godlike_medal_def",		"../hud/medals/powerupmassacre_medal_def",	"../hud/medals/showstopper_medal_def",	"../hud/medals/partybreaker_medal_def",
  "../hud/medals/denied_medal_def" };

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
void CTFHudMedals::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	// load control settings...
	LoadControlSettings("Resource/UI/HudMedals.res");
}

CTFHudMedals::CTFHudMedals(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudMedals")
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	m_pMedalImage = new ImagePanel(this, "MedalDrawer");

	//Set events to catch
	ListenForGameEvent("player_hurt");
	ListenForGameEvent("player_death");
	ListenForGameEvent("teamplay_win_panel");

	m_pMedalImage->SetVisible(false);
	
	died = 0;
	drawTime = 0;
	for (int i = 0; i <= DENIED; i++)
		medals_counter[i] = 0;
	medalsQueue.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: decide if HUD element should be drawn or not
//-----------------------------------------------------------------------------
bool CTFHudMedals::ShouldDraw(void)
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if (!pPlayer || !TFGameRules()->IsDMGamemode() || !medalsQueue.Size())
		return false;
	return CHudElement::ShouldDraw();
}

void CTFHudMedals::Reset(void)
{
	m_pMedalImage->SetVisible(false);

	died = 0;
	drawTime = 0;
	for (int i = 0; i <= DENIED; i++)
		medals_counter[i] = 0;
	medalsQueue.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: control the medal drawing
//-----------------------------------------------------------------------------

void CTFHudMedals::OnThink(void)
{
	if (!medalsQueue.Size())
		return;

	//Initialize the time frame medal should be drawn
	if (!drawTime)
	{
		// if things crash here, uncomment the line below
		// if( TFGameRules() )
		TFGameRules()->BroadcastSound( TEAM_UNASSIGNED, medalsQueue[0].medal_sound );

		m_pMedalImage->SetImage(medalsQueue[0].medal_name);
		m_pMedalImage->SetVisible(true);
		drawTime = gpGlobals->curtime + MEDAL_TIME;
	}

	m_pMedalImage->SetPos((GetWide() - MEDAL_SIZE) / 2, MEDAL_SIZE);

	if (gpGlobals->curtime <= drawTime)
		return;

	//Remove medal
	medalsQueue.Remove(0);
	drawTime = 0;
	if (!medalsQueue.Size())
		m_pMedalImage->SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: award medals to player if certain conditions are met
//-----------------------------------------------------------------------------

void CTFHudMedals::FireGameEvent(IGameEvent *event)
{
	if (!event || TFGameRules()->IsInWaitingForPlayers())
		return;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if (!pPlayer)
		return;

	const char *eventname = event->GetName();
	int pIndex = pPlayer->GetUserID();

	if (!Q_strcmp("teamplay_win_panel", eventname)) //perfect
	{
		if (event->GetInt("player_1") == pPlayer->entindex() && !died)
			AddMedal(PERFECT);

		medalsQueue.Purge(); //no medals shown after match is over
		m_pMedalImage->SetVisible(false);
	}
	else if (!Q_strcmp("player_hurt", eventname))
	{
		if (event->GetInt("attacker") != pIndex)
			return;

		//Impressive
		if (event->GetBool("impressive"))
			AddMedal(IMPRESSIVE);
	}
	else if (!Q_strcmp("player_death", eventname))
	{
		//you killed
		if (event->GetInt("attacker") == pIndex)
		{
			//Kamikaze
			if (event->GetBool("kamikaze"))
				AddMedal(KAMIKAZE);
			
			//Midair
			if (event->GetBool("midair"))
				AddMedal(MIDAIR);

			//Humiliation
			if (event->GetBool("humiliation"))
				AddMedal(HUMILIATION);

			//First blood
			if (event->GetBool("firstblood"))
				AddMedal(FIRSTBLOOD);

			//Telefrag
			if (event->GetInt("customkill") == TF_DMG_CUSTOM_TELEFRAG)
				AddMedal(PERFORATED);

			//Headshot
			if (event->GetInt("customkill") == TF_DMG_CUSTOM_HEADSHOT || event->GetInt("customkill") == TF_DMG_CUSTOM_HEADSHOT_DECAPITATION)
				AddMedal(HEADSHOT);

			//Excellent
			int ex_streak = event->GetInt("ex_streak");
			if (ex_streak == 1)
				AddMedal(EXCELLENT);
			else if (ex_streak == 2)
				AddMedal(MULTIKILL);
			else if (ex_streak == 4)
				AddMedal(ULTRAKILL);
			else if (ex_streak == 7)
				AddMedal(HOLYSHIT);

			//Killing Spree
			int kspree_streak = event->GetInt("killer_kspree");
			if (kspree_streak == 5)
				AddMedal(KILLINGSPREE);
			else if (kspree_streak == 10)
				AddMedal(RAMPAGE);
			else if (kspree_streak == 15)
				AddMedal(DOMINATING);
			else if (kspree_streak == 20)
				AddMedal(UNSTOPPABLE);
			else if (kspree_streak == 25)
				AddMedal(GODLIKE);

			//Powerup Massacre
			if (event->GetInt("killer_pupkills") == 10)
				AddMedal(POWERUPMASSACRE);

			//Showstopper
			if (event->GetInt("victim_kspree") >= 5)
				AddMedal(SHOWSTOPPER);

			//Denied-Party Breaker
			int powerupKills = event->GetInt("victim_pupkills");
			if (powerupKills == -1)
				return;
			if (powerupKills)
				AddMedal(PARTYBREAKER);
			else
				AddMedal(DENIED);
		}

		//you were killed
		if (event->GetInt("userid") == pIndex)
			died = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: add a earned medal at the end of the array
//-----------------------------------------------------------------------------
void CTFHudMedals::AddMedal(int medalIndex)
{
	if (of_show_medals.GetBool())
		medalsQueue.AddToTail({ medalPaths[medalIndex], medalNames[medalIndex] });

	medals_counter[medalIndex] = medalIndex < 2 ? 1 : medals_counter[medalIndex] + 1;
}

//-----------------------------------------------------------------------------
// Purpose: return earned medals to be drawn by the parent menu
//-----------------------------------------------------------------------------
int CTFHudMedals::GetMedalCount(int medalIndex)
{
	return medals_counter[medalIndex];
}