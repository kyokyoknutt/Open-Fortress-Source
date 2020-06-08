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

#undef fopen

#if defined( WIN32 ) && !defined( _X360 )
#include <windows.h> // SRC only!!
#include <io.h>
#endif

#include "customizationdialogsub.h"
#include <stdio.h>

#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>
#include "tier1/KeyValues.h"
#include <vgui_controls/Label.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/Cursor.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/FileOpenDialog.h>
#include <vgui_controls/MessageBox.h>
#include <vgui/IVGui.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>
#include <vgui_controls/MessageBox.h>

#include "CvarTextEntry.h"
#include "CvarToggleCheckButton.h"
#include "CvarSlider.h"
#include "LabeledCommandComboBox.h"
#include "filesystem.h"
#include "EngineInterface.h"
#include "BitmapImagePanel.h"
#include "utlbuffer.h"
#include "ModInfo.h"
#include "tier1/convar.h"

#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"
#include <setjmp.h>

#include "ivtex.h"

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

//-----------------------------------------------------------------------------
class MercenaryImagePanel : public ImagePanel
{
	typedef ImagePanel BaseClass;
public:
			 MercenaryImagePanel( Panel *parent, const char *name );
	virtual ~MercenaryImagePanel();

	virtual void Paint();

	void UpdateMercenary( int r, int g, int b );

protected:
	int m_R, m_G, m_B;

	// material
	int				m_iMercenaryTextureID;
	IVguiMatInfo	*m_pMercenary;
};

//-----------------------------------------------------------------------------
MercenaryImagePanel::MercenaryImagePanel( Panel *parent, const char *name ) : ImagePanel( parent, name )
{
	m_pMercenary = NULL;

	m_iMercenaryTextureID = vgui::surface()->CreateNewTextureID();
	ConVar *of_color_r = cvar->FindVar("of_color_r");
	ConVar *of_color_g = cvar->FindVar("of_color_g");
	ConVar *of_color_b = cvar->FindVar("of_color_b");
	if ( of_color_r && of_color_g && of_color_b )
		UpdateMercenary( of_color_r->GetInt(), of_color_g->GetInt(), of_color_b->GetInt() );
}

MercenaryImagePanel::~MercenaryImagePanel()
{
	if ( m_pMercenary )
	{
		delete m_pMercenary;
		m_pMercenary = NULL;
	}
}

//-----------------------------------------------------------------------------
void MercenaryImagePanel::UpdateMercenary( int r, int g, int b )
{
	m_R = r;
	m_G = g;
	m_B = b;

	ConVar *of_tennisball = cvar->FindVar( "of_tennisball" );
	if ( of_tennisball && of_tennisball->GetBool() )
		vgui::surface()->DrawSetTextureFile(m_iMercenaryTextureID, "vgui/vgui_mercenarytennisball", true, false );
	else
		vgui::surface()->DrawSetTextureFile(m_iMercenaryTextureID, "vgui/vgui_mercenarycolor", true, false );

	if ( m_pMercenary )
		delete m_pMercenary;

	m_pMercenary = vgui::surface()->DrawGetTextureMatInfoFactory( m_iMercenaryTextureID );
}

//-----------------------------------------------------------------------------
void MercenaryImagePanel::Paint()
{
	BaseClass::Paint();

	int wide, tall;
	GetSize( wide, tall );

	int iClipX0, iClipY0, iClipX1, iClipY1;
	ipanel()->GetClipRect(GetVPanel(), iClipX0, iClipY0, iClipX1, iClipY1 );

	float x, y;

	// assume square
	float flDrawWidth = (float)wide;
	int flHalfWidth = (int)(flDrawWidth / 2);

	x = wide / 2 - flHalfWidth;
	y = tall / 2 - flHalfWidth;

	ConVar *of_tennisball = cvar->FindVar( "of_tennisball" );
	if ( of_tennisball && of_tennisball->GetBool() )
		vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
	else
		vgui::surface()->DrawSetColor( m_R, m_G, m_B, 255 );
	vgui::surface()->DrawSetTexture( m_iMercenaryTextureID );
	vgui::surface()->DrawTexturedRect( x, y, x + flDrawWidth, y + flDrawWidth );
	vgui::surface()->DrawSetTexture(0);
}

