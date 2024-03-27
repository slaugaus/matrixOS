#include "getColor.h"


static rgb24 colorTable[] = {
  {0, 0, 0},
  {0, 55, 218},
  {19, 161, 14},
  {58, 150, 221},
  {197, 15, 31},
  {136, 23, 152},
  {193, 156, 0},
  {204, 204, 204},
  {118, 118, 118},
  {59, 120, 255},
  {22, 198, 12},
  {97, 214, 214},
  {231, 72, 86},
  {180, 0, 158},
  {249, 241, 165},
  {255, 255, 255}
};

bool invalidColor = false;
rgb24 getColor(char ch){
  invalidColor = false;
  rgb24 color = (rgb24) {0, 0, 0};
    if (ch >= '0' && ch <= '9'){
      ch -= 48;
      color = colorTable[(unsigned char) ch];
    }
    else if (ch >= 'A' && ch <= 'F'){
      ch -= 55;
      color = colorTable[(unsigned char) ch];
    }
    else if (ch >= 'a' && ch <= 'f'){
      ch -= 87;
      color = colorTable[(unsigned char) ch];
    }
    else{
      invalidColor = true;
    }
  return color;
}