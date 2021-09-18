#include "expansion.h"
#include "expand.h"

float approximate_length_premodified_group(group *g)
{
  float l = 0;
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
      l = 0;
      for(int i = 0; i < g->n; i++)
        l += approximate_length_modified_group(&g->childs[i]);
      break;
    case GROUP_TYPE_OPTION:
      l = 0;
      for(int i = 0; i < g->n; i++)
        l += approximate_length_modified_group(&g->childs[i]);
      l /= g->n;
      break;
    case GROUP_TYPE_PERMUTE:
      l = 0;
      for(int i = 0; i < g->n; i++)
        l += approximate_length_modified_group(&g->childs[i]);
      break;
    case GROUP_TYPE_CHARS:
      l = g->n;
      break;
    default: //appease compile
      break;
  }
  return l;
}

float approximate_length_modified_group(group *g)
{
  float l = approximate_length_premodified_group(g);
  if(g->n_mods)
  {
    float accrue_a = 0;
    int a = 0;
    for(int i = 0; i < g->n_mods; i++)
    {
      modification *m = &g->mods[i];
    //if(l > 0                                             && m->n_smart_substitutions > 0) ; //do nothing
      if(l+m->n_injections > 0                             && m->n_injections          > 0) a += m->n_injections;
    //if(l-m->n_smart_substitutions > 0                    && m->n_substitutions       > 0) ; //do nothing
      if(l-m->n_smart_substitutions-m->n_substitutions > 0 && m->n_deletions           > 0) a -= m->n_deletions;
      accrue_a += a;
      a = 0;
    }
    l += accrue_a/g->n_mods;
  }

  return l;
}

void absorb_tags(group *dst, group *src)
{
  dst->tag_u |= src->tag_u;
  dst->tag_g |= src->tag_g;
  src->tag_u = 0;
  src->tag_g = 0;
}

void absorb_gamuts(modification *m, group *g)
{
  for(int i = 0; i < g->n_mods; i++)
  {
    if(g->mods[i].n == m->n && g->mods[i].chars != m->chars)
    {
      char *a = g->mods[i].chars;
      char *b = m->chars;
      int j;
      for(j = 0; j < m->n && *a == *b; j++)
      {
        a++;
        b++;
      }
      if(j == m->n) //equal
      {
        free(g->mods[i].chars);
        g->mods[i].chars = m->chars;
      }
    }
  }
  if(g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION || g->type == GROUP_TYPE_PERMUTE)
  {
    for(int i = 0; i < g->n; i++)
      absorb_gamuts(m, &g->childs[i]);
  }
}

void elevate_tags(group *g, group *parent, group *parent_option_child)
{
  if(parent && parent->type == GROUP_TYPE_OPTION) parent_option_child = g;
  else if(parent_option_child) absorb_tags(parent_option_child,g);
  if(g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION || g->type == GROUP_TYPE_PERMUTE)
    for(int i = 0; i < g->n; i++) elevate_tags(&g->childs[i],g,parent_option_child);

  g->sum_tag_u = g->tag_u | g->child_tag_u;
  g->sum_tag_g = g->tag_g | g->child_tag_g;
  g->zerod_sum_tag_u |= g->tag_u;
  g->zerod_sum_tag_g |= g->tag_g;

  if(parent)
  {
    parent->child_tag_u |= (g->tag_u | g->child_tag_u);
    parent->child_tag_g |= (g->tag_g | g->child_tag_g);
    if(parent->type != GROUP_TYPE_OPTION || g == &parent->childs[0])
    {
      parent->zerod_sum_tag_u |= g->zerod_sum_tag_u;
      parent->zerod_sum_tag_g |= g->zerod_sum_tag_g;
    }
  }
}

