/**********************************************************************

Audacity: A Digital Audio Editor

NumberScale.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_NUMBER_SCALE__
#define __AUDACITY_NUMBER_SCALE__

#include <algorithm>
#include <cmath>
#include <wx/defs.h>
#include <wx/debug.h>

enum NumberScaleType {
   nstLinear,
   nstLogarithmic,
   nstMel,
   nstBark,
   nstErb,
   nstPeriod,

   nstNumScaleTypes,
};


class NumberScale
{
public:
   NumberScale()
      : mType(nstLinear), mValue0(0), mValue1(1)
   {}

   NumberScale(NumberScaleType type, float value0, float value1)
      : mType(type)
   {
      switch (mType) {
      case nstLinear:
      {
         mValue0 = value0;
         mValue1 = value1;
      }
      break;
      case nstLogarithmic:
      {
         mValue0 = logf(value0);
         mValue1 = logf(value1);
      }
      break;
      case nstMel:
      {
         mValue0 = hzToMel(value0);
         mValue1 = hzToMel(value1);
      }
      break;
      case nstBark:
      {
         mValue0 = hzToBark(value0);
         mValue1 = hzToBark(value1);
      }
      break;
      case nstErb:
      {
         mValue0 = hzToErb(value0);
         mValue1 = hzToErb(value1);
      }
      break;
      case nstPeriod:
      {
         mValue0 = hzToPeriod(value0);
         mValue1 = hzToPeriod(value1);
      }
      break;
      default:
         wxASSERT(false);
      }
   }

   NumberScale Reversal() const
   {
      NumberScale result(*this);
      std::swap(result.mValue0, result.mValue1);
      return result;
   }

   bool operator == (const NumberScale& other) const
   {
      return mType == other.mType
         && mValue0 == other.mValue0
         && mValue1 == other.mValue1;
   }

   bool operator != (const NumberScale &other) const
   {
      return !(*this == other);
   }

   static inline float hzToMel(float hz)
   {
      return 1127 * logf(1 + hz / 700.0f);
   }

   static inline float melToHz(float mel)
   {
      return 700 * (expf(mel / 1127) - 1);
   }

   static inline float hzToBark(float hz)
   {
      // Traunmueller's formula
      const float z1 = 26.81f * hz / (1960 + hz) - 0.53f;
      if (z1 < 2.0f)
         return z1 + 0.15f * (2.0f - z1);
      else if (z1 > 20.1f)
         return z1 + 0.22f * (z1 - 20.1f);
      else
         return z1;
   }

   static inline float barkToHz(float z1)
   {
      if (z1 < 2.0f)
         z1 = 2.0f + (z1 - 2.0f) / 0.85f;
      else if (z1 > 20.1f)
         z1 = 20.1f + (z1 - 20.1f) / 1.22f;
      return 1960 * (z1 + 0.53f) / (26.28f - z1);
   }

   static inline float hzToErb(float hz)
   {
      return 11.17268f * logf(1 + (46.06538f * hz) / (hz + 14678.49f));
   }

   static inline float erbToHz(float erb)
   {
      return 676170.4f / (47.06538f - expf(0.08950404f * erb)) - 14678.49f;
   }

   static inline float hzToPeriod(float hz)
   {
      return -1.0f / std::max (1.0f, hz);
   }

   static inline float periodToHz(float u)
   {
      return -1.0f / u;
   }

   // Random access
   float PositionToValue(float pp) const
   {
      switch (mType) {
      default:
         wxASSERT(false);
      case nstLinear:
         return mValue0 + pp * (mValue1 - mValue0);
      case nstLogarithmic:
         return expf(mValue0 + pp * (mValue1 - mValue0));
      case nstMel:
         return melToHz(mValue0 + pp * (mValue1 - mValue0));
      case nstBark:
         return barkToHz(mValue0 + pp * (mValue1 - mValue0));
      case nstErb:
         return erbToHz(mValue0 + pp * (mValue1 - mValue0));
      case nstPeriod:
         return periodToHz(mValue0 + pp * (mValue1 - mValue0));
      }
   }

   // STL-idiom iteration

   class Iterator
   {
   public:
      Iterator(NumberScaleType type, float step, float value)
         : mType(type), mStep(step), mValue(value)
      {
      }

      float operator * () const
      {
         switch (mType) {
         default:
            wxASSERT(false);
         case nstLinear:
         case nstLogarithmic:
            return mValue;
         case nstMel:
            return melToHz(mValue);
         case nstBark:
            return barkToHz(mValue);
         case nstErb:
            return erbToHz(mValue);
         case nstPeriod:
            return periodToHz(mValue);
         }
      }

      Iterator &operator ++()
      {
         switch (mType) {
         case nstLinear:
         case nstMel:
         case nstBark:
         case nstErb:
         case nstPeriod:
            mValue += mStep;
            break;
         case nstLogarithmic:
            mValue *= mStep;
            break;
         default:
            wxASSERT(false);
         }
         return *this;
      }

   private:
      const NumberScaleType mType;
      const float mStep;
      float mValue;
   };

   Iterator begin(float nPositions) const
   {
      switch (mType) {
      default:
         wxASSERT(false);
      case nstLinear:
      case nstMel:
      case nstBark:
      case nstErb:
      case nstPeriod:
         return Iterator
            (mType,
             nPositions == 1 ? 0 : (mValue1 - mValue0) / (nPositions - 1),
             mValue0);
      case nstLogarithmic:
         return Iterator
            (mType,
             nPositions == 1 ? 1 : exp((mValue1 - mValue0) / (nPositions - 1)),
             expf(mValue0));
      }
   }

   // Inverse
   float ValueToPosition(float val) const
   {
      switch (mType) {
      default:
         wxASSERT(false);
      case nstLinear:
         return ((val - mValue0) / (mValue1 - mValue0));
      case nstLogarithmic:
         return ((logf(val) - mValue0) / (mValue1 - mValue0));
      case nstMel:
         return ((hzToMel(val) - mValue0) / (mValue1 - mValue0));
      case nstBark:
         return ((hzToBark(val) - mValue0) / (mValue1 - mValue0));
      case nstErb:
         return ((hzToErb(val) - mValue0) / (mValue1 - mValue0));
      case nstPeriod:
         return ((hzToPeriod(val) - mValue0) / (mValue1 - mValue0));
      }
   }

private:
   NumberScaleType mType;
   float mValue0;
   float mValue1;
};

#endif
