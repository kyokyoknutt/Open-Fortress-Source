//=============================================================================
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "tf_shareddefs.h"
#if GAME_DLL
#include "tf_player.h"
#else
#include "c_tf_player.h"
#endif
#include "of_shared_schemas.h"

#include "ienginevgui.h"
#include "engine/IEngineSound.h"

#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
	#define SHARED_ARGS VarArgs
#else
	#define SHARED_ARGS UTIL_VarArgs
#endif

KeyValues* gSoundManifest;
KeyValues* GlobalSoundManifest()
{
	return gSoundManifest;
}

void InitGlobalSoundManifest()
{
	if( gSoundManifest )
	{
		gSoundManifest->deleteThis();
	}
	gSoundManifest = new KeyValues( "GlobalSoundManifest" );
}

KeyValues* gLevelSoundManifest;
KeyValues* LevelSoundManifest()
{
	return gLevelSoundManifest;
}

void InitLevelSoundManifest()
{
	if( gLevelSoundManifest )
	{
		gLevelSoundManifest->deleteThis();
	}
	gLevelSoundManifest = new KeyValues( "LevelSoundManifest" );
}

void ParseSoundManifest( void )
{	
	InitGlobalSoundManifest();
	
	KeyValues *pManifestFile = new KeyValues( "game_sounds_manifest" );
	pManifestFile->LoadFromFile( filesystem, "scripts/game_sounds_manifest.txt" );
	
	if ( pManifestFile )
	{
		KeyValues *pManifest = new KeyValues( "Manifest" );
		pManifest = pManifestFile->GetFirstValue();
		for( pManifest; pManifest != NULL; pManifest = pManifest->GetNextValue() ) // Loop through all the keyvalues
		{
			KeyValues *pSoundFile = new KeyValues( "SoundFile" );
			pSoundFile->LoadFromFile( filesystem, pManifest->GetString() );
			if( pSoundFile )
			{
				KeyValues *pSound = new KeyValues( "SoundScript" );
				pSound = pSoundFile;
				GlobalSoundManifest()->AddSubKey( pSound );

				for( pSound; pSound != NULL; pSound = pSound->GetNextKey() ) // Loop through all the keyvalues
				{
					if( pSound )
					{
						DevMsg( "Parsed: %s\n", pSound->GetString( "wave" ) );
					}
				}
			}
		}	
	}
}

void CheckGlobalSounManifest( void )
{
	if ( GlobalSoundManifest() )
	{		
		KeyValues *pSound = new KeyValues( "SoundScript" );
		pSound = GlobalSoundManifest()->GetFirstSubKey();
		for( pSound; pSound != NULL; pSound = pSound->GetNextKey() ) // Loop through all the keyvalues
		{
			if( strcmp( pSound->GetString( "wave" ), "" ) )
				DevMsg( "Sound name: %s\n", pSound->GetString( "wave" ) );
		}
	}
}

void ParseLevelSoundManifest( void )
{	
	InitLevelSoundManifest();
	
	char mapsounds[MAX_PATH];
	char mapname[ 256 ];
	Q_StripExtension( 
#if CLIENT_DLL
	engine->GetLevelName()
#else
	STRING(gpGlobals->mapname)
#endif
	, mapname, sizeof( mapname ) );
	Q_snprintf( mapsounds, sizeof( mapsounds ), "%s_level_sounds.txt", mapname );
	if( !filesystem->FileExists( mapsounds , "GAME" ) )
	{
		DevMsg( "%s not present, not parsing", mapsounds );
		return;
	}
	DevMsg("%s\n", mapsounds);
	KeyValues *pSoundFile = new KeyValues( "level_sounds" );
	pSoundFile->LoadFromFile( filesystem, mapsounds, "GAME" );

	if( pSoundFile )
	{
		KeyValues *pSound = new KeyValues( "SoundScript" );
		pSound = pSoundFile;
		LevelSoundManifest()->AddSubKey( pSound );
		for( pSound; pSound != NULL; pSound = pSound->GetNextKey() ) // Loop through all the keyvalues
		{
			if( pSound )
			{
				DevMsg( "Parsed: %s\n", pSound->GetString( "wave" ) );
			}
		}
	}
}

