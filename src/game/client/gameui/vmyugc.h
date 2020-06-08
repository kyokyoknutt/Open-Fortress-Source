#ifndef __VMYUGC_H__
#define __VMYUGC_H__

#include "basemodui.h"
#include "VGenericPanelList.h"
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/FileOpenDialog.h>
#include <vgui_controls/MessageBox.h>
#include "gameui_util.h"

class CNB_Button;
class CNB_Header_Footer;

namespace BaseModUI {

enum PopUpType
{
	POPUP_CREATE,
	POPUP_EDIT,
	POPUP_DELETE,
	POPUP_PUBLISH
};

enum ConversionErrorType
{
	CE_SUCCESS,
	CE_MEMORY_ERROR,
	CE_CANT_OPEN_SOURCE_FILE,
	CE_ERROR_PARSING_SOURCE,
	CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED,
	CE_ERROR_WRITING_OUTPUT_FILE,
	CE_ERROR_LOADING_DLL
};

class MyUGCEntry;

class MyUGCPopUp : public CBaseModFrame
{
	DECLARE_CLASS_SIMPLE( MyUGCPopUp, CBaseModFrame );

public:
	MyUGCPopUp( vgui::Panel *parent, const char *panelName, unsigned int type );
	~MyUGCPopUp();
	void OnCommand( const char *command );
	virtual void OnMessage(const KeyValues *params, vgui::VPANEL ifromPanel);
	MESSAGE_FUNC_CHARPTR( OnFileSelected, "FileSelected", fullpath );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	ConversionErrorType SConvertJPEGToTGA( const char *jpegpath, const char *tgaPath );
	ConversionErrorType ConvertTGAToVTF( const char *tgaPath );
	ConversionErrorType WriteVMT( const char *vtfPath );

	CNB_Header_Footer *m_pHeaderFooter;
	MyUGCEntry *m_UGCNameEntry;
	vgui::FileOpenDialog *m_hImportDialog;
	vgui::ImagePanel *m_ImgUGC;
	unsigned int m_nType;
};

class MyUGCEntry : public vgui::TextEntry
{
	typedef vgui::TextEntry BaseClass;
public:
	MyUGCEntry( vgui::Panel *parent, char const *panelName )
		: BaseClass( parent, panelName )
	{
		SetCatchEnterKey( true );
		SetAllowNonAsciiCharacters( true );
		SetDrawLanguageIDAtLeft( true );
	}

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings(pScheme);

		SetPaintBorderEnabled( false );
	}

	virtual void OnKeyCodeTyped(vgui::KeyCode code)
	{
		PostMessage(GetParent(), new KeyValues("OnType"));
		BaseClass::OnKeyCodeTyped( code );
	}
};

class UGCListItem : public vgui::EditablePanel, IGenericPanelListItem
{
	DECLARE_CLASS_SIMPLE( UGCListItem, vgui::EditablePanel );

public:
	UGCListItem(vgui::Panel *parent, const char *panelName);
	~UGCListItem();
	void SetName( const char* name );
	void SetImage( const char* name );
	void SetPublished( bool bPublished );
	bool IsPublished( void ) { return m_bPublished; }

	// Inherited from IGenericPanelListItem
	virtual bool IsLabel() { return false; }
	void OnMousePressed( vgui::MouseCode code );
	virtual void OnMessage(const KeyValues *params, vgui::VPANEL ifromPanel);
	virtual void Paint();

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	vgui::Label* m_LblName;
	vgui::HFont	m_hTextFont;
	vgui::ImagePanel* m_ImgUGC;
	bool m_bPublished;
	bool m_bCurrentlySelected;

	CPanelAnimationVarAliasType( float, m_flDetailsExtraHeight, "DetailsExtraHeight", "0", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDetailsRowHeight, "DetailsRowHeight", "0", "proportional_float" );
};

class MyUGC : public CBaseModFrame
{
	DECLARE_CLASS_SIMPLE( MyUGC, CBaseModFrame );

public:
	MyUGC(vgui::Panel *parent, const char *panelName);
	~MyUGC();
	void Activate();
	void PaintBackground( void );
	void OnCommand( const char *command );
	virtual void OnMessage(const KeyValues *params, vgui::VPANEL ifromPanel);

	virtual void OnThink();

protected:
	void ApplySchemeSettings(vgui::IScheme *pScheme);

private:
	void UpdateFooter( void );
	void OnPublishedUGCReceived( SteamUGCQueryCompleted_t *pPublishedUGC, bool bIOFailure );

	CNB_Header_Footer *m_pHeaderFooter;
	GenericPanelList* m_GplUGC;
	CCallResult< MyUGC, SteamUGCQueryCompleted_t > m_SteamCallResultPublishedUGC;
	vgui::Label *m_LblNoUGC;
	KeyValues *m_pUGCList;
	MyUGCPopUp *m_PopUpPanel;
//	CUtlVector<Addons::AddonInfo> m_addonInfoList;
};

}; //BaseModUI

#endif
