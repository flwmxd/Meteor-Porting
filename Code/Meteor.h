/**
 * suggestions for Meteor-Remake version.
 * 1. All Game Logic should be written by c++ because of efficiency and code security.
 * 2. All Editor extension should be written by C#
 */

//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef BUILD_EDITOR
#	include "Editor.h"
#else
#	include "Application.h"
#endif

namespace meteor 
{

}
