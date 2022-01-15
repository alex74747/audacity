/**********************************************************************

  Audacity: A Digital Audio Editor

  @file ShuttleAutomation.h

  Paul Licameli split from Shuttle.h

**********************************************************************/

#ifndef __AUDACITY_SHUTTLE_AUTOMATION__
#define __AUDACITY_SHUTTLE_AUTOMATION__

#include "Shuttle.h"

/**************************************************************************//**
\brief An object that stores callback functions, generated from the constructor
arguments.
For each variable passed to the constructor:
   Reset resets it to a default
   Visit visits it with a ShuttleParams object
   Get serializes it to a string
   Set deserializes it from a string and returns a success flag
The constructor arguments are alternating references to variables and
EffectParameter objects (and optionally a first argument which is a function
to be called at the end of Reset or Set, and returning a value for Set).
********************************************************************************/
struct CapturedParameters {
public:
   // Types of the callbacks
   using ResetFunction = std::function< void() >;
   using VisitFunction = std::function< void( ShuttleParams & S ) >;
   using GetFunction = std::function< void( CommandParameters & parms ) >;
   // Returns true if successful:
   using SetFunction = std::function< bool ( CommandParameters & parms ) >;

   // Another helper type
   using PostSetFunction = std::function< bool() >;

   // Constructors
   CapturedParameters() = default;

   template< typename ...Args >
   // Arguments are references to variables, alternating with references to
   // EffectParameter objects detailing the callback operations on them.
   CapturedParameters( Args &...args )
      : CapturedParameters( PostSetFunction{}, args... ) {}

   template< typename Fn, typename ...Args,
      // SFINAE: require that first arg be callable
      typename = decltype( std::declval<Fn>()() )
   >
   // Like the previous, but with an extra first argument,
   // which is called at the end of Reset or Set.  Its return value is
   // ignored in Reset() and passed as the result of Set.
   CapturedParameters( const Fn &PostSet, Args &...args )
   {
      PostSetFunction fn = PostSet;
      Reset = [&args..., fn]{ DoReset( fn, args... ); };
      Visit = [&args...]( ShuttleParams &S ){ DoVisit( S, args... ); };
      Get = [&args...]( CommandParameters &parms ){ DoGet( parms, args... ); };
      Set = [&args..., fn]( CommandParameters &parms ){
         return DoSet( parms, fn, args... ); };
   }

   // True iff this was default-constructed
   bool empty() const { return !Reset; }

   // Callable as if they were const member functions
   ResetFunction Reset;
   VisitFunction Visit;
   GetFunction Get;
   SetFunction Set;

private:
   // Base cases stop recursions
   static void DoReset( const PostSetFunction &fn ){ if (fn) fn(); }
   static void DoVisit(ShuttleParams &){}
   static void DoGet(CommandParameters &) {}
   static bool DoSet( CommandParameters &, const PostSetFunction &fn )
      { return !fn || fn(); }

   // Recursive cases
   template< typename Var, typename Type, typename ...Args >
   static void DoReset( const PostSetFunction &fn,
      Var &var, EffectParameter< Type > &param, Args &...others )
   {
      var = param.def;
      DoReset( fn, others... );
   }

   template< typename Var, typename Type, typename ...Args >
   static void DoVisit( ShuttleParams & S,
      Var &var, EffectParameter< Type > &param, Args &...others )
   {
      S.Define( var, param.key, param.def, param.min, param.max, param.scale );
      DoVisit( S, others... );
   }
   template< typename ...Args >
   static void DoVisit( ShuttleParams & S,
      int &var, EnumEffectParameter &param, Args &...others )
   {
      S.DefineEnum( var, param.key, param.def, param.symbols, param.nSymbols );
      DoVisit( S, others... );
   }

   template< typename Var, typename Type, typename ...Args >
   static void DoGet( CommandParameters & parms,
      Var &var, EffectParameter< Type > &param, Args &...others )
   {
      parms.Write( param.key, static_cast<Type>(var) );
      DoGet( parms, others... );
   }
   template< typename ...Args >
   static void DoGet( CommandParameters & parms,
      int &var, EnumEffectParameter &param, Args &...others )
   {
      parms.Write( param.key, param.symbols[ var ].Internal() );
      DoGet( parms, others... );
   }

   template< typename Var, typename Type, typename ...Args >
   static bool DoSet( CommandParameters & parms, const std::function<bool()> &fn,
      Var &var, EffectParameter< Type > &param, Args &...others )
   {
      if (!parms.ReadAndVerify(param.key, &param.cache, param.def,
         param.min, param.max))
         return false;
      var = param;
      return DoSet( parms, fn, others... );
   }
   template< typename ...Args >
   static bool DoSet( CommandParameters & parms, const std::function<bool()> &fn,
      int &var, EnumEffectParameter &param, Args &...others )
   {
      if (!parms.ReadAndVerify(param.key, &param.cache, param.def,
         param.symbols, param.nSymbols))
         return false;
      var = param;
      return DoSet( parms, fn, others... );
   }
};

