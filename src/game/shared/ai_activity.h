//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef AI_ACTIVITY_H
#define AI_ACTIVITY_H
#ifdef _WIN32
#pragma once
#endif

#define ACTIVITY_NOT_AVAILABLE		-1

typedef enum
{
	ACT_INVALID = -1,			// So we have something more succint to check for than '-1'
#define ACT(ACTION) ACTION,
#include "shared_activities.h"
#undef ACT
	// this is the end of the global activities, private per-monster activities start here.
	LAST_SHARED_ACTIVITY,
} Activity;

COMPILE_TIME_ASSERT(ACT_RESET==0);

#endif // AI_ACTIVITY_H

