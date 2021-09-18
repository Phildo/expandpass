#include "validate.h"

int validate(validation *v, char *passholder, int len)
{
  if(v->length_min <= len && v->length_max >= len)
  {
    int alpha_e            = 0;
    int up_alpha_e         = 0;
    int low_alpha_e        = 0;
    int numeric_e          = 0;
    int alphanumeric_e     = 0;
    int non_alphanumeric_e = 0;
    while(*passholder != '\0')
    {
           if(*passholder >= 'A' && *passholder <= 'Z') { up_alpha_e++;  alpha_e++; alphanumeric_e++; }
      else if(*passholder >= 'a' && *passholder <= 'z') { low_alpha_e++; alpha_e++; alphanumeric_e++; }
      else if(*passholder >= '0' && *passholder <= '9') { numeric_e++;              alphanumeric_e++; }
      else                                              { non_alphanumeric_e++;                       }
      passholder++;
    }
    if(
      alpha_e            >= v->alpha &&
      up_alpha_e         >= v->up_alpha &&
      low_alpha_e        >= v->low_alpha &&
      numeric_e          >= v->numeric &&
      alphanumeric_e     >= v->alphanumeric &&
      non_alphanumeric_e >= v->non_alphanumeric
    )
      return 1;
  }
  return false;
}

