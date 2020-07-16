//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_LOADOUT_H
#define OF_LOADOUT_H
#ifdef _WIN32
#pragma once
#endif

#include "of_loadoutheader.h"
#include "of_dmmodelpanel.h"
#include "of_scrollablepanellist.h"
#include "of_selectionpanel.h"
#include "basemodframe.h"

class CStudioHdr;
class CCvarToggleCheckButton;
class CModelPanel;

namespace BaseModUI
{

	class DMLoadout : public CBaseModFrame
	{
	private:
		DECLARE_CLASS_SIMPLE(DMLoadout, CBaseModFrame);

	public:
		DMLoadout(Panel *parent, const char *panelName);

		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void ApplySettings(KeyValues* inResourceData);
		virtual void OnCommand(const char * command);
		virtual void PerformLayout();
		virtual void PaintBackground();
		void SelectWeapon(int iSlot, const char *szWeapon, bool bChangeSelection = false);
		vgui::DMModelPanel *GetClassModel() { return m_pClassModel; };
		vgui::EditablePanel *GetArsenalPanel() { return pArsenalPanel; };
	private:

		vgui::Button *m_pNextTipButton;
		vgui::Button *m_pCloseButton;

		CUtlVector<CTFScrollableItemList*> m_pItemCategories;
		CTFLoadoutHeader *m_pItemHeader;
		vgui::EditablePanel *pCosmeticPanel;
		vgui::EditablePanel *pArsenalPanel;
		vgui::EditablePanel *pVisualPanel;
		CTFScrollablePanelList *pParticleList;
		CTFScrollablePanelList *pAnnouncerList;

		CTFSelectionPanel *pPrimaryToggle;
		CTFSelectionPanel *pSecondaryToggle;
		CTFSelectionPanel *pMeleeToggle;

		CTFScrollablePanelList *pWeaponList[3];

		vgui::DMModelPanel *m_pClassModel;

		bool m_bInteractive;							// are we in interactive mode
		bool m_bControlsLoaded;							// have we loaded controls yet

		bool m_bParsedParticles; // this is only used so that particles dont crash 
								 // the memory when we reload the panel
	public:
		CTFSelectionPanel *m_pSelectedOptions;
	};
}

#endif // OF_LOADOUT_H