//-----------------------------------------------------------------------------
class CosmeticImagePanel : public ImagePanel
{
	typedef ImagePanel BaseClass;
public:
			 CosmeticImagePanel( Panel *parent, const char *name );
	virtual ~CosmeticImagePanel();

	virtual void Paint();

	void UpdateCosmetic( int index );
	void SpawnCosmetic();

protected:
	// material
	int				m_iCosmeticIndex;
	int				m_iCosmeticTextureID;
	IVguiMatInfo	*m_pCosmetic;
};

//-----------------------------------------------------------------------------
CosmeticImagePanel::CosmeticImagePanel( Panel *parent, const char *name ) : ImagePanel( parent, name )
{
	m_pCosmetic = NULL;
	m_iCosmeticTextureID = vgui::surface()->CreateNewTextureID();
	m_iCosmeticIndex = -1;
	SpawnCosmetic();
}

CosmeticImagePanel::~CosmeticImagePanel()
{
	if ( m_pCosmetic )
	{
		delete m_pCosmetic;
		m_pCosmetic = NULL;
	}
}

void CosmeticImagePanel::SpawnCosmetic()
{
	ConVar *of_mercenary_hat = cvar->FindVar( "of_mercenary_hat" );
	if ( !of_mercenary_hat )
		return;

	m_iCosmeticIndex = of_mercenary_hat->GetInt();

	UpdateCosmetic( -1 );
}

//-----------------------------------------------------------------------------
void CosmeticImagePanel::UpdateCosmetic( int index )
{
	if ( index != -1 )
		m_iCosmeticIndex = index;

	char cosmeticname[64];

	switch ( m_iCosmeticIndex )
	{
		default:
		case 0:
			Q_strncpy( cosmeticname, "blank", sizeof( cosmeticname ) );
			break;
		case 1:
			Q_strncpy( cosmeticname, "camocap", sizeof( cosmeticname ) );
			break;
		case 2:
			Q_strncpy( cosmeticname, "hellmerc", sizeof( cosmeticname ) );
			break;
		case 3:
			Q_strncpy( cosmeticname, "western_hat", sizeof( cosmeticname ) );
			break;
		case 4:
			Q_strncpy( cosmeticname, "boomer_bucket", sizeof( cosmeticname ) );
			break;
		case 5:
			Q_strncpy( cosmeticname, "headset", sizeof( cosmeticname ) );
			break;
		case 6:
			Q_strncpy( cosmeticname, "surveyor", sizeof( cosmeticname ) );
			break;
		case 7:
			Q_strncpy( cosmeticname, "chicken_connoisseur", sizeof( cosmeticname ) );
			break;
		case 8:
			Q_strncpy( cosmeticname, "headband", sizeof( cosmeticname ) );
			break;
		case 9:
			Q_strncpy( cosmeticname, "pith_helmet", sizeof( cosmeticname ) );
			break;
		case 10:
			Q_strncpy( cosmeticname, "beret", sizeof( cosmeticname ) );
			break;
	}

	char texture[256];
	Q_snprintf( texture, sizeof(texture), "backpack/hats/%s", cosmeticname );

	vgui::surface()->DrawSetTextureFile( m_iCosmeticTextureID, texture, true, false );

	if ( m_pCosmetic )
		delete m_pCosmetic;

	m_pCosmetic = vgui::surface()->DrawGetTextureMatInfoFactory( m_iCosmeticTextureID );
}

