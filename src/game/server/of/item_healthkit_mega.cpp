#include "cbase.h"
#include "basecombatweapon.h"
#include "gamerules.h"
#include "items.h"
#include "engine/IEngineSound.h"
#include "item_healthkit_mega.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef OF_DLL
ConVar sk_item_healthkit_mega("sk_item_healthkit_mega", "100");

#define TF_HEALTHKIT_PICKUP_SOUND	"HealthKitMega.Touch"

LINK_ENTITY_TO_CLASS(item_healthkit_mega, CHealthKitMega);
PRECACHE_REGISTER(item_healthkit_mega);

BEGIN_DATADESC(CHealthKitMega)
END_DATADESC()

bool ITEM_GiveTFMegaHealth(CBasePlayer *pPlayer)
{
	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if (!pTFPlayer)
		return false;

	int iHealthToAdd = pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - pTFPlayer->GetHealth();
	iHealthToAdd = clamp(iHealthToAdd, 0, pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - pTFPlayer->GetHealth());
	pPlayer->TakeHealth(iHealthToAdd, DMG_IGNORE_MAXHEALTH);
	pTFPlayer->m_Shared.m_flMegaOverheal = pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - pTFPlayer->m_Shared.GetDefaultHealth();

	return true;
}
#endif

void CHealthKitMega::Precache(void)
{
	BaseClass::Precache();

	PrecacheScriptSound(TF_HEALTHKIT_PICKUP_SOUND);
}

bool CHealthKitMega::DoPowerupEffect( CTFPlayer *pTFPlayer )
{
	if (ITEM_GiveTFMegaHealth(pTFPlayer))
	{
		CSingleUserRecipientFilter filter(pTFPlayer);
		EmitSound(filter, entindex(), STRING(m_iszPickupSound));
		return true;
	}
	else
	{
		return false;
	}
}