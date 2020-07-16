//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_MODEL_PANEL_H
#define OF_MODEL_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "basemodel_panel.h"

class CStudioHdr;
class CCvarToggleCheckButton;
class CModelPanel;

namespace vgui
{
	class CTFModelPanel : public CBaseModelPanel
	{
		DECLARE_CLASS_SIMPLE(CTFModelPanel, CBaseModelPanel);

	public:
		CTFModelPanel(vgui::Panel *pParent, const char *pszName);

		virtual void	ApplySettings(KeyValues *inResourceData);
		virtual void	OnThink();
		virtual void	Update();
		virtual void	Paint();

		virtual void	SetModelName(const char* pszModelName, int nSkin = 0);
		virtual void	SetAnimationIndex(int index) { m_iAnimationIndex = index; };
		void SetParticleName(const char* name);
		void SetBodygroup(int iGroup, int iValue);
		int GetBodygroup(int iGroup);

		const char *GetBodygroupName(int iGroup);
		int FindBodygroupByName(const char *name);
		int GetBodygroupCount(int iGroup);
		int GetNumBodyGroups(void);

		Vector			m_vecDefPosition;
		QAngle			m_vecDefAngles;

		void			RefreshModel();
		CStudioHdr		*GetModelPtr();
		int				m_iAnimationIndex;
		float			m_flParticleZOffset;
		bool			m_bLoopParticle;
		float			m_flLoopTime;
		float			m_flLoopParticleAfter;
		char			szLoopingParticle[128];
	private:
		CStudioHdr		m_StudioHdr;
		particle_data_t *m_pData;
	};
}

#endif // OF_LOADOUT_H
