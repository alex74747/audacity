/**********************************************************************

  Audacity: A Digital Audio Editor

  ProjectFileIORegistry.h

  Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_PROJECT_FILE_IO_REGISTRY__
#define __AUDACITY_PROJECT_FILE_IO_REGISTRY__

#include "Audacity.h"
#include <functional>

class AudacityProject;
class XMLTagHandler;
class wxString;

namespace ProjectFileIORegistry {

// Type of functions returning objects that interpret a part of the saved XML
using TagHandlerFactory =
   std::function< XMLTagHandler *( AudacityProject & ) >;

// Typically statically constructed
struct AUDACITY_DLL_API Entry{
   Entry( const wxString &tag, const TagHandlerFactory &factory );

   struct AUDACITY_DLL_API Init{ Init(); };
};

TagHandlerFactory Lookup( const wxString &tag );

}

// Guarantees registry exists before attempts to use it
static ProjectFileIORegistry::Entry::Init sInitProjectFileIORegistryEntry;

#endif
