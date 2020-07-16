#ifndef TF_HUD_MEDALS_H
#define TF_HUD_MEDALS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>

using namespace vgui;

#define TF_MAX_FILENAME_LENGTH	128

//-----------------------------------------------------------------------------
// Purpose:  Displays Deathmatch Medals
//-----------------------------------------------------------------------------

enum { FIRSTBLOOD,	PERFECT,		IMPRESSIVE,		PERFORATED,		HUMILIATION,	KAMIKAZE,	MIDAIR,				HEADSHOT,		EXCELLENT,		MULTIKILL,	ULTRAKILL,
	   HOLYSHIT,	KILLINGSPREE,	RAMPAGE,		DOMINATING,		UNSTOPPABLE,	GODLIKE,	POWERUPMASSACRE,	SHOWSTOPPER,	PARTYBREAKER,	DENIED					};

class CTFHudMedals : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE(CTFHudMedals, EditablePanel);

public:

	static const char *medalNames[DENIED + 1];
	static const char *medalPaths[DENIED + 1];

	CTFHudMedals(const char *pElementName);

	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void FireGameEvent(IGameEvent *event);
	virtual void Reset();
	virtual bool ShouldDraw(void);
	virtual int GetMedalCount(int medalIndex);

protected:

	float	drawTime;

	struct  medal_info
	{
		const char *medal_name;
		const char *medal_sound;
	};
	CUtlVector<medal_info> medalsQueue;

	virtual void AddMedal(int medalIndex);
	virtual void OnThink(void);

private:

	bool died;
	int medals_counter[DENIED + 1];
	
	ImagePanel *m_pMedalImage;
};

#endif	// TF_HUD_MEDALS_H