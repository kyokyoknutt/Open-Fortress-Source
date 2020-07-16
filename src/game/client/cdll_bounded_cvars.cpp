//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#include "cbase.h"
#include "cdll_bounded_cvars.h"
#include "convar_serverbounded.h"
#include "tier0/icommandline.h"


bool g_bForceCLPredictOff = false;

// ------------------------------------------------------------------------------------------ //
// cl_predict.
// ------------------------------------------------------------------------------------------ //

class CBoundedCvar_Predict : public ConVar_ServerBounded
{
public:
	CBoundedCvar_Predict() :
	  ConVar_ServerBounded( "cl_predict", 
		  "1.0", 
#if defined(DOD_DLL) || defined(CSTRIKE_DLL)
		  FCVAR_USERINFO | FCVAR_CHEAT, 
#else
		  FCVAR_USERINFO | FCVAR_NOT_CONNECTED, 
#endif
		  "Perform client side prediction." )
	  {
	  }

	  virtual float GetFloat() const
	  {
		  // Used temporarily for CS kill cam.
		  if ( g_bForceCLPredictOff )
			  return 0;

		  static const ConVar *pClientPredict = g_pCVar->FindVar( "sv_client_predict" );
		  if ( pClientPredict && pClientPredict->GetInt() != -1 )
		  {
			  // Ok, the server wants to control this value.
			  return pClientPredict->GetFloat();
		  }
		  else
		  {
			  return GetBaseFloatValue();
		  }
	  }
};

static CBoundedCvar_Predict cl_predict_var;
ConVar_ServerBounded *cl_predict = &cl_predict_var;

// ------------------------------------------------------------------------------------------ //
// cl_interp_ratio.
// ------------------------------------------------------------------------------------------ //

class CBoundedCvar_InterpRatio : public ConVar_ServerBounded
{
public:
	CBoundedCvar_InterpRatio() :
#ifdef OF_CLIENT_DLL
	  ConVar_ServerBounded( "cl_interp_ratio", 
		  "1.0", 
		  FCVAR_USERINFO | FCVAR_NOT_CONNECTED, 
		  "Sets the interpolation amount (final amount is cl_interp_ratio / cl_updaterate)." )
#else
	  ConVar_ServerBounded( "cl_interp_ratio", 
		  "2.0", 
		  FCVAR_USERINFO | FCVAR_NOT_CONNECTED, 
		  "Sets the interpolation amount (final amount is cl_interp_ratio / cl_updaterate)." )	
#endif
	  {
	  }

	  virtual float GetFloat() const
	  {
		  static const ConVar *pMin = g_pCVar->FindVar( "sv_client_min_interp_ratio" );
		  static const ConVar *pMax = g_pCVar->FindVar( "sv_client_max_interp_ratio" );
		  if ( pMin && pMax && pMin->GetFloat() != -1 )
		  {
			  return clamp( GetBaseFloatValue(), pMin->GetFloat(), pMax->GetFloat() );
		  }
		  else
		  {
			  return GetBaseFloatValue();
		  }
	  }
};

static CBoundedCvar_InterpRatio cl_interp_ratio_var;
ConVar_ServerBounded *cl_interp_ratio = &cl_interp_ratio_var;


// ------------------------------------------------------------------------------------------ //
// cl_interp
// ------------------------------------------------------------------------------------------ //

class CBoundedCvar_Interp : public ConVar_ServerBounded
{
public:
	CBoundedCvar_Interp() :
#ifdef OF_CLIENT_DLL	
	  ConVar_ServerBounded( "cl_interp", 
		  "0.05", 
		  FCVAR_USERINFO | FCVAR_NOT_CONNECTED, 
		  "Sets the interpolation amount (bounded on low side by server interp ratio settings).", true, 0.0f, true, 0.5f )
#else
	  ConVar_ServerBounded( "cl_interp", 
		  "0.1", 
		  FCVAR_USERINFO | FCVAR_NOT_CONNECTED, 
		  "Sets the interpolation amount (bounded on low side by server interp ratio settings).", true, 0.0f, true, 0.5f )	
#endif
	  {
	  }

	  virtual float GetFloat() const
	  {
		  static const ConVar *pUpdateRate = g_pCVar->FindVar( "cl_updaterate" );
		  static const ConVar *pMin = g_pCVar->FindVar( "sv_client_min_interp_ratio" );
		  if ( pUpdateRate && pMin && pMin->GetFloat() != -1 )
		  {
#if defined ( OF_CLIENT_DLL )
			  static const ConVar_ServerBounded *pUpdateRateBounded = dynamic_cast<const ConVar_ServerBounded*>( pUpdateRate );
			  return MAX( GetBaseFloatValue(), pMin->GetFloat() / ( pUpdateRateBounded ? pUpdateRateBounded : pUpdateRate )->GetFloat() );
#else
			  return MAX( GetBaseFloatValue(), pMin->GetFloat() / pUpdateRate->GetFloat() );
#endif
		  }
		  else
		  {
			  return GetBaseFloatValue();
		  }
	  }
};

static CBoundedCvar_Interp cl_interp_var;
ConVar_ServerBounded *cl_interp = &cl_interp_var;

float GetClientInterpAmount()
{
	static const ConVar *pUpdateRate = g_pCVar->FindVar( "cl_updaterate" );
	if ( pUpdateRate )
	{
#if defined ( OF_CLIENT_DLL )
		static const ConVar_ServerBounded *pUpdateRateBounded = dynamic_cast<const ConVar_ServerBounded*>( pUpdateRate );
		return MAX( cl_interp->GetFloat(), cl_interp_ratio->GetFloat() / ( pUpdateRateBounded ? pUpdateRateBounded : pUpdateRate )->GetFloat() );
#else
		// #define FIXME_INTERP_RATIO
		return MAX( cl_interp->GetFloat(), cl_interp_ratio->GetFloat() / pUpdateRate->GetFloat() );
#endif
	}
	else
	{
		if ( !HushAsserts() )
		{
			AssertMsgOnce( false, "GetInterpolationAmount: can't get cl_updaterate cvar." );
		}
	
		return 0.1;
	}
}

