//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BASEMODELPANEL_H
#define BASEMODELPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ModelPanel.h>
#include "GameEventListener.h"
#include "KeyValues.h"

class C_SceneEntity;

class CModelPanelModel : public C_BaseFlex
{
public:
	CModelPanelModel(){}
	DECLARE_CLASS( CModelPanelModel, C_BaseFlex );

	virtual bool IsMenuModel() const{ return true; }
	
#ifdef OF_CLIENT_DLL
	float red, green, blue;
	
	virtual Vector GetItemTintColor( void ) 
	{
		Vector m_vecModelColor;
		m_vecModelColor.x = red;
		m_vecModelColor.y = green;
		m_vecModelColor.z = blue;	
		return m_vecModelColor; 
	}	
	virtual void SetModelColor( Vector vecColor ) { red = vecColor.x; green = vecColor.y; blue = vecColor.z; }
	virtual C_BaseEntity	*GetItemTintColorOwner( void ) { return this; }
#endif
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CModelPanelModelAnimation
{
public:
	CModelPanelModelAnimation()
	{
		m_pszName = NULL;
		m_pszSequence = NULL;
		m_pszActivity = NULL;
		m_pPoseParameters = NULL;
		m_bDefault = false;
	}

	~CModelPanelModelAnimation()
	{
		if ( m_pszName && m_pszName[0] )
		{
			delete [] m_pszName;
			m_pszName = NULL;
		}

		if ( m_pszSequence && m_pszSequence[0] )
		{
			delete [] m_pszSequence;
			m_pszSequence = NULL;
		}

		if ( m_pszActivity && m_pszActivity[0] )
		{
			delete [] m_pszActivity;
			m_pszActivity = NULL;
		}

		if ( m_pPoseParameters )
		{
			m_pPoseParameters->deleteThis();
			m_pPoseParameters = NULL;
		}
	}

public:
	const char	*m_pszName;
	const char	*m_pszSequence;
	const char	*m_pszActivity;
	KeyValues	*m_pPoseParameters;
	bool		m_bDefault;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CModelPanelAttachedModelInfo
{
public:
	CModelPanelAttachedModelInfo()
	{
		m_pszModelName = NULL;
#ifdef OF_CLIENT_DLL
		m_nSkin = -1;
#else
		m_nSkin = 0;
#endif
	}

	~CModelPanelAttachedModelInfo()
	{
		if ( m_pszModelName && m_pszModelName[0] )
		{
			delete [] m_pszModelName;
			m_pszModelName = NULL;
		}
	}

public:
	const char	*m_pszModelName;
	int			m_nSkin;
};

#ifdef OF_CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CModelPanelBodygroupInfo
{
public:
	CModelPanelBodygroupInfo()
	{
		m_pszGroup = NULL;
		m_nBody = -1;
	}

	~CModelPanelBodygroupInfo()
	{
		if ( m_pszGroup && m_pszGroup[0] )
		{
			delete[] m_pszGroup;
			m_pszGroup = NULL;
		}
	}

public:
	const char* m_pszGroup;
	int			m_nBody;
};
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CModelPanelModelInfo
{
public:
	CModelPanelModelInfo()
	{
		m_pszModelName = NULL;
		m_nSkin = -1;
		m_vecAbsAngles.Init();
		m_vecOriginOffset.Init();
		m_vecFramedOriginOffset.Init();
		m_bUseSpotlight = false;
	}

	~CModelPanelModelInfo()
	{
		if ( m_pszModelName && m_pszModelName[0] )
		{
			delete [] m_pszModelName;
			m_pszModelName = NULL;
		}

		if ( m_pszVCD && m_pszVCD[0] )
		{
			delete [] m_pszVCD;
			m_pszVCD = NULL;
		}

		m_Animations.PurgeAndDeleteElements();
		m_AttachedModelsInfo.PurgeAndDeleteElements();
#ifdef OF_CLIENT_DLL
		m_Bodygroups.PurgeAndDeleteElements();
#endif
	}

public:
	const char	*m_pszModelName;
	int			m_nSkin;
	const char	*m_pszVCD;
	Vector		m_vecAbsAngles;
	Vector		m_vecOriginOffset;
	Vector2D	m_vecViewportOffset;
	Vector		m_vecFramedOriginOffset;
	bool		m_bUseSpotlight;

	CUtlVector<CModelPanelModelAnimation*>		m_Animations;
	CUtlVector<CModelPanelAttachedModelInfo*>	m_AttachedModelsInfo;
#ifdef OF_CLIENT_DLL
	CUtlVector<CModelPanelBodygroupInfo*>		m_Bodygroups;
#endif
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
#ifdef OF_CLIENT_DLL
class CModelPanel : public vgui::EditablePanel, public CGameEventListener, public vgui::AnimContModelPanel
#else
class CModelPanel : public vgui::EditablePanel, public CGameEventListener	
#endif
{
public:
	DECLARE_CLASS_SIMPLE( CModelPanel, vgui::EditablePanel );

	CModelPanel( vgui::Panel *parent, const char *name );
	virtual ~CModelPanel();

	virtual void Paint();
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void DeleteVCDData( void );
	virtual void DeleteModelData( void );
	void PurgeAttachedModels( void );

	virtual void SetFOV( int nFOV ){ m_nFOV = nFOV; }
	virtual void SetPanelDirty( void ){ m_bPanelDirty = true; }
	virtual bool SetSequence( const char *pszSequence );

	MESSAGE_FUNC_PARAMS( OnAddAnimation, "AddAnimation", data );
	MESSAGE_FUNC_PARAMS( OnSetAnimation, "SetAnimation", data );

	void	SetSkin( int nSkin );
	void	SetAttachmentsSkin( int nSkin );
	void	SetDefaultAnimation( const char *pszName );
#ifdef OF_CLIENT_DLL
	void	SetBodygroup( const char* pszName, int nBody );
	void	AddAttachment( const char* pszAttached );
#endif
#ifdef OF_CLIENT_DLL
	void	SwapModel( const char *pszName, const char *pszAttached = NULL, const char *pszVCD = NULL );
#else
	void	SwapModel( const char *pszName, const char *pszAttached = NULL );
#endif
#ifdef OF_CLIENT_DLL	
	virtual void GetHUDModelPos( float &x, float &y, float &z );
	virtual void SetHUDModelPos( float x, float y, float z );
	
	virtual void GetHUDModelAng( float &x, float &y, float &z );
	virtual void SetHUDModelAng( float x, float y, float z );	
	virtual void ResetAnim();
#endif
	virtual void ParseModelInfo( KeyValues *inResourceData );

	void		ClearAttachedModelInfos( void );

	void		CalculateFrameDistance( void );
	void		ZoomToFrameDistance( void );

	void		UpdateModel();
#ifdef OF_CLIENT_DLL
	virtual void		SetModelColor( Vector vecColor );
	Vector ModelColor;
#endif
public: // IGameEventListener:
	virtual void FireGameEvent( IGameEvent * event );

protected:
	virtual void SetupModel( void );
	virtual void SetupVCD( void );
	virtual const char *GetModelName( void );

private:
	void InitCubeMaps();
	int FindAnimByName( const char *pszName );
	void CalculateFrameDistanceInternal( const model_t *pModel );

public:
	int								m_nFOV;
	float							m_flFrameDistance;
	bool							m_bStartFramed;
	CModelPanelModelInfo			*m_pModelInfo;

	CHandle<CModelPanelModel>				m_hModel;
	CUtlVector<CHandle<C_BaseAnimating> >	m_AttachedModels;

	CHandle<C_SceneEntity>			m_hScene;

private:
	bool	m_bPanelDirty;
	int		m_iDefaultAnimation;

	bool	m_bAllowOffscreen;

	CTextureReference m_DefaultEnvCubemap;
	CTextureReference m_DefaultHDREnvCubemap;
};


#endif // BASEMODELPANEL_H
