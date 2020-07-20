//========= Copyright © 1996-2005, Valve LLC, All rights reserved. ============
//
// Purpose:
//
//=============================================================================
#ifndef TF_PLAYERCLASS_SHARED_H
#define TF_PLAYERCLASS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_shareddefs.h"

#define TF_NAME_LENGTH		128

// Client specific.
#ifdef CLIENT_DLL

EXTERN_RECV_TABLE( DT_TFPlayerClassShared );

// Server specific.
#else

EXTERN_SEND_TABLE( DT_TFPlayerClassShared );

#endif


//-----------------------------------------------------------------------------
// Cache structure for the TF player class data (includes citizen). 
//-----------------------------------------------------------------------------

#define MAX_PLAYERCLASS_SOUND_LENGTH	128

struct TFPlayerClassData_t
{
	DECLARE_CLASS_NOBASE( TFPlayerClassData_t );
	DECLARE_EMBEDDED_NETWORKVAR();
#ifndef CLIENT_DLL
	DECLARE_SIMPLE_DATADESC();
#endif

	char		m_szClassName[TF_NAME_LENGTH];

	CNetworkString( m_szModelName, TF_NAME_LENGTH );
	CNetworkString( m_szArmModelName, TF_NAME_LENGTH );
	CNetworkString( m_szLocalizableName, TF_NAME_LENGTH );
	CNetworkString( m_szJumpSound, TF_NAME_LENGTH );

	CNetworkVar( float, m_flMaxSpeed );
	CNetworkVar( int, m_nMaxHealth );
	CNetworkVar( int, m_nMaxArmor );
	
	CNetworkArray( int, m_aWeapons, TF_PLAYER_WEAPON_COUNT );
	
	CNetworkVar( int, m_iWeaponCount );
	
	CNetworkArray( int, m_aGrenades, TF_PLAYER_GRENADE_COUNT );
	CNetworkArray( int, m_aAmmoMax, TF_AMMO_COUNT );
	CNetworkArray( int, m_aBuildable, TF_PLAYER_BUILDABLE_COUNT );

	CNetworkVar( int, m_nCapNumber );
	CNetworkVar( int, m_nMaxAirDashCount );
	CNetworkVar( bool, m_bDontDoAirwalk );
	CNetworkVar( bool, m_bDontDoNewJump );
	
	CNetworkVar( bool, m_bSpecialClass );

	CNetworkString( m_szClassSelectImageRed, TF_NAME_LENGTH );
	CNetworkString( m_szClassSelectImageBlue, TF_NAME_LENGTH );
	CNetworkString( m_szClassSelectImageMercenary, TF_NAME_LENGTH );	
	
	CNetworkString( m_szClassImageRed, TF_NAME_LENGTH );
	CNetworkString( m_szClassImageBlue, TF_NAME_LENGTH );
	CNetworkString( m_szClassImageMercenary, TF_NAME_LENGTH );		
	CNetworkString( m_szClassImageColorless, TF_NAME_LENGTH );	
	
	CNetworkString( m_szClassIcon, TF_NAME_LENGTH );

	
	CNetworkVar( int, m_nViewVector );

	bool		m_bParsed;

#ifdef GAME_DLL
	// sounds
	char		m_szDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
	char		m_szCritDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
	char		m_szMeleeDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
	char		m_szExplosionDeathSound[MAX_PLAYERCLASS_SOUND_LENGTH];
#endif

	TFPlayerClassData_t();
	const char *GetModelName() const;
	const char *GetArmModelName() const { return m_szArmModelName; }
	const char *GetClassSelectImageRed() const { return m_szClassSelectImageRed; }
	const char *GetClassSelectImageBlue() const { return m_szClassSelectImageBlue; }
	const char *GetClassSelectImageMercenary() const { return m_szClassSelectImageMercenary; }
	
	const char *GetClassImageRed() const { return m_szClassImageRed; }
	const char *GetClassImageBlue() const { return m_szClassImageBlue; }
	const char *GetClassImageMercenary() const { return m_szClassImageMercenary; }
	const char *GetClassImageColorless() const { return m_szClassImageColorless; }
	
	const char *GetClassIcon() const { return m_szClassIcon; }
	
	const char *GetJumpSound() const { return m_szJumpSound; }
	void Parse( const char *pszClassName );

	// Parser for the class data.
	void ParseData( KeyValues *pKeyValuesData );
};

//-----------------------------------------------------------------------------
// TF Player Class Shared
//-----------------------------------------------------------------------------
class CTFPlayerClassShared
{
public:

	CTFPlayerClassShared();

	DECLARE_EMBEDDED_NETWORKVAR()
	DECLARE_CLASS_NOBASE( CTFPlayerClassShared );

	bool		Init( int iClass, int iModifiers = 0 );
	void		SetModifier( int iModifiers );
	bool		IsClass( int iClass ) const						{ return ( m_iClass == iClass ); }
	int			GetClassIndex( void )							{ return m_iClass; }
	
	char		*GetName( void );
#ifndef CLIENT_DLL
	void		SetCustomModel( const char *pszModelName );
	void		SetCustomArmModel( const char *pszModelName );
#endif	
	char		*GetModelName( void );
	char 		*GetSetCustomModel ( void );		
	bool 		UsesCustomModel ( void );	
	char 		*GetSetCustomArmModel ( void );		
	bool 		UsesCustomArmModel ( void );		

	char		*GetArmModelName( void );

	float		GetMaxSpeed( void );
	int			GetMaxHealth( void );
	int			GetMaxArmor( void );
	bool		IsSpecialClass( void );
	bool		CanAirDash( void );
	int			MaxAirDashCount( void );
	int			GetCapNumber( void );
	const char *GetJumpSound();

	TFPlayerClassData_t  *GetData( void );

	// If needed, put this into playerclass scripts
	bool CanBuildObject( int iObjectType );

	CNetworkVarEmbedded( TFPlayerClassData_t, pLocalData );
	
	const int GetClass ( void ) const { return m_iClass; }
	const int GetModifiers ( void ) const { return m_iModifiers; }
	const int GetOldModifiers ( void ) const { return m_iOldModifiers; }
	const int GetOldClass ( void ) const { return m_iOldClass; }
	void SetClass ( int iClass ) { m_iClass = iClass; }
	void SetOldModifiers ( int iOldModifiers ) { m_iOldModifiers = iOldModifiers; }
	void SetOldClass ( int iOldClass ) { m_iOldClass = iOldClass; }
	
protected:


	CNetworkVar( int,	m_iClass );
	CNetworkVar( int,	m_iOldClass );
	CNetworkVar( int,	m_iModifiers );
	CNetworkVar( int,	m_iOldModifiers );
	CNetworkVar( bool,	m_bRefresh );
#ifdef CLIENT_DLL
	char	m_iszSetCustomModel[MAX_PATH];
	char	m_iszSetCustomArmModel[MAX_PATH];
	
	bool	bRefresh;
#else
	CNetworkVar(string_t, m_iszSetCustomModel);
	CNetworkVar(string_t, m_iszSetCustomArmModel);
#endif
};

void InitPlayerClasses( void );
TFPlayerClassData_t *GetPlayerClassData( int iClass );

#endif // TF_PLAYERCLASS_SHARED_H