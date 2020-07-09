//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_FISTS_H
#define TF_WEAPON_FISTS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_melee.h"

#ifdef CLIENT_DLL
#define CTFFists C_TFFists
#define CTFBerserk C_TFBerserk
#define CTFClaws C_TFClaws
#define CTFJuggernaught C_TFJuggernaught
#endif

//=============================================================================
//
// Fists weapon class.
//
class CTFFists : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFFists, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFFists() {}
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_FISTS; }

	virtual void PrimaryAttack();
	virtual void SecondaryAttack();
	virtual bool CanSoftZoom( void ) { return false; }

	virtual void SendPlayerAnimEvent( CTFPlayer *pPlayer );

	virtual void DoViewModelAnimation( void );

	void Punch( void );

private:

	CTFFists( const CTFFists & ) {}
};

class CTFBerserk : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFBerserk, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFBerserk() {}
	virtual bool	CanHolster( void ) const { return false; }
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_BERSERK; }
	int			GetGGLevel( void ){ return 999; }

private:

	CTFBerserk( const CTFBerserk & ) {}
};

class CTFClaws : public CTFWeaponBaseMelee
{
public:

	DECLARE_CLASS( CTFClaws, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFClaws() {}
	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_CLAWS; }
	virtual void		ItemPostFrame(void);
	bool				CanPrimaryAttack(void) { return m_flNextPrimaryAttack < gpGlobals->curtime; }

private:

	CTFClaws( const CTFClaws & ) {}
};

class CTFJuggernaught : public CTFClaws
{
public:

	DECLARE_CLASS( CTFJuggernaught, CTFWeaponBaseMelee );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	
	CTFJuggernaught() {}
	virtual bool		CanHolster( void ) const { return false; }
	virtual int			GetWeaponID( void ) const { return TF_WEAPON_JUGGERNAUGHT; }

private:

	CTFJuggernaught( const CTFJuggernaught & ) {}
};

#endif // TF_WEAPON_FISTS_H
