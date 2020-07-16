#include "cbase.h"
#include "vmyugc.h"
#include "vgui/ISurface.h"
#include "filesystem.h"
#include "VFooterPanel.h"
#include "vhybridbutton.h"
#include "nb_header_footer.h"
#include "nb_button.h"

// use the JPEGLIB_USE_STDIO define so that we can read in jpeg's from outside the game directory tree.  For Spray Import.
#define JPEGLIB_USE_STDIO
#include "jpeglib/jpeglib.h"
#undef JPEGLIB_USE_STDIO
#include <setjmp.h>
#include "bitmap/tgawriter.h"
#include "ivtex.h"
#include "vgetlegacydata.h"

using namespace vgui;
using namespace BaseModUI;

//We need it later
void GetPrimaryModDirectory( char *pcModPath, int nSize );

//=============================================================================
MyUGCPopUp::MyUGCPopUp( Panel *parent, const char *panelName, unsigned int type ):
BaseClass( parent, panelName, false, true )
{
	GameUI().PreventEngineHideGameUI();

	SetDeleteSelfOnClose(true);
	SetProportional( true );

	m_pHeaderFooter = new CNB_Header_Footer( this, "HeaderFooter" );
	m_pHeaderFooter->SetTitle( "" );
	m_pHeaderFooter->SetHeaderEnabled( false );
	m_pHeaderFooter->SetGradientBarEnabled( true );
	m_pHeaderFooter->SetGradientBarPos( 75, 350 );

	m_UGCNameEntry = new MyUGCEntry(this, "UGCNameEntry");
	//60 because yes
	m_UGCNameEntry->SetMaximumCharCount(60);

	m_hImportDialog = NULL;

	m_ImgUGC = new ImagePanel(this, "ImgUGC");

	CBaseModFooterPanel *footer = BaseModUI::CBaseModPanel::GetSingleton().GetFooterPanel();
	if ( footer )
	{
		footer->SetButtons( FB_BBUTTON, FF_AB_ONLY, false );
		footer->SetButtonText( FB_BBUTTON, "#L4D360UI_Done" );
	}

	m_nType = type;
}

MyUGCPopUp::~MyUGCPopUp()
{
	delete m_pHeaderFooter;
	delete m_UGCNameEntry;
}

//=============================================================================
void MyUGCPopUp::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	switch (m_nType)
	{
	case POPUP_CREATE:
		LoadControlSettings("Resource/UI/BaseModUI/MyUGCPopUpCreate.res");
	default:
		break;
	}
}

//=============================================================================
void MyUGCPopUp::OnCommand( const char *command )
{
	if( V_strcmp( command, "Back" ) == 0 )
	{
		OnKeyCodePressed( KEY_XBUTTON_B );
	}
	else if ( !V_strcmp( command, "Import" ) ) 
	{
		if (m_hImportDialog == NULL)
		{
			m_hImportDialog = new FileOpenDialog(NULL, "Import Thumbnail", true);
			m_hImportDialog->AddFilter("*.tga,*.jpg,*.bmp,*.vtf", "#GameUI_All_Images", true);
			m_hImportDialog->AddFilter("*.tga", "#GameUI_TGA_Images", false);
			m_hImportDialog->AddFilter("*.jpg", "#GameUI_JPEG_Images", false);
			m_hImportDialog->AddFilter("*.bmp", "#GameUI_BMP_Images", false);
			m_hImportDialog->AddFilter("*.vtf", "#GameUI_VTF_Images", false);
			m_hImportDialog->AddActionSignalTarget(this);
		}
		m_hImportDialog->DoModal(false);
		m_hImportDialog->Activate();
	}
	else if ( !V_strcmp( command, "Done" ) ) 
	{

	}
	else
	{
		BaseClass::OnCommand( command );
	}	
}

