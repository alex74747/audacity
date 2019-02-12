/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 @file wxArrayStringEx.cpp
 
 Paul Licameli split from MemoryX.h
 
 **********************************************************************/

// This .cpp exists just to ensure there is at least one that includes
// the header before any other header, and compiles
#include "wxArrayStringEx.h"

StringArray::StringArray( const wxArrayString &strings )
{
}

StringArray::operator wxArrayString() const
{
   return {};
}

int StringArray::Index( const wxString &string, bool bCase ) const
{
   return -1;
}

void StringArray::RemoveAt( size_t index )
{
}

void StringArray::Remove( const wxString &string )
{
}

void StringArray::Insert( const wxString &string, size_t index )
{
}
