set( SOURCES
   BrushHandle.h
   BrushHandle.cpp
   SpectralDataDialog.cpp
   SpectralDataManager.cpp
   SpectralDataManager.h
   SpectralSelectHandle.cpp
   SpectralSelectHandle.h
   SpectrumCache.cpp
   SpectrumCache.h
   SpectrumPrefs.cpp
   SpectrumPrefs.h
   SpectrumVRulerControls.cpp
   SpectrumVRulerControls.h
   SpectrumVZoomHandle.cpp
   SpectrumVZoomHandle.h
   SpectrumView.cpp
   SpectrumView.h
   SpectrogramSettings.cpp
   SpectrogramSettings.h
)
set( LIBRARIES
   mod-wave-track-ui-interface
   Audacity
)
audacity_module( mod-spectrum-ui "${SOURCES}" "${LIBRARIES}"
   "" "" )