KeyValues* GetSoundscript( const char *szSoundScript )
{
	KeyValues *pSound = new KeyValues( "SoundScript" );
			
	char mapsounds[MAX_PATH];
	char mapname[ 256 ];
	Q_StripExtension( 
#if CLIENT_DLL
	engine->GetLevelName()
#else
	STRING(gpGlobals->mapname)
#endif
	, mapname, sizeof( mapname ) );
	Q_snprintf( mapsounds, sizeof( mapsounds ), "%s_level_sounds.txt", mapname );
			
	if( filesystem->FileExists( mapsounds , "GAME" ) )
	{
		pSound = LevelSoundManifest()->FindKey( szSoundScript );
		if( !pSound )
		{
			DevMsg("Key not found in level sounds\n");
			return GlobalSoundManifest()->FindKey( szSoundScript );
		}
		else
			return pSound;
	}
	else if ( GlobalSoundManifest() )	
	{
		return GlobalSoundManifest()->FindKey( szSoundScript );
	}
	
	return NULL;
}
#ifdef CLIENT_DLL
void PrecacheUISoundScript( char *szSoundScript )
{
	KeyValues *pSoundScript = GetSoundscript( szSoundScript );
	if( !pSoundScript )
		return;
	
	KeyValues *pWave = pSoundScript->FindKey("rndwave");
	if( pWave )
	{
		bool bSuccess = false;
		for( KeyValues *pSub = pWave->GetFirstSubKey(); pSub != NULL ; pSub = pSub->GetNextKey() )
		{
			bSuccess = true;
			enginesound->PrecacheSound( pSub->GetString(), true, false );
		}

		if( !bSuccess )
		{
			Warning("Failed to precache UI sound %s", szSoundScript);
			return;
		}
	}
	else
	{
		pWave = pSoundScript->FindKey("wave");
		if( !pWave )
		{
			Warning("Failed to precache UI sound %s", szSoundScript);
			return;
		}
		
		enginesound->PrecacheSound( pWave->GetString(), true, false );
	}
}
#endif
char *GetSoundScriptWave( char *szSoundScript )
{
	if( !szSoundScript )
		return NULL;

	char *szReturn = new char[128];
	KeyValues *pSoundScript = GetSoundscript( szSoundScript );
	if( !pSoundScript )
		return NULL;
	
	KeyValues *pWave = pSoundScript->FindKey("rndwave");
	if( pWave )
	{
		KeyValues *pRand = new KeyValues("Rand");
		int iMaxSounds = 0;
		for( KeyValues *pSub = pWave->GetFirstSubKey(); pSub != NULL ; pSub = pSub->GetNextKey() )
		{
			pRand->SetString(SHARED_ARGS("%d",iMaxSounds),pSub->GetString());
			iMaxSounds++;
		}

		if( iMaxSounds )
		{
			Q_strncpy(szReturn, pRand->GetString(SHARED_ARGS("%d", random->RandomInt( 0, iMaxSounds - 1 ) ) ), 128 );
			return szReturn;
		}
		else
			Warning("Failed to Play UI sound %s\n", szSoundScript);
		
		// We dont need rand anymore, so clear it to not cause memory leaks
		pRand->deleteThis();
		return NULL;
	}
	else
	{
		pWave = pSoundScript->FindKey("wave");
		if( !pWave )
		{
			Warning("Failed to play UI sound %s\n", szSoundScript);
			return NULL;
		}
		Q_strncpy(szReturn, pWave->GetString(), 128 );
		return szReturn;
	}
}

KeyValues* gItemsGame;
KeyValues* GetItemsGame()
{
	return gItemsGame;
}

void InitItemsGame()
{
	if( gItemsGame )
	{
		gItemsGame->deleteThis();
	}
	gItemsGame = new KeyValues( "ItemsGame" );
}

void ParseItemsGame( void )
{	
	InitItemsGame();
	
	GetItemsGame()->LoadFromFile( filesystem, "scripts/items/items_game.txt" );
	
	KeyValues *pCosmetics = GetItemsGame()->FindKey("Cosmetics");
	if( pCosmetics )
	{
		int i = 0;
		FOR_EACH_SUBKEY( pCosmetics, kvSubKey )
		{
			i++;
		}
		GetItemsGame()->SetInt("cosmetic_count", i);
	}

	KeyValues *pWeapons = GetItemsGame()->FindKey("Weapons");
	if( pWeapons )
	{
		FOR_EACH_SUBKEY( pWeapons, kvSubKey )
		{
			GetItemSchema()->AddWeapon( kvSubKey->GetName() );
		}
	}	
	
}

