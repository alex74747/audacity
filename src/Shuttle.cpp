/**********************************************************************

  Shuttle.cpp

  James Crook
  (C) Audacity Developers, 2007

  wxWidgets license. See Licensing.txt

*******************************************************************//**

\file Shuttle.cpp
\brief Implements Shuttle, ShuttleParams, their subclasses, and Enums.

*//****************************************************************//**

\class Shuttle
\brief Moves data from one place to another, converting it as required.

  Shuttle provides a base class for transferring parameter data into and
  out of classes into some other structure.  This is a common
  requirement and is needed for:
    - Prefs data
    - Command line parameter data
    - Project data in XML

  The 'Master' is the string side of the shuttle transfer, the 'Client'
  is the binary data side of the transfer.

  \see ShuttleBase
  \see ShuttleGui

*//****************************************************************//**

\class Enums
\brief Enums is a helper class for Shuttle.  It defines enumerations
which are used in effects dialogs, in the effects themselves and in
preferences.

(If it grows big, we will move it out of shuttle.h).

*//*******************************************************************/


#include "Shuttle.h"

#include <wx/defs.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>

//#include "effects/Effect.h"


#ifdef _MSC_VER
// If this is compiled with MSVC (Visual Studio)
#pragma warning( push )
#pragma warning( disable: 4100 ) // unused parameters.
#endif //_MSC_VER


// The ShouldSet and CouldGet functions have an important side effect
// on the pOptionalFlag.  They 'use it up' and clear it down for the next parameter.


// Tests for parameter being optional.
// Prepares for next parameter by clearing the pointer.
// Reports on whether the parameter should be set, i.e. should set 
// if it was chosen to be set, or was not optional.
bool ShuttleParams::ShouldSet(){
   if( !pOptionalFlag )
      return true;
   bool result = *pOptionalFlag;
   pOptionalFlag = NULL;
   return result;
}
// These are functions to override.  They do nothing.
void ShuttleParams::Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin, const bool vmax, const bool vscl){;};
void ShuttleParams::Define( size_t & var,   const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ){;};
void ShuttleParams::Define( int & var,      const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ){;};
void ShuttleParams::Define( float & var,    const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ){;};
void ShuttleParams::Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ){;};
void ShuttleParams::Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl ){;};
void ShuttleParams::Define( wxString &var, const wxChar * key, const wxString vdefault, const wxString vmin, const wxString vmax, const wxString vscl ){;};
void ShuttleParams::DefineEnum( int &var, const wxChar * key, const int vdefault, const EnumValueSymbol strings[], size_t nStrings ){;};



/*
void ShuttleParams::DefineEnum( int &var, const wxChar * key, const int vdefault, const EnumValueSymbol strings[], size_t nStrings )
{
}
*/

#ifdef _MSC_VER
// If this is compiled with MSVC (Visual Studio)
#pragma warning( pop )
#endif //_MSC_VER
