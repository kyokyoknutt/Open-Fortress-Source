//=============================================================================//
//
// Purpose: Mega Health "powerup".
//
//=============================================================================//
#include "entity_condpowerup.h"
#include "tf_player.h"

class CHealthKitMega : public CCondPowerup
{
public:

	DECLARE_CLASS(CHealthKitMega, CCondPowerup);

	virtual const char *GetPowerupModel(void) { return "models/pickups/megahealth.mdl"; }
	powerupsize_t GetPowerupSize(void) { return POWERUP_MEGA; }
	string_t m_iszPickupSound = MAKE_STRING("HealthKitMega.Touch");

	void Precache(void);
	virtual bool RemoveIfDuel() { return false; }
	virtual bool DoPowerupEffect( CTFPlayer *pTFPlayer );

	virtual bool   IsMega(void) { return true; }

	DECLARE_DATADESC();
};