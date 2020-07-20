//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"

#ifdef CLIENT_DLL
	#include "c_team.h"
	#include "filesystem.h"
#else
	#include "team.h"
#endif

extern ConVar of_infiniteammo;
ConVar sv_unlockedchapters( "sv_unlockedchapters", "99" );

#ifdef CLIENT_DLL
const char *GetRPCMapImage( char m_szLatchedMapname[MAX_MAP_NAME], const char *pMapIcon )
{
	KeyValues* pDiscordRPC = new KeyValues( "Discord" );
	pDiscordRPC->LoadFromFile( filesystem, "scripts/discord_rpc.txt" );
	if ( pDiscordRPC )
	{				
		KeyValues *pMaps = pDiscordRPC->FindKey( "Maps" );
		if( pMaps )
		{
				return pMaps->GetString( m_szLatchedMapname, pMapIcon );
		}
		pMaps->deleteThis();
		pDiscordRPC->deleteThis();
	}
	return "missing";
}
#endif

bool IsGameTeam( int iTeam )
{
	return ( iTeam > LAST_SHARED_TEAM && iTeam < TF_TEAM_COUNT ); 
}

bool IsTeamName( const char *str )
{
	for (int i = 0; i < g_Teams.Size(); ++i)
	{
#if defined( CLIENT_DLL )
		if (FStrEq( str, g_Teams[i]->Get_Name() ))
			return true;
#else
		if (FStrEq( str, g_Teams[i]->GetName() ))
			return true;
#endif
	}

	return Q_strcasecmp( str, "spectate" ) == 0;
}

//-----------------------------------------------------------------------------
// Teams.
//-----------------------------------------------------------------------------
const char *g_aTeamNames[TF_TEAM_COUNT] =
{
	"Unassigned",
	"Spectator",
	"Red",
	"Blue",
	"Mercenary",
	"NPC" //add team
};

color32 g_aTeamColors[TF_TEAM_COUNT] = 
{
	{ 0, 0, 0, 0 },
	{ 0, 0, 0, 0 },
	{ 255, 0, 0, 0 },
	{ 0, 0, 255, 0 }
};

//-----------------------------------------------------------------------------
// Classes.
//-----------------------------------------------------------------------------

const char *g_aPlayerClassNames[] =
{
	"#TF_Class_Name_Undefined",
	"#TF_Class_Name_Scout",
	"#TF_Class_Name_Sniper",
	"#TF_Class_Name_Soldier",
	"#TF_Class_Name_Demoman",
	"#TF_Class_Name_Medic",
	"#TF_Class_Name_HWGuy",
	"#TF_Class_Name_Pyro",
	"#TF_Class_Name_Spy",
	"#TF_Class_Name_Engineer",
	"#TF_Class_Name_Mercenary",
	"#TF_Class_Name_Civilian",
	"#TF_Class_Name_Juggernaut"
};

const char *g_aPlayerClassNames_NonLocalized[] =
{
	"Undefined",
	"Scout",
	"Sniper",
	"Soldier",
	"Demoman",
	"Medic",
	"Heavy",
	"Pyro",
	"Spy",
	"Engineer",
	"Mercenary",
	"Civilian",
	"Juggernaut",
};

const char *g_aLoadoutConvarNames[] =
{
	"_undefined_cosmetic_loadout",
	"_scout_cosmetic_loadout",
	"_sniper_cosmetic_loadout",
	"_soldier_cosmetic_loadout",
	"_demoman_cosmetic_loadout",
	"_medic_cosmetic_loadout",
	"_heavy_cosmetic_loadout",
	"_pyro_cosmetic_loadout",
	"_spy_cosmetic_loadout",
	"_engineer_cosmetic_loadout",
	"_mercenary_cosmetic_loadout",
	"_civilian_cosmetic_loadout",
	"_juggernaut_cosmetic_loadout",
};

const char *g_aArsenalConvarNames[] =
{
	"_undefined_weapon_loadout",
	"_scout_weapon_loadout",
	"_sniper_weapon_loadout",
	"_soldier_weapon_loadout",
	"_demoman_weapon_loadout",
	"_medic_weapon_loadout",
	"_heavy_weapon_loadout",
	"_pyro_weapon_loadout",
	"_spy_weapon_loadout",
	"_engineer_weapon_loadout",
	"_mercenary_weapon_loadout",
	"_civilian_weapon_loadout",
	"_juggernaut_weapon_loadout",
};

const char *g_aPlayerMutatorNames[] =
{
	"None",
	"TFC",
};

const char *g_aLoadoutCategories[] =
{
	"cosmetics",
	"weapons",
};

bool IsPlayerClassName( char const *str )
{
	for ( int i = 1; i < TF_CLASS_COUNT_ALL; ++i )
	{
		TFPlayerClassData_t *data = GetPlayerClassData( i );

		if ( FStrEq( str, data->m_szClassName ) )
		{
			return true;
		}
	}

	return false;
}

int GetClassIndexFromString( char const *name, int maxClass )
{
	for ( int i = TF_FIRST_NORMAL_CLASS; i <= maxClass; ++i )
	{
		size_t length = strlen( g_aPlayerClassNames_NonLocalized[i] );

		if ( length <= strlen( name ) && !Q_strnicmp( g_aPlayerClassNames_NonLocalized[i], name, length ) )
		{
			return i;
		}
	}

	return TF_CLASS_UNDEFINED;
}


