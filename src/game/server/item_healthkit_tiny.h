//=============================================================================//
//
// Purpose: Quake-like health shards in the form of pills. Louis would be very happy.
//
//=============================================================================//
#include "entity_healthkit.h"
#include "tf_player.h"

class CHealthKitTiny : public CHealthKit
{
public:

	DECLARE_CLASS(CHealthKitTiny, CHealthKit);

	virtual const char *GetPowerupModel(void) { return "models/items/medkit_overheal.mdl"; }
	powerupsize_t GetPowerupSize(void) { return POWERUP_TINY; }
	string_t m_iszPickupSound = MAKE_STRING("HealthKitTiny.Touch");

	bool MyTouch(CBasePlayer *pPlayer);
	void Precache(void);

	virtual bool   IsTiny(void) { return true; }

	DECLARE_DATADESC();
};