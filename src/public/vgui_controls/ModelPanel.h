//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ANIMCONTMODELPANEL_H
#define ANIMCONTMODELPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui_controls/EditablePanel.h>
#include "tier1/interface.h"
#include "KeyValues.h"

namespace vgui
{
	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	abstract_class AnimContModelPanel : public IBaseInterface
	{
		public:
			
			virtual void SetHUDModelPos(float x, float y, float z){ return; };
			virtual void GetHUDModelPos(float &x, float &y, float &z){ return; };
			
			virtual void SetHUDModelAng(float x, float y, float z){ return; };
			virtual void GetHUDModelAng(float &x, float &y, float &z){ return; };			
			
			virtual void ResetAnim(){ return; };
	};
}
#endif // ANIMCONTMODELPANEL_H