//-----------------------------------------------------------------------------
// Gametypes.
//-----------------------------------------------------------------------------
const char *g_aGameTypeNames[] =
{
	"Undefined",
	"#Gametype_CTF",
	"#Gametype_CP",
	"#Gametype_PAYLOAD",
	"#Gametype_ARENA",
	"#Gametype_MVM",
	"#Gametype_RD",
	"#Gametype_PASSTIME",
	"#Gametype_PD",
	"#Gametype_ESC",
	"#Gametype_DM",
	"#Gametype_TDM",
	"#Gametype_DOM",
	"#Gametype_GG",
	"#Gametype_3WAVE",
	"#Gametype_ZS",
	"#Gametype_COOP",
	"#Gametype_INF",
	"#Gametype_JUG"
};

//-----------------------------------------------------------------------------
// Ammo.
//-----------------------------------------------------------------------------
const char *g_aAmmoNames[] =
{
	"DUMMY AMMO",

	"TF_AMMO_PRIMARY",
	"TF_AMMO_SECONDARY",
	"TF_AMMO_METAL",
	"TF_AMMO_GRENADES1",
	"TF_AMMO_GRENADES2",
	"TF_AMMO_GRENADES3",

	"WEAPON_AMMO",
};

const Vector g_vecFixedPattern[] =
{
	Vector( 0, 0, 0 ),
	Vector( 1, 0, 0 ),
	Vector( -1, 0, 0 ),
	
	Vector( 0, -1, 0 ),
	Vector( 0, 1, 0 ),
	Vector( 0.85, -0.85, 0 ),
	
	Vector( 0.85, 0.85, 0 ),
	Vector( -0.85, -0.85, 0 ),
	Vector( -0.85, 0.85, 0 ),
	
	Vector( 0, 0, 0 ),
	
	Vector( 2, 0, 0 ),
	Vector( -2, 0, 0 ),
	Vector( 0, -2, 0 ),
	
	Vector( 0, 2, 0 ),
	Vector( 0.42, -0.42, 0 ),
	Vector( 0.42, 0.42, 0 ),

	Vector( -0.42, -0.42, 0 ),
	Vector( -0.42, 0.42, 0 ),
};

//-----------------------------------------------------------------------------
// Weapons.
//-----------------------------------------------------------------------------
const char *g_aWeaponNames[] =
{
	"TF_WEAPON_NONE",

	"TF_WEAPON_BAT",
	"TF_WEAPON_BOTTLE", 
	"TF_WEAPON_FIREAXE",
	"TF_WEAPON_CLUB",
	"TF_WEAPON_CROWBAR",
	"TF_WEAPON_KNIFE",
	"TF_WEAPON_FISTS",
	"TF_WEAPON_SHOVEL",
	"TF_WEAPON_WRENCH",
	"TF_WEAPON_BONESAW",
	"TF_WEAPON_SHOTGUN",
	"TF_WEAPON_SCATTERGUN",
	"TF_WEAPON_SNIPERRIFLE",
	"TF_WEAPON_MINIGUN",
	"TF_WEAPON_SMG",
	"TF_WEAPON_SYRINGEGUN_MEDIC",
	"TF_WEAPON_TRANQ",
	"TF_WEAPON_ROCKETLAUNCHER",
	"TF_WEAPON_GRENADELAUNCHER",
	"TF_WEAPON_PIPEBOMBLAUNCHER",
	"TF_WEAPON_FLAMETHROWER",
	"TF_WEAPON_PISTOL",
	"TF_WEAPON_PISTOL_SCOUT",
	"TF_WEAPON_REVOLVER",
	"TF_WEAPON_NAILGUN",
	"TF_WEAPON_PDA",
	"TF_WEAPON_PDA_ENGINEER_BUILD",
	"TF_WEAPON_PDA_ENGINEER_DESTROY",
	"TF_WEAPON_PDA_SPY",
	"TF_WEAPON_BUILDER",
	"TF_WEAPON_MEDIGUN",
	"TF_WEAPON_FLAMETHROWER_ROCKET",
	"TF_WEAPON_SENTRY_BULLET",
	"TF_WEAPON_SENTRY_ROCKET",
	"TF_WEAPON_DISPENSER",
	"TF_WEAPON_INVIS",
	"TF_WEAPON_RAILGUN",
	"TF_WEAPON_SUPERSHOTGUN",
	"TF_WEAPON_ETERNALSHOTGUN",
	"TF_WEAPON_PISTOL_MERCENARY",
	"TF_WEAPON_REVOLVER_MERCENARY",
	"TF_WEAPON_GATLINGGUN",
	"TF_WEAPON_PISTOL_AKIMBO",
	"TF_WEAPON_UMBRELLA",
	"TF_WEAPON_SMG_MERCENARY",
	"TF_WEAPON_TOMMYGUN",
	"TF_WEAPON_GRENADELAUNCHER_MERCENARY",
	"TF_WEAPON_ROCKETLAUNCHER_DM",
	"TF_WEAPON_ASSAULTRIFLE",
	"TF_WEAPON_C4",
	"TF_WEAPON_BERSERK",
	"TF_WEAPON_PHYSCANNON",
	"TF_WEAPON_SUPER_ROCKETLAUNCHER",
	"TF_WEAPON_CHAINSAW",
	"TF_WEAPON_DYNAMITE_BUNDLE",
	"TF_WEAPON_LIGHTNING_GUN",
	"TF_WEAPON_GRAPPLE",
	"TF_WEAPON_GIB",
	"TF_WEAPON_CLAWS",
	"TF_WEAPON_JUGGERNAUGHT",
	"TF_WEAPON_COMBATKNIFE",

	"TFC_WEAPON_SHOTGUN_SB",
	"TFC_WEAPON_SHOTGUN_DB",
	"TFC_WEAPON_CROWBAR",
	"TFC_WEAPON_UMBRELLA",
	"TFC_WEAPON_RAILPISTOL",
	"TFC_WEAPON_ASSAULTCANNON",
	"TFC_WEAPON_NAILGUN",
	"TFC_WEAPON_NAILGUN_SUPER",
	"TFC_WEAPON_KNIFE",
	"TFC_WEAPON_TRANQ",
	"TFC_WEAPON_RPG",
	"TFC_WEAPON_SNIPER_RIFLE",
	"TFC_WEAPON_ASSAULT_RIFLE",
	"TFC_WEAPON_FLAMETHROWER",
	"TFC_WEAPON_INCENDIARYCANNON",
	"TFC_WEAPON_MEDKIT",
	"TFC_WEAPON_WRENCH",
	"TFC_WEAPON_GRENADELAUNCHER",
	"TFC_WEAPON_PIPEBOMBLAUNCHER",

	//NOTENOTE: Not normal throwable grenades, these are for the grenade launcher projectiles
	"TF_WEAPON_GRENADE_DEMOMAN",
	"TF_WEAPON_GRENADE_PIPEBOMB",
	"TF_WEAPON_GRENADE_MIRV",
	"TF_WEAPON_GRENADE_MIRVBOMB",

	"TF_WEAPON_COUNT",	// end marker, do not add below here 
};

