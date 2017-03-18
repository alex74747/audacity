#ifndef __BIQUAD_H__
#define __BIQUAD_H__

struct BiquadStruct {
   float* pfIn {};
   float* pfOut {};
   float fNumerCoeffs [3] { 1.0f, 0.0f, 0.0f };	// B0 B1 B2
   float fDenomCoeffs [2] { 0.0f, 0.0f };	// A1 A2
   float fPrevIn {};
   float fPrevPrevIn {};
   float fPrevOut {};
   float fPrevPrevOut {};

   BiquadStruct()
   {
      fNumerCoeffs[0] = 1.0f,
         fNumerCoeffs[1] = fNumerCoeffs[2] = 0.0f;
      fDenomCoeffs[0] = fDenomCoeffs[1] = 0.0f;
   }
};



void Biquad_Process (BiquadStruct* pBQ, int iNumSamples);
void ComplexDiv (float fNumerR, float fNumerI, float fDenomR, float fDenomI, float* pfQuotientR, float* pfQuotientI);
bool BilinTransform (float fSX, float fSY, float* pfZX, float* pfZY);
float Calc2D_DistSqr (float fX1, float fY1, float fX2, float fY2);

#endif
