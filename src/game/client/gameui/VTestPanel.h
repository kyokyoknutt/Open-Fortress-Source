//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#ifndef __VTestPanel_H__
#define __VTestPanel_H__


#include "basemodui.h"
#include "VFlyoutMenu.h"


namespace BaseModUI {

class DropDownMenu;
class SliderControl;
class BaseModHybridButton;

class TestPanel : public CBaseModFrame
{
	DECLARE_CLASS_SIMPLE( TestPanel, CBaseModFrame );

public:
	TestPanel(vgui::Panel *parent, const char *panelName);
	~TestPanel();

protected:
	virtual void PaintBackground();
	virtual void ApplySchemeSettings( vgui::IScheme* pScheme );
	virtual void OnCommand( const char *command );
};

};

#endif // __VTestPanel_H__