/**********************************************************************

   Audacity - A Digital Audio Editor
   Copyright 1999-2009 Audacity Team
   License: GPL v2 - see LICENSE.txt

   Dan Horgan

******************************************************************//**

\file TimeWarper.cpp
\brief Contains definitions for IdentityTimeWarper, ShiftTimeWarper,
LinearTimeWarper, LogarithmicTimeWarper, QuadraticTimeWarper,
Geometric TimeWarper classes

*//*******************************************************************/

#include "Audacity.h"
#include "TimeWarper.h"

#include <wx/string.h>
#include <math.h>

TimeWarper IdentityTimeWarper()
{
   return [](double originalTime ){ return originalTime; };
}

TimeWarper ShiftTimeWarper(
   const TimeWarper &warper, double shiftAmount )
{
   return [warper, shiftAmount]( double originalTime ) {
      return warper(originalTime + shiftAmount);
   };
}

TimeWarper LinearTimeWarper(
   double tBefore0, double tAfter0,
   double tBefore1, double tAfter1 )
{
   auto scale = ((tAfter1 - tAfter0)/(tBefore1 - tBefore0)),
      shift = (tAfter0 - scale * tBefore0);
   return [scale, shift]( double originalTime ){
      return originalTime * scale + shift;
   };
}


TimeWarper LinearInputRateTimeWarper(
   double tStart, double tEnd,
   double rStart, double rEnd)
{
   wxASSERT(rStart != 0.0);
   wxASSERT(tStart < tEnd);
   auto warper = LinearTimeWarper(tStart, rStart, tEnd, rEnd);
   auto scale = ((tEnd-tStart)/(rEnd-rStart));
   return [warper, tStart, rStart, scale]( double originalTime ){
      auto rate = warper(originalTime);
      return tStart + scale * log(rate/rStart);
   };
}

TimeWarper LinearOutputRateTimeWarper(
   double tStart, double tEnd,
   double rStart, double rEnd)
{
   wxASSERT(rStart != rEnd);
   wxASSERT(rStart > 0.0);
   wxASSERT(rEnd > 0.0);
   wxASSERT(tStart < tEnd);
   
  auto scale = (2.0*(tEnd-tStart)/(rEnd*rEnd-rStart*rStart)),
     c1 = (rStart*rStart), c2 = (rEnd*rEnd-rStart*rStart);
   auto warper = LinearTimeWarper(tStart, 0.0, tEnd, 1.0);
   return [warper, tStart, rStart, scale, c1, c2]( double originalTime ){
      double scaledTime = warper(originalTime);
      return tStart + scale * (sqrt(c1 + scaledTime * c2) - rStart);
   };
}

TimeWarper LinearInputStretchTimeWarper(
   double tStart, double tEnd,
   double rStart, double rEnd )
{
   wxASSERT(rStart > 0.0);
   wxASSERT(rEnd > 0.0);
   wxASSERT(tStart < tEnd);
   
   auto warper = LinearTimeWarper(tStart, 0.0, tEnd, 1.0);
   auto c1 = ((tEnd-tStart)/rStart), c2 = (0.5*(rStart/rEnd - 1.0));
   return [warper, tStart, c1, c2]( double originalTime ){
      double scaledTime = warper(originalTime);
      return tStart + c1 * scaledTime * (1.0 + c2 * scaledTime);
   };
}

TimeWarper LinearOutputStretchTimeWarper(
   double tStart, double tEnd,
   double rStart, double rEnd)
{
   wxASSERT(rStart != rEnd);
   wxASSERT(rStart > 0.0);
   wxASSERT(rEnd > 0.0);
   wxASSERT(tStart < tEnd);
   
   auto warper = LinearTimeWarper(tStart, 0.0, tEnd, 1.0);
   auto c1 = ((tEnd-tStart)/(rStart*log(rStart/rEnd))), c2 = (rStart/rEnd);
   return [warper, tStart, c1, c2]( double originalTime ){
      double scaledTime = warper(originalTime);
      return tStart + c1 * (pow(c2, scaledTime) - 1.0);
   };
}

TimeWarper GeometricInputTimeWarper(
   double tStart, double tEnd,
   double rStart, double rEnd)
{
   wxASSERT(rStart != rEnd);
   wxASSERT(rStart > 0.0);
   wxASSERT(rEnd > 0.0);
   wxASSERT(tStart < tEnd);

   auto warper = LinearTimeWarper(tStart, 0.0, tEnd, 1.0);
   auto scale = ((tEnd-tStart)/(log(rStart/rEnd)*rStart)), ratio = (rStart/rEnd);
   return [warper, tStart, scale, ratio]( double originalTime ){
      double scaledTime = warper(originalTime);
      return tStart + scale * (pow(ratio,scaledTime) - 1.0);
   };
}

TimeWarper GeometricOutputTimeWarper(
   double tStart, double tEnd,
   double rStart, double rEnd)
{
   wxASSERT(rStart > 0.0);
   wxASSERT(rEnd > 0.0);
   wxASSERT(tStart < tEnd);

   auto warper = LinearTimeWarper(tStart, 0.0, tEnd, 1.0);
   auto scale = ((tEnd-tStart)/(rEnd-rStart)), c0 = ((rEnd-rStart)/rStart);
   return [warper, tStart, scale, c0]( double originalTime ){
      double scaledTime = warper(originalTime);
      return tStart + scale * log1p(c0 * scaledTime);
   };
}

TimeWarper StepTimeWarper(double tStep, double offset)
{
   return [tStep, offset]( double originalTime ){
      return originalTime + ((originalTime > tStep) ? offset : 0.0);
   };
}

TimeWarper RegionTimeWarper(
   double tStart, double tEnd, const TimeWarper &warper )
{
   auto offset = (warper(tEnd) - tEnd);
   return [warper, tStart, tEnd, offset]( double originalTime ) {
      if (originalTime < tStart)
      {
         return originalTime;
      }
      else if (originalTime < tEnd)
      {
         return warper(originalTime);
      }
      else
      {
         return offset + originalTime;
      }
   };
}