/**************************************************************************//**
\brief Shuttle that gets parameter values into a string.
********************************************************************************/
class AUDACITY_DLL_API ShuttleGetAutomation final : public ShuttleParams
{
public:
   ShuttleParams & Optional( bool & var ) override;
   void Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin, const bool vmax, const bool vscl ) override;
   void Define( int & var,      const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ) override;
   void Define( size_t & var,   const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ) override;
   void Define( float & var,    const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ) override;
   void Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ) override;
   void Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl ) override;
   void Define( wxString &var,  const wxChar * key, const wxString vdefault, const wxString vmin, const wxString vmax, const wxString vscl ) override;
   void DefineEnum( int &var, const wxChar * key, const int vdefault,
      const EnumValueSymbol strings[], size_t nStrings ) override;
};

/**************************************************************************//**
\brief Shuttle that sets parameters to a value (from a string)
********************************************************************************/
class AUDACITY_DLL_API ShuttleSetAutomation final : public ShuttleParams
{
public:
   ShuttleSetAutomation(){ bWrite = false; bOK = false;};
   bool bOK;
   bool bWrite;
   ShuttleParams & Optional( bool & var ) override;
   bool CouldGet(const wxString &key);
   void SetForValidating( CommandParameters * pEap){ mpEap=pEap; bOK=true;bWrite=false;};
   void SetForWriting(CommandParameters * pEap){ mpEap=pEap;bOK=true;bWrite=true;};
   void Define( bool & var,     const wxChar * key, const bool vdefault, const bool vmin, const bool vmax, const bool vscl ) override;
   void Define( int & var,      const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ) override;
   void Define( size_t & var,   const wxChar * key, const int vdefault, const int vmin, const int vmax, const int vscl ) override;
   void Define( float & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ) override;
   void Define( double & var,   const wxChar * key, const float vdefault, const float vmin, const float vmax, const float vscl ) override;
   void Define( double & var,   const wxChar * key, const double vdefault, const double vmin, const double vmax, const double vscl ) override;
   void Define( wxString &var,  const wxChar * key, const wxString vdefault, const wxString vmin, const wxString vmax, const wxString vscl ) override;
   void DefineEnum( int &var, const wxChar * key, const int vdefault,
      const EnumValueSymbol strings[], size_t nStrings ) override;
};


/**************************************************************************//**
\brief Shuttle that sets parameters to their default values.
********************************************************************************/
class ShuttleDefaults final : public ShuttleParams
{
public:
   wxString Result;
   virtual ShuttleParams & Optional( bool & var )override{  var = true; pOptionalFlag = NULL;return *this;};
   virtual ShuttleParams & OptionalY( bool & var )override{ var = true; pOptionalFlag = NULL;return *this;};
   virtual ShuttleParams & OptionalN( bool & var )override{ var = false;pOptionalFlag = NULL;return *this;};

   void Define( bool & var,          const wxChar * WXUNUSED(key),  const bool     vdefault,
      const bool     WXUNUSED(vmin), const bool     WXUNUSED(vmax), const bool     WXUNUSED(vscl) )
      override { var = vdefault;};
   void Define( int & var,           const wxChar * WXUNUSED(key),  const int      vdefault,
      const int      WXUNUSED(vmin), const int      WXUNUSED(vmax), const int      WXUNUSED(vscl) )
      override { var = vdefault;};
   void Define( size_t & var,        const wxChar * WXUNUSED(key),  const int      vdefault,
      const int      WXUNUSED(vmin), const int      WXUNUSED(vmax), const int      WXUNUSED(vscl) )
      override{ var = vdefault;};
   void Define( float & var,         const wxChar * WXUNUSED(key),  const float    vdefault,
      const float    WXUNUSED(vmin), const float    WXUNUSED(vmax), const float    WXUNUSED(vscl) )
      override { var = vdefault;};
   void Define( double & var,        const wxChar * WXUNUSED(key),  const float    vdefault,
      const float    WXUNUSED(vmin), const float    WXUNUSED(vmax), const float    WXUNUSED(vscl) )
      override { var = vdefault;};
   void Define( double & var,        const wxChar * WXUNUSED(key),  const double   vdefault,
      const double   WXUNUSED(vmin), const double   WXUNUSED(vmax), const double   WXUNUSED(vscl) )
      override { var = vdefault;};
   void Define( wxString &var,       const wxChar * WXUNUSED(key),  const wxString vdefault,
      const wxString WXUNUSED(vmin), const wxString WXUNUSED(vmax), const wxString WXUNUSED(vscl) )
      override { var = vdefault;};
   void DefineEnum( int &var,        const wxChar * WXUNUSED(key),  const int vdefault,
      const EnumValueSymbol WXUNUSED(strings) [], size_t WXUNUSED( nStrings ) )
      override { var = vdefault;};
};

#endif
