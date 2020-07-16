//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef OF_COLORPANEL2_H
#define OF_COLORPANEL2_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Slider.h>

class CTFColorPanel;
namespace vgui
{
	class CTFColorSlider : public Slider
	{
		DECLARE_CLASS_SIMPLE(CTFColorSlider, Slider);
	public:
		CTFColorSlider(Panel *parent, const char *panelName);
		virtual void SetValue(int value, bool bTriggerChangeMessage = true);
		void SetValueRaw(int value, bool bTriggerChangeMessage = true);
		virtual void DrawNob();
		virtual void PaintBackground();
		virtual void ApplySettings(KeyValues *inResourceData);
	private:
		CTFColorPanel *pParent;
		int iSliderTextureID;
		int iSliderWidth;
	};
}

class CTFColorPanel : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE(CTFColorPanel, vgui::EditablePanel);
public:
	CTFColorPanel(Panel *parent, const char *panelName);
	void OnColorChanged(bool bTriggerChangeMessage = true, bool bUpdateHexValue = true);
	virtual void PaintBackground();
	virtual void OnThink();
	virtual void ApplySettings(KeyValues *inResourceData);
	bool ShouldUpdateHex(void) { return bUpdateHexValue; };
	void RecalculateColorValues();
private:
	MESSAGE_FUNC_PTR(OnControlModified, "ControlModified", panel);
	MESSAGE_FUNC_PTR(OnTextChanged, "TextChanged", panel);
	MESSAGE_FUNC_PTR(OnCheckButtonChecked, "CheckButtonChecked", panel)
	{
		OnControlModified(panel);
	}

	vgui::CTFColorSlider *pHue;
	vgui::CTFColorSlider *pSaturation;
	vgui::CTFColorSlider *pBrightness;

	vgui::TextEntry *pHueEntry;
	vgui::TextEntry *pSaturationEntry;
	vgui::TextEntry *pBrightnessEntry;

	vgui::CTFColorSlider *pRed;
	vgui::CTFColorSlider *pGreen;
	vgui::CTFColorSlider *pBlue;

	vgui::TextEntry *pRedEntry;
	vgui::TextEntry *pGreenEntry;
	vgui::TextEntry *pBlueEntry;

	vgui::TextEntry *pHexEntry;

	Color cHueS;
	Color cHueB;
	Color cHueBnoS;

	int iCurrRed;
	int iCurrGreen;
	int iCurrBlue;

	CCvarToggleCheckButton *pRGBToggle;

	bool bUpdateHexValue;

	bool bRGBOn;

	bool bReset;
};


#endif // OF_LOADOUT_H
