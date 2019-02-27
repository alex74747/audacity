/**********************************************************************

  Audacity: A Digital Audio Editor

  ShuttlePrefs.cpp

  Dominic Mazzoni
  James Crook

  Implements ShuttlePrefs

********************************************************************//*!

\class ShuttlePrefs

\brief
  A kind of Shuttle to exchange data with preferences e.g. the registry

  This class may be used by ShuttleGui to do the two step exchange,

\verbatim
     Gui -- Data -- Prefs
\endverbatim

*//*******************************************************************/


#include "ShuttlePrefs.h"

#include <wx/defs.h>

#include "WrappedType.h"
#include "Prefs.h"

bool ShuttlePrefs::TransferBool( const RegistryPath & Name, bool & bValue, const bool & bDefault )
{
   if( mbStoreInClient )
   {
      bValue = bDefault;
      gPrefs->Read( Name.GET(), &bValue );
   }
   else
   {
      return gPrefs->Write( Name.GET(), bValue );
   }
   return true;
}

bool ShuttlePrefs::TransferDouble( const RegistryPath & Name, double & dValue, const double &dDefault )
{
   if( mbStoreInClient )
   {
      dValue = dDefault;
      gPrefs->Read( Name.GET(), &dValue );
   }
   else
   {
      return gPrefs->Write( Name.GET(), dValue );
   }
   return true;
}

bool ShuttlePrefs::TransferInt( const RegistryPath & Name, int & iValue, const int &iDefault )
{
   if( mbStoreInClient )
   {
      iValue = iDefault;
      gPrefs->Read( Name.GET(), &iValue );
   }
   else
   {
      return gPrefs->Write( Name.GET(), iValue );
   }
   return true;
}

bool ShuttlePrefs::TransferString( const RegistryPath & Name, wxString & strValue, const wxString &strDefault )
{
   if( mbStoreInClient )
   {
      strValue = strDefault;
      gPrefs->Read( Name.GET(), &strValue );
   }
   else
   {
      return gPrefs->Write( Name.GET(), strValue );
   }
   return true;
}

bool ShuttlePrefs::TransferWrappedType( const RegistryPath & Name, WrappedType & W )
{
   switch( W.eWrappedType )
   {
   case eWrappedString:
      return TransferString( Name, *W.mpStr, *W.mpStr );
      break;
   case eWrappedInt:
      return TransferInt( Name, *W.mpInt, *W.mpInt );
      break;
   case eWrappedDouble:
      return TransferDouble( Name, *W.mpDouble, *W.mpDouble );
      break;
   case eWrappedBool:
      return TransferBool( Name, *W.mpBool, *W.mpBool );
      break;
   case eWrappedEnum:
      wxASSERT( false );
      break;
   default:
      wxASSERT( false );
      break;
   }
   return false;
}

bool ShuttlePrefs::ExchangeWithMaster(const RegistryPath & WXUNUSED(Name))
{
   // ShuttlePrefs is unusual in that it overloads ALL the Transfer functions
   // which it supports.  It doesn't do any string conversion, because wxConv will
   // do so if it is required.
   // So, ExchangeWithMaster should never get called...  Hence the ASSERT here.
   wxASSERT( false );
   return false;
}
