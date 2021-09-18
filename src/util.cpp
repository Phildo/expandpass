#include "util.h"

int ***cache_permute_indices;
int n_cached_permute_indices = 0;

void cache_permute_indices_to(int n)
{
  if(n_cached_permute_indices <= n)
  {
    if(!n_cached_permute_indices) cache_permute_indices = (int ***)safe_malloc(sizeof(int **)*(n+1));
    else                          cache_permute_indices = (int ***)safe_realloc(cache_permute_indices,sizeof(int **)*(n+1));
    for(int i = n_cached_permute_indices; i < n+1; i++) cache_permute_indices[i] = 0;
    n_cached_permute_indices = n+1;
  }
  if(cache_permute_indices[n] == 0)
  {
    int f = n;
    for(int i = n-1; i > 1; i--) f *= i;
    cache_permute_indices[n] = (int **)safe_malloc(sizeof(int *)*(f+1));
    cache_permute_indices[n][0] = 0;
    cache_permute_indices[n][0] += f;
    for(int i = 0; i < f; i++)
      cache_permute_indices[n][i+1] = (int *)safe_malloc(sizeof(int)*n);

    //heap's algo- don't really understand
    int *c = (int *)safe_malloc(sizeof(int)*n);
    for(int i = 0; i < n; i++) c[i] = 0;
    for(int i = 0; i < n; i++) cache_permute_indices[n][1][i] = i;
    if(f > 1) for(int i = 0; i < n; i++) cache_permute_indices[n][2][i] = i;
    int i = 0;
    int j = 2;
    while(i < n)
    {
      if(c[i] < i)
      {
        if(i%2 == 0) { int t = cache_permute_indices[n][j][0   ]; cache_permute_indices[n][j][0   ] = cache_permute_indices[n][j][i]; cache_permute_indices[n][j][i] = t; }
        else         { int t = cache_permute_indices[n][j][c[i]]; cache_permute_indices[n][j][c[i]] = cache_permute_indices[n][j][i]; cache_permute_indices[n][j][i] = t; }
        j++;
        if(j < f+1) for(int i = 0; i < n; i++) cache_permute_indices[n][j][i] = cache_permute_indices[n][j-1][i];
        c[i]++;
        i = 0;
      }
      else
      {
        c[i] = 0;
        i++;
      }
    }
    free(c);

    /*
    //verify
    for(int i = 0; i < f; i++)
    {
      for(int j = 0; j < n; j++)
      {
        printf("%d",cache_permute_indices[n][i+1][j]);
      }
      printf("\n");
    }
    */
  }
}