bool WeaponID_IsSniperRifle( int iWeaponID )
{
	return iWeaponID == TF_WEAPON_SNIPERRIFLE || 
		iWeaponID == TF_WEAPON_RAILGUN ||
		iWeaponID == TFC_WEAPON_SNIPER_RIFLE;
}

bool WeaponID_IsRocketWeapon( int iWeaponID )
{
	return iWeaponID == TF_WEAPON_ROCKETLAUNCHER || 
		iWeaponID == TF_WEAPON_ROCKETLAUNCHER_DM || 
		iWeaponID == TF_WEAPON_SUPER_ROCKETLAUNCHER || 
		iWeaponID == TFC_WEAPON_INCENDIARYCANNON || 
		iWeaponID == TFC_WEAPON_RPG;
}

bool WeaponID_IsGrenadeWeapon( int iWeaponID )
{
	return iWeaponID == TF_WEAPON_GRENADELAUNCHER || 
		iWeaponID == TF_WEAPON_PIPEBOMBLAUNCHER || 
		iWeaponID == TF_WEAPON_GRENADELAUNCHER_MERCENARY || 
		iWeaponID == TF_WEAPON_DYNAMITE_BUNDLE || 
		iWeaponID == TFC_WEAPON_PIPEBOMBLAUNCHER || 
		iWeaponID == TFC_WEAPON_GRENADELAUNCHER;
}

bool WeaponID_IsMeleeWeapon(int iWeaponID)
{
	return iWeaponID == TF_WEAPON_BAT ||
		iWeaponID == TF_WEAPON_BOTTLE ||
		iWeaponID == TF_WEAPON_FIREAXE ||
		iWeaponID == TF_WEAPON_CLUB ||
		iWeaponID == TF_WEAPON_CROWBAR ||
		iWeaponID == TF_WEAPON_KNIFE ||
		iWeaponID == TF_WEAPON_FISTS ||
		iWeaponID == TF_WEAPON_SHOVEL ||
		iWeaponID == TF_WEAPON_WRENCH ||
		iWeaponID == TF_WEAPON_BONESAW ||
		iWeaponID == TF_WEAPON_KNIFE ||
		iWeaponID == TF_WEAPON_UMBRELLA ||
		iWeaponID == TFC_WEAPON_CROWBAR ||
		iWeaponID == TFC_WEAPON_UMBRELLA ||
		iWeaponID == TFC_WEAPON_KNIFE ||
		iWeaponID == TFC_WEAPON_WRENCH;
}

const char *g_aGrenadeNames[] =
{
	"TF_GRENADE_NONE",

	"TF_GRENADE_NORMAL",
	"TF_GRENADE_NORMAL_ENGINEER",
	"TF_GRENADE_CONCUSSION",
	"TF_GRENADE_NAIL",
	"TF_GRENADE_MIRV",
	"TF_GRENADE_NAPALM",
	"TF_GRENADE_GAS",
	"TF_GRENADE_EMP",
	"TF_GRENADE_CALTROP",
	"TF_GRENADE_PIPEBOMB",
	"TF_GRENADE_SMOKE_BOMB",
	"TF_GRENADE_HEAL",
	"TF_GRENADE_FLARE",
	"TF_GRENADE_HALLUC",

	"TF_GRENADE_COUNT",	// end marker, do not add below here 
};