void ReloadItemsSchema()
{
	GetItemSchema()->PurgeSchema();
	InitItemsGame();
	ParseItemsGame();
#ifdef CLIENT_DLL
	engine->ExecuteClientCmd( "schema_reload_items_game_server" );
#endif
}

static ConCommand schema_reload_items_game( 
#ifdef CLIENT_DLL
"schema_reload_items_game",
#else
"schema_reload_items_game_server",
#endif
 ReloadItemsSchema, "Reloads the items game.", FCVAR_NONE );

KeyValues* GetCosmetic( int iID )
{
	if( !GetItemsGame() )
		return NULL;
	
	KeyValues *pCosmetics = GetItemsGame()->FindKey("Cosmetics");
	if( !pCosmetics )
		return NULL;
	
	KeyValues *pCosmetic = pCosmetics->FindKey( SHARED_ARGS( "%d", iID ) );
	if( !pCosmetic )
		return NULL;
	
	return pCosmetic;
}

KeyValues* GetWeaponFromSchema( const char *szName )
{
	if( !GetItemsGame() )
		return NULL;
	
	KeyValues *pWeapons = GetItemsGame()->FindKey("Weapons");
	if( !pWeapons )
		return NULL;
	
	KeyValues *pWeapon = pWeapons->FindKey( szName );
	if( !pWeapon )
		return NULL;
	
	return pWeapon;
}

KeyValues* GetRespawnParticle( int iID )
{
	if( !GetItemsGame() )
		return NULL;
	
	KeyValues *pParticles = GetItemsGame()->FindKey("RespawnParticles");
	if( !pParticles )
		return NULL;
	
	KeyValues *pParticle = pParticles->FindKey( SHARED_ARGS( "%d", iID ) );
	if( !pParticle )
		return NULL;
	
	return pParticle;
}

CTFItemSchema *gItemSchema;
CTFItemSchema *GetItemSchema()
{
	return gItemSchema;
}

void InitItemSchema()
{
	gItemSchema = new CTFItemSchema();
}

CTFItemSchema::CTFItemSchema()
{
}

void CTFItemSchema::PurgeSchema()
{
	m_hWeaponNames.Purge();
}

void CTFItemSchema::AddWeapon( const char *szWeaponName )
{
	m_hWeaponNames.AddToTail( szWeaponName );
}

KeyValues *CTFItemSchema::GetWeapon( int iID )
{
	if( iID >= m_hWeaponNames.Count() || iID < 0 )
		return NULL;
	
	return GetWeaponFromSchema( m_hWeaponNames[iID] );
}

KeyValues *CTFItemSchema::GetWeapon( const char *szWeaponName )
{
	return GetWeaponFromSchema( szWeaponName );
}

int CTFItemSchema::GetWeaponID( const char *szWeaponName )
{
	int iMax = m_hWeaponNames.Count();
	for( int i = 0; i < iMax; i++ )
	{
		if( FStrEq(m_hWeaponNames[i], szWeaponName) )
			return i;
	}

	return 0;
}

#ifdef CLIENT_DLL

KeyValues* gLoadout;
KeyValues* GetLoadout()
{
	return gLoadout;
}

KeyValues* GetCosmeticLoadoutForClass( int iClass )
{
	if( iClass > TF_CLASS_JUGGERNAUT )
		return NULL;

	if( !GetLoadout() )
		return NULL;
	
	KeyValues *kvCosmetics = GetLoadout()->FindKey("Cosmetics");
	if( !kvCosmetics )
		return NULL;
	
	KeyValues *kvClass = kvCosmetics->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	
	return kvClass;
	
}

KeyValues* GetWeaponLoadoutForClass( int iClass )
{
	if( iClass > TF_CLASS_JUGGERNAUT )
		return NULL;

	if( !GetLoadout() )
		return NULL;
	
	KeyValues *kvWeapons = GetLoadout()->FindKey("Weapons");
	if( !kvWeapons )
		return NULL;
	
	KeyValues *kvClass = kvWeapons->FindKey(g_aPlayerClassNames_NonLocalized[iClass]);
	
	return kvClass;
	
}

