//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======//
//
// Purpose: CTF AmmoPack.
//
//=============================================================================//
#ifndef ENTITY_AMMOPACK_H
#define ENTITY_AMMOPACK_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"

//=============================================================================
//
// CTF AmmoPack class.
//

DECLARE_AUTO_LIST(IAmmoPackAutoList)

class CAmmoPack : public CTFPowerup, public IAmmoPackAutoList
{
public:
	DECLARE_CLASS( CAmmoPack, CTFPowerup );

	void	Spawn( void );
	void	Precache( void );
	bool	MyTouch( CBasePlayer *pPlayer );

	powerupsize_t	GetPowerupSize( void ) { return POWERUP_FULL; }
	string_t m_iszModel=MAKE_STRING( "" );
	string_t m_iszModelOLD=MAKE_STRING( "" );
	string_t m_iszPickupSound=MAKE_STRING( "AmmoPack.Touch" );
	virtual const char *GetPowerupModel( void ) { return "models/items/ammopack_large.mdl"; }
	DECLARE_DATADESC();
};

class CAmmoPackSmall : public CAmmoPack
{
public:
	DECLARE_CLASS( CAmmoPackSmall, CAmmoPack );
	powerupsize_t	GetPowerupSize( void ) { return POWERUP_SMALL; }
	virtual const char *GetPowerupModel( void ) { return "models/items/ammopack_small.mdl"; }
	DECLARE_DATADESC();
};

class CAmmoPackMedium : public CAmmoPack
{
public:
	DECLARE_CLASS( CAmmoPackMedium, CAmmoPack );
	powerupsize_t	GetPowerupSize( void ) { return POWERUP_MEDIUM; }
	virtual const char *GetPowerupModel( void ) { return "models/items/ammopack_medium.mdl"; }
	DECLARE_DATADESC();
};

#endif // ENTITY_AMMOPACK_H


