#ifndef UTIL_H
#define UTIL_H

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cstdarg>
#ifndef _WIN32
#include <unistd.h> //for isatty()
#endif //!_WIN32

static const int max_sprintf_len = 1024;

//#define BOGUS_SAFETY //"safe" functions just quit on anything funky (allows compilation on VS without complaint)
#ifdef BOGUS_SAFETY
void *safe_malloc(size_t size)
{
  void *ptr = malloc(size);
  if(!ptr)
  {
    fprintf(stderr,"malloc failed");
    exit(1);
  }
  return ptr;
}
void *safe_realloc(void *ptr, size_t size)
{
  void *nptr = realloc(ptr,size);
  if(!nptr)
  {
    fprintf(stderr,"realloc failed");
    exit(1);
  }
  return nptr;
}
char *safe_strcpy(char *dst, char *src)
{
  errno_t ret = strcpy_s(dst,max_read_line_len,src); //this is extra bogus; does NOT protect against overflow!
  if(ret)
  {
    fprintf(stderr,"strcpy failed");
    exit(1);
  }
  return dst;
}
int safe_sprintf(char *buff, const char *const fmt, ...)
{
  va_list arg;
  va_start(arg,fmt);
  char const *vfmt = va_arg(arg, const char *const);
  int ret = vsprintf_s(buff, max_sprintf_len, fmt, arg);
  if(ret < 0)
  {
    fprintf(stderr,"sprintf failed");
    exit(1);
  }
  va_end(arg);
  return ret;
}
FILE *safe_fopen(const char *filename, const char *mode)
{
  FILE *out;
  errno_t ret = fopen_s(&out, filename, mode);
  if(ret != 0)
  {
    return 0;
    //fprintf(stderr,"fopen failed");
    //exit(1);
  }
  return out;
}
int safe_fscanf(FILE *const stream, const char *const fmt, ...)
{
  va_list arg;
  va_start(arg,fmt);
  char const *vfmt = va_arg(arg, const char *const);
  int ret = fscanf_s(stream,fmt,arg);
  if(ret == 0 || ret == EOF)
  {
    fprintf(stderr,"fscanf failed");
    exit(1);
  }
  va_end(arg);
  return ret;
}
#else
#define safe_malloc malloc
#define safe_realloc realloc
#define safe_strcpy strcpy
#define safe_sprintf sprintf
#define safe_fopen fopen
#define safe_fscanf fscanf
#endif

#ifdef _WIN32
size_t getline(char **lineptr, size_t *n, FILE *stream) //hack recreation of this c11 api
{
  const int bsize = max_read_line_len;
  char buffer[bsize];
  if(fgets(buffer,*n,stream) == 0) return 0;
  for(int i = 0; i < bsize; i++)
  {
    if(buffer[i] == '\n' || buffer[i] == '\0')
    {
      if(i == bsize) { fprintf(stderr,"Error reading too large a line on windows (this is not officially supported with windows)"); exit(1); }
      int len = i+1;
      char hold = buffer[len];
      buffer[len] = '\0';
      safe_strcpy(*lineptr,buffer);
      buffer[len] = hold;
      for(i++; i < bsize; i++)
      {
        if(buffer[i] == '\0')
        {
          fseek(stream,-(i-len),SEEK_CUR);//rewind
          return len+1;
        }
      }
      fseek(stream,-(bsize-len),SEEK_CUR);//rewind
      return len+1;
    }
  }
  fprintf(stderr,"Error reading too large a line on windows (this is not officially supported with windows)"); exit(1);
}
#endif

inline int parse_number(char *b, int *n)
{
  int d = 0;
  if(!(*b >= '0' && *b <= '9')) return -1;
  *n = 0;
  while(*b >= '0' && *b <= '9')
  {
    *n *= 10;
    *n += (*b-'0');
    b++;
    d++;
  }
  return d;
}
inline void print_number(int n)
{
  int place = 10;
  while(place <= n) place *= 10;
  place /= 10;
  while(place > 0)
  {
    int d = n/place;
    printf("%c",'0'+d);
    n -= d*place;
    place /= 10;
  }
}

extern int ***cache_permute_indices;
extern int n_cached_permute_indices;
void cache_permute_indices_to(int n);

inline unsigned long long int ppow(int choose, int option)
{
  unsigned long long int t = option;
  for(int i = 0; i < choose-1; i++)
    t *= option;
  return t;
}

inline unsigned long long int permcomb(int len, int choose, int option)
{
  if(choose == 1) return option*len;
  if(len == choose) return ppow(choose,option);

  unsigned long long int t = 0;
  for(int len_i = len-1; len_i >= (choose-1); len_i--)
    t += permcomb(len_i, (choose-1), option)*option;

  return t;
}

#endif //UTIL_H

