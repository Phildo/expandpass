#ifndef VALIDATE_H
#define VALIDATE_H

struct validation
{
  int alpha;
  int up_alpha;
  int low_alpha;
  int numeric;
  int alphanumeric;
  int non_alphanumeric;
  int length_min;
  int length_max;
};
void zero_validation(validation *v)
{
  v->alpha            = 0;
  v->up_alpha         = 0;
  v->low_alpha        = 0;
  v->numeric          = 0;
  v->alphanumeric     = 0;
  v->non_alphanumeric = 0;
  v->length_min       = 0;
  v->length_max       = max_pass_len;
}

int validate(validation *v, char *passholder, int len);

#endif //VALIDATE_H