void MyUGCPopUp::OnMessage( const KeyValues *params, vgui::VPANEL ifromPanel )
{
	BaseClass::OnMessage( params, ifromPanel );
	
	if ( V_strcmp( params->GetName(), "OnType" ) == 0 ) 
	{
		if ( m_UGCNameEntry->GetTextLength() )
		{
			vgui::ImagePanel *pIcon = dynamic_cast< vgui::ImagePanel* >(FindChildByName("IconForwardArrow"));
			if ( pIcon )
				pIcon->SetVisible( true );

			BaseModHybridButton* pButton = (BaseModHybridButton *)FindChildByName("BtnDone");
			if (pButton)
				pButton->SetVisible( true );
		}
		else
		{
			vgui::ImagePanel *pIcon = dynamic_cast< vgui::ImagePanel* >(FindChildByName("IconForwardArrow"));
			if ( pIcon )
				pIcon->SetVisible( false );

			BaseModHybridButton* pButton = (BaseModHybridButton *)FindChildByName("BtnDone");
			if (pButton)
				pButton->SetVisible( false );
		}
	}
}


// file selected.  This can only happen when someone selects an image to be imported as a spray logo.
void MyUGCPopUp::OnFileSelected( const char *fullpath )
{
	if ((fullpath == NULL) || (fullpath[0] == 0))
	{
		return;
	}

	char tgaFilename[MAX_PATH];
	char vtfFilename[MAX_PATH];
	char modPath[MAX_PATH];

	GetPrimaryModDirectory( modPath, MAX_PATH );
	V_snprintf( tgaFilename, sizeof( tgaFilename ), "%s%s%s%c%s", modPath, "materials/vgui/", "ugctemp", CORRECT_PATH_SEPARATOR, "addonimage.tga" );	

	V_snprintf( vtfFilename, sizeof( vtfFilename ), "%s%s%s%c%s", modPath, "materials/vgui/", "ugctemp", CORRECT_PATH_SEPARATOR, "addonimage.vtf" );

	if ( CE_SUCCESS == SConvertJPEGToTGA( fullpath, tgaFilename) )
	{
		if ( CE_SUCCESS == ConvertTGAToVTF( tgaFilename ) )
		{
			if ( CE_SUCCESS == WriteVMT( vtfFilename ) )
			{
				m_ImgUGC->SetImage("ugctemp/addonimage");
				m_ImgUGC->SetVisible(true);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Everything below was copied from the UI options page for converting sprays.
// TODO: Move these functions to a library so that they can be shared more 
//       sanely.
//-----------------------------------------------------------------------------
struct ValveJpegErrorHandler_t 
{
	// The default manager
	struct jpeg_error_mgr	m_Base;
	// For handling any errors
	jmp_buf					m_ErrorContext;
};

//-----------------------------------------------------------------------------
// Purpose: We'll override the default error handler so we can deal with errors without having to exit the engine
//-----------------------------------------------------------------------------
static void ValveJpegErrorHandler( j_common_ptr cinfo )
{
	ValveJpegErrorHandler_t *pError = reinterpret_cast< ValveJpegErrorHandler_t * >( cinfo->err );

	char buffer[ JMSG_LENGTH_MAX ];

	/* Create the message */
	( *cinfo->err->format_message )( cinfo, buffer );

	Warning( "%s\n", buffer );

	// Bail
	longjmp( pError->m_ErrorContext, 1 );
}

// convert the JPEG file given to a TGA file at the given output path.
ConversionErrorType MyUGCPopUp::SConvertJPEGToTGA( const char *jpegpath, const char *tgaPath )
{
#if !defined( _X360 )
	struct jpeg_decompress_struct jpegInfo;
	struct ValveJpegErrorHandler_t jerr;
	JSAMPROW row_pointer[1];
	int row_stride;
	int cur_row = 0;

	// image attributes
	int image_height;
	int image_width;

	// open the jpeg image file.
	FILE *infile = fopen(jpegpath, "rb");
	if (infile == NULL)
	{
		return CE_CANT_OPEN_SOURCE_FILE;
	}

	// setup error to print to stderr.
	jpegInfo.err = jpeg_std_error(&jerr.m_Base);

	jpegInfo.err->error_exit = &ValveJpegErrorHandler;

	// create the decompress struct.
	jpeg_create_decompress(&jpegInfo);

	if ( setjmp( jerr.m_ErrorContext ) )
	{
		// Get here if there is any error
		jpeg_destroy_decompress( &jpegInfo );

		fclose(infile);

		return CE_ERROR_PARSING_SOURCE;
	}

	jpeg_stdio_src(&jpegInfo, infile);

	// read in the jpeg header and make sure that's all good.
	if (jpeg_read_header(&jpegInfo, TRUE) != JPEG_HEADER_OK)
	{
		fclose(infile);
		return CE_ERROR_PARSING_SOURCE;
	}

	// start the decompress with the jpeg engine.
	if ( !jpeg_start_decompress(&jpegInfo) )
	{
		jpeg_destroy_decompress(&jpegInfo);
		fclose(infile);
		return CE_ERROR_PARSING_SOURCE;
	}

	// now that we've started the decompress with the jpeg lib, we have the attributes of the
	// image ready to be read out of the decompress struct.
	row_stride = jpegInfo.output_width * jpegInfo.output_components;
	image_height = jpegInfo.image_height;
	image_width = jpegInfo.image_width;
	int mem_required = jpegInfo.image_height * jpegInfo.image_width * jpegInfo.output_components;

	// allocate the memory to read the image data into.
	unsigned char *buf = (unsigned char *)malloc(mem_required);
	if (buf == NULL)
	{
		jpeg_destroy_decompress(&jpegInfo);
		fclose(infile);
		return CE_MEMORY_ERROR;
	}

	// read in all the scan lines of the image into our image data buffer.
	bool working = true;
	while (working && (jpegInfo.output_scanline < jpegInfo.output_height))
	{
		row_pointer[0] = &(buf[cur_row * row_stride]);
		if ( !jpeg_read_scanlines(&jpegInfo, row_pointer, 1) )
		{
			working = false;
		}
		++cur_row;
	}

	if (!working)
	{
		free(buf);
		jpeg_destroy_decompress(&jpegInfo);
		fclose(infile);
		return CE_ERROR_PARSING_SOURCE;
	}

	jpeg_finish_decompress(&jpegInfo);

	fclose(infile);

	// ok, at this point we have read in the JPEG image to our buffer, now we need to write it out as a TGA file.
	CUtlBuffer outBuf;
	bool bRetVal = TGAWriter::WriteToBuffer( buf, outBuf, image_width, image_height, IMAGE_FORMAT_RGB888, IMAGE_FORMAT_RGB888 );
	if ( bRetVal )
	{
		if ( !g_pFullFileSystem->WriteFile( tgaPath, NULL, outBuf ) )
		{
			bRetVal = false;
		}
	}

	free(buf);
	return bRetVal ? CE_SUCCESS : CE_ERROR_WRITING_OUTPUT_FILE;

#else
	return CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
#endif
}

struct TGAHeader {
	byte  identsize;          // size of ID field that follows 18 byte header (0 usually)
	byte  colourmaptype;      // type of colour map 0=none, 1=has palette
	byte  imagetype;          // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

	short colourmapstart;     // first colour map entry in palette
	short colourmaplength;    // number of colours in palette
	byte  colourmapbits;      // number of bits per palette entry 15,16,24,32

	short xstart;             // image x origin
	short ystart;             // image y origin
	short width;              // image width in pixels
	short height;             // image height in pixels
	byte  bits;               // image bits per pixel 8,16,24,32
	byte  descriptor;         // image descriptor bits (vh flip bits)
};


static void ReadTGAHeader(FILE *infile, TGAHeader &header)
{
	if (infile == NULL)
	{
		return;
	}

	fread(&header.identsize, sizeof(header.identsize), 1, infile);
	fread(&header.colourmaptype, sizeof(header.colourmaptype), 1, infile);
	fread(&header.imagetype, sizeof(header.imagetype), 1, infile);
	fread(&header.colourmapstart, sizeof(header.colourmapstart), 1, infile);
	fread(&header.colourmaplength, sizeof(header.colourmaplength), 1, infile);
	fread(&header.colourmapbits, sizeof(header.colourmapbits), 1, infile);
	fread(&header.xstart, sizeof(header.xstart), 1, infile);
	fread(&header.ystart, sizeof(header.ystart), 1, infile);
	fread(&header.width, sizeof(header.width), 1, infile);
	fread(&header.height, sizeof(header.height), 1, infile);
	fread(&header.bits, sizeof(header.bits), 1, infile);
	fread(&header.descriptor, sizeof(header.descriptor), 1, infile);
}

// convert TGA file at the given location to a VTF file of the same root name at the same location.
ConversionErrorType MyUGCPopUp::ConvertTGAToVTF( const char *tgaPath )
{
	FILE *infile = fopen(tgaPath, "rb");
	if (infile == NULL)
	{
		return CE_CANT_OPEN_SOURCE_FILE;
	}

	// read out the header of the image.
	TGAHeader header;
	ReadTGAHeader(infile, header);

	// check to make sure that the TGA has the proper dimensions and size.
	if (!IsPowerOfTwo(header.width) || !IsPowerOfTwo(header.height))
	{
		fclose(infile);
		return CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED;
	}

	int imageMemoryFootprint = header.width * header.height * header.bits / 8;

	CUtlBuffer inbuf(0, imageMemoryFootprint);

	// read in the image
	int nBytesRead = fread(inbuf.Base(), imageMemoryFootprint, 1, infile);

	fclose(infile);
	inbuf.SeekPut( CUtlBuffer::SEEK_HEAD, nBytesRead );

	// load vtex_dll.dll and get the interface to it.
	CSysModule *vtexmod = Sys_LoadModule("vtex_dll");
	if (vtexmod == NULL)
	{
		return CE_ERROR_LOADING_DLL;
	}

	CreateInterfaceFn factory = Sys_GetFactory(vtexmod);
	if (factory == NULL)
	{
		Sys_UnloadModule(vtexmod);
		return CE_ERROR_LOADING_DLL;
	}

	IVTex *vtex = (IVTex *)factory(IVTEX_VERSION_STRING, NULL);
	if (vtex == NULL)
	{
		Sys_UnloadModule(vtexmod);
		return CE_ERROR_LOADING_DLL;
	}

	char *vtfParams[4];

	// the 0th entry is skipped cause normally thats the program name.
	vtfParams[0] = "";
	vtfParams[1] = "-quiet";
	vtfParams[2] = "-dontusegamedir";
	vtfParams[3] = (char *)tgaPath;

	// call vtex to do the conversion.
	vtex->VTex(4, vtfParams);

	Sys_UnloadModule(vtexmod);

	return CE_SUCCESS;
}

// write a VMT file for the spray VTF file at the given path.
ConversionErrorType MyUGCPopUp::WriteVMT( const char *vtfPath )
{
	if (vtfPath == NULL)
	{
		return CE_ERROR_WRITING_OUTPUT_FILE;
	}

	// make the vmt filename
	char vmtPath[MAX_PATH*4];
	Q_strncpy(vmtPath, vtfPath, sizeof(vmtPath));
	char *c = vmtPath + strlen(vmtPath);
	while ((c > vmtPath) && (*(c-1) != '.'))
	{
		--c;
	}
	Q_strncpy(c, "vmt", sizeof(vmtPath) - (c - vmtPath));

	// get the root filename for the vtf file
	char filename[MAX_PATH];
	while ((c > vmtPath) && (*(c-1) != '/') && (*(c-1) != '\\'))
	{
		--c;
	}

	int i = 0;
	while ((*c != 0) && (*c != '.'))
	{
		filename[i++] = *(c++);
	}
	filename[i] = 0;

	// create the vmt file.
	FILE *vmtFile = fopen(vmtPath, "w");
	if (vmtFile == NULL)
	{
		return CE_ERROR_WRITING_OUTPUT_FILE;
	}

	// write the contents of the file.
	fprintf(vmtFile, "\"UnlitGeneric\"\n{\n\t\"$basetexture\"	\"%s%s%c%s\"\n\t\"$translucent\" 1\n\t\"$vertexcolor\" 1\n\t\"$vertexalpha\" 1\n\t\"$no_fullbright\" 1\n\t\"$ignorez\" 1\n\t\"$nolod\" 1\n}\n", "vgui/", "ugctemp", '/', "addonimage" );

	fclose(vmtFile);

	return CE_SUCCESS;
}


//=============================================================================
UGCListItem::UGCListItem(Panel *parent, const char *panelName):
BaseClass(parent, panelName)
{
	SetProportional( true );

	m_LblName = new Label( this, "LblName", "" );
	m_ImgUGC = new ImagePanel(this, "ImgUGC");
	m_LblName->DisableMouseInputForThisPanel( true );
	m_ImgUGC->DisableMouseInputForThisPanel( true );

	m_bCurrentlySelected = false;
}

UGCListItem::~UGCListItem()
{
	delete m_LblName;
	delete m_ImgUGC;
}

//=============================================================================
void UGCListItem::SetName( const char* name )
{
	m_LblName->SetText( name );
}

//=============================================================================
void UGCListItem::SetImage( const char* name )
{
	m_ImgUGC->SetImage(name);
}

void UGCListItem::SetPublished( bool bPublished )
{
	m_bPublished = bPublished;
}
/*
//=============================================================================
bool UGCListItem::GetAddonEnabled( )
{
	return m_BtnEnabled->IsSelected();
}
*/
//=============================================================================
void UGCListItem::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("Resource/UI/BaseModUI/UGCListItem.res");

	SetBgColor( pScheme->GetColor( "Button.BgColor", Color( 64, 64, 64, 255 ) ) );

	m_hTextFont = pScheme->GetFont( "DefaultLarge", true );
}

void UGCListItem::OnMousePressed( vgui::MouseCode code )
{
	if ( MOUSE_LEFT == code )
	{
		GenericPanelList *pGenericList = dynamic_cast<GenericPanelList*>( GetParent() );

		unsigned short nindex;
		if ( pGenericList && pGenericList->GetPanelItemIndex( this, nindex ) )
		{
			pGenericList->SelectPanelItem( nindex, GenericPanelList::SD_DOWN );
		}
	}

	BaseClass::OnMousePressed( code );
}

void UGCListItem::OnMessage( const KeyValues *params, vgui::VPANEL ifromPanel )
{
	BaseClass::OnMessage( params, ifromPanel );

	if ( !V_strcmp( params->GetName(), "PanelSelected" ) ) 
	{
		m_bCurrentlySelected = true;
	}
	if ( !V_strcmp( params->GetName(), "PanelUnSelected" ) ) 
	{
		m_bCurrentlySelected = false;
	}

}

void UGCListItem::Paint( )
{
	BaseClass::Paint();

	// Draw the graded outline for the selected item only
	if ( m_bCurrentlySelected )
	{
		int nPanelWide, nPanelTall;
		GetSize( nPanelWide, nPanelTall );

		//surface()->DrawSetColor( Color( 240, 0, 0, 255 ) );
		surface()->DrawSetColor( Color( 170, 170, 170, 128 ) );

		// Top lines
		surface()->DrawFilledRectFade( 0, 0, 0.5f * nPanelWide, 2, 0, 255, true );
		surface()->DrawFilledRectFade( 0.5f * nPanelWide, 0, nPanelWide, 2, 255, 0, true );

		// Bottom lines
		surface()->DrawFilledRectFade( 0, nPanelTall-2, 0.5f * nPanelWide, nPanelTall, 0, 255, true );
		surface()->DrawFilledRectFade( 0.5f * nPanelWide, nPanelTall-2, nPanelWide, nPanelTall, 255, 0, true );

		// Text Blotch
		int nTextWide, nTextTall, nNameX, nNameY, nNameWide, nNameTall;
		wchar_t wsAddonName[120];

		m_LblName->GetPos( nNameX, nNameY );
		m_LblName->GetSize( nNameWide, nNameTall );
		m_LblName->GetText( wsAddonName, sizeof( wsAddonName ) );
		surface()->GetTextSize( m_hTextFont, wsAddonName, nTextWide, nTextTall );
		int nBlotchWide = nTextWide + vgui::scheme()->GetProportionalScaledValueEx( GetScheme(), 75 );

		surface()->DrawFilledRectFade( 0, 2, 0.50f * nBlotchWide, nPanelTall-2, 0, 50, true );
		surface()->DrawFilledRectFade( 0.50f * nBlotchWide, 2, nBlotchWide, nPanelTall-2, 50, 0, true );
	}
}

//=============================================================================
//
//=============================================================================
MyUGC::MyUGC( Panel *parent, const char *panelName ):
BaseClass( parent, panelName, false, true )
{
	GameUI().PreventEngineHideGameUI();

	SetDeleteSelfOnClose(true);
	SetProportional( true );

	m_pHeaderFooter = new CNB_Header_Footer( this, "HeaderFooter" );
	m_pHeaderFooter->SetTitle( "" );
	m_pHeaderFooter->SetHeaderEnabled( false );
	m_pHeaderFooter->SetGradientBarEnabled( true );
	m_pHeaderFooter->SetGradientBarPos( 75, 350 );

	m_GplUGC = new GenericPanelList(this, "GplUGC", GenericPanelList::ISM_PERITEM);
	m_GplUGC->ShowScrollProgress( false );
	m_GplUGC->SetScrollBarVisible( false );

	m_LblNoUGC = new Label( this, "LblNoUGC", "" );

	SetLowerGarnishEnabled( true );

	LoadControlSettings("Resource/UI/BaseModUI/MyUGC.res");

	UpdateFooter();
}

//=============================================================================
MyUGC::~MyUGC()
{
	GameUI().AllowEngineHideGameUI();
}

//=============================================================================
void MyUGC::Activate()
{
	BaseClass::Activate();
	
	vgui::ImagePanel *pSpinner = dynamic_cast< vgui::ImagePanel* >(FindChildByName("ImgLoadingIcon"));
	if ( pSpinner )
		pSpinner->SetVisible( true );

	BaseModHybridButton* pButton = (BaseModHybridButton *)FindChildByName("BtnDelete");
	if (pButton)
		pButton->SetVisible( false );

	ImagePanel* pIcon = (ImagePanel *)FindChildByName("BtnDelete");
	if (pIcon)
		pIcon->SetVisible( false );

	pButton = (BaseModHybridButton *)FindChildByName("BtnPublish");
	if ( pButton )
		pButton->SetVisible( false );

	pIcon = (ImagePanel *)FindChildByName("IconPublish");
	if ( pIcon )
		pIcon->SetVisible( false );

	m_GplUGC->RemoveAllPanelItems();

	UGCQueryHandle_t PublishedUGCRequest = steamapicontext->SteamUGC()->CreateQueryUserUGCRequest(
		steamapicontext->SteamUser()->GetSteamID().GetAccountID(),
		k_EUserUGCList_Published,
		k_EUGCMatchingUGCType_Items,
		k_EUserUGCListSortOrder_CreationOrderDesc,
		799900,
		799900,
		1);
	SteamAPICall_t hSteamAPICall = steamapicontext->SteamUGC()->SendQueryUGCRequest(PublishedUGCRequest);

	m_SteamCallResultPublishedUGC.Set(hSteamAPICall, this, &MyUGC::OnPublishedUGCReceived);
}

void MyUGC::OnPublishedUGCReceived( SteamUGCQueryCompleted_t *pPublishedUGC, bool bIOFailure )
{
	for ( uint32 i = 1; i < pPublishedUGC->m_unNumResultsReturned; i++ )
	{
		SteamUGCDetails_t UGCDetails;
		steamapicontext->SteamUGC()->GetQueryUGCResult(pPublishedUGC->m_handle, i, &UGCDetails);
		UGCListItem* panelItem = m_GplUGC->AddPanelItem<UGCListItem>("UGCListItem");
		panelItem->SetParent(m_GplUGC);
		panelItem->SetName(UGCDetails.m_rgchTitle);
		panelItem->SetPublished(true);
	}

	char ugclistFilename[MAX_PATH];
	char modPath[MAX_PATH];

	g_pFullFileSystem->GetSearchPath("MOD", false, modPath, MAX_PATH);

	// It's possible that we have multiple MOD directories. If that's the case get the first one
	// in the semi-colon delimited list
	char *pSemi = strchr(modPath, ';');
	if ( pSemi )
	{
		V_strncpy(modPath, pSemi, MAX_PATH);
	}
	V_snprintf( ugclistFilename, sizeof( ugclistFilename), "%s%s", modPath, "myugc.txt" );
	m_pUGCList = new KeyValues( "MyUGC" );

	m_pUGCList->LoadFromFile(g_pFullFileSystem, ugclistFilename);
	for (KeyValues *pCur = m_pUGCList->GetFirstValue(); pCur; pCur = pCur->GetNextValue())
	{
		char addoninfoFilename[MAX_PATH];
		KeyValues *pAddonInfo = new KeyValues("AddonInfo");
		V_snprintf( addoninfoFilename, sizeof( addoninfoFilename ), "%s%s%c%s%c%s", modPath, "addons", CORRECT_PATH_SEPARATOR, pCur->GetName(), CORRECT_PATH_SEPARATOR, "addoninfo.txt" );	
		pAddonInfo->LoadFromFile(g_pFullFileSystem, addoninfoFilename);

		UGCListItem* panelItem = m_GplUGC->AddPanelItem<UGCListItem>("UGCListItem");
		char vmtFilename[MAX_PATH];
		V_snprintf( vmtFilename, sizeof( vmtFilename ), "%s%c%s%c%s", "addons", CORRECT_PATH_SEPARATOR, pCur->GetName(), CORRECT_PATH_SEPARATOR, "addonimage" );

		//Set the info
		panelItem->SetParent(m_GplUGC);
		panelItem->SetName(pAddonInfo->GetString("addontitle"));
		panelItem->SetImage(vmtFilename);
		panelItem->SetPublished( false );
	}

	if ( !m_GplUGC->GetPanelItemCount() )
	{
		m_LblNoUGC->SetVisible( true );
	}
	vgui::ImagePanel *pSpinner = dynamic_cast< vgui::ImagePanel* >(FindChildByName("ImgLoadingIcon"));
	if ( pSpinner )
		pSpinner->SetVisible( false );

	// Focus on the first item in the list
	if ( m_GplUGC->GetPanelItemCount() > 0 )
	{
		m_GplUGC->NavigateTo();
		m_GplUGC->SelectPanelItem( 0, GenericPanelList::SD_DOWN );

		BaseModHybridButton* pButton = (BaseModHybridButton *)FindChildByName("BtnDelete");
		if ( pButton )
			pButton->SetVisible( true );

		ImagePanel* pIcon = (ImagePanel *)FindChildByName("BtnDelete");
		if ( pIcon )
			pIcon->SetVisible( true );
	}
}

void MyUGC::UpdateFooter()
{
	CBaseModFooterPanel *footer = BaseModUI::CBaseModPanel::GetSingleton().GetFooterPanel();
	if ( footer )
	{
		footer->SetButtons( FB_BBUTTON, FF_AB_ONLY, false );
		footer->SetButtonText( FB_BBUTTON, "#L4D360UI_Done" );
	}
}



//=============================================================================
void MyUGC::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	SetupAsDialogStyle();
}

//=============================================================================
void MyUGC::PaintBackground()
{
	
}

//=============================================================================
void MyUGC::OnCommand(const char *command)
{
	if( V_strcmp( command, "Back" ) == 0 )
	{
		OnKeyCodePressed( KEY_XBUTTON_B );
	}
	else if( V_strcmp( command, "Create" ) == 0 )
	{
		CBaseModPanel::GetSingleton().OpenWindow( WT_MYUGCPOPUP, this, true );
	}
	else if( V_strcmp( command, "Delete" ) == 0 )
	{
		unsigned short index = 0;
		m_GplUGC->GetPanelItemIndex(m_GplUGC->GetSelectedPanelItem(), index);
		m_GplUGC->RemovePanelItem(index);
	}
	else
	{
		BaseClass::OnCommand( command );
	}	
}

void MyUGC::OnMessage(const KeyValues *params, vgui::VPANEL ifromPanel)
{
	BaseClass::OnMessage( params, ifromPanel );
	
	if ( Q_strcmp( params->GetName(), "OnItemSelected" ) == 0 ) 
	{
		int index = ((KeyValues*)params)->GetInt( "index" );

		UGCListItem* pUGCItem = (UGCListItem *)m_GplUGC->GetPanelItem( index );
		if ( pUGCItem->IsPublished() )
		{
			BaseModHybridButton* pButton = (BaseModHybridButton *)FindChildByName("BtnEdit");
			if ( pButton )
				pButton->SetVisible( true );

			ImagePanel* pIcon = (ImagePanel *)FindChildByName("IconEdit");
			if ( pIcon )
				pIcon->SetVisible( true );
		}
		else
		{
			BaseModHybridButton* pButton = (BaseModHybridButton *)FindChildByName("BtnPublish");
			if ( pButton )
				pButton->SetVisible( true );

			ImagePanel* pIcon = (ImagePanel *)FindChildByName("IconPublish");
			if ( pIcon )
				pIcon->SetVisible( true );
		}
	}
}

void MyUGC::OnThink()
{
	BaseClass::OnThink();
}