//====== Copyright ? 1996-2003, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "tf_weapon_parse.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------

FileWeaponInfo_t *CreateWeaponInfo()
{
	return new CTFWeaponInfo;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponInfo::CTFWeaponInfo()
{
	m_WeaponData[0].Init();
	m_WeaponData[1].Init();

	m_bGrenade = false;
	m_flDamageRadius = -1.0f;
	m_flPrimerTime = 0.0f;
	m_bSuppressGrenTimer = false;
	m_bLowerWeapon = false;

	m_bHasTeamSkins_Viewmodel = false;
	m_bHasTeamSkins_Worldmodel = false;
	
	m_bHolyShit = false;
	
	m_bUsesCritAnimation = false;
	
	m_bNeverStrip = false;
	
	m_bBuyable = false;
	
	m_bLoadsManualy = false;
	m_bNoSniperCharge = false;
	m_bAlwaysDrop = false;
	m_bCanSoftZoom = true;
	
	m_flDespawnTime = 30.0f;
	
	szScoutViewModel[0] = 0;
	szSoldierViewModel[0] = 0;
	szPyroViewModel[0] = 0;
	szDemomanViewModel[0] = 0;
	szHeavyViewModel[0] = 0;
	szEngineerViewModel[0] = 0;
	szMedicViewModel[0] = 0;
	szSniperViewModel[0] = 0;
	szSpyViewModel[0] = 0;
	szMercenaryViewModel[0] = 0;
	szCivilianViewModel[0] = 0;
	szJuggernautViewModel[0] = 0;
	m_szBombletModel[0] = 0;
	m_szBombletTrailParticle[0] = 0;
	
	m_nProjectileModel[0] = 0;

	m_szMuzzleFlashModel[0] = '\0';
	m_flMuzzleFlashModelDuration = 0;
	m_szMuzzleFlashParticleEffect[0] = '\0';
	
	m_bTeamColorMuzzleFlash = false;
	
	m_szMuzzleFlashParticleEffectRed[0] = '\0';
	m_szMuzzleFlashParticleEffectBlue[0] = '\0';
	m_szMuzzleFlashParticleEffectDM[0] = '\0';
	
	m_szTracerEffect[0] = '\0';

	m_szBrassModel[0] = '\0';
	m_bDoInstantEjectBrass = true;

	m_szExplosionSound[0] = '\0';
	m_szExplosionEffect[0] = '\0';
	m_szExplosionPlayerEffect[0] = '\0';
	m_szExplosionWaterEffect[0] = '\0';
	
	m_szExplosionEffectBomblets[0] = '\0';
	m_szExplosionPlayerEffectBomblets[0] = '\0';
	m_szExplosionWaterEffectBomblets[0] = '\0';
	
	m_bTeamExplosion = false;

	m_iWeaponType = TF_WPN_TYPE_PRIMARY;
	m_iCost = 0;
	
	m_flWindupTime = 0.0f;
	m_flChargeDuration = 0.0f;
	m_flChargeRechargeRate = 0.0f;
	m_flCritOnChargeLevel = -1.0f;
	
	m_flFuseTime = -1.0f;
	m_flImpactBeforeTime = 0.0f;
	
	m_iContinuousFireDamageIncrease = 0.0f;
	m_flContinuousFireBlastRadiusIncrease = 0.0f;

	m_flMinViewmodelOffsetX = 0.0f;
	m_flMinViewmodelOffsetY = 0.0f;
	m_flMinViewmodelOffsetZ = 0.0f;

	m_flCenteredViewmodelOffsetX = 0.0f;
	m_flCenteredViewmodelOffsetY = 0.0f;
	m_flCenteredViewmodelOffsetZ = 0.0f;

	m_flCenteredViewmodelAngleX = 0.0f;
	m_flCenteredViewmodelAngleY = 0.0f;
	m_flCenteredViewmodelAngleZ = 0.0f;
	
	for ( int i = TF_CLASS_SCOUT; i < TF_CLASS_COUNT_ALL; i++ )
	{
		m_iClassSlot[i]	= -1;
	}
	
}

CTFWeaponInfo::~CTFWeaponInfo()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	int i;
	m_bDropsMag = pKeyValuesData->GetBool("DropsMag");
	Q_strncpy(m_szMagModel, pKeyValuesData->GetString("MagModel"), sizeof(m_szMagModel));
	m_iMagBodygroup = pKeyValuesData->GetInt("magazine");
	
	
	Q_strncpy( m_nProjectileModel, pKeyValuesData->GetString( "ProjectileModel" ), MAX_WEAPON_STRING );
	// Primary fire mode.
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nDamage				= pKeyValuesData->GetInt( "Damage", 0 );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nInstagibDamage		= pKeyValuesData->GetInt( "InstagibDamage", 0 );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flRange				= pKeyValuesData->GetFloat( "Range", 8192.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nBulletsPerShot		= pKeyValuesData->GetInt( "BulletsPerShot", 0 );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flSpread				= pKeyValuesData->GetFloat( "Spread", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flPunchAngle			= pKeyValuesData->GetFloat( "PunchAngle", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeFireDelay		= pKeyValuesData->GetFloat( "TimeFireDelay", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeIdle			= pKeyValuesData->GetFloat( "TimeIdle", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeIdleEmpty		= pKeyValuesData->GetFloat( "TimeIdleEmpy", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReloadStart	= pKeyValuesData->GetFloat( "TimeReloadStart", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload			= pKeyValuesData->GetFloat( "TimeReload", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bDrawCrosshair		= pKeyValuesData->GetInt( "DrawCrosshair", 1 ) > 0;
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iAmmoPerShot			= pKeyValuesData->GetInt( "AmmoPerShot", 1 );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bUseRapidFireCrits	= ( pKeyValuesData->GetInt( "UseRapidFireCrits", 0 ) != 0 );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bCenterfireProjectile= (pKeyValuesData->GetInt( "CenterfireProjectile", 0 ) != 0 );
	
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flBurstFireDelay	= pKeyValuesData->GetFloat( "BurstFireDelay", 0.0f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nBurstSize	= pKeyValuesData->GetInt( "BurstSize", 0 );
	
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iProjectile = TF_PROJECTILE_NONE;
	const char *pszProjectileType = pKeyValuesData->GetString( "ProjectileType", "projectile_none" );

	for ( i=0;i<TF_NUM_PROJECTILES;i++ )
	{
		if ( FStrEq( pszProjectileType, g_szProjectileNames[i] ) )
		{
			m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iProjectile = i;
			break;
		}
	}	 

	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flProjectileSpeed	= pKeyValuesData->GetFloat( "ProjectileSpeed", 0.0f );

	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flSmackDelay			= pKeyValuesData->GetFloat( "SmackDelay", 0.2f );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flSmackDelay		= pKeyValuesData->GetFloat( "Secondary_SmackDelay", 0.2f );
	m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flMeleeRange			= pKeyValuesData->GetFloat( "MeleeRange", 48 );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flMeleeRange		= pKeyValuesData->GetFloat( "Secondary_MeleeRange", 48 );	

	m_bDoInstantEjectBrass = ( pKeyValuesData->GetInt( "DoInstantEjectBrass", 1 ) != 0 );
	const char *pszBrassModel = pKeyValuesData->GetString( "BrassModel", NULL );
	if ( pszBrassModel )
	{
		Q_strncpy( m_szBrassModel, pszBrassModel, sizeof( m_szBrassModel ) );
	}

	// Secondary fire mode.
	// Inherit from primary fire mode
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_nDamage			= pKeyValuesData->GetInt( "Secondary_Damage", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nDamage );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flRange			= pKeyValuesData->GetFloat( "Secondary_Range", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flRange );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_nBulletsPerShot	= pKeyValuesData->GetInt( "Secondary_BulletsPerShot", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nBulletsPerShot );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flSpread			= pKeyValuesData->GetFloat( "Secondary_Spread", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flSpread );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flPunchAngle		= pKeyValuesData->GetFloat( "Secondary_PunchAngle", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flPunchAngle );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeFireDelay	= pKeyValuesData->GetFloat( "Secondary_TimeFireDelay", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeFireDelay );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeIdle			= pKeyValuesData->GetFloat( "Secondary_TimeIdle", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeIdle );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeIdleEmpty	= pKeyValuesData->GetFloat( "Secondary_TimeIdleEmpy", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeIdleEmpty );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeReloadStart	= pKeyValuesData->GetFloat( "Secondary_TimeReloadStart", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReloadStart );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flTimeReload		= pKeyValuesData->GetFloat( "Secondary_TimeReload", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeReload );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_bDrawCrosshair		= pKeyValuesData->GetInt( "Secondary_DrawCrosshair", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_bDrawCrosshair ) > 0;
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_iAmmoPerShot		= pKeyValuesData->GetInt( "Secondary_AmmoPerShot", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iAmmoPerShot );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_bUseRapidFireCrits	= ( pKeyValuesData->GetInt( "Secondary_UseRapidFireCrits", 0 ) != 0 );
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_bCenterfireProjectile= ( pKeyValuesData->GetInt( "Secondary_CenterfireProjectile", 0 ) != 0 );

	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_flBurstFireDelay	= pKeyValuesData->GetFloat( "BurstFireDelay", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flBurstFireDelay );	
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_nBurstSize	= pKeyValuesData->GetFloat( "BurstSize", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nBurstSize );
	
	m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_iProjectile = m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_iProjectile;
	pszProjectileType = pKeyValuesData->GetString( "Secondary_ProjectileType", "projectile_none" );

	for ( i=0;i<TF_NUM_PROJECTILES;i++ )
	{
		if ( FStrEq( pszProjectileType, g_szProjectileNames[i] ) )
		{
			m_WeaponData[TF_WEAPON_SECONDARY_MODE].m_iProjectile = i;
			break;
		}
	}	

	const char *pszWeaponType = pKeyValuesData->GetString( "WeaponType" );

	if ( !Q_strcmp( pszWeaponType, "primary" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_PRIMARY;
	}
	else if ( !Q_strcmp( pszWeaponType, "secondary" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_SECONDARY;
	}
	else if ( !Q_strcmp( pszWeaponType, "melee" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_MELEE;
	}
	else if ( !Q_strcmp( pszWeaponType, "grenade" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_GRENADE;
	}
	else if ( !Q_strcmp( pszWeaponType, "building" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_BUILDING;
	}
	else if ( !Q_strcmp( pszWeaponType, "pda" ) )
	{
		m_iWeaponType = TF_WPN_TYPE_PDA;
	}
	
	for( int i = TF_CLASS_SCOUT; i < TF_CLASS_COUNT_ALL; i++ )
	{
		char pKeyValueName[128];
		Q_snprintf( pKeyValueName, sizeof( pKeyValueName ), "%s_WeaponType", g_aPlayerClassNames_NonLocalized[i] );
		pszWeaponType = pKeyValuesData->GetString( pKeyValueName, "error" );

		if( !Q_strcmp( pszWeaponType, "error" ) )
		{
			m_iClassWeaponType[i] = -1;
			continue;
		}
		
		if ( !Q_strcmp( pszWeaponType, "primary" ) )
		{
			m_iClassWeaponType[i] = TF_WPN_TYPE_PRIMARY;
		}
		else if ( !Q_strcmp( pszWeaponType, "secondary" ) )
		{
			m_iClassWeaponType[i] = TF_WPN_TYPE_SECONDARY;
		}
		else if ( !Q_strcmp( pszWeaponType, "melee" ) )
		{
			m_iClassWeaponType[i] = TF_WPN_TYPE_MELEE;
		}
		else if ( !Q_strcmp( pszWeaponType, "grenade" ) )
		{
			m_iClassWeaponType[i] = TF_WPN_TYPE_GRENADE;
		}
		else if ( !Q_strcmp( pszWeaponType, "building" ) )
		{
			m_iClassWeaponType[i] = TF_WPN_TYPE_BUILDING;
		}
		else if ( !Q_strcmp( pszWeaponType, "pda" ) )
		{
			m_iClassWeaponType[i] = TF_WPN_TYPE_PDA;
		}
	}
	
	m_nBlastJumpDamageForce	= pKeyValuesData->GetInt( "BlastJumpDamageForce", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_nDamage );
	m_flLastShotDelay = pKeyValuesData->GetFloat( "LastShotTimeFireDelay", m_WeaponData[TF_WEAPON_PRIMARY_MODE].m_flTimeFireDelay );
	m_bLastShotAnim = pKeyValuesData->GetBool( "LastShotAnim", 0 );
	m_bSwapFireAnims = pKeyValuesData->GetBool( "SwapFireAnims", 0 );
	m_bNoFixedSpread = pKeyValuesData->GetBool( "NoFixedSpread", 0 );
	
	m_iContinuousFireDamageIncrease = pKeyValuesData->GetInt( "ContinuousFire_DamageIncrease", 0 );
	m_flContinuousFireBlastRadiusIncrease = pKeyValuesData->GetFloat( "ContinuousFire_BlastRadiusIncrease", 0.0f );
	
	// Grenade data.
	m_bGrenade				= ( pKeyValuesData->GetInt( "Grenade", 0 ) != 0 );
	m_flDamageRadius		= pKeyValuesData->GetFloat( "DamageRadius", -1.0f );
	m_flPrimerTime			= pKeyValuesData->GetFloat( "PrimerTime", 0.0f );
	m_bSuppressGrenTimer	= ( pKeyValuesData->GetInt( "PlayGrenTimer", 1 ) <= 0 );
	
	m_flFuseTime			= pKeyValuesData->GetFloat( "FuseTime", -1.0f );
	m_bExplodeOnImpact		= ( pKeyValuesData->GetInt( "ExplodeOnImpact", 0 ) != 0 );
	m_flImpactBeforeTime	= pKeyValuesData->GetFloat( "ImpactBeforeTime", 0.0f );
	m_bAlwaysEnableTouch	= ( pKeyValuesData->GetInt( "AlwaysEnableTouch", 0 ) != 0 );
	
	m_iBombletAmount				= pKeyValuesData->GetInt( "BombletAmount", 0.0f );
	m_flBombletTimer				= pKeyValuesData->GetFloat( "BombletTimer", 0.0f );
	m_flBombletDamage				= pKeyValuesData->GetFloat( "BombletDamage", 0.0f );
	m_flBombletDamageRadius			= pKeyValuesData->GetFloat( "BombletDamageRadius", 0.0f );
	m_bBombletImpact				= pKeyValuesData->GetBool( "BombletImpact", 0.0f );
	m_bBombletEffectTeamColored		= pKeyValuesData->GetBool( "BombletEffectTeamColored", 0.0f );
	
	Q_strncpy( m_szBombletModel, pKeyValuesData->GetString( "BombletModel" ), MAX_WEAPON_STRING );
	Q_strncpy( m_szBombletTrailParticle, pKeyValuesData->GetString( "BombletParticleTrail" ), MAX_WEAPON_STRING );
	
	m_iCost		= pKeyValuesData->GetInt( "Cost", -1 );
	
	m_bLowerWeapon			= ( pKeyValuesData->GetInt( "LowerMainWeapon", 0 ) != 0 );
	m_bHasTeamSkins_Viewmodel	= ( pKeyValuesData->GetInt( "HasTeamSkins_Viewmodel", 0 ) != 0 );
	m_bHasTeamSkins_Worldmodel	= ( pKeyValuesData->GetInt( "HasTeamSkins_Worldmodel", 0 ) != 0 );
	m_bHolyShit = ( pKeyValuesData->GetInt( "HolyShit", 0 ) != 0 );
	m_bUsesCritAnimation	= ( pKeyValuesData->GetInt( "UsesCritAnimation", 0 ) != 0 );
	m_bNeverStrip	= ( pKeyValuesData->GetInt( "NeverStrip", 0 ) != 0 );
	m_bGibOnOverkill	= ( pKeyValuesData->GetInt( "GibOnOverkill", 0 ) != 0 );
	m_bGibOnHeadshot	= ( pKeyValuesData->GetInt( "GibOnHeadshot", 0 ) != 0 );
	m_bDisableSecondaryAttack	= ( pKeyValuesData->GetInt( "DisableSecondaryAttack", 0 ) != 0 );
	m_bAllowDrop	= ( pKeyValuesData->GetInt( "AllowDrop", 0 ) != 0 );
	m_bDropBomblets	= ( pKeyValuesData->GetInt( "DropBomblets", 0 ) != 0 );
	m_flPickupMultiplier		= pKeyValuesData->GetFloat( "PickupMultiplier", 1.0f );
	m_bDropOnNoAmmo	= ( pKeyValuesData->GetInt( "DropOnNoAmmo", 0 ) != 0 );
	m_bBuyable	= ( pKeyValuesData->GetInt( "Buyable", 0 ) != 0 );
	m_bLoadsManualy	= ( pKeyValuesData->GetInt( "LoadsManualy", 0 ) != 0 );
	m_bNoSniperCharge = ( pKeyValuesData->GetInt( "NoSniperCharge", 0 ) != 0 );
	m_bAlwaysDrop = ( pKeyValuesData->GetInt( "AlwaysDrop", 0 ) != 0 );
	m_bCanSoftZoom = ( pKeyValuesData->GetInt( "CanSoftZoom", 1 ) != 0 );
	
	m_flDespawnTime	= pKeyValuesData->GetFloat( "DespawnTime", 30.0f );

	m_bCanShieldCharge = ( pKeyValuesData->GetInt( "CanShieldCharge", 0 ) != 0 );
	
	m_flChargeDuration = pKeyValuesData->GetFloat( "ChargeDuration", 0.0f );
	m_flChargeRechargeRate = pKeyValuesData->GetFloat( "ChargeRechargeRate", 0.0f );
	m_flCritOnChargeLevel = pKeyValuesData->GetFloat( "CritOnChargeLevel", -1.0f );
	m_flWindupTime = pKeyValuesData->GetFloat( "WindupTime", 0.0f );
	
	m_flCenteredViewmodelOffsetX = pKeyValuesData->GetFloat( "CenteredViewmodelOffset_X", 0.0f );
	m_flCenteredViewmodelOffsetY = pKeyValuesData->GetFloat( "CenteredViewmodelOffset_Y", 0.0f );
	m_flCenteredViewmodelOffsetZ = pKeyValuesData->GetFloat( "CenteredViewmodelOffset_Z", 0.0f );

	m_flMinViewmodelOffsetX = pKeyValuesData->GetFloat( "MinViewmodelOffset_X", 0.0f );
	m_flMinViewmodelOffsetY = pKeyValuesData->GetFloat( "MinViewmodelOffset_Y", 0.0f );
	m_flMinViewmodelOffsetZ = pKeyValuesData->GetFloat( "MinViewmodelOffset_Z", 0.0f );

	m_flCenteredViewmodelAngleX = pKeyValuesData->GetFloat("CenteredViewmodelAngle_X", 0.0f );
	m_flCenteredViewmodelAngleY = pKeyValuesData->GetFloat("CenteredViewmodelAngle_Y", 0.0f );
	m_flCenteredViewmodelAngleZ = pKeyValuesData->GetFloat("CenteredViewmodelAngle_Z", 0.0f );	
	
	Q_strncpy( szScoutViewModel, pKeyValuesData->GetString( "scout_viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szSoldierViewModel, pKeyValuesData->GetString( "soldier_viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szPyroViewModel, pKeyValuesData->GetString( "pyro_viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szDemomanViewModel, pKeyValuesData->GetString( "demoman_viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szHeavyViewModel, pKeyValuesData->GetString( "heavy_viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szEngineerViewModel, pKeyValuesData->GetString( "engineer_viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szMedicViewModel, pKeyValuesData->GetString( "medic_viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szSniperViewModel, pKeyValuesData->GetString( "sniper_viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szSpyViewModel, pKeyValuesData->GetString( "spy_viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szMercenaryViewModel, pKeyValuesData->GetString( "mercenary_viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szCivilianViewModel, pKeyValuesData->GetString( "civilian_viewmodel" ), MAX_WEAPON_STRING );
	Q_strncpy( szJuggernautViewModel, pKeyValuesData->GetString( "juggernaut_viewmodel" ), MAX_WEAPON_STRING );
	
	for ( int i = TF_CLASS_SCOUT; i < TF_CLASS_COUNT_ALL; i++ )
	{
		char pKeyValueName[128];
		Q_snprintf( pKeyValueName, sizeof( pKeyValueName ), "%s_slot", g_aPlayerClassNames_NonLocalized[i] );
		m_iClassSlot[i]	= pKeyValuesData->GetInt( pKeyValueName, -1 );
	}
	// Model muzzleflash
	const char *pszMuzzleFlashModel = pKeyValuesData->GetString( "MuzzleFlashModel", NULL );

	if ( pszMuzzleFlashModel )
	{
		Q_strncpy( m_szMuzzleFlashModel, pszMuzzleFlashModel, sizeof( m_szMuzzleFlashModel ) );
	}

	m_flMuzzleFlashModelDuration = pKeyValuesData->GetFloat( "MuzzleFlashModelDuration", 0.2 );

	const char *pszMuzzleFlashParticleEffect = pKeyValuesData->GetString( "MuzzleFlashParticleEffect", NULL );

	m_bTeamColorMuzzleFlash = pKeyValuesData->GetBool("TeamColorMuzzleFlash");
	
	if ( pszMuzzleFlashParticleEffect )
	{
		Q_strncpy( m_szMuzzleFlashParticleEffect, pszMuzzleFlashParticleEffect, sizeof( m_szMuzzleFlashParticleEffect ) );
		if( m_bTeamColorMuzzleFlash )
		{
			Q_snprintf( m_szMuzzleFlashParticleEffectRed, sizeof(m_szMuzzleFlashParticleEffectRed), "%s_red", pKeyValuesData->GetString( "MuzzleFlashParticleEffect", NULL ));
			Q_snprintf( m_szMuzzleFlashParticleEffectBlue, sizeof(m_szMuzzleFlashParticleEffectBlue), "%s_blue", pKeyValuesData->GetString( "MuzzleFlashParticleEffect", NULL ));
			Q_snprintf( m_szMuzzleFlashParticleEffectDM, sizeof(m_szMuzzleFlashParticleEffectDM), "%s_dm", pKeyValuesData->GetString( "MuzzleFlashParticleEffect", NULL ));
		}
	}

	// Tracer particle effect
	const char *pszTracerEffect = pKeyValuesData->GetString( "TracerEffect", NULL );

	if ( pszTracerEffect )
	{
		Q_strncpy( m_szTracerEffect, pszTracerEffect, sizeof( m_szTracerEffect ) );
	}


	// Explosion effects (used for grenades)
	const char *pszSound = pKeyValuesData->GetString( "ExplosionSound", NULL );
	if ( pszSound )
	{
		Q_strncpy( m_szExplosionSound, pszSound, sizeof( m_szExplosionSound ) );
	}

	const char *pszEffect = pKeyValuesData->GetString( "ExplosionEffect", NULL );
	if ( pszEffect )
		Q_strncpy( m_szExplosionEffect, pszEffect, sizeof( m_szExplosionEffect ) );

	pszEffect = pKeyValuesData->GetString( "ExplosionPlayerEffect", NULL );
	if ( pszEffect )
		Q_strncpy( m_szExplosionPlayerEffect, pszEffect, sizeof( m_szExplosionPlayerEffect ) );

	pszEffect = pKeyValuesData->GetString( "ExplosionWaterEffect", NULL );
	if ( pszEffect )
		Q_strncpy( m_szExplosionWaterEffect, pszEffect, sizeof( m_szExplosionWaterEffect ) );
	
	// Bomblets
	
	pszEffect = pKeyValuesData->GetString( "ExplosionEffectBomblets", NULL );
	if ( pszEffect )
		Q_strncpy( m_szExplosionEffectBomblets, pszEffect, sizeof( m_szExplosionEffectBomblets ) );

	pszEffect = pKeyValuesData->GetString( "ExplosionPlayerEffectBomblets", NULL );
	if ( pszEffect )
		Q_strncpy( m_szExplosionPlayerEffectBomblets, pszEffect, sizeof( m_szExplosionPlayerEffectBomblets ) );

	pszEffect = pKeyValuesData->GetString( "ExplosionWaterEffectBomblets", NULL );
	if ( pszEffect )
		Q_strncpy( m_szExplosionWaterEffectBomblets, pszEffect, sizeof( m_szExplosionWaterEffectBomblets ) );

	m_bTeamExplosion = ( pKeyValuesData->GetInt( "TeamExplosion", 0 ) != 0 );
	
	m_bDontDrop = ( pKeyValuesData->GetInt( "DontDrop", 0 ) > 0 );
}