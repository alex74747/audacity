/**********************************************************************

  Audacity: A Digital Audio Editor

  FileFormats.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_FILE_FORMATS__
#define __AUDACITY_FILE_FORMATS__

#include <wx/list.h>
#include <wx/arrstr.h>
#include <wx/string.h>

#include "Audacity.h"

#include "sndfile.h"

//
// enumerating headers
//

/** @brief Get the number of container formats supported by libsndfile
 *
 * Uses SFC_GET_FORMAT_MAJOR_COUNT in sf_command interface */
unsigned sf_num_headers();

/** @brief Get the name of a container format from libsndfile
 *
 * Uses SFC_GET_FORMAT_MAJOR in the sf_command() interface. Resulting C string
 * from libsndfile is converted to a wxString
 * @param format_num The libsndfile format number for the container format
 * required
 */
wxString sf_header_index_name(unsigned format_num);

unsigned int sf_header_index_to_type(unsigned format_num);

//
// enumerating encodings
//
/** @brief Get the number of data encodings libsndfile supports (in any
 * container or none */
unsigned sf_num_encodings();
/** @brief Get the string name of the data encoding of the requested format
 *
 * uses SFC_GET_FORMAT_SUBTYPE */
wxString sf_encoding_index_name(unsigned encoding_num);
unsigned sf_encoding_index_to_subtype(unsigned encoding_num);

//
// getting info about an actual SF format
//
/** @brief Get the string name of the specified container format
 *
 * AND format with SF_FORMAT_TYPEMASK to get only the container format and
 * then use SFC_GET_FORMAT_INFO to get the description
 * @param format the libsndfile format to get the name for (only the container
 * part is used) */
wxString sf_header_name(unsigned format);
/** @brief Get an abbreviated form of the string name of the specified format
 *
 * Do sf_header_name() then truncate the string at the first space in the name
 * to get just the first word of the format name.
 * @param format the libsndfile format to get the name for (only the container
 * part is used) */
wxString sf_header_shortname(unsigned format);
/** @brief Get the most common file extension for the given format
 *
 * AND the given format with SF_FORMAT_TYPEMASK to get just the container
 * format, then retreive the most common extension using SFC_GET_FORMAT_INFO.
 * @param format the libsndfile format to get the name for (only the container
 * part is used) */
wxString sf_header_extension(unsigned format);
/** @brief Get the string name of the specified data encoding
 *
 * AND encoding_num with SF_FORMAT_SUBMASK to get only the data encoding and
 * then use SFC_GET_FORMAT_INFO to get the description
 * @param encoding_num the libsndfile encoding to get the name for (only the
 * data encoding is used) */
wxString sf_encoding_name(unsigned encoding_num);

//
// simple formats
//

unsigned sf_num_simple_formats();
SF_FORMAT_INFO *sf_simple_format(unsigned i);

//
// other utility functions
//

bool sf_subtype_more_than_16_bits(unsigned format);
bool sf_subtype_is_integer(unsigned format);

wxArrayString sf_get_all_extensions();

wxString sf_normalize_name(const char *name);

//
// Mac OS 4-char type
//

#ifdef __WXMAC__
# ifdef __UNIX__
#  include <CoreServices/CoreServices.h>
# else
#  include <Types.h>
# endif

OSType sf_header_mactype(unsigned format);
#endif

// This function wrapper uses a mutex to serialize calls to the SndFile library.
#include "MemoryX.h"
#include "ondemand/ODTaskThread.h"
extern ODLock libSndFileMutex;
template<typename R, typename F, typename... Args>
inline R SFCall(F fun, Args&&... args)
{
   ODLocker locker{ &libSndFileMutex };
   return fun(std::forward<Args>(args)...);
}

//RAII for SNDFILE*
struct SFFileCloser { void operator () (SNDFILE*) const; };
using SFFile = std::unique_ptr<SNDFILE, SFFileCloser>;

#endif
