//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Apparently fuckin NOTHING cause it's not included in viewpostprocess.cpp ?!?!?!??! WHAT THE FU-
//
//=============================================================================

#ifndef VIEWPOSTPROCESS_H
#define VIEWPOSTPROCESS_H

#if defined( _WIN32 )
#pragma once
#endif

void DoEnginePostProcessing(int x, int y, int w, int h, bool bFlashlightIsOn, bool bPostVGui = false);
void DoImageSpaceMotionBlur(const CViewSetup &view, int x, int y, int w, int h);
void DumpTGAofRenderTarget(const int width, const int height, const char *pFilename);

#endif // VIEWPOSTPROCESS_H


#ifdef OF_CLIENT_DLL
// Exposes enabling of the DOF Blur shader (e.g. allow weapon wheel to affect its state)
void SetDOFBlurScale(float scale);
void SetDOFBlurDistance(float dist);
void SetDOFBlurEnabled(bool enabled);
#endif