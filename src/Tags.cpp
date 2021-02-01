/**********************************************************************

  Audacity: A Digital Audio Editor

  Tags.cpp

  Dominic Mazzoni

*******************************************************************//**

\class Tags
\brief ID3 Tags (for MP3)

  This class started as an ID3 tag

  This class holds a few informational tags, such as Title, Author,
  etc. that can be associated with a project or other audio file.
  It is modeled after the ID3 format for MP3 files, and it can
  both import and export ID3 tags from/to MP2, MP3, and AIFF files.

  It can present the user with a dialog for editing this information.

  Use of this functionality requires that libid3tag be compiled in
  with Audacity.

*//****************************************************************//**

\class TagsEditorDialog
\brief Derived from ExpandingToolBar, this dialog allows editing of Tags.

*//*******************************************************************/


#include "Tags.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#include <wx/setup.h> // for wxUSE_* macros
#include <wx/log.h>

#ifndef WX_PRECOMP
// Include your minimal set of headers here, or wx.h
#endif

#include "FileNames.h"
#include "Prefs.h"
#include "Project.h"

static const wxChar *DefaultGenres[] =
{
   L"Blues",
   L"Classic Rock",
   L"Country",
   L"Dance",
   L"Disco",
   L"Funk",
   L"Grunge",
   L"Hip-Hop",
   L"Jazz",
   L"Metal",
   L"New Age",
   L"Oldies",
   L"Other",
   L"Pop",
   L"R&B",
   L"Rap",
   L"Reggae",
   L"Rock",
   L"Techno",
   L"Industrial",
   L"Alternative",
   L"Ska",
   L"Death Metal",
   L"Pranks",
   L"Soundtrack",
   L"Euro-Techno",
   L"Ambient",
   L"Trip-Hop",
   L"Vocal",
   L"Jazz+Funk",
   L"Fusion",
   L"Trance",
   L"Classical",
   L"Instrumental",
   L"Acid",
   L"House",
   L"Game",
   L"Sound Clip",
   L"Gospel",
   L"Noise",
   L"Alt. Rock",
   L"Bass",
   L"Soul",
   L"Punk",
   L"Space",
   L"Meditative",
   L"Instrumental Pop",
   L"Instrumental Rock",
   L"Ethnic",
   L"Gothic",
   L"Darkwave",
   L"Techno-Industrial",
   L"Electronic",
   L"Pop-Folk",
   L"Eurodance",
   L"Dream",
   L"Southern Rock",
   L"Comedy",
   L"Cult",
   L"Gangsta Rap",
   L"Top 40",
   L"Christian Rap",
   L"Pop/Funk",
   L"Jungle",
   L"Native American",
   L"Cabaret",
   L"New Wave",
   L"Psychedelic",
   L"Rave",
   L"Showtunes",
   L"Trailer",
   L"Lo-Fi",
   L"Tribal",
   L"Acid Punk",
   L"Acid Jazz",
   L"Polka",
   L"Retro",
   L"Musical",
   L"Rock & Roll",
   L"Hard Rock",
   L"Folk",
   L"Folk/Rock",
   L"National Folk",
   L"Swing",
   L"Fast-Fusion",
   L"Bebob",
   L"Latin",
   L"Revival",
   L"Celtic",
   L"Bluegrass",
   L"Avantgarde",
   L"Gothic Rock",
   L"Progressive Rock",
   L"Psychedelic Rock",
   L"Symphonic Rock",
   L"Slow Rock",
   L"Big Band",
   L"Chorus",
   L"Easy Listening",
   L"Acoustic",
   L"Humour",
   L"Speech",
   L"Chanson",
   L"Opera",
   L"Chamber Music",
   L"Sonata",
   L"Symphony",
   L"Booty Bass",
   L"Primus",
   L"Porn Groove",
   L"Satire",
   L"Slow Jam",
   L"Club",
   L"Tango",
   L"Samba",
   L"Folklore",
   L"Ballad",
   L"Power Ballad",
   L"Rhythmic Soul",
   L"Freestyle",
   L"Duet",
   L"Punk Rock",
   L"Drum Solo",
   L"A Cappella",
   L"Euro-House",
   L"Dance Hall",
   L"Goa",
   L"Drum & Bass",
   L"Club-House",
   L"Hardcore",
   L"Terror",
   L"Indie",
   L"BritPop",

   // Standard name is offensive (see "http://www.audacityteam.org/forum/viewtopic.php?f=11&t=3924").
   L"Offensive", // L"Negerpunk",

   L"Polsk Punk",
   L"Beat",
   L"Christian Gangsta Rap",
   L"Heavy Metal",
   L"Black Metal",
   L"Crossover",
   L"Contemporary Christian",
   L"Christian Rock",
   L"Merengue",
   L"Salsa",
   L"Thrash Metal",
   L"Anime",
   L"JPop",
   L"Synthpop"
};

