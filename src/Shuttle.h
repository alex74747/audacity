/**********************************************************************

  Audacity: A Digital Audio Editor

  Shuttle.h

  James Crook

**********************************************************************/

#ifndef __AUDACITY_SHUTTLE__
#define __AUDACITY_SHUTTLE__

#include "ComponentInterfaceSymbol.h"
#include "EffectAutomationParameters.h" // for CommandParameters

class ComponentInterfaceSymbol;

template<typename Type> struct EffectParameter {
   const wxChar *const key;
   const Type def;
   const Type min;
   const Type max;
   const Type scale;
   mutable Type cache;
   inline operator const Type& () const { return cache; }
};

template<typename Type>
inline const EffectParameter<Type> Parameter(
   const wxChar *key,
   const Type &def, const Type &min, const Type &max, const Type &scale )
{ return { key, def, min, max, scale, {} }; }

struct EnumEffectParameter : EffectParameter<int>
{
   EnumEffectParameter(
      const wxChar *key, int def, int min, int max, int scale,
      const EnumValueSymbol *symbols_, size_t nSymbols_ )
      : EffectParameter<int>{ key, def, min, max, scale, {} }
      , symbols{ symbols_ }
      , nSymbols{ nSymbols_ }
   {}

   const EnumValueSymbol *const symbols;
   const size_t nSymbols;
};

using EnumParameter = EnumEffectParameter;

class Shuttle /* not final */ {
 public:
   // constructors and destructors
   Shuttle();
   virtual ~Shuttle() {}

 public:
   bool mbStoreInClient;
   wxString mValueString;
   // Even though virtual, mostly the transfer functions won't change
   // for special kinds of archive.
   virtual bool TransferBool( const wxString & Name, bool & bValue, const bool & bDefault );
   virtual bool TransferFloat( const wxString & Name, float & fValue, const float &fDefault );
   virtual bool TransferDouble( const wxString & Name, double & dValue, const double &dDefault );
   virtual bool TransferInt( const wxString & Name, int & iValue, const int &iDefault );
   virtual bool TransferInt( const wxString & Name, wxLongLong_t & iValue, const wxLongLong_t &iDefault );
   virtual bool TransferLongLong( const wxString & Name, wxLongLong_t & iValue, const wxLongLong_t &iDefault );
   virtual bool TransferString( const wxString & Name, wxString & strValue, const wxString &strDefault );
   virtual bool TransferEnum( const wxString & Name, int & iValue,
      const int nChoices, const wxString * pFirstStr);
   virtual bool ExchangeWithMaster(const wxString & Name);
};

class CommandParameters;
/**************************************************************************//**
\brief Shuttle that deals with parameters.  This is a base class with lots of
virtual functions that do nothing by default.
Unrelated to class Shuttle.
********************************************************************************/
class AUDACITY_DLL_API ShuttleParams /* not final */
{
public:
   wxString mParams;
   bool *pOptionalFlag;
   CommandParameters * mpEap;
   ShuttleParams() { mpEap = NULL; pOptionalFlag = NULL; }
   virtual ~ShuttleParams() {}
   bool ShouldSet();
   virtual ShuttleParams & Optional( bool & WXUNUSED(var) ){ pOptionalFlag = NULL;return *this;};
   virtual ShuttleParams & OptionalY( bool & var ){ return Optional( var );};
   virtual ShuttleParams & OptionalN( bool & var ){ return Optional( var );};
   virtual void Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin=false, const bool vmax=false, const bool vscl=false );
   virtual void Define( size_t & var,   const wxChar * key, const int vdefault, const int vmin=0, const int vmax=100000, const int vscl=1 );
   virtual void Define( int & var,      const wxChar * key, const int vdefault, const int vmin=0, const int vmax=100000, const int vscl=1 );
   virtual void Define( float & var,    const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl=1.0f );
   virtual void Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl=1.0f );
   virtual void Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl=1.0f );
   virtual void Define( wxString &var, const wxChar * key, const wxString vdefault, const wxString vmin = {}, const wxString vmax = {}, const wxString vscl = {} );
   virtual void DefineEnum( int &var, const wxChar * key, const int vdefault,
      const EnumValueSymbol strings[], size_t nStrings );

   template< typename Var, typename Type >
   void SHUTTLE_PARAM( Var &var, const EffectParameter< Type > &name )
   { Define( var, name.key, name.def, name.min, name.max, name.scale ); }

   void SHUTTLE_PARAM( int &var, const EnumEffectParameter &name )
   { DefineEnum( var, name.key, name.def, name.symbols, name.nSymbols ); }
};

#endif
