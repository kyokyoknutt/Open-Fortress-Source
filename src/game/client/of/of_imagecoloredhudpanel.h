#include "vgui_controls/ScalableImagePanel.h"

#define MAX_BG_LENGTH		128

class CTFImageColoredHudPanel : public vgui::ScalableImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFImageColoredHudPanel, vgui::ScalableImagePanel );

	CTFImageColoredHudPanel(vgui::Panel *parent, const char *name);

	virtual void PaintBackground();
};
