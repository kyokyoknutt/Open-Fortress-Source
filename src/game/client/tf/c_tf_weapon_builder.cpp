//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's CWeaponBuilder class
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_tf_weapon_builder.h"
#include "c_weapon__stubs.h"
#include "c_tf_player.h"

STUB_WEAPON_CLASS_IMPLEMENT( tf_weapon_builder, C_TFWeaponBuilder );

// Recalc object sprite when we receive a new object type to build
void RecvProxy_ObjectType( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	// Pass to normal Int recvproxy
	RecvProxy_Int32ToInt32( pData, pStruct, pOut );

	// Reset the object sprite
	C_TFWeaponBuilder *pBuilder = ( C_TFWeaponBuilder * )pStruct;
	pBuilder->SetupObjectSelectionSprite();
}

BEGIN_NETWORK_TABLE_NOBASE( C_TFWeaponBuilder, DT_BuilderLocalData )
	RecvPropInt( RECVINFO(m_iObjectType), 0, RecvProxy_ObjectType ),
	RecvPropInt( RECVINFO(m_iAltMode), 0, RecvProxy_ObjectType ),
	RecvPropEHandle( RECVINFO(m_hObjectBeingBuilt) ),
END_NETWORK_TABLE()


IMPLEMENT_CLIENTCLASS_DT(C_TFWeaponBuilder, DT_TFWeaponBuilder, CTFWeaponBuilder)
	RecvPropInt( RECVINFO(m_iBuildState) ),
	RecvPropTime( RECVINFO( m_flSecondaryTimeout ) ),
	RecvPropDataTable( "BuilderLocalData", 0, 0, &REFERENCE_RECV_TABLE( DT_BuilderLocalData ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFWeaponBuilder::C_TFWeaponBuilder()
{
	m_iBuildState = 0;
	m_iObjectType = BUILDER_INVALID_OBJECT;
	m_iAltMode = 0;
	m_pSelectionTextureActive = NULL;
	m_pSelectionTextureInactive = NULL;
	m_iValidBuildPoseParam = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFWeaponBuilder::~C_TFWeaponBuilder()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *C_TFWeaponBuilder::GetCurrentSelectionObjectName( void )
{
	if ( m_iObjectType == -1 || (m_iBuildState == BS_SELECTING) )
		return "";

	return GetObjectInfo( m_iObjectType )->m_pBuilderWeaponName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool C_TFWeaponBuilder::Deploy( void )
{
	m_iViewModelIndex = modelinfo->GetModelIndex( GetViewModel( 0 ) );
	m_iWorldModelIndex = modelinfo->GetModelIndex( GetWorldModel() );

	bool bDeploy = BaseClass::Deploy();

	if ( bDeploy )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.35f;
		m_flNextSecondaryAttack = gpGlobals->curtime;		// asap

		CTFPlayer *pPlayer = ToTFPlayer( GetOwner() );
		if (!pPlayer)
			return false;

		pPlayer->SetNextAttack( gpGlobals->curtime );
	}

	return bDeploy;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFWeaponBuilder::SecondaryAttack( void )
{
	if ( gpGlobals->curtime < m_flSecondaryTimeout )
		return;

	if ( m_bInAttack2 )
		return;

	// require a re-press
	m_bInAttack2 = true;

	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( !pOwner->m_bHauling )
		pOwner->DoClassSpecialSkill();

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.2f;
}

//-----------------------------------------------------------------------------
// Purpose: cache the build pos pose param
//-----------------------------------------------------------------------------
CStudioHdr *C_TFWeaponBuilder::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	m_iValidBuildPoseParam = LookupPoseParameter( "valid_build_pos" );

	return hdr;
}

//-----------------------------------------------------------------------------
// Purpose: only called for local player
//-----------------------------------------------------------------------------
void C_TFWeaponBuilder::Redraw()
{
	if ( m_iValidBuildPoseParam >= 0 )
	{
		CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
		if ( !pOwner )
			return;

		// Assuming here that our model is the same as our viewmodel's model!
		CBaseViewModel *pViewModel = pOwner->GetViewModel(0);

		if ( pViewModel )
		{
			float flPoseParamValue = pViewModel->GetPoseParameter( m_iValidBuildPoseParam );

			C_BaseObject *pObj = m_hObjectBeingBuilt.Get();

			if ( pObj && pObj->WasLastPlacementPosValid() )
			{
				// pose param approach 1.0
				flPoseParamValue = Approach( 1.0, flPoseParamValue, 3.0 * gpGlobals->frametime );
			}
			else
			{
				// pose param approach 0.0
				flPoseParamValue = Approach( 0.0, flPoseParamValue, 1.5 * gpGlobals->frametime );
			}

			pViewModel->SetPoseParameter( m_iValidBuildPoseParam, flPoseParamValue );
		}
	}

	BaseClass::Redraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool C_TFWeaponBuilder::IsPlacingObject( void )
{
	if ( m_iBuildState == BS_PLACING || m_iBuildState == BS_PLACING_INVALID )
		return true;

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFWeaponBuilder::GetSlot( void ) const
{
	return GetObjectInfo( m_iObjectType )->m_SelectionSlot;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_TFWeaponBuilder::GetPosition( void ) const
{
	return GetObjectInfo( m_iObjectType )->m_SelectionPosition;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFWeaponBuilder::SetupObjectSelectionSprite( void )
{
#ifdef CLIENT_DLL
	// it appears this isnt used for anything and it just causes random crashes, so disabled!
	m_pSelectionTextureActive = NULL;
	m_pSelectionTextureInactive = NULL;

	return;

	// Use the sprite details from the text file, with a custom sprite
	char *iconTexture = GetObjectInfo( m_iObjectType )->m_pIconActive;
	if ( iconTexture && iconTexture[ 0 ] )
	{
		m_pSelectionTextureActive = gHUD.GetIcon( iconTexture );
	}
	else
	{
		m_pSelectionTextureActive = NULL;
	}

	iconTexture = GetObjectInfo( m_iObjectType )->m_pIconInactive;
	if ( iconTexture && iconTexture[ 0 ] )
	{
		m_pSelectionTextureInactive = gHUD.GetIcon( iconTexture );
	}
	else
	{
		m_pSelectionTextureInactive = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *C_TFWeaponBuilder::GetSpriteActive( void ) const
{
	return m_pSelectionTextureActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture const *C_TFWeaponBuilder::GetSpriteInactive( void ) const
{
	return m_pSelectionTextureInactive;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : char const
//-----------------------------------------------------------------------------
const char *C_TFWeaponBuilder::GetPrintName( void ) const
{
	return GetObjectInfo( m_iObjectType )->m_pStatusName;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	C_TFWeaponBuilder::GetSubType( void )
{
	return m_iObjectType;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon can be selected via the weapon selection
//-----------------------------------------------------------------------------
bool C_TFWeaponBuilder::CanBeSelected( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return false;

	if ( pOwner->CanBuild( m_iObjectType, m_iAltMode ) != CB_CAN_BUILD )
		return false;

	return HasAmmo();
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon should be visible in the weapon selection
//-----------------------------------------------------------------------------
bool C_TFWeaponBuilder::VisibleInWeaponSelection( void )
{
	return GetObjectInfo( m_iObjectType )->m_bVisibleInWeaponSelection;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this weapon has some ammo
//-----------------------------------------------------------------------------
bool C_TFWeaponBuilder::HasAmmo( void )
{
	CTFPlayer *pOwner = ToTFPlayer( GetOwner() );
	if ( !pOwner )
		return false;

	int iCost = CalculateObjectCost( m_iObjectType );
	return ( pOwner->GetBuildResources() >= iCost );
}

// -----------------------------------------------------------------------------
// Purpose:
// -----------------------------------------------------------------------------
const char *C_TFWeaponBuilder::GetViewModel( int iViewModel ) const
{
	if ( GetPlayerOwner() == NULL )
	{
		return BaseClass::GetViewModel();
	}

	if ( m_iObjectType != BUILDER_INVALID_OBJECT )
	{
		return GetObjectInfo( m_iObjectType )->m_pViewModel;
	}

	return BaseClass::GetViewModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *C_TFWeaponBuilder::GetWorldModel( void ) const
{
	if ( GetPlayerOwner() == NULL )
	{
		return BaseClass::GetWorldModel();
	}

	if ( m_iObjectType != BUILDER_INVALID_OBJECT )
	{
		return GetObjectInfo( m_iObjectType )->m_pPlayerModel;
	}

	return BaseClass::GetWorldModel();
}

Activity C_TFWeaponBuilder::GetDrawActivity( void )
{
	// sapper used to call different draw animations , one when invis and one when not.
	// now you can go invis *while* deploying, so let's always use the one-handed deploy.
	if ( GetSubType() == OBJ_ATTACHMENT_SAPPER )
	{
		return ACT_VM_DRAW_DEPLOYED;
	}
	else
	{
		return BaseClass::GetDrawActivity();
	}
}


acttable_t C_TFWeaponBuilder::m_acttableBuildingDeployed[] =
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_BUILDING_DEPLOYED,			false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_BUILDING_DEPLOYED,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_BUILDING_DEPLOYED,			false },
	{ ACT_MP_WALK,				ACT_MP_WALK_BUILDING_DEPLOYED,			false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_BUILDING_DEPLOYED,		false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_BUILDING_DEPLOYED,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_BUILDING_DEPLOYED,			false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_BUILDING_DEPLOYED,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_BUILDING_DEPLOYED,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_BUILDING_DEPLOYED,		false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_BUILDING_DEPLOYED,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_BUILDING_DEPLOYED,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_BUILDING_DEPLOYED,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_BUILDING_DEPLOYED,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_BUILDING_DEPLOYED,	false },

	{ ACT_MP_ATTACK_STAND_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED,	false },
	{ ACT_MP_ATTACK_CROUCH_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED,	false },
	{ ACT_MP_ATTACK_SWIM_GRENADE,		ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED,	false },
	{ ACT_MP_ATTACK_AIRWALK_GRENADE,	ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_BUILDING,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_BUILDING,	false },
};

//Act table remapping
acttable_t *C_TFWeaponBuilder::ActivityList( int &iActivityCount )
{
	C_TFPlayer *pPlayer = ToTFPlayer( GetOwner() );

	if ( pPlayer && pPlayer->m_bHauling )
	{
		iActivityCount = ARRAYSIZE( m_acttableBuildingDeployed );
		return m_acttableBuildingDeployed;
	}
	else
	{
		return BaseClass::ActivityList( iActivityCount );
	}
}