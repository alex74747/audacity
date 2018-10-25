/**********************************************************************

Audacity: A Digital Audio Editor

Handle.cpp

Paul Licameli

**********************************************************************/

#include "../../Audacity.h"
#include "Handle.h"
#include "RefreshCode.h"

UIHandle::~UIHandle()
{
}

void UIHandle::Enter(bool)
{
}

bool UIHandle::HasRotation() const
{
   return false;
}

bool UIHandle::Rotate(bool)
{
   return false;
}

bool UIHandle::HasEscape() const
{
   return false;
}

bool UIHandle::Escape()
{
   return false;
}

void UIHandle::DrawExtras
   (DrawingPass, wxDC *, const wxRegion &, const wxRect &)
{
}

bool UIHandle::StopsOnKeystroke()
{
   return false;
}

void UIHandle::OnProjectChange(AudacityProject *)
{
}