//-----------------------------------------------------------------------------
void CosmeticImagePanel::Paint()
{
	BaseClass::Paint();

	int wide, tall;
	GetSize( wide, tall );

	int iClipX0, iClipY0, iClipX1, iClipY1;
	ipanel()->GetClipRect(GetVPanel(), iClipX0, iClipY0, iClipX1, iClipY1 );

	float x, y;

	// assume square
	float flDrawWidth = (float)wide;
	int flHalfWidth = (int)(flDrawWidth / 2);

	x = wide / 2 - flHalfWidth;
	y = tall / 2 - flHalfWidth;

	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
	vgui::surface()->DrawSetTexture( m_iCosmeticTextureID );
	vgui::surface()->DrawTexturedRect( x, y, x + flDrawWidth, y + flDrawWidth );
	vgui::surface()->DrawSetTexture(0);
}

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CCustomizationDialogSub::CCustomizationDialogSub(vgui::Panel *parent) : vgui::PropertyPage(parent, "CustomizationDialogSub") 
{
	Button *cancel = new Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Close" );

	Button *ok = new Button( this, "OK", "#GameUI_OK" );
	ok->SetCommand( "Ok" );

	Button *apply = new Button( this, "Apply", "#GameUI_Apply" );
	apply->SetCommand( "Apply" );

	// mercenary
	//==========
	m_pMercenaryRedSlider = new CCvarSlider( this, "Red Color Slider", "#TFOption_red",
		0.0f, 255.0f, "of_color_r" );
	m_pMercenaryGreenSlider = new CCvarSlider( this, "Green Color Slider", "#TFOption_green",
		0.0f, 255.0f, "of_color_g" );
	m_pMercenaryBlueSlider = new CCvarSlider( this, "Blue Color Slider", "#TFOption_blue",
		0.0f, 255.0f, "of_color_b" );

	m_pMercenaryRedSlider->AddActionSignalTarget( this );
	m_pMercenaryGreenSlider->AddActionSignalTarget( this );
	m_pMercenaryBlueSlider->AddActionSignalTarget( this );

	m_pMercenaryImage = new MercenaryImagePanel( this, "MercenaryImage" );
	m_pCosmeticImage = new CosmeticImagePanel(this, "CosmeticImage");
	m_pCosmeticList = new CLabeledCommandComboBox( this, "CosmeticList" );
	m_pTennisballCheckbox = new CCvarToggleCheckButton( this, "TennisballCheckbox", "#OF_Options_Tennisball", "of_tennisball" );
	m_pAnnouncerEventCheckbox = new CCvarToggleCheckButton( this, "AnnouncerEventCheckbox", "#OF_Options_AnnouncerEvents", "of_announcer_events" );
	m_pSpawnParticleList = new CLabeledCommandComboBox( this, "SpawnParticleList" );
	m_pAnnouncerList = new CLabeledCommandComboBox( this, "AnnouncerList" );

	InitCosmeticList( m_pCosmeticList );
	InitSpawnParticleList( m_pSpawnParticleList );
	InitAnnouncerList( m_pAnnouncerList );

	ConVar *of_mercenary_hat = cvar->FindVar( "of_mercenary_hat" );
	if ( of_mercenary_hat )
		RedrawCosmeticImage( of_mercenary_hat->GetInt() );
	else
		RedrawCosmeticImage( -1 );

	RedrawMercenaryImage();

	//=========

	LoadControlSettings("Resource/customizationdialogsub.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCustomizationDialogSub::~CCustomizationDialogSub()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCustomizationDialogSub::InitAnnouncerList( CLabeledCommandComboBox *cb )
{
	cb->DeleteAllItems();

	ConVar *of_announcer_override = cvar->FindVar("of_announcer_override");
	if ( !of_announcer_override )
		return;

	// hardcoded since this customization panel is temporary
	cb->AddItem( "Benjamoose (default)", "of_announcer_override Benja");
	cb->AddItem( "TF2 Announcer", "of_announcer_override Announcer" );
	cb->AddItem( "Tyler McVicker", "of_announcer_override Tyler" );
	cb->AddItem( "HECU", "of_announcer_override HECU" );

	const char *announcer = of_announcer_override->GetString();

	if ( announcer )
	{
		if ( V_strcmp( announcer, "Benja" ) == 0 )
			cb->SetInitialItem( 0 );
		else if ( V_strcmp( announcer, "Announcer" ) == 0 )
			cb->SetInitialItem( 1 );
		else if ( V_strcmp( announcer, "Tyler" ) == 0 )
			cb->SetInitialItem( 2 );
		else if ( V_strcmp( announcer, "HECU" ) == 0 )
			cb->SetInitialItem( 3 );
		else
			cb->SetInitialItem( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCustomizationDialogSub::InitCosmeticList( CLabeledCommandComboBox *cb )
{
	cb->DeleteAllItems();

	ConVar *of_mercenary_hat = cvar->FindVar("of_mercenary_hat");
	if ( !of_mercenary_hat )
		return;

	// hardcoded since this customization panel is temporary
	cb->AddItem( "None", "of_mercenary_hat 0" );
	cb->AddItem( "Camouflaged Cap", "of_mercenary_hat 1" );
	cb->AddItem( "The Helmet", "of_mercenary_hat 2" );
	cb->AddItem( "The Eastwood", "of_mercenary_hat 3" );
	cb->AddItem( "Boomer Bucket", "of_mercenary_hat 4" );
	cb->AddItem( "Hardline Headset", "of_mercenary_hat 5" );
	cb->AddItem( "Societal Surveyor", "of_mercenary_hat 6" );
	cb->AddItem( "Chicken Connoisseur", "of_mercenary_hat 7");
	cb->AddItem( "Headhunter's Headband", "of_mercenary_hat 8" );
	cb->AddItem( "Crusader's Canopy", "of_mercenary_hat 9" );
	cb->AddItem( "Badlands Beret", "of_mercenary_hat 10" );

	cb->SetInitialItem( of_mercenary_hat->GetInt() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCustomizationDialogSub::InitSpawnParticleList( CLabeledCommandComboBox *cb )
{
	cb->DeleteAllItems();

	ConVar *of_respawn_particle = cvar->FindVar("of_respawn_particle");
	if ( !of_respawn_particle )
		return;

	// hardcoded since this customization panel is temporary
	cb->AddItem( "None", "of_respawn_particle 0");
	cb->AddItem( "#TF_DM_PARTICLE01", "of_respawn_particle 1" );
	cb->AddItem( "#TF_DM_PARTICLE02", "of_respawn_particle 2" );
	cb->AddItem( "#TF_DM_PARTICLE03", "of_respawn_particle 3" );
	cb->AddItem( "#TF_DM_PARTICLE04", "of_respawn_particle 4" );
	cb->AddItem( "#TF_DM_PARTICLE05", "of_respawn_particle 5" );
	cb->AddItem( "#TF_DM_PARTICLE06", "of_respawn_particle 6" );
	cb->AddItem( "#TF_DM_PARTICLE07", "of_respawn_particle 7" );
	cb->AddItem( "#TF_DM_PARTICLE08", "of_respawn_particle 8" );
	cb->AddItem( "#TF_DM_PARTICLE09", "of_respawn_particle 9" );
	cb->AddItem( "#TF_DM_PARTICLE10", "of_respawn_particle 10" );
	cb->AddItem( "#TF_DM_PARTICLE11", "of_respawn_particle 11" );
	cb->AddItem( "#TF_DM_PARTICLE12", "of_respawn_particle 12" );
	cb->AddItem( "#TF_DM_PARTICLE13", "of_respawn_particle 13" );
	cb->AddItem( "#TF_DM_PARTICLE14", "of_respawn_particle 14" );
	cb->AddItem( "#TF_DM_PARTICLE15", "of_respawn_particle 15" );
	cb->AddItem( "#TF_DM_PARTICLE16", "of_respawn_particle 16" );
	cb->AddItem( "#TF_DM_PARTICLE17", "of_respawn_particle 17" );
	cb->AddItem( "#TF_DM_PARTICLE18", "of_respawn_particle 18" );
	cb->AddItem( "#TF_DM_PARTICLE19", "of_respawn_particle 19" );
	cb->AddItem( "#TF_DM_PARTICLE20", "of_respawn_particle 20" );
	cb->AddItem( "#TF_DM_PARTICLE21", "of_respawn_particle 21" );
	cb->AddItem( "#TF_DM_PARTICLE22", "of_respawn_particle 22" );
	cb->AddItem( "#TF_DM_PARTICLE23", "of_respawn_particle 23" );
	cb->AddItem( "#TF_DM_PARTICLE24", "of_respawn_particle 24" );
	cb->AddItem( "#TF_DM_PARTICLE25", "of_respawn_particle 25" );
	cb->AddItem( "#TF_DM_PARTICLE26", "of_respawn_particle 26" );
	cb->AddItem( "#TF_DM_PARTICLE27", "of_respawn_particle 27" );
	cb->AddItem( "#TF_DM_PARTICLE28", "of_respawn_particle 28" );
	cb->AddItem( "#TF_DM_PARTICLE29", "of_respawn_particle 29" );
	cb->AddItem( "#TF_DM_PARTICLE30", "of_respawn_particle 30" );
	cb->AddItem( "#TF_DM_PARTICLE31", "of_respawn_particle 31" );
	cb->AddItem( "#TF_DM_PARTICLE32", "of_respawn_particle 32" );
	cb->AddItem( "#TF_DM_PARTICLE33", "of_respawn_particle 33" );
	cb->AddItem( "#TF_DM_PARTICLE34", "of_respawn_particle 34" );
	cb->AddItem( "#TF_DM_PARTICLE35", "of_respawn_particle 35" );

	cb->SetInitialItem( of_respawn_particle->GetInt() );
}

//-----------------------------------------------------------------------------
void CCustomizationDialogSub::RedrawMercenaryImage()
{
	// get the color selected in the combo box.
	int r,g,b;

	r = clamp( m_pMercenaryRedSlider->GetSliderValue(), 0, 255 );
	g = clamp( m_pMercenaryGreenSlider->GetSliderValue(), 0, 255 );
	b = clamp( m_pMercenaryBlueSlider->GetSliderValue(), 0, 255 );

	if ( m_pMercenaryImage )
	{
		m_pMercenaryImage->UpdateMercenary( r, g, b );
	}
}

//-----------------------------------------------------------------------------
void CCustomizationDialogSub::RedrawCosmeticImage( int index )
{
	if ( m_pCosmeticImage )
	{
		m_pCosmeticImage->UpdateCosmetic( index );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCustomizationDialogSub::OnCommand( const char *command )
{
	BaseClass::OnCommand( command );
}


//-----------------------------------------------------------------------------
// Purpose: Called whenever text of a panel changes
//-----------------------------------------------------------------------------
void CCustomizationDialogSub::OnTextChanged(vgui::Panel *panel)
{
	RedrawMercenaryImage();

	if ( m_pCosmeticList )
		RedrawCosmeticImage( m_pCosmeticList->m_iCurrentSelection );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCustomizationDialogSub::OnSliderMoved(KeyValues *data)
{
	RedrawMercenaryImage();

	if ( m_pCosmeticList )
		RedrawCosmeticImage( m_pCosmeticList->m_iCurrentSelection );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCustomizationDialogSub::OnApplyButtonEnable()
{
	PostMessage(GetParent(), new KeyValues("ApplyButtonEnable"));
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCustomizationDialogSub::OnResetData()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCustomizationDialogSub::OnApplyChanges()
{
	if ( m_pMercenaryRedSlider )
		m_pMercenaryRedSlider->ApplyChanges();
	if ( m_pMercenaryGreenSlider )
		m_pMercenaryGreenSlider->ApplyChanges();
	if ( m_pMercenaryBlueSlider )
		m_pMercenaryBlueSlider->ApplyChanges();
	if ( m_pCosmeticList && m_pCosmeticList->GetActiveItemCommand() )
		engine->ClientCmd_Unrestricted( m_pCosmeticList->GetActiveItemCommand() );
	if ( m_pSpawnParticleList && m_pSpawnParticleList->GetActiveItemCommand() )
		engine->ClientCmd_Unrestricted( m_pSpawnParticleList->GetActiveItemCommand() );
	if ( m_pAnnouncerList && m_pAnnouncerList->GetActiveItemCommand() )
		engine->ClientCmd_Unrestricted( m_pAnnouncerList->GetActiveItemCommand() );
	if ( m_pTennisballCheckbox )
		m_pTennisballCheckbox->ApplyChanges();
	if ( m_pAnnouncerEventCheckbox )
		m_pAnnouncerEventCheckbox->ApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCustomizationDialogSub::OnButtonChecked()
{
	if ( m_pTennisballCheckbox )
	{
		m_pTennisballCheckbox->ApplyChanges();

		if ( m_pMercenaryImage )
		{
			int r,g,b;

			r = clamp( m_pMercenaryRedSlider->GetSliderValue(), 0, 255 );
			g = clamp( m_pMercenaryGreenSlider->GetSliderValue(), 0, 255 );
			b = clamp( m_pMercenaryBlueSlider->GetSliderValue(), 0, 255 );

			m_pMercenaryImage->UpdateMercenary( r, g, b );
		}
	}

	if ( m_pAnnouncerEventCheckbox )
		m_pAnnouncerEventCheckbox->ApplyChanges();
}