void ResetLoadout( const char *szCatName )
{
	if( !gLoadout )
		gLoadout = new KeyValues( "Loadout" );
	
	char szCatNameFull[64];
	Q_strncpy(szCatNameFull, szCatName, sizeof(szCatNameFull) );
	strlwr(szCatNameFull);
	
	int iCategory = 0;
	
	for( int i = 0; i < 2 ; i++ )
	{
		if( !Q_strcmp( szCatNameFull, g_aLoadoutCategories[i] ) )
		{
			iCategory = i;
			break;
		}
	}
	
	KeyValues *pCategory = new KeyValues( szCatNameFull );
	gLoadout->AddSubKey( pCategory );

	for ( int i = 0; i < TF_CLASS_COUNT_ALL; i++ )
	{
		KeyValues *pClass = new KeyValues( g_aPlayerClassNames_NonLocalized[i] );
		
		switch( iCategory )
		{
			case 0:
			pClass->SetString( "hat", "0" );
			break;
			case 1:
			if( i == TF_CLASS_MERCENARY )
			{
				pClass->SetString( "1", "tf_weapon_assaultrifle" );
				pClass->SetString( "2", "tf_weapon_pistol_mercenary" );
				pClass->SetString( "3", "tf_weapon_crowbar" );
			}
			break;
		}
		pCategory->AddSubKey( pClass );
	}
	gLoadout->SaveToFile( filesystem, "cfg/loadout.cfg" );
}

void ParseLoadout( void )
{	
	if ( !filesystem->FileExists( "cfg/loadout.cfg" , "MOD" ) )
	{
		ResetLoadout( "Cosmetics" );
		ResetLoadout( "Weapons" );
	}
	else
	{
		gLoadout = new KeyValues( "Loadout" );
		GetLoadout()->LoadFromFile( filesystem, "cfg/loadout.cfg" );
	}
}

CTFLoadoutHandler *gLoadoutHandle;
CTFLoadoutHandler *GetLoadoutHandle()
{
	return gLoadoutHandle;
}

void InitLoadoutHandle()
{
	gLoadoutHandle = new CTFLoadoutHandler();
}

extern const char *g_aLoadoutConvarNames[];
extern const char *g_aArsenalConvarNames[];

CTFLoadoutHandler::CTFLoadoutHandler()
{
	gLoadoutHandle = this;
	for( int i = TF_CLASS_UNDEFINED + 1; i < TF_CLASS_COUNT_ALL; i++ )
	{
		char szCommand[128];
		szCommand[0] = '\0';

		// Cosmetics here
		KeyValues *kvClass = GetCosmeticLoadoutForClass( i );
		if( kvClass )
		{
			for( KeyValues *sub = kvClass->GetFirstValue(); sub != NULL; sub = sub->GetNextValue() )
			{
				if( szCommand[0] == '\0' )
					Q_snprintf( szCommand, sizeof(szCommand), "%s", sub->GetString() );
				else
					Q_snprintf( szCommand, sizeof(szCommand), "%s %s", szCommand, sub->GetString() );
			}
		}
		ConVar *pLol = new ConVar( g_aLoadoutConvarNames[i], "", FCVAR_USERINFO );
		pLol->SetValue(szCommand);
		
		m_hClassLoadouts.AddToTail( pLol );
		
		
		// Weapons here
		kvClass = GetWeaponLoadoutForClass( i );
		if( kvClass )
		{
			for( KeyValues *sub = kvClass->GetFirstValue(); sub != NULL; sub = sub->GetNextValue() )
			{
				if( szCommand[0] == '\0' )
					Q_snprintf( szCommand, sizeof(szCommand), "%s", sub->GetString() );
				else
					Q_snprintf( szCommand, sizeof(szCommand), "%s %s", szCommand, sub->GetString() );
			}
		}
		ConVar *pNewLol = new ConVar( g_aArsenalConvarNames[i], "", FCVAR_USERINFO );
		pNewLol->SetValue(szCommand);
		
		m_hClassArsenal.AddToTail( pNewLol );
	}
}
#endif