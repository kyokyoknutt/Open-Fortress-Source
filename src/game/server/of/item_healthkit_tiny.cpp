#include "cbase.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "items.h"
#include "engine/IEngineSound.h"
#include "item_healthkit_tiny.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef OF_DLL
ConVar sk_item_healthkit_tiny("sk_item_healthkit_tiny", "8");

#define TF_HEALTHKIT_PICKUP_SOUND	"HealthKitTiny.Touch"

LINK_ENTITY_TO_CLASS(item_healthkit_tiny, CHealthKitTiny);
PRECACHE_REGISTER(item_healthkit_tiny);

BEGIN_DATADESC(CHealthKitTiny)

// Inputs.
DEFINE_KEYFIELD(m_iszModel, FIELD_STRING, "model"),
DEFINE_KEYFIELD(m_iszModelOLD, FIELD_STRING, "powerup_model"),
DEFINE_KEYFIELD(m_iszPickupSound, FIELD_STRING, "pickup_sound"),

END_DATADESC()

bool ITEM_GiveTFTinyHealth(CBasePlayer *pPlayer)
{
	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if (!pTFPlayer)
		return false;

	int iHealthBefore = pTFPlayer->GetHealth();
	int iHealthToAdd = clamp(sk_item_healthkit_tiny.GetInt(), 0, pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - iHealthBefore);
	pPlayer->TakeHealth(iHealthToAdd, DMG_IGNORE_MAXHEALTH);

	if (iHealthBefore < pTFPlayer->m_Shared.GetDefaultHealth())
		iHealthToAdd = max(0, pTFPlayer->GetHealth() - pTFPlayer->m_Shared.GetDefaultHealth());

	pTFPlayer->m_Shared.m_flMegaOverheal += iHealthToAdd;

	return true;
}
#endif

void CHealthKitTiny::Precache(void)
{
	if (m_iszModel == MAKE_STRING(""))
	{
		if (m_iszModelOLD != MAKE_STRING(""))
			PrecacheModel(STRING(m_iszModelOLD));
		else
			PrecacheModel(GetPowerupModel());
	}
	else
	{
		PrecacheModel(STRING(m_iszModel));
	}

	PrecacheScriptSound(TF_HEALTHKIT_PICKUP_SOUND);
}

bool CHealthKitTiny::MyTouch(CBasePlayer *pPlayer)
{
	bool m_bDoHeal = false;

	if (!ValidTouch(pPlayer))
		return m_bDoHeal;

	if (ITEM_GiveTFTinyHealth(pPlayer))
	{
		CSingleUserRecipientFilter filter(pPlayer);
		EmitSound(filter, entindex(), STRING(m_iszPickupSound));
		AddEffects(EF_NODRAW);
		m_bDoHeal = true;
	}

	return m_bDoHeal;
}