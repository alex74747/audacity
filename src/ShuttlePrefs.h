/**********************************************************************

  Audacity: A Digital Audio Editor

  ShuttlePrefs.h

  Dominic Mazzoni
  James Crook

**********************************************************************/

#ifndef __AUDACITY_SHUTTLE_PREFS__
#define __AUDACITY_SHUTTLE_PREFS__

#include "Shuttle.h"

class ShuttlePrefs final : public Shuttle
{
public:
   // constructors and destructors
   ShuttlePrefs(){;};
   virtual ~ShuttlePrefs() {};

public:
   bool TransferBool( const RegistryPath & Name, bool & bValue, const bool & bDefault ) override;
//   bool TransferFloat( const wxString & Name, float & fValue, const float &fDefault ) override;
   bool TransferDouble( const RegistryPath & Name, double & dValue, const double &dDefault ) override;
   bool TransferInt(const RegistryPath & Name, int & iValue, const int &iDefault) override;
   bool TransferString(const RegistryPath & Name, wxString & strValue, const wxString &strDefault) override;
   bool TransferWrappedType(const RegistryPath & Name, WrappedType & W) override;
   bool ExchangeWithMaster(const RegistryPath & Name) override;
};

#endif
