#pragma once

#include "quantum.h"

#define KNO KC_NO

#define LAYOUT_all( \
  K00, K01, K02, K03, K04,      K06, K07, K08, K09, K0A, K0B, K0C, K0D,      K0F, \
  K10, K11, K12, K13, K14, K15, K16, K17, K18, K19, K1A, K1B, K1C, K1D, K1E, K1F, \
  K20, K21, K22, K23, K24, K25, K26, K27, K28, K29, K2A, K2B, K2C, K2D,      K2F, \
  K30, K31, K32, K33, K34, K35, K36, K37, K38, K39, K3A, K3B, K3C, K3D,      K3F, \
  K40, K41, K42, K43, K44, K45, K46, K47, K48, K49, K4A, K4B, K4C, K4D,      K4F, \
  K50, K51, K52,                K56,           K59, K5A, K5B, K5C, K5D,      K5F \
) { \
  {  K00, K01, K02, K03, K04, KNO, K06, K07, K08, K09, K0A, K0B, K0C, K0D, KNO, K0F  }, \
  {  K10, K11, K12, K13, K14, K15, K16, K17, K18, K19, K1A, K1B, K1C, K1D, K1E, K1F  }, \
  {  K20, K21, K22, K23, K24, K25, K26, K27, K28, K29, K2A, K2B, K2C, K2D, KNO, K2F  }, \
  {  K30, K31, K32, K33, K34, K35, K36, K37, K38, K39, K3A, K3B, K3C, K3D, KNO, K3F  }, \
  {  K40, K41, K42, K43, K44, K45, K46, K47, K48, K49, K4A, K4B, K4C, K4D, KNO, K4F  }, \
  {  K50, K51, K52, KNO, KNO, KNO, K56, KNO, KNO, K59, K5A, K5B, K5C, K5D, KNO, K5F  }  \
}
