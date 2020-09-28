/**********************************************************************

  Audacity: A Digital Audio Editor

  Benchmark.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_BENCHMARK__
#define __AUDACITY_BENCHMARK__

#include "Audacity.h"

class AudacityProject;

AUDACITY_DLL_API
void RunBenchmark( wxWindow *parent, AudacityProject &project );

#endif // define __AUDACITY_BENCHMARK__
