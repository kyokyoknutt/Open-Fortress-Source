//
//
// TODO: REMOVE THIS FILE WHEN THE LOADOUT IS ADDED!
//
//

//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CUSTOMIZATIONDIALOGSUB_H
#define CUSTOMIZATIONDIALOGSUB_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>

class CLabeledCommandComboBox;
class CBitmapImagePanel;

class CCvarToggleCheckButton;
class CCvarTextEntry;
class CCvarSlider;

class CMultiplayerAdvancedDialog;

class MercenaryImagePanel;
class CosmeticImagePanel;

//-----------------------------------------------------------------------------
// Purpose: multiplayer options property page
//-----------------------------------------------------------------------------
class CCustomizationDialogSub : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( CCustomizationDialogSub, vgui::PropertyPage );

public:
	CCustomizationDialogSub(vgui::Panel *parent);
	~CCustomizationDialogSub();

protected:
	// Called when page is loaded.  Data should be reloaded from document into controls.
	virtual void OnResetData();
	// Called when the OK / Apply button is pressed.  Changed data should be written into document.
	virtual void OnApplyChanges();

	virtual void OnCommand( const char *command );

private:
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", data );
	MESSAGE_FUNC( OnApplyButtonEnable, "ControlModified" );
	MESSAGE_FUNC( OnButtonChecked, "CheckButtonChecked" );

	MercenaryImagePanel *m_pMercenaryImage;
	CCvarSlider *m_pMercenaryRedSlider;		
	CCvarSlider *m_pMercenaryBlueSlider;
	CCvarSlider *m_pMercenaryGreenSlider;
	void RedrawMercenaryImage();

	CLabeledCommandComboBox *m_pCosmeticList;
	void InitCosmeticList( CLabeledCommandComboBox *cb );

	CLabeledCommandComboBox *m_pSpawnParticleList;
	void InitSpawnParticleList( CLabeledCommandComboBox *cb );

	CLabeledCommandComboBox *m_pAnnouncerList;
	void InitAnnouncerList( CLabeledCommandComboBox *cb );

	CosmeticImagePanel *m_pCosmeticImage;
	void RedrawCosmeticImage( int index );

	CCvarToggleCheckButton *m_pTennisballCheckbox;
	CCvarToggleCheckButton *m_pAnnouncerEventCheckbox;
};

#endif // OPTIONSSUBMULTIPLAYER_H
