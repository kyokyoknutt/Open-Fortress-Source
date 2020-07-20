//========= Copyright © 1996-2004, Valve LLC, All rights reserved. ============
//
//=============================================================================

#include "cbase.h"
#include "tf_playerclass_shared.h"

ConVar of_airdashcount("of_airdashcount", "-1", FCVAR_NOTIFY | FCVAR_REPLICATED );

#define TF_CLASS_UNDEFINED_FILE			""
#define TF_CLASS_SCOUT_FILE				"scripts/playerclasses/scout"
#define TF_CLASS_SNIPER_FILE			"scripts/playerclasses/sniper"
#define TF_CLASS_SOLDIER_FILE			"scripts/playerclasses/soldier"
#define TF_CLASS_DEMOMAN_FILE			"scripts/playerclasses/demoman"
#define TF_CLASS_MEDIC_FILE				"scripts/playerclasses/medic"
#define TF_CLASS_HEAVYWEAPONS_FILE		"scripts/playerclasses/heavyweapons"
#define TF_CLASS_PYRO_FILE				"scripts/playerclasses/pyro"
#define TF_CLASS_SPY_FILE				"scripts/playerclasses/spy"
#define TF_CLASS_ENGINEER_FILE			"scripts/playerclasses/engineer"
#define TF_CLASS_MERCENARY_FILE			"scripts/playerclasses/mercenary"
#define TF_CLASS_CIVILIAN_FILE			"scripts/playerclasses/civilian"
#define TF_CLASS_JUGGERNAUT_FILE		"scripts/playerclasses/juggernaut"

const char *s_aPlayerClassFiles[] =
{
	TF_CLASS_UNDEFINED_FILE,
	TF_CLASS_SCOUT_FILE,
	TF_CLASS_SNIPER_FILE,
	TF_CLASS_SOLDIER_FILE,
	TF_CLASS_DEMOMAN_FILE,
	TF_CLASS_MEDIC_FILE,
	TF_CLASS_HEAVYWEAPONS_FILE,
	TF_CLASS_PYRO_FILE,
	TF_CLASS_SPY_FILE,
	TF_CLASS_ENGINEER_FILE,
	TF_CLASS_MERCENARY_FILE,
	TF_CLASS_CIVILIAN_FILE,
	TF_CLASS_JUGGERNAUT_FILE,
};

TFPlayerClassData_t s_aTFPlayerClassData[TF_CLASS_COUNT_ALL];
KeyValues* s_aTFPlayerClassRaw[TF_CLASS_COUNT_ALL];

BEGIN_NETWORK_TABLE_NOBASE( TFPlayerClassData_t, DT_PlayerClassData )

