//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_HEADERITEM_H
#define OF_HEADERITEM_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Slider.h>
#include "of_editablebutton.h"
#include "tf_controls.h"

class CStudioHdr;
class CCvarToggleCheckButton;
class CModelPanel;

class CTFHeaderItem : public CTFEditableButton
{
private:
	DECLARE_CLASS_SIMPLE( CTFHeaderItem, CTFEditableButton );

public:
	CTFHeaderItem(vgui::Panel *parent, const char *panelName);	 

	virtual void ApplySettings(KeyValues *inResourceData);
	virtual void OnReleasedSelected();
	virtual void OnReleasedUnselected();
	virtual void PerformLayout();
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void SetSelected( bool bSelected );
	void	SetConnectedPanel( const char *szConnectedPanel );
	void CalculateBoxSize();
public:
	vgui::Panel		*pConnectedPanel;

	int iBaseHeight;
	vgui::HFont 	m_hTextFont;
	CExLabel		*pLabel;
};

#endif // OF_LOADOUT_H
