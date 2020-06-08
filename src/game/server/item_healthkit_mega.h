//=============================================================================//
//
// Purpose: Mega Health "powerup".
//
//=============================================================================//
#include "entity_healthkit.h"
#include "tf_player.h"

class CHealthKitMega : public CHealthKit
{
public:

	DECLARE_CLASS(CHealthKitMega, CHealthKit);

	virtual const char *GetPowerupModel(void) { return "models/pickups/megahealth.mdl"; }
	powerupsize_t GetPowerupSize(void) { return POWERUP_MEGA; }
	string_t m_iszPickupSound = MAKE_STRING("HealthKitMega.Touch");

	bool MyTouch(CBasePlayer *pPlayer);
	void Precache(void);

	virtual bool   IsMega(void) { return true; }

	DECLARE_DATADESC();
};