unsigned long long int estimate_group(group *g)
{
  unsigned long long int a = 1;
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
      a = 1;
      for(int i = 0; i < g->n; i++)
        a *= estimate_group(&g->childs[i]);
      break;
    case GROUP_TYPE_OPTION:
      a = 0;
      for(int i = 0; i < g->n; i++)
        a += estimate_group(&g->childs[i]);
      break;
    case GROUP_TYPE_PERMUTE:
      a = 1;
      for(int i = 0; i < g->n; i++)
      {
        a *= estimate_group(&g->childs[i]);
        a *= (i+1);
      }
      break;
    case GROUP_TYPE_CHARS:
      a = 1;
      break;
    default: //appease compile
      break;
  }
  if(g->n_mods)
  {
    int l = approximate_length_premodified_group(g);
    unsigned long long int accrue_a = 0;
    unsigned long long int base_a = a;
    a = 1;
    for(int i = 0; i < g->n_mods; i++)
    {
      modification *m = &g->mods[i];
      if(l > 0                                             && m->n_smart_substitutions > 0) a *= permcomb(l,                                             m->n_smart_substitutions, 2); //assuming 2 is average smart sub
      if(l+m->n_injections > 0                             && m->n_injections          > 0) a *= permcomb(l+m->n_injections,                             m->n_injections,          m->n);
      if(l-m->n_smart_substitutions > 0                    && m->n_substitutions       > 0) a *= permcomb(l-m->n_smart_substitutions,                    m->n_substitutions,       m->n);
      if(l-m->n_smart_substitutions-m->n_substitutions > 0 && m->n_deletions           > 0) a *= permcomb(l-m->n_smart_substitutions-m->n_substitutions, m->n_deletions,           1);
      accrue_a += a*base_a;
      a = 1;
    }
    a = accrue_a;
  }
  return a;
}

group *unroll_group(group *g, int threshhold, char *devnull)
{
  int doit = 0;
  if(g->child_tag_u) return g;
  if(g->child_tag_g) return g;
  if(estimate_group(g) < threshhold)
  {
    if(!doit && g->n_mods) doit = 1;
    if(!doit && (g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_PERMUTE)) doit = 1;
    if(!doit && g->type == GROUP_TYPE_OPTION)
    {
      for(int i = 0; !doit && i < g->n; i++)
        if(g->childs[i].n_mods || g->childs[i].type != GROUP_TYPE_CHARS)
          doit = 1;
    }
    if(doit)//if !doit, then it's already optimal options or chars
    {
      char *passholder = (char *)safe_malloc(sizeof(char)*max_pass_len);
      char *passholder_p = passholder;
      char *lockholder = (char *)safe_malloc(sizeof(char)*max_pass_len);
      for(int i = 0; i < max_pass_len; i++) lockholder[i] = 0;

      int done = 0;
      group *ng = (group *)safe_malloc(sizeof(group));
      zero_group(ng);
      ng->type = GROUP_TYPE_OPTION;
      absorb_tags(ng,g);
      int utag_dirty = 0; //irrelevant
      int gtag_dirty = 0; //irrelevant
      group *cg;
      while(!done)
      {
        done = !sprint_group(g, 0, &utag_dirty, &gtag_dirty, lockholder, &passholder_p, devnull);
        *passholder_p = '\0';
        ng->n++;
        if(ng->n == 1) ng->childs = (group *)safe_malloc(sizeof(group));
        else           ng->childs = (group *)safe_realloc(ng->childs,sizeof(group)*ng->n);
        cg = &ng->childs[ng->n-1];
        zero_group(cg);
        cg->type = GROUP_TYPE_CHARS;
        cg->n = passholder_p-passholder;
        cg->chars = (char *)safe_malloc(sizeof(char)*(cg->n+1));
        safe_strcpy(cg->chars,passholder);
        passholder_p = passholder;
      }

      free(passholder);
      free(lockholder);
      //TODO: recursively free g's contents, g's modifications, etc... (but leave g to be freed by caller)
      return ng;
    }
  }
  else if(g->type != GROUP_TYPE_CHARS)
  {
    group *og;
    group *ng;
    for(int i = 0; i < g->n; i++)
    {
      og = &g->childs[i];
      ng = unroll_group(og, threshhold, devnull);
      if(og != ng)
      {
        *og = *ng;
        free(ng);
      }
    }
  }
  return g;
}

