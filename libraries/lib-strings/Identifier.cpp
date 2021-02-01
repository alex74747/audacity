/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 Identifier.cpp
 
 Paul Licameli split from Internat.cpp
 
 **********************************************************************/

#include "Identifier.h"
#include <wx/arrstr.h> // for wxSplit

Identifier::Identifier(
   std::initializer_list<Identifier> components, wxChar separator )
{
   if( components.size() < 2 )
   {
      wxASSERT( false );
      return;
   }
   auto iter = components.begin(), end = components.end();
   value = (*iter++).value;
   while (iter != end)
      value += separator + (*iter++).value;
}

std::vector< Identifier > Identifier::split( wxChar separator ) const
{
   auto strings = ::wxSplit( value, separator );
   return { strings.begin(), strings.end() };
}

Identifier::Identifier(const value_type &str) : value{ str } {}
Identifier::Identifier(const wxString &str) : value{ str } {}
Identifier::Identifier(const char_type *str) : value{ str } {}
Identifier::Identifier(const char *str) : value( str, str + strlen(str) ) {}
Identifier::Identifier( wxString && str ) :value{ str.wc_str() } {}

auto Identifier::GET() const -> value_type { return value; }

bool operator == ( const Identifier &x, const Identifier &y )
{ return x.GET() == y.GET(); }

bool operator < ( const Identifier &x, const Identifier &y )
{ return x.GET() < y.GET(); }

size_t std::hash< Identifier >::operator () (const Identifier &id) const
{
   return hash<wxString>{}( id.GET() );
}

wxString wxToString( const Identifier& str ) { return str.GET(); }

int Identifier::Compare(const Identifier &a, const Identifier &b)
{
   auto &astr = a.value, &bstr = b.value;
   auto aEnd = astr.end(), bEnd = bstr.end();
   auto pair = std::mismatch(astr.begin(), aEnd, bstr.begin(), bEnd);
   if (pair.first == aEnd)
      return pair.second == bEnd ? 0 : -1;
   if (pair.second == bEnd)
      return 1;
   return *pair.first - *pair.second;
}

int Identifier::CompareNoCase(const Identifier &a, const Identifier &b)
{
   auto &astr = a.value, &bstr = b.value;
   auto aEnd = astr.end(), bEnd = bstr.end();
   auto pair = std::mismatch(astr.begin(), aEnd, bstr.begin(), bEnd,
      [](char_type achar, char_type bchar){
         return std::towupper(achar) == std::towupper(bchar); });
   if (pair.first == aEnd)
      return pair.second == bEnd ? 0 : -1;
   if (pair.second == bEnd)
      return 1;
   return std::towupper(*pair.first) - std::towupper(*pair.second);
}