const char *g_aExplosiveNames[] =
{
	"TF_GRENADE_NORMAL",
	"TF_GRENADE_NORMAL_ENGINEER",
	"TF_GRENADE_MIRV",
	"TF_GRENADE_PIPEBOMB",
	"TF_GRENADE_FLARE",
	"TF_PROJECTILE_ROCKET",
	"TF_PROJECTILE_PIPEBOMB",
	"TF_PROJECTILE_INCENDROCKET",
	"TF_GRENADE_NORMAL",
};

bool IsExplosiveProjectile(const char *alias)
{
	if (alias)
	{
		for (int i = 0; g_aExplosiveNames[i] != NULL; ++i)
			if (!Q_stricmp(g_aExplosiveNames[i], alias))
				return true;
	}

	return false;
}

int AliasToWeaponID( const char *alias )
{
	if (alias)
	{
		for( int i=0; g_aWeaponNames[i] != NULL; ++i )
			if (!Q_stricmp( g_aWeaponNames[i], alias ))
				return i;
	}

	return TF_WEAPON_NONE;
}

uint g_aWeaponDamageTypes[] =
{
	DMG_GENERIC,	// TF_WEAPON_NONE

	DMG_CLUB,		// TF_WEAPON_BAT,
	DMG_CLUB,		// TF_WEAPON_BOTTLE, 
	DMG_SLASH,		// TF_WEAPON_FIREAXE,
	DMG_CLUB,		// TF_WEAPON_CLUB,
	DMG_CLUB,		// TF_WEAPON_CROWBAR,
	DMG_SLASH,		// TF_WEAPON_KNIFE,
	DMG_CLUB,		// TF_WEAPON_FISTS,
	DMG_CLUB,		// TF_WEAPON_SHOVEL,
	DMG_CLUB,		// TF_WEAPON_WRENCH,
	DMG_SLASH,		// TF_WEAPON_BONESAW,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,	// TF_WEAPON_SHOTGUN,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD,  // TF_WEAPON_SCATTERGUN,
	DMG_BULLET | DMG_USE_HITLOCATIONS,	// TF_WEAPON_SNIPERRIFLE,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_MINIGUN,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_SMG,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_SYRINGEGUN_MEDIC,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE | DMG_PARALYZE,		// TF_WEAPON_TRANQ,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_ROCKETLAUNCHER,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_GRENADELAUNCHER,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_PIPEBOMBLAUNCHER,
	DMG_IGNITE | DMG_PREVENT_PHYSICS_FORCE | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_FLAMETHROWER,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_PISTOL,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_PISTOL_SCOUT,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_REVOLVER,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD ,		// TF_WEAPON_NAILGUN,
	DMG_BULLET,		// TF_WEAPON_PDA,
	DMG_BULLET,		// TF_WEAPON_PDA_ENGINEER_BUILD,
	DMG_BULLET,		// TF_WEAPON_PDA_ENGINEER_DESTROY,
	DMG_BULLET,		// TF_WEAPON_PDA_SPY,
	DMG_BULLET,		// TF_WEAPON_BUILDER
	DMG_BULLET,		// TF_WEAPON_MEDIGUN
	DMG_BLAST | DMG_IGNITE | DMG_RADIUS_MAX,		// TF_WEAPON_FLAMETHROWER_ROCKET
	DMG_GENERIC,	// TF_WEAPON_SENTRY_BULLET
	DMG_GENERIC,	// TF_WEAPON_SENTRY_ROCKET
	DMG_GENERIC,	// TF_WEAPON_DISPENSER
	DMG_GENERIC,	// TF_WEAPON_INVIS
	DMG_BULLET | DMG_USE_HITLOCATIONS,	// TF_WEAPON_RAILGUN,
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD, //TF_WEAPON_SUPERSHOTGUN
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD, //TF_WEAPON_ETERNALSHOTGUN
	DMG_BULLET  | DMG_USEDISTANCEMOD,	// TF_WEAPON_PISTOL_MERCENARY,
	DMG_BULLET  | DMG_USEDISTANCEMOD,	// TF_WEAPON_REVOLVER_MERCENARY,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TF_WEAPON_GATLINGGUN,
	DMG_BULLET | DMG_USEDISTANCEMOD,	// TF_WEAPON_PISTOL_AKIMBO,
	DMG_CLUB,		// TF_WEAPON_UMBRELLA,
	DMG_BULLET | DMG_USEDISTANCEMOD,	// TF_WEAPON_SMG_MERCENARY,
	DMG_BULLET | DMG_USEDISTANCEMOD,	// TF_WEAPON_TOMMYGUN,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_GRENADELAUNCHER_MERCENARY,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_ROCKETLAUNCHER_DM,
	DMG_BULLET | DMG_USEDISTANCEMOD ,	// TF_WEAPON_ASSAULTRIFLE,
	DMG_CLUB, //TF_WEAPON_C4
	DMG_ALWAYSGIB, //TF_WEAPON_BERSERK
	DMG_DISSOLVE, //TF_WEAPON_PHYSCANNON
	DMG_BLAST,		// TF_WEAPON_SUPER_ROCKETLAUNCHER,
	DMG_SLASH | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_CHAINSAW,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_DYNAMITE_BUNDLE,
	DMG_DISSOLVE,		// TF_WEAPON_LIGHTNING_GUN,
	DMG_GENERIC,		// TF_WEAPON_GRAPPLE,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TF_WEAPON_GIB,
	DMG_SLASH, // TF_WEAPON_CLAWS
	DMG_CLUB,		// TF_WEAPON_JUGGERNAUGHT,
	DMG_SLASH, // TF_WEAPON_COMBATKNIFE
	
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD, //TFC_WEAPON_SHOTGUN_SB
	DMG_BUCKSHOT | DMG_USEDISTANCEMOD, //TFC_WEAPON_SHOTGUN_DB
	DMG_CLUB,		// TFC_WEAPON_CROWBAR,
	DMG_CLUB,		// TFC_WEAPON_UMBRELLA,
	DMG_BULLET,		// TFC_WEAPON_RAILPISTOL,
	DMG_BULLET | DMG_USEDISTANCEMOD,		// TFC_WEAPON_ASSAULTCANNON
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD ,		// TFC_WEAPON_NAILGUN,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_NOCLOSEDISTANCEMOD ,		// TFC_WEAPON_NAILGUN_SUPER,
	DMG_SLASH,		// TFC_WEAPON_KNIFE,
	DMG_BULLET | DMG_USEDISTANCEMOD | DMG_PREVENT_PHYSICS_FORCE | DMG_PARALYZE,		// TFC_WEAPON_TRANQ,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TFC_WEAPON_RPG,
	DMG_BULLET | DMG_USE_HITLOCATIONS,	// TFC_WEAPON_SNIPER_RIFLE,
	DMG_BULLET | DMG_USEDISTANCEMOD ,	// TFC_WEAPON_ASSAULT_RIFLE,
	DMG_IGNITE | DMG_PREVENT_PHYSICS_FORCE | DMG_PREVENT_PHYSICS_FORCE,		// TF_WEAPON_FLAMETHROWER,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD | DMG_IGNITE,		// TFC_WEAPON_INCENDIARYCANNON,
	DMG_CLUB,		// TFC_WEAPON_MEDKIT, 
	DMG_CLUB,		// TFC_WEAPON_WRENCH, 
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_USEDISTANCEMOD,		// TFC_WEAPON_GRENADELAUNCHER,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TFC_WEAPON_PIPEBOMBLAUNCHER,

	//NOTENOTE: Not normal throwable grenades, these are for the grenade launcher projectiles
	DMG_BLAST | DMG_HALF_FALLOFF,					// TF_WEAPON_GRENADE_DEMOMAN
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_WEAPON_GRENADE_PIPEBOMB,
	DMG_BLAST,		// TF_WEAPON_GRENADE_MIRV,
	DMG_BLAST,		// TF_WEAPON_GRENADE_MIRVBOMB,

	// This is a special entry that must match with TF_WEAPON_COUNT
	// to protect against updating the weapon list without updating this list
	TF_DMG_SENTINEL_VALUE
};