static ProjectFileIORegistry::ObjectReaderEntry readerEntry{
   "tags",
   []( AudacityProject &project ){ return &Tags::Get( project ); }
};

static const AudacityProject::AttachedObjects::RegisteredFactory key{
  [](AudacityProject &){ return std::make_shared< Tags >(); }
};

Tags &Tags::Get( AudacityProject &project )
{
   return project.AttachedObjects::Get< Tags >( key );
}

const Tags &Tags::Get( const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

Tags &Tags::Set( AudacityProject &project, const std::shared_ptr< Tags > &tags )
{
   auto &result = *tags;
   project.AttachedObjects::Assign( key, tags );
   return result;
}

Tags::Tags()
{
   LoadDefaults();
   LoadGenres();
}

Tags::~Tags()
{
}

std::shared_ptr<Tags> Tags::Duplicate() const
{
   return std::make_shared<Tags>(*this);
}

void Tags::Merge( const Tags &other )
{
   for ( auto &pair : other.mMap ) {
      SetTag( pair.first, pair.second );
   }
}

Tags & Tags::operator=(const Tags & src)
{
   mXref.clear();
   mXref = src.mXref;
   mMap.clear();
   mMap = src.mMap;

   mGenres.clear();
   mGenres = src.mGenres;

   return *this;
}

void Tags::LoadDefaults()
{
   wxString path;
   wxString name;
   wxString value;
   long ndx;
   bool cont;

   // Set the parent group
   path = gPrefs->GetPath();
   gPrefs->SetPath(L"/Tags");

   // Process all entries in the group
   cont = gPrefs->GetFirstEntry(name, ndx);
   while (cont) {
      gPrefs->Read(name, &value, L"");

      if (name == L"ID3V2") {
         // LLL:  This is obsolute, but it must be handled and ignored.
      }
      else {
         SetTag(name, value);
      }

      cont = gPrefs->GetNextEntry(name, ndx);
   }

   // Restore original group
   gPrefs->SetPath(path);
}

bool Tags::IsEmpty()
{
   // At least one of these should be filled in, otherwise
   // it's assumed that the tags have not been set...
   if (HasTag(TAG_TITLE) || HasTag(TAG_ARTIST) || HasTag(TAG_ALBUM)) {
      return false;
   }

   return true;
}

void Tags::Clear()
{
   mXref.clear();
   mMap.clear();
}

namespace {
   bool EqualMaps(const TagMap &map1, const TagMap &map2)
   {
      // Maps are unordered, hash maps; can't just iterate in tandem and
      // compare.
      if (map1.size() != map2.size())
         return false;

      for (const auto &pair : map2) {
         auto iter = map1.find(pair.first);
         if (iter == map1.end() || iter->second != pair.second)
            return false;
      }

      return true;
   }
}

bool operator== (const Tags &lhs, const Tags &rhs)
{
   if (!EqualMaps(lhs.mXref, rhs.mXref))
      return false;

   if (!EqualMaps(lhs.mMap, rhs.mMap))
      return false;

   return lhs.mGenres == rhs.mGenres;
}

int Tags::GetNumUserGenres()
{
   return mGenres.size();
}

void Tags::LoadDefaultGenres()
{
   mGenres.clear();
   for (size_t i = 0; i < WXSIZEOF(DefaultGenres); i++) {
      mGenres.push_back(DefaultGenres[i]);
   }
}

void Tags::LoadGenres()
{
   wxFileName fn(FileNames::DataDir(), L"genres.txt");
   wxTextFile tf(fn.GetFullPath());

   if (!tf.Exists() || !tf.Open()) {
      LoadDefaultGenres();
      return;
   }

   mGenres.clear();

   int cnt = tf.GetLineCount();
   for (int i = 0; i < cnt; i++) {
      mGenres.push_back(tf.GetLine(i));
   }
}

wxString Tags::GetUserGenre(int i)
{
   if (i >= 0 && i < GetNumUserGenres()) {
      return mGenres[i];
   }

   return L"";
}

wxString Tags::GetGenre(int i)
{
   int cnt = WXSIZEOF(DefaultGenres);

   if (i >= 0 && i < cnt) {
      return DefaultGenres[i];
   }

   return L"";
}

int Tags::GetGenre(const wxString & name)
{
   int cnt = WXSIZEOF(DefaultGenres);

   for (int i = 0; i < cnt; i++) {
      if (name.CmpNoCase(DefaultGenres[i])) {
         return i;
      }
   }

   return 255;
}

bool Tags::HasTag(const wxString & name) const
{
   wxString key = name;
   key.UpperCase();

   auto iter = mXref.find(key);
   return (iter != mXref.end());
}

wxString Tags::GetTag(const wxString & name) const
{
   wxString key = name;
   key.UpperCase();

   auto iter = mXref.find(key);

   if (iter == mXref.end()) {
      return wxEmptyString;
   }

   auto iter2 = mMap.find(iter->second);
   if (iter2 == mMap.end()) {
      wxASSERT(false);
      return wxEmptyString;
   }
   else
      return iter2->second;
}

Tags::Iterators Tags::GetRange() const
{
   return { mMap.begin(), mMap.end() };
}

void Tags::SetTag(const wxString & name, const wxString & value, const bool bSpecialTag)
{
   // We don't like empty names
   if (name.empty()) {
      return;
   }

   // Tag name must be ascii
   if (!name.IsAscii()) {
      wxLogError("Tag rejected (Non-ascii character in name)");
      return;
   }

   // All keys are uppercase
   wxString key = name;
   key.UpperCase();

   // Look it up
   TagMap::iterator iter = mXref.find(key);

   // The special tags, if empty, should not exist.
   // However it is allowable for a custom tag to be empty.
   // See Bug 440 and Bug 1382 
   if (value.empty() && bSpecialTag) {
      // Erase the tag
      if (iter == mXref.end())
         // nothing to do
         ;
      else {
         mMap.erase(iter->second);
         mXref.erase(iter);
      }
   }
   else 
   {
      if (iter == mXref.end()) {
         // Didn't find the tag

         // Add a NEW tag
         mXref[key] = name;
         mMap[name] = value;
      }
      else if (iter->second != name) {
         // Watch out for case differences!
         mMap[name] = value;
         mMap.erase(iter->second);
         iter->second = name;
      }
      else {
         // Update the value
         mMap[iter->second] = value;
      }
   }
}

void Tags::SetTag(const wxString & name, const int & value)
{
   SetTag(name, wxString::Format(L"%d", value));
}

bool Tags::HandleXMLTag(const std::string_view& tag, const AttributesList &attrs)
{
   if (tag == "tags") {
      return true;
   }

   if (tag == "tag") {
      wxString n, v;

      for (auto pair : attrs)
      {
         auto attr = pair.first;
         auto value = pair.second;

         if (attr == "name") {
            n = value.ToWString();
         }
         else if (attr == "value") {
            v = value.ToWString();
         }
      }

      if (n == L"id3v2") {
         // LLL:  This is obsolete, but it must be handled and ignored.
      }
      else {
         SetTag(n, v);
      }

      return true;
   }

   return false;
}

XMLTagHandler *Tags::HandleXMLChild(const std::string_view& tag)
{
   if (tag == "tags") {
      return this;
   }

   if (tag == "tag") {
      return this;
   }

   return NULL;
}

void Tags::WriteXML(XMLWriter &xmlFile) const
// may throw
{
   xmlFile.StartTag(L"tags");

   for (const auto &pair : GetRange()) {
      const auto &n = pair.first;
      const auto &v = pair.second;
      xmlFile.StartTag(L"tag");
      xmlFile.WriteAttr(L"name", n);
      xmlFile.WriteAttr(L"value", v);
      xmlFile.EndTag(L"tag");
   }

   xmlFile.EndTag(L"tags");
}

static ProjectFileIORegistry::ObjectWriterEntry entry {
[](const AudacityProject &project, XMLWriter &xmlFile){
   Tags::Get(project).WriteXML(xmlFile);
}
};