#ifdef CLIENT_DLL

	RecvPropString( RECVINFO( m_szModelName ) ),
	RecvPropString( RECVINFO( m_szArmModelName ) ),
	RecvPropString( RECVINFO( m_szLocalizableName ) ),
	RecvPropString( RECVINFO( m_szJumpSound ) ),
	RecvPropFloat( RECVINFO( m_flMaxSpeed ) ),
	RecvPropInt( RECVINFO( m_nMaxHealth ) ),
	RecvPropInt( RECVINFO( m_nMaxArmor ) ),
	
	RecvPropArray3( RECVINFO_ARRAY(m_aWeapons), RecvPropInt( RECVINFO( m_aWeapons[0] ) ) ),
	
	RecvPropInt( RECVINFO( m_iWeaponCount ) ),
	
	RecvPropArray3( RECVINFO_ARRAY(m_aGrenades), RecvPropInt( RECVINFO( m_aGrenades[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_aAmmoMax), RecvPropInt( RECVINFO( m_aAmmoMax[0] ) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_aBuildable), RecvPropInt( RECVINFO( m_aBuildable[0] ) ) ),

	RecvPropInt( RECVINFO( m_nCapNumber) ),
	RecvPropInt( RECVINFO( m_nMaxAirDashCount) ),
	
	RecvPropBool( RECVINFO( m_bDontDoAirwalk) ),
	RecvPropBool( RECVINFO( m_bDontDoNewJump) ),
	
	RecvPropBool( RECVINFO( m_bSpecialClass) ),
	RecvPropString( RECVINFO( m_szClassSelectImageRed ) ),
	RecvPropString( RECVINFO( m_szClassSelectImageBlue ) ),
	RecvPropString( RECVINFO( m_szClassSelectImageMercenary ) ),	
	
	RecvPropString( RECVINFO( m_szClassImageRed ) ),
	RecvPropString( RECVINFO( m_szClassImageBlue ) ),
	RecvPropString( RECVINFO( m_szClassImageMercenary ) ),		
	RecvPropString( RECVINFO( m_szClassImageColorless ) ),

	RecvPropString( RECVINFO( m_szClassIcon ) ),

	RecvPropInt( RECVINFO( m_nViewVector ) ),
/*
	RecvPropBool( RECVINFO( m_bParsed ) ),
*/
#else

	SendPropString( SENDINFO( m_szModelName ) ),
	SendPropString( SENDINFO( m_szArmModelName ) ),
	SendPropString( SENDINFO( m_szLocalizableName ) ),
	SendPropString( SENDINFO( m_szJumpSound ) ),
	SendPropFloat( SENDINFO( m_flMaxSpeed ) ),
	SendPropInt( SENDINFO( m_nMaxHealth ) ),
	SendPropInt( SENDINFO( m_nMaxArmor ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_aWeapons), SendPropInt( SENDINFO_ARRAY(m_aWeapons) ) ),

	SendPropInt( SENDINFO( m_iWeaponCount ) ),
	
	SendPropArray3( SENDINFO_ARRAY3(m_aGrenades), SendPropInt( SENDINFO_ARRAY(m_aGrenades) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_aAmmoMax), SendPropInt( SENDINFO_ARRAY(m_aAmmoMax) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_aBuildable), SendPropInt( SENDINFO_ARRAY(m_aBuildable) ) ),

	SendPropInt( SENDINFO( m_nCapNumber ) ),
	SendPropInt( SENDINFO( m_nMaxAirDashCount ) ),
	SendPropBool( SENDINFO( m_bDontDoAirwalk ) ),
	SendPropBool( SENDINFO( m_bDontDoNewJump ) ),
	
	SendPropBool( SENDINFO( m_bSpecialClass ) ),
	SendPropString( SENDINFO( m_szClassSelectImageRed ) ),
	SendPropString( SENDINFO( m_szClassSelectImageBlue ) ),
	SendPropString( SENDINFO( m_szClassSelectImageMercenary ) ),	
	
	SendPropString( SENDINFO( m_szClassImageRed ) ),
	SendPropString( SENDINFO( m_szClassImageBlue ) ),
	SendPropString( SENDINFO( m_szClassImageMercenary ) ),		
	SendPropString( SENDINFO( m_szClassImageColorless ) ),	
	
	SendPropString( SENDINFO( m_szClassIcon ) ),	

	SendPropInt( SENDINFO( m_nViewVector ) ),
/*
	SendPropBool( SENDINFO( m_bParsed ) ),
*/
#endif 
END_NETWORK_TABLE()

#ifndef CLIENT_DLL
BEGIN_SIMPLE_DATADESC( TFPlayerClassData_t )
	DEFINE_FIELD( m_szModelName, FIELD_STRING ),
	DEFINE_FIELD( m_szArmModelName, FIELD_STRING ),
	DEFINE_FIELD( m_szLocalizableName, FIELD_STRING ),
	DEFINE_FIELD( m_szJumpSound, FIELD_STRING ),
	DEFINE_FIELD( m_flMaxSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_nMaxHealth, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMaxArmor, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_aWeapons, FIELD_INTEGER ),
	
	DEFINE_FIELD( m_iWeaponCount, FIELD_INTEGER ),
	
	DEFINE_AUTO_ARRAY( m_aGrenades, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_aAmmoMax, FIELD_INTEGER ),
	DEFINE_AUTO_ARRAY( m_aBuildable, FIELD_INTEGER ),
	

	DEFINE_FIELD( m_nCapNumber, FIELD_INTEGER ),
	DEFINE_FIELD( m_nMaxAirDashCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_bDontDoAirwalk, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDontDoNewJump, FIELD_BOOLEAN ),
	
	DEFINE_FIELD( m_bSpecialClass, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_szClassSelectImageRed, FIELD_STRING ),
	DEFINE_FIELD( m_szClassSelectImageBlue, FIELD_STRING ),
	DEFINE_FIELD( m_szClassSelectImageMercenary, FIELD_STRING ),
	
	DEFINE_FIELD( m_szClassImageRed, FIELD_STRING ),
	DEFINE_FIELD( m_szClassImageBlue, FIELD_STRING ),
	DEFINE_FIELD( m_szClassImageMercenary, FIELD_STRING ),
	DEFINE_FIELD( m_szClassImageColorless, FIELD_STRING ),
	
	DEFINE_FIELD( m_szClassIcon, FIELD_STRING ),
	
	DEFINE_FIELD( m_nViewVector, FIELD_INTEGER ),

	DEFINE_FIELD( m_bParsed, FIELD_BOOLEAN ),
END_DATADESC()
#endif
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
TFPlayerClassData_t::TFPlayerClassData_t()
{
//	m_szClassName.Set( NULL_STRING );
//	m_szModelName.Set( NULL_STRING );
//	m_szArmModelName.Set( NULL_STRING );
//	m_szLocalizableName.Set( NULL_STRING );
//	m_szJumpSound.Set( NULL_STRING );
	m_flMaxSpeed = 0.0f;
	m_nMaxHealth = 0;
	m_nMaxArmor = 0;
	
	m_nMaxAirDashCount = 0;
	
	m_nCapNumber = 1;

#ifdef GAME_DLL
	m_szDeathSound[0] = '\0';
	m_szCritDeathSound[0] = '\0';
	m_szMeleeDeathSound[0] = '\0';
	m_szExplosionDeathSound[0] = '\0';
#endif

//	m_szClassSelectImageRed.Set( NULL_STRING );
//	m_szClassSelectImageBlue.Set( NULL_STRING );
//	m_szClassSelectImageMercenary.Set( NULL_STRING );
	
//	m_szClassImageRed.Set( NULL_STRING );
//	m_szClassImageBlue.Set( NULL_STRING );
//	m_szClassImageMercenary.Set( NULL_STRING );
//	m_szClassImageColorless.Set( NULL_STRING );

	for ( int iWeapon = 0; iWeapon < TF_PLAYER_WEAPON_COUNT; ++iWeapon )
	{
		m_aWeapons.GetForModify(iWeapon) = TF_WEAPON_NONE;
	}
	
	m_iWeaponCount = 0;
	
	for ( int iGrenade = 0; iGrenade < TF_PLAYER_GRENADE_COUNT; ++iGrenade )
	{
		m_aGrenades.GetForModify(iGrenade) = TF_WEAPON_NONE;
	}

	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		m_aAmmoMax.GetForModify(iAmmo) = TF_AMMO_DUMMY;
	}

	for ( int iBuildable = 0; iBuildable < TF_PLAYER_BUILDABLE_COUNT; ++iBuildable )
	{
		m_aBuildable.GetForModify(iBuildable) = OBJ_LAST;
	}

	m_bParsed = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFPlayerClassData_t::Parse( const char *szName )
{
	// Have we parsed this file already?
	if ( m_bParsed )
		return;

	
	// No filesystem at this point????  Hmmmm......

	// Parse class file.

	const unsigned char *pKey = NULL;

	if ( g_pGameRules )
	{
		pKey = g_pGameRules->GetEncryptionKey();
	}

	KeyValues *pKV = ReadEncryptedKVFile( filesystem, szName, pKey );
	
	if ( pKV )
	{
		ParseData( pKV );
		pKV->deleteThis();
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *TFPlayerClassData_t::GetModelName() const
{
#ifdef CLIENT_DLL	
	return m_szModelName;
	
#else
	return m_szModelName;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Loads up class.txt @modelsetc
//-----------------------------------------------------------------------------
void TFPlayerClassData_t::ParseData( KeyValues *pKeyValuesData )
{
	// Attributes.
	Q_strncpy( m_szClassName, pKeyValuesData->GetString( "name" ), TF_NAME_LENGTH );

	// Load the high res model or the lower res model.
	Q_strncpy( m_szArmModelName.GetForModify(), pKeyValuesData->GetString( "arm_model" ), TF_NAME_LENGTH );
	Q_strncpy( m_szModelName.GetForModify(), pKeyValuesData->GetString( "model" ), TF_NAME_LENGTH );
	Q_strncpy( m_szLocalizableName.GetForModify(), pKeyValuesData->GetString( "localize_name" ), TF_NAME_LENGTH );
	Q_strncpy( m_szJumpSound.GetForModify(), pKeyValuesData->GetString( "jump_sound" ), TF_NAME_LENGTH );
	
	m_flMaxSpeed = pKeyValuesData->GetFloat( "speed_max" );
	m_nMaxHealth = pKeyValuesData->GetInt( "health_max" );
	m_nMaxArmor = pKeyValuesData->GetInt( "armor_max" );
	
	m_nMaxAirDashCount = pKeyValuesData->GetInt( "MaxAirDashCount" );
	
	m_nCapNumber = pKeyValuesData->GetInt( "CapMultiplier" );
	m_iWeaponCount = pKeyValuesData->GetInt( "WeaponCount", 1 );
	
	// Weapons.
	int i;
	char buf[32];
	for ( i=0;i<TF_PLAYER_WEAPON_COUNT;i++ )
	{
		Q_snprintf( buf, sizeof(buf), "weapon%d", i+1 );
		int iID = GetWeaponId(pKeyValuesData->GetString(buf));
		m_aWeapons.GetForModify(i) = iID;
	}

	// Grenades.
	m_aGrenades.GetForModify(0) = GetWeaponId(pKeyValuesData->GetString("grenade1"));
	m_aGrenades.GetForModify(1) = GetWeaponId(pKeyValuesData->GetString("grenade2"));
	m_aGrenades.GetForModify(2) = GetWeaponId(pKeyValuesData->GetString("grenade3"));

	// Ammo Max.
	KeyValues *pAmmoKeyValuesData = pKeyValuesData->FindKey( "AmmoMax" );
	if ( pAmmoKeyValuesData )
	{
		for ( int iAmmo = 1; iAmmo < TF_AMMO_COUNT; ++iAmmo )
		{
			m_aAmmoMax.GetForModify(iAmmo) = pAmmoKeyValuesData->GetInt(g_aAmmoNames[iAmmo], 0);
		}
	}

	// Buildables
	for ( i=0;i<TF_PLAYER_BUILDABLE_COUNT;i++ )
	{
		Q_snprintf( buf, sizeof(buf), "buildable%d", i+1 );		
		m_aBuildable.GetForModify(i) = GetBuildableId(pKeyValuesData->GetString(buf));
	}

	// Temp animation flags
	m_bDontDoAirwalk = ( pKeyValuesData->GetInt( "DontDoAirwalk", 0 ) > 0 );
	m_bDontDoNewJump = ( pKeyValuesData->GetInt( "DontDoNewJump", 0 ) > 0 );
	
	// Open Fortress
	
	m_nViewVector = pKeyValuesData->GetInt( "ViewVector" );
	
	m_bSpecialClass = ( pKeyValuesData->GetInt( "SpecialClass", 0 ) > 0 );

#ifdef GAME_DLL		// right now we only emit these sounds from server. if that changes we can do this in both dlls

	// Death Sounds
	Q_strncpy( m_szDeathSound, pKeyValuesData->GetString( "sound_death", "Player.Death" ), MAX_PLAYERCLASS_SOUND_LENGTH );
	Q_strncpy( m_szCritDeathSound, pKeyValuesData->GetString( "sound_crit_death", "TFPlayer.CritDeath" ), MAX_PLAYERCLASS_SOUND_LENGTH );
	Q_strncpy( m_szMeleeDeathSound, pKeyValuesData->GetString( "sound_melee_death", "Player.MeleeDeath" ), MAX_PLAYERCLASS_SOUND_LENGTH );
	Q_strncpy( m_szExplosionDeathSound, pKeyValuesData->GetString( "sound_explosion_death", "Player.ExplosionDeath" ), MAX_PLAYERCLASS_SOUND_LENGTH );
#endif

	Q_strncpy( m_szClassSelectImageRed.GetForModify(), 		pKeyValuesData->GetString( "ClassSelectImageRed" ), 		TF_NAME_LENGTH );
	Q_strncpy( m_szClassSelectImageBlue.GetForModify(), 		pKeyValuesData->GetString( "ClassSelectImageBlue" ), 		TF_NAME_LENGTH );
	Q_strncpy( m_szClassSelectImageMercenary.GetForModify(), 	pKeyValuesData->GetString( "ClassSelectImageMercenary" ), 	TF_NAME_LENGTH );	

	Q_strncpy( m_szClassImageRed.GetForModify(), 		pKeyValuesData->GetString( "ClassImageRed" ), 		TF_NAME_LENGTH );
	Q_strncpy( m_szClassImageBlue.GetForModify(), 		pKeyValuesData->GetString( "ClassImageBlue" ), 		TF_NAME_LENGTH );
	Q_strncpy( m_szClassImageMercenary.GetForModify(), pKeyValuesData->GetString( "ClassImageMercenary" ), TF_NAME_LENGTH );
	Q_strncpy( m_szClassImageColorless.GetForModify(), pKeyValuesData->GetString( "ClassImageColorless" ), TF_NAME_LENGTH );
	
	Q_strncpy( m_szClassIcon.GetForModify(), pKeyValuesData->GetString( "ClassIcon", "../hud/leaderboard_class_tank" ), 	TF_NAME_LENGTH );
	
	// The file has been parsed.
	m_bParsed = true;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the player class data (keep a cache).
//-----------------------------------------------------------------------------
void InitPlayerClasses( void )
{
	// Special case the undefined class.
	TFPlayerClassData_t *pClassData = &s_aTFPlayerClassData[TF_CLASS_UNDEFINED];
	Assert( pClassData );
	Q_strncpy( pClassData->m_szClassName, "undefined", TF_NAME_LENGTH );
	Q_strncpy( pClassData->m_szModelName.GetForModify(), "models/error.mdl", TF_NAME_LENGTH );	// Undefined players still need a model
	Q_strncpy( pClassData->m_szArmModelName.GetForModify(), "models/error.mdl", TF_NAME_LENGTH );	// Undefined players still need a Arm model
	Q_strncpy( pClassData->m_szLocalizableName.GetForModify(), "undefined", TF_NAME_LENGTH );

	Q_strncpy( pClassData->m_szClassSelectImageRed.GetForModify(), "class_sel_sm_civilian_red", TF_NAME_LENGTH );	// Undefined players still need a class Image
	Q_strncpy( pClassData->m_szClassSelectImageBlue.GetForModify(), "class_sel_sm_civilian_blu", TF_NAME_LENGTH );
	Q_strncpy( pClassData->m_szClassSelectImageMercenary.GetForModify(), "class_sel_sm_civilian_mercenary", TF_NAME_LENGTH );
	
	// Initialize the classes.
	for ( int iClass = 1; iClass < TF_CLASS_COUNT_ALL; ++iClass )
	{
		TFPlayerClassData_t *pClassData = &s_aTFPlayerClassData[iClass];
		Assert( pClassData );
		pClassData->Parse( s_aPlayerClassFiles[iClass] );
	}
	
	const unsigned char *pKey = NULL;

	if ( g_pGameRules )
	{
		pKey = g_pGameRules->GetEncryptionKey();
	}
	
	for( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
	{	
		KeyValues *pKV = ReadEncryptedKVFile( filesystem, s_aPlayerClassFiles[i], pKey );

		if( !s_aPlayerClassFiles[i] )
		{
			continue;
		}
		s_aTFPlayerClassRaw[i] = pKV;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Helper function to get player class data.
//-----------------------------------------------------------------------------
TFPlayerClassData_t *GetPlayerClassData( int iClass )
{
	Assert ( ( iClass >= 0 ) && ( iClass < TF_CLASS_COUNT_ALL ) );
	return &s_aTFPlayerClassData[iClass];
}

//=============================================================================
//
// Shared player class data.
//

//=============================================================================
//
// Tables.
//

// Client specific.
#ifdef CLIENT_DLL

BEGIN_RECV_TABLE_NOBASE( CTFPlayerClassShared, DT_TFPlayerClassShared )
	RecvPropInt( RECVINFO( m_iClass ) ),
	RecvPropInt( RECVINFO( m_iModifiers ) ),
	RecvPropInt( RECVINFO( m_iOldClass ) ),
	RecvPropInt( RECVINFO( m_iOldModifiers ) ),
	RecvPropDataTable( RECVINFO_DT( pLocalData ), 0, &REFERENCE_RECV_TABLE( DT_PlayerClassData ) ),
	RecvPropBool( RECVINFO( m_bRefresh ) ),
	RecvPropString( RECVINFO( m_iszSetCustomModel ) ),
	RecvPropString( RECVINFO( m_iszSetCustomArmModel ) ),
END_RECV_TABLE()

// Server specific.
#else

BEGIN_SEND_TABLE_NOBASE( CTFPlayerClassShared, DT_TFPlayerClassShared )
	SendPropInt( SENDINFO( m_iClass ), Q_log2( TF_CLASS_COUNT_ALL )+1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iModifiers ), TF_CLASSMOD_LAST , SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iOldClass ), Q_log2( TF_CLASS_COUNT_ALL )+1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_iOldModifiers ), TF_CLASSMOD_LAST , SPROP_UNSIGNED ),
	SendPropDataTable( SENDINFO_DT( pLocalData ), &REFERENCE_SEND_TABLE( DT_PlayerClassData ) ),
	SendPropBool( SENDINFO( m_bRefresh ) ),
	SendPropStringT( SENDINFO( m_iszSetCustomModel ) ),
	SendPropStringT( SENDINFO( m_iszSetCustomArmModel ) ),
END_SEND_TABLE()
#endif


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFPlayerClassShared::CTFPlayerClassShared()
{
	m_iClass.Set( TF_CLASS_UNDEFINED );
	m_iOldClass.Set( TF_CLASS_UNDEFINED );
	m_iModifiers.Set( 0 );
	m_iOldModifiers.Set( 0 );
	m_bRefresh = false;
#ifdef CLIENT_DLL
	m_iszSetCustomModel[0] = '\0';
	bRefresh = false;
#else
	m_iszSetCustomModel.Set( NULL_STRING );
#endif
}

#ifndef CLIENT_DLL
void CTFPlayerClassShared::SetCustomModel( const char *pszModelName )
{
	if (pszModelName && pszModelName[0])
	{
		bool bPrecache = CBaseEntity::IsPrecacheAllowed();
		CBaseEntity::SetAllowPrecache( true );
		CBaseEntity::PrecacheModel( pszModelName );
		CBaseEntity::SetAllowPrecache( bPrecache );
		m_iszSetCustomModel.Set( AllocPooledString( pszModelName ) );
	}
	else
		m_iszSetCustomModel.Set( NULL_STRING );
}
void CTFPlayerClassShared::SetCustomArmModel( const char *pszModelName )
{
	if (pszModelName && pszModelName[0])
	{
		bool bPrecache = CBaseEntity::IsPrecacheAllowed();
		CBaseEntity::SetAllowPrecache( true );
		CBaseEntity::PrecacheModel( pszModelName );
		CBaseEntity::SetAllowPrecache( bPrecache );
		m_iszSetCustomArmModel.Set( AllocPooledString( pszModelName ) );
	}
	else
		m_iszSetCustomArmModel.Set( NULL_STRING );
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Initialize the player class.
//-----------------------------------------------------------------------------
bool CTFPlayerClassShared::Init( int iClass, int iModifiers )
{
	Assert ( ( iClass >= TF_FIRST_NORMAL_CLASS ) && ( iClass <= TF_LAST_NORMAL_CLASS ) );
	m_iOldClass = m_iClass;
	m_iClass = iClass;
	m_iOldModifiers = m_iModifiers;
	m_iModifiers = iModifiers;
#ifdef CLIENT_DLL
	m_iszSetCustomModel[0] = '\0';
#else
	m_iszSetCustomModel.Set( NULL_STRING );
#endif
	return true;
}

void CTFPlayerClassShared::SetModifier( int iModifiers )
{
	m_iOldModifiers = m_iModifiers;
	m_iModifiers = iModifiers;
}

// If needed, put this into playerclass scripts
bool CTFPlayerClassShared::CanBuildObject( int iObjectType )
{
	bool bFound = false;

	TFPlayerClassData_t  *pData = GetData();

	int i;
	for ( i=0;i<TF_PLAYER_BUILDABLE_COUNT;i++ )
	{
		if ( iObjectType == pData->m_aBuildable[i] )
		{
			bFound = true;
			break;
		}
	}

	return bFound;
}

char	*CTFPlayerClassShared::GetModelName( void )						
{ 
#ifdef CLIENT_DLL
	if ( m_iszSetCustomModel[0] ) return m_iszSetCustomModel;
#else
	static char tmp[ 256 ];
	Q_strncpy( tmp, STRING( m_iszSetCustomModel.Get() ), sizeof( tmp ) );
	if ( m_iszSetCustomModel.Get() != NULL_STRING ) return tmp;
#endif
	static char modelFilename[ 256 ];
	Q_strncpy( modelFilename, GetData()->GetModelName(), sizeof( modelFilename ) );
	return modelFilename;
}

char	*CTFPlayerClassShared::GetSetCustomModel( void )						
{ 
#ifdef CLIENT_DLL
	return m_iszSetCustomModel;
#else
	static char tmp[ 256 ];
	Q_strncpy( tmp, STRING(m_iszSetCustomModel.Get()), sizeof(tmp) );
	return tmp;
#endif
}

bool CTFPlayerClassShared::UsesCustomModel( void )
{
#ifdef CLIENT_DLL
	if ( m_iszSetCustomModel[0] ) return true;
#else
	if ( m_iszSetCustomModel.Get() != NULL_STRING ) return true;
#endif	
return false;
}

char	*CTFPlayerClassShared::GetArmModelName( void )						
{ 
#ifdef CLIENT_DLL
	if ( m_iszSetCustomArmModel[0] ) return m_iszSetCustomArmModel;
#else
	static char tmp[ 256 ];
	Q_strncpy( tmp, STRING( m_iszSetCustomArmModel.Get() ), sizeof( tmp ) );
	if (m_iszSetCustomArmModel.Get() != NULL_STRING) return tmp;
#endif
	static char modelFilename[ 256 ];
	Q_strncpy( modelFilename, GetData()->GetArmModelName(), sizeof( modelFilename ) );
	
	return modelFilename;
}

int CTFPlayerClassShared::MaxAirDashCount( void )
{
	if ( of_airdashcount.GetInt() > GetData()->m_nMaxAirDashCount )
		return of_airdashcount.GetInt();
	else
		return GetData()->m_nMaxAirDashCount; 
}

bool CTFPlayerClassShared::CanAirDash( void )
{
	return GetData()->m_nMaxAirDashCount > 0 || of_airdashcount.GetInt() > 0; 
}

char	*CTFPlayerClassShared::GetSetCustomArmModel( void )						
{ 
#ifdef CLIENT_DLL
	return m_iszSetCustomArmModel;
#else
	static char tmp[ 256 ];
	Q_strncpy( tmp, STRING( m_iszSetCustomArmModel.Get() ), sizeof( tmp ) );
	return tmp;
#endif
}

bool CTFPlayerClassShared::UsesCustomArmModel( void )
{
#ifdef CLIENT_DLL
	if ( m_iszSetCustomArmModel[0] ) return true;
#else
	if ( m_iszSetCustomArmModel.Get() != NULL_STRING ) return true;
#endif	
return false;
}

TFPlayerClassData_t *CTFPlayerClassShared::GetData( void )
{ 
	if ( ( GetClass() <= 0 ) && ( GetClass() >= TF_CLASS_COUNT_ALL ) )
		return GetPlayerClassData( m_iClass );
	if( GetModifiers() > 0 )
	{
		TFPlayerClassData_t *pLocalPointer = &pLocalData;
		if( !pLocalPointer || GetModifiers() != GetOldModifiers() || GetClass() != GetOldClass() )
		{
			// Special case the undefined class.
			TFPlayerClassData_t *pClassData = &s_aTFPlayerClassData[TF_CLASS_UNDEFINED];
			Assert( pClassData );
			
			KeyValues* kvFinal = s_aTFPlayerClassRaw[GetClass()];
			
			if( !kvFinal )
			{
				DevWarning("Cant get class keyvalue for class %s\n", g_aPlayerClassNames_NonLocalized[GetClass()]);
				return &s_aTFPlayerClassData[GetClass()];
			}

			if( ( GetModifiers() & (1<<TF_CLASSMOD_TFC) ) != 0 )
			{
				KeyValues* kvTFC = kvFinal->FindKey("TFC");
				if( kvTFC )
				{
					KeyValues* pValue = kvTFC->GetFirstValue();
					for( pValue; pValue != NULL; pValue = pValue->GetNextValue() ) // Loop through all the keyvalues
					{
						DevWarning("%s %s\n", pValue->GetName(), pValue->GetString() );
						kvFinal->SetString( pValue->GetName(), pValue->GetString() );
					}
				}
			}
			
			if( ( GetModifiers() & (1<<TF_CLASSMOD_ZOMBIE) ) != 0 )
			{
				KeyValues* kvZombie = kvFinal->FindKey( "Zombie" );
				if( kvZombie )
				{
					KeyValues* pValue = kvZombie->GetFirstValue();
					for( pValue; pValue != NULL; pValue = pValue->GetNextValue() ) // Loop through all the keyvalues
					{
						DevWarning("%s %s\n", pValue->GetName(), pValue->GetString() );
						kvFinal->SetString( pValue->GetName(), pValue->GetString() );
					}
				}
			}
			pClassData->ParseData(kvFinal);
			const TFPlayerClassData_t *pTempData = pClassData;
			pLocalData.CopyFrom(*pTempData);
			m_bRefresh = !m_bRefresh;
			SetOldModifiers( GetModifiers() );
			SetOldClass( GetClass() );
			return &pLocalData;
		}
		else
		{
			return &pLocalData;
		}
	}
	return GetPlayerClassData( m_iClass );
}

int	CTFPlayerClassShared::GetCapNumber( void )
{ return GetData()->m_nCapNumber; }

bool CTFPlayerClassShared::IsSpecialClass( void )
{ return GetData()->m_bSpecialClass; }

int	CTFPlayerClassShared::GetMaxArmor( void )
{ return GetData()->m_nMaxArmor; }

int	CTFPlayerClassShared::GetMaxHealth( void )
{ return GetData()->m_nMaxHealth; }

float CTFPlayerClassShared::GetMaxSpeed( void )
{ return GetData()->m_flMaxSpeed; }

char	*CTFPlayerClassShared::GetName( void )
{ return GetData()->m_szClassName; }

const char *CTFPlayerClassShared::GetJumpSound( void )
{ return GetData()->GetJumpSound(); }