uint g_aGrenadeDamageTypes[] =
{
	DMG_GENERIC,	// TF_GRENADE_NONE

	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_GRENADE_NORMAL,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_GRENADE_NORMAL_ENGINEER,
	DMG_SONIC | DMG_HALF_FALLOFF,		// TF_GRENADE_CONCUSSION,
	DMG_BULLET | DMG_HALF_FALLOFF,		// TF_GRENADE_NAIL,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_GRENADE_MIRV,
	DMG_BURN | DMG_RADIUS_MAX,		// TF_GRENADE_NAPALM,
	DMG_POISON | DMG_HALF_FALLOFF,		// TF_GRENADE_GAS,
	DMG_BLAST | DMG_HALF_FALLOFF | DMG_PREVENT_PHYSICS_FORCE,		// TF_GRENADE_EMP,
	DMG_GENERIC,	// TF_GRENADE_CALTROP,
	DMG_BLAST | DMG_HALF_FALLOFF,		// TF_GRENADE_PIPEBOMB,
	DMG_GENERIC,	// TF_GRENADE_SMOKE_BOMB,
	DMG_GENERIC,	// TF_GRENADE_HEAL,
	DMG_BURN | DMG_BLAST,		// TF_GRENADE_FLARE,
	DMG_GENERIC,	// TF_GRENADE_HALLUC,

	// This is a special entry that must match with TF_WEAPON_COUNT
	// to protect against updating the weapon list without updating this list
	TF_DMG_SENTINEL_VALUE
};

const char *g_szProjectileNames[] =
{
	"",
	"projectile_bullet",
	"projectile_rocket",
	"projectile_pipe",
	"projectile_pipe_remote",
	"projectile_syringe",
	"projectile_nail",
	"projectile_tripmine",
	"projectile_incendrocket",
	"projectile_pipe_dm",
	"projectile_tranq",
	"projectile_coom",
};

//NOTENOTE: This has been reworked, above char list not related anymore
int g_iProjectileWeapons[] = 
{
	TF_WEAPON_NONE,
	TF_WEAPON_PISTOL,
	TF_WEAPON_ROCKETLAUNCHER,
	TF_WEAPON_PIPEBOMBLAUNCHER,
	TF_WEAPON_GRENADELAUNCHER,
	TF_WEAPON_SYRINGEGUN_MEDIC,
	TF_WEAPON_GRENADELAUNCHER_MERCENARY,
	TF_WEAPON_ROCKETLAUNCHER_DM,
	TF_WEAPON_SUPER_ROCKETLAUNCHER,
	TF_WEAPON_DYNAMITE_BUNDLE,
	TFC_WEAPON_INCENDIARYCANNON,
};