void preprocess_group(group *g)
{
  //alloc/init modifications
  modification *m;
  for(int i = 0; i < g->n_mods; i++)
  {
    m = &g->mods[i];

    if(m->n_injections)          { m->injection_i          = (int *)safe_malloc(sizeof(int)*m->n_injections);          for(int j = 0; j < m->n_injections;          j++) m->injection_i[j]          = 0; }
    if(m->n_smart_substitutions) { m->smart_substitution_i = (int *)safe_malloc(sizeof(int)*m->n_smart_substitutions); for(int j = 0; j < m->n_smart_substitutions; j++) m->smart_substitution_i[j] = j; m->smart_substitution_i_c = (char *)safe_malloc(sizeof(char)*m->n_smart_substitutions); }
    if(m->n_substitutions)       { m->substitution_i       = (int *)safe_malloc(sizeof(int)*m->n_substitutions);       for(int j = 0; j < m->n_substitutions;       j++) m->substitution_i[j]       = j; }
    if(m->n_deletions)           { m->deletion_i           = (int *)safe_malloc(sizeof(int)*m->n_deletions);           for(int j = 0; j < m->n_deletions;           j++) m->deletion_i[j]           = 0; }

    if(m->n_injections)          { m->injection_sub_i          = (int *)safe_malloc(sizeof(int)*m->n_injections);          for(int j = 0; j < m->n_injections;          j++) m->injection_sub_i[j]          = 0; }
    if(m->n_smart_substitutions) { m->smart_substitution_sub_i = (int *)safe_malloc(sizeof(int)*m->n_smart_substitutions); for(int j = 0; j < m->n_smart_substitutions; j++) m->smart_substitution_sub_i[j] = 0; }
    if(m->n_substitutions)       { m->substitution_sub_i       = (int *)safe_malloc(sizeof(int)*m->n_substitutions);       for(int j = 0; j < m->n_substitutions;       j++) m->substitution_sub_i[j]       = 0; }
  }

  //ensure permutations cached
  if(g->type == GROUP_TYPE_PERMUTE) cache_permute_indices_to(g->n);

  //recurse
  if(g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION || g->type == GROUP_TYPE_PERMUTE)
    for(int i = 0; i < g->n; i++)
      preprocess_group(&g->childs[i]);
}

void collapse_group(group *g, group *root, int handle_gamuts)
{
  //remove null modifications
  if(g->n_mods == 1 && g->mods[0].n_injections+g->mods[0].n_smart_substitutions+g->mods[0].n_substitutions+g->mods[0].n_deletions+g->mods[0].n_copys == 0)
  {
    g->n_mods = 0;
    free(g->mods);
  }

  //collapse single-or-fewer-child, no modification groups
  if(g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION || g->type == GROUP_TYPE_PERMUTE)
  {
    for(int i = 0; i < g->n; i++)
    {
      group *c = &g->childs[i];
      if(c->type == GROUP_TYPE_SEQUENCE || c->type == GROUP_TYPE_OPTION || c->type == GROUP_TYPE_PERMUTE)
      {
        if(c->n == 1 && c->n_mods == 0 && !c->tag_g) //if c has group tag, then being single-member is potentially meaningful
        {
          absorb_tags(&c->childs[0],c); //childs[0] will become c
          group *oc = c->childs;
          *c = c->childs[0];
          free(oc);
          i--;
        }
        else if(c->n < 1)
        {
          for(int j = i+1; j < g->n; j++)
            g->childs[j-1] = g->childs[j];
          //TODO: free memory associated with modifications, etc... (though there really shouldn't be any)
          g->n--;
          i--;
        }
      }
    }
  }

  //collapse like-parent/child options/sequences
  if(g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION)
  {
    for(int i = 0; i < g->n; i++)
    {
      group *c = &g->childs[i];
      if(c->n_mods == 0 && c->type == g->type)
      {
        if(g->type == GROUP_TYPE_OPTION && c->tag_u) continue;
        absorb_tags(g,c);
        g->childs = (group *)safe_realloc(g->childs,sizeof(group)*(g->n-1+c->n));
        c = &g->childs[i]; //need to re-assign c, as g has been realloced
        group *oc = c->childs;
        int ocn = c->n;
        for(int j = 0; j < g->n-i; j++)
          g->childs[g->n-1+ocn-j-1] = g->childs[g->n-1-j];
        for(int j = 0; j < ocn; j++)
          g->childs[i+j] = oc[j];
        free(oc);
        g->n += ocn-1;
        i--;
      }
    }
  }

  //collapse gamuts
  if(handle_gamuts) //gives option to leave gamuts un-absorbed so they can be freed in further optimization
  {
    modification *m;
    for(int i = 0; i < g->n_mods; i++)
    {
      m = &g->mods[i];
      if(m->n > 0)
        absorb_gamuts(m, root);
    }
  }

  //recurse
  if(g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION || g->type == GROUP_TYPE_PERMUTE)
  {
    for(int i = 0; i < g->n; i++)
      collapse_group(&g->childs[i], root, handle_gamuts);
  }
}