const char *g_pszHintMessages[] =
{
	"#Hint_spotted_a_friend",
	"#Hint_spotted_an_enemy",
	"#Hint_killing_enemies_is_good",
	"#Hint_out_of_ammo",
	"#Hint_turn_off_hints",
	"#Hint_pickup_ammo",
	"#Hint_Cannot_Teleport_With_Flag",
	"#Hint_Cannot_Cloak_With_Flag",
	"#Hint_Cannot_Disguise_With_Flag",
	"#Hint_Cannot_Attack_While_Cloaked",
	"#Hint_ClassMenu",

// Grenades
	"#Hint_gren_caltrops",
	"#Hint_gren_concussion",
	"#Hint_gren_emp",
	"#Hint_gren_gas",
	"#Hint_gren_mirv",
	"#Hint_gren_nail",
	"#Hint_gren_napalm",
	"#Hint_gren_normal",

// Altfires
	"#Hint_altfire_sniperrifle",
	"#Hint_altfire_flamethrower",
	"#Hint_altfire_grenadelauncher",
	"#Hint_altfire_pipebomblauncher",
	"#Hint_altfire_rotate_building",

// Soldier
	"#Hint_Soldier_rpg_reload",

// Engineer
	"#Hint_Engineer_use_wrench_onown",
	"#Hint_Engineer_use_wrench_onother",
	"#Hint_Engineer_use_wrench_onfriend",
	"#Hint_Engineer_build_sentrygun",
	"#Hint_Engineer_build_dispenser",
	"#Hint_Engineer_build_teleporters",
	"#Hint_Engineer_pickup_metal",
	"#Hint_Engineer_repair_object",
	"#Hint_Engineer_metal_to_upgrade",
	"#Hint_Engineer_upgrade_sentrygun",

	"#Hint_object_has_sapper",

	"#Hint_object_your_object_sapped",
	"#Hint_enemy_using_dispenser",
	"#Hint_enemy_using_tp_entrance",
	"#Hint_enemy_using_tp_exit",
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetWeaponId( const char *pszWeaponName )
{
	// if this doesn't match, you need to add missing weapons to the array
	assert( ARRAYSIZE( g_aWeaponNames ) == ( TF_WEAPON_COUNT + 1 ) );

	for ( int iWeapon = 0; iWeapon < ARRAYSIZE( g_aWeaponNames ); ++iWeapon )
	{
		if ( !Q_stricmp( pszWeaponName, g_aWeaponNames[iWeapon] ) )
			return iWeapon;
	}

	return TF_WEAPON_NONE;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *WeaponIdToAlias( int iWeapon )
{
	// if this doesn't match, you need to add missing weapons to the array
	assert( ARRAYSIZE( g_aWeaponNames ) == ( TF_WEAPON_COUNT + 1 ) );

	if ( ( iWeapon >= ARRAYSIZE( g_aWeaponNames ) ) || ( iWeapon < 0 ) )
		return NULL;

	return g_aWeaponNames[iWeapon];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *WeaponIdToClassname( int iWeapon )
{
	const char *pszAlias = WeaponIdToAlias( iWeapon );
	if ( pszAlias == NULL )
		return NULL;

	static char szClassname[128];
	V_strncpy( szClassname, pszAlias, sizeof( szClassname ) );
	V_strlower( szClassname );

	return szClassname;
}

#ifdef GAME_DLL

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetWeaponFromDamage( const CTakeDamageInfo &info )
{
	int iWeapon = TF_WEAPON_NONE;

	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = TFGameRules()->GetKillingWeaponName( info, NULL );

	if ( !Q_strnicmp( killer_weapon_name, "tf_projectile", 13 ) )
	{
		for( int i = 0; i < ARRAYSIZE( g_szProjectileNames ); i++ )
		{
			if ( !Q_stricmp( &killer_weapon_name[ 3 ], g_szProjectileNames[ i ] ) )
			{
				iWeapon = g_iProjectileWeapons[ i ];
				break;
			}
		}
	}
	else
	{
		int iLen = Q_strlen( killer_weapon_name );

		// strip off _projectile from projectiles shot from other projectiles
		if ( ( iLen < 256 ) && ( iLen > 11 ) && !Q_stricmp( &killer_weapon_name[ iLen - 11 ], "_projectile" ) )
		{
			char temp[ 256 ];
			Q_strcpy( temp, killer_weapon_name );
			temp[ iLen - 11 ] = 0;

			// set the weapon used
			iWeapon = GetWeaponId( temp );
		}
		else
		{
			// set the weapon used
			iWeapon = GetWeaponId( killer_weapon_name );
		}
	}

	return iWeapon;
}

#endif

// ------------------------------------------------------------------------------------------------ //
// CObjectInfo tables.
// ------------------------------------------------------------------------------------------------ //

CObjectInfo::CObjectInfo( char *pObjectName )
{
	m_pObjectName = pObjectName;
	m_pClassName = NULL;
	m_flBuildTime = -9999;
	m_nMaxObjects = -9999;
	m_Cost = -9999;
	m_CostMultiplierPerInstance = -999;
	m_UpgradeCost = -9999;
	m_flUpgradeDuration = -9999;
	m_MaxUpgradeLevel = -9999;
	m_pBuilderWeaponName = NULL;
	m_pBuilderPlacementString = NULL;
	m_SelectionSlot = -9999;
	m_SelectionPosition = -9999;
	m_bSolidToPlayerMovement = false;
	m_pIconActive = NULL;
	m_pIconInactive = NULL;
	m_pViewModel = NULL;
	m_pPlayerModel = NULL;
	m_iDisplayPriority = 0;
	m_bVisibleInWeaponSelection = true;
	m_pExplodeSound = NULL;
	m_pUpgradeSound = NULL;
	m_pExplosionParticleEffect = NULL;
	m_bAutoSwitchTo = false;
}


CObjectInfo::~CObjectInfo()
{
	delete [] m_pClassName;
	delete [] m_pStatusName;
	delete [] m_pModeName0;
	delete [] m_pModeName1;
	delete [] m_pBuilderWeaponName;
	delete [] m_pBuilderPlacementString;
	delete [] m_pIconActive;
	delete [] m_pIconInactive;
	delete [] m_pViewModel;
	delete [] m_pPlayerModel;
	delete [] m_pExplodeSound;
	delete [] m_pUpgradeSound;
	delete [] m_pExplosionParticleEffect;
}

CObjectInfo g_ObjectInfos[OBJ_LAST] =
{
	CObjectInfo( "OBJ_DISPENSER" ),
	CObjectInfo( "OBJ_TELEPORTER" ),
	CObjectInfo( "OBJ_SENTRYGUN" ),
	CObjectInfo( "OBJ_ATTACHMENT_SAPPER" ),
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int GetBuildableId( const char *pszBuildableName )
{
	for ( int iBuildable = 0; iBuildable < OBJ_LAST; ++iBuildable )
	{
		if ( !Q_stricmp( pszBuildableName, g_ObjectInfos[iBuildable].m_pObjectName ) )
			return iBuildable;
	}

	return OBJ_LAST;
}


bool AreObjectInfosLoaded()
{
	return g_ObjectInfos[0].m_pClassName != NULL;
}


void LoadObjectInfos( IBaseFileSystem *pFileSystem )
{
	const char *pFilename = "scripts/objects.txt";

	// Make sure this stuff hasn't already been loaded.
	Assert( !AreObjectInfosLoaded() );

	KeyValues *pValues = new KeyValues( "Object descriptions" );
	if ( !pValues->LoadFromFile( pFileSystem, pFilename, "GAME" ) )
	{
		Error( "Can't open %s for object info.", pFilename );
		pValues->deleteThis();
		return;
	}

	// Now read each class's information in.
	for ( int iObj=0; iObj < ARRAYSIZE( g_ObjectInfos ); iObj++ )
	{
		CObjectInfo *pInfo = &g_ObjectInfos[iObj];
		KeyValues *pSub = pValues->FindKey( pInfo->m_pObjectName );
		if ( !pSub )
		{
			Error( "Missing section '%s' from %s.", pInfo->m_pObjectName, pFilename );
			pValues->deleteThis();
			return;
		}

		// Read all the info in.
		if ( (pInfo->m_flBuildTime = pSub->GetFloat( "BuildTime", -999 )) == -999 ||
			(pInfo->m_nMaxObjects = pSub->GetInt( "MaxObjects", -999 )) == -999 ||
			(pInfo->m_Cost = pSub->GetInt( "Cost", -999 )) == -999 ||
			(pInfo->m_CostMultiplierPerInstance = pSub->GetFloat( "CostMultiplier", -999 )) == -999 ||
			(pInfo->m_UpgradeCost = pSub->GetInt( "UpgradeCost", -999 )) == -999 ||
			(pInfo->m_flUpgradeDuration = pSub->GetFloat("UpgradeDuration", -999)) == -999 ||
			(pInfo->m_MaxUpgradeLevel = pSub->GetInt( "MaxUpgradeLevel", -999 )) == -999 ||
			(pInfo->m_SelectionSlot = pSub->GetInt( "SelectionSlot", -999 )) == -999 ||
			(pInfo->m_SelectionPosition = pSub->GetInt( "SelectionPosition", -999 )) == -999 )
		{
			Error( "Missing data for object '%s' in %s.", pInfo->m_pObjectName, pFilename );
			pValues->deleteThis();
			return;
		}

		pInfo->m_pClassName = ReadAndAllocStringValue( pSub, "ClassName", pFilename );
		pInfo->m_pStatusName = ReadAndAllocStringValue( pSub, "StatusName", pFilename );
		pInfo->m_pBuilderWeaponName = ReadAndAllocStringValue( pSub, "BuilderWeaponName", pFilename );
		pInfo->m_pBuilderPlacementString = ReadAndAllocStringValue( pSub, "BuilderPlacementString", pFilename );
		pInfo->m_bSolidToPlayerMovement = pSub->GetInt( "SolidToPlayerMovement", 0 ) ? true : false;
		pInfo->m_pIconActive = ReadAndAllocStringValue( pSub, "IconActive", pFilename );
		pInfo->m_pIconInactive = ReadAndAllocStringValue( pSub, "IconInactive", pFilename );
		pInfo->m_pViewModel = ReadAndAllocStringValue( pSub, "Viewmodel", pFilename );
		pInfo->m_pPlayerModel = ReadAndAllocStringValue( pSub, "Playermodel", pFilename );
		pInfo->m_iDisplayPriority = pSub->GetInt( "DisplayPriority", 0 );
		pInfo->m_pHudStatusIcon = ReadAndAllocStringValue( pSub, "HudStatusIcon", pFilename );
		pInfo->m_bVisibleInWeaponSelection = ( pSub->GetInt( "VisibleInWeaponSelection", 1 ) > 0 );
		pInfo->m_pExplodeSound = ReadAndAllocStringValue( pSub, "ExplodeSound", pFilename );
		pInfo->m_pUpgradeSound = ReadAndAllocStringValue( pSub, "UpgradeSound", pFilename );
		pInfo->m_pExplosionParticleEffect = ReadAndAllocStringValue( pSub, "ExplodeEffect", pFilename );
		pInfo->m_bAutoSwitchTo = ( pSub->GetInt( "autoswitchto", 0 ) > 0 );

		pInfo->m_iMetalToDropInGibs = pSub->GetInt( "MetalToDropInGibs", 0 );

		KeyValues *pSub1 = pSub->FindKey( "AltModes" );
		if ( pSub1 )
		{
			KeyValues *pSub2 = pSub1->FindKey( "AltMode0" );

			if ( pSub2 )
				pInfo->m_pModeName0 = ReadAndAllocStringValue( pSub2, "ModeName", pFilename );

			KeyValues *pSub3 = pSub1->FindKey( "AltMode1" );
			if ( pSub3 )
				pInfo->m_pModeName1 = ReadAndAllocStringValue( pSub3, "ModeName", pFilename );
		}	
	}

	pValues->deleteThis();
}


const CObjectInfo* GetObjectInfo( int iObject )
{
	Assert( iObject >= 0 && iObject < OBJ_LAST );
	Assert( AreObjectInfosLoaded() );
	return &g_ObjectInfos[iObject];
}

ConVar tf_cheapobjects( "tf_cheapobjects","0", FCVAR_CHEAT | FCVAR_REPLICATED, "Set to 1 and all objects will cost 0" );

//-----------------------------------------------------------------------------
// Purpose: Return the cost of another object of the specified type
//			If bLast is set, return the cost of the last built object of the specified type
// 
// Note: Used to contain logic from tf2 that multiple instances of the same object
//       cost different amounts. See tf2/game_shared/tf_shareddefs.cpp for details
//-----------------------------------------------------------------------------
int CalculateObjectCost( int iObjectType )
{
	if ( tf_cheapobjects.GetInt() )
	{
		return 0;
	}
	
	if ( of_infiniteammo.GetBool() )
	{
		return 0;
	}
	int iCost = GetObjectInfo( iObjectType )->m_Cost;

	return iCost;
}

//-----------------------------------------------------------------------------
// Purpose: Calculate the cost to upgrade an object of a specific type
//-----------------------------------------------------------------------------
int	CalculateObjectUpgrade( int iObjectType, int iObjectLevel )
{
	// Max level?
	if ( iObjectLevel >= GetObjectInfo( iObjectType )->m_MaxUpgradeLevel )
		return 0;

	int iCost = GetObjectInfo( iObjectType )->m_UpgradeCost;
	for ( int i = 0; i < (iObjectLevel - 1); i++ )
	{
		iCost *= OBJECT_UPGRADE_COST_MULTIPLIER_PER_LEVEL;
	}

	return iCost;
}

bool IsSpaceToSpawnHere( const Vector &vecPos )
{
	Vector mins = VEC_HULL_MIN - Vector( -5.0f, -5.0f, 0 );
	Vector maxs = VEC_HULL_MAX + Vector( 5.0f, 5.0f, 5.0f );
	trace_t tr;
	UTIL_TraceHull( vecPos, vecPos, mins, maxs, MASK_PLAYERSOLID, nullptr, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );
	return tr.fraction >= 1.0f;
}

void BuildBigHeadTransformation( CBaseAnimating *pAnimating, CStudioHdr *pStudio, Vector *pos, Quaternion *q, matrix3x4_t const &cameraTransformation, int boneMask, CBoneBitList &boneComputed, float flScale )
{
	if ( pAnimating == nullptr )
		return;

	if ( flScale == 1.0f )
		return;

	int headBone = pAnimating->LookupBone( "bip_head" );
	if ( headBone == -1 )
		return;

#if defined( CLIENT_DLL )
	matrix3x4_t &head = pAnimating->GetBoneForWrite( headBone );

	Vector oldTransform, newTransform;
	MatrixGetColumn( head, 3, &oldTransform );
	MatrixScaleBy( flScale, head );

	int helmetBone = pAnimating->LookupBone( "prp_helmet" );
	if ( helmetBone != -1 )
	{
		matrix3x4_t &helmet = pAnimating->GetBoneForWrite( helmetBone );
		MatrixScaleBy( flScale, helmet );

		MatrixGetColumn( helmet, 3, &newTransform );
		Vector transform = ( ( newTransform - oldTransform ) * flScale ) + oldTransform;
		MatrixSetColumn( transform, 3, helmet );
	}

	int hatBone = pAnimating->LookupBone( "prp_hat" );
	if ( hatBone != -1 )
	{
		matrix3x4_t &hat = pAnimating->GetBoneForWrite( hatBone );
		MatrixScaleBy( flScale, hat );

		MatrixGetColumn( hat, 3, &newTransform );
		Vector transform = ( ( newTransform - oldTransform ) * flScale ) + oldTransform;
		MatrixSetColumn( transform, 3, hat );
	}
#endif
}

