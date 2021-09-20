#include "expansion.h"
#include "expand.h"

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
      l /= g->n; //TODO: taking the average here renders further estimation approximate (ie, uncountable)
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

expand_iter estimate_group(group *g)
{
  expand_iter a = 1;
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
    expand_iter accrue_a = 0;
    expand_iter base_a = a;
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
  g->estimate = a;
  return a;
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

void prepare_group_iteration(group *g)
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
      prepare_group_iteration(&g->childs[i]);
}

void unroll_group(group *g, int threshhold, char *devnull)
{
  if(g->child_tag_u || g->child_tag_g) return;
  int doit = 0;
  expand_iter a = estimate_group(g);
  if(a < threshhold)
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
      group ng;
      zero_group(&ng);
      ng.type = GROUP_TYPE_OPTION;
      absorb_tags(&ng,g);
      int utag_dirty = 0; //irrelevant
      int gtag_dirty = 0; //irrelevant
      group *cg;
      int child_malloced = a;
      ng.childs = (group *)safe_malloc(sizeof(group)*child_malloced);
      while(!done)
      {
        done = !sprint_group(g, 0, &utag_dirty, &gtag_dirty, lockholder, &passholder_p, devnull);
        *passholder_p = '\0';
        ng.n++;
        if(ng.n > child_malloced)
        {
          child_malloced *= 2;
          ng.childs = (group *)safe_realloc(ng.childs,sizeof(group)*child_malloced);
        }
        cg = &ng.childs[ng.n-1];
        zero_group(cg);
        cg->type = GROUP_TYPE_CHARS;
        cg->n = passholder_p-passholder;
        cg->chars = (char *)safe_malloc(sizeof(char)*(cg->n+1));
        safe_strcpy(cg->chars,passholder);
        passholder_p = passholder;
      }

      free(passholder);
      free(lockholder);
      free_group_contents(g);
      *g = ng;
    }
  }
  else if(g->type != GROUP_TYPE_CHARS)
  {
    for(int i = 0; i < g->n; i++)
      unroll_group(&g->childs[i], threshhold, devnull);
  }
}

void propagate_countability(group *g)
{
  if(g->type == GROUP_TYPE_CHARS) return;
  for(int i = 0; i < g->n; i++)
  {
    propagate_countability(&g->childs[i]);
    if(!g->childs[i].countable) g->countable = 0;
  }
}

expand_iter state_from_countable_group(group *g)
{
  expand_iter state = 0;
  expand_iter tmp = 0;
  expand_iter mul = 0;
  switch(g->type)
  {
    case GROUP_TYPE_OPTION:
      for(int i = 0; i < g->i; i++)
        state += g->childs[i].estimate;
      state += state_from_countable_group(&g->childs[g->i]);
      break;
    case GROUP_TYPE_PERMUTE:
      state = 1;
      for(int i = 0; i < g->n; i++)
        state *= g->childs[i].estimate;
      state = g->i*state;
      //no break!
    case GROUP_TYPE_SEQUENCE:
      mul = 1;
      for(int i = 0; i < g->n; i++)
          mul *= g->childs[i].estimate;
      for(int i = 0; i < g->n; i++)
      {
        mul /= g->childs[i].estimate;
        state += mul*state_from_countable_group(&g->childs[g->i]);
      }
      break;
    case GROUP_TYPE_CHARS:
    case GROUP_TYPE_MODIFICATION:
    default:
      state = 1; //not stateful
  }

/*
  int l = approximate_length_premodified_group(g);
  for(int i = 0; i < g->n_mods; i++)
  {
    modification *m = &g->mods[i];
    //TODO
  }
*/

  return state;
}

void resume_countable_group(group *g, expand_iter state)
{
  //TODO:
}

void free_modification_contents(modification *m)
{
  if(m->chars)                    free(m->chars);
  if(m->injection_i)              free(m->injection_i);
  if(m->injection_sub_i)          free(m->injection_sub_i);
  if(m->substitution_i)           free(m->substitution_i);
  if(m->substitution_sub_i)       free(m->substitution_sub_i);
  if(m->smart_substitution_i)     free(m->smart_substitution_i);
  if(m->smart_substitution_i_c)   free(m->smart_substitution_i_c);
  if(m->smart_substitution_sub_i) free(m->smart_substitution_sub_i);
  if(m->deletion_i)               free(m->deletion_i);
}

void free_group_contents(group *g)
{
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
    case GROUP_TYPE_OPTION:
    case GROUP_TYPE_PERMUTE:
      for(int i = 0; i < g->n; i++)
        free_group_contents(&g->childs[i]);
      free(g->childs);
      break;
    case GROUP_TYPE_CHARS:
      free(g->chars);
      break;
    case GROUP_TYPE_NULL:
    case GROUP_TYPE_MODIFICATION:
    case GROUP_TYPE_COUNT:
    default:
      //should never hit
      break;
  }
  if(g->n_mods)
  {
    for(int i = 0; i < g->n_mods; i++)
      free_modification_contents(&g->mods[i]);
    free(g->mods);
  }
}

void print_tag(tag t, int u)
{
  if(u) printf("U");
  else  printf("G");
  int first = 1;
  for(int i = 0; i < max_tag_count; i++)
  {
    if(t & (1 << i))
    {
      if(!first)printf(",");first = 0;
      print_number(i+1);
    }
  }
}

void print_seed(group *g, int print_progress, int selected, int indent)
{
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
      for(int i = 0; i < indent; i++) printf("  "); if(selected) printf("*"); printf("<"); if(g->tag_u) print_tag(g->tag_u,1); if(g->tag_g) print_tag(g->tag_g,0); printf("\n");
      for(int i = 0; i < g->n; i++) print_seed(&g->childs[i],print_progress,selected,indent+1);
      for(int i = 0; i < indent; i++) printf("  "); printf(">"); if(selected) printf("*"); printf("\n");
      break;
    case GROUP_TYPE_OPTION:
      for(int i = 0; i < indent; i++) printf("  "); if(selected) printf("*"); if(g->tag_u || g->tag_g) { printf("<"); if(g->tag_u) print_tag(g->tag_u,1); if(g->tag_g) print_tag(g->tag_g,0); } printf("{"); printf("\n");
      for(int i = 0; i < g->n; i++) { print_seed(&g->childs[i],print_progress,print_progress && selected && i == g->i,indent+1); }
      for(int i = 0; i < indent; i++) printf("  "); printf("}"); if(g->tag_u || g->tag_g) printf(">"); if(selected) printf("*"); printf("\n");
      break;
    case GROUP_TYPE_PERMUTE:
      for(int i = 0; i < indent; i++) printf("  "); if(selected) printf("*"); if(g->tag_u || g->tag_g) { printf("<"); if(g->tag_u) print_tag(g->tag_u,1); if(g->tag_g) print_tag(g->tag_g,0); } printf("("); printf("\n");
      if(print_progress) { for(int i = 0; i < g->n; i++) print_seed(&g->childs[cache_permute_indices[g->n][g->i+1][i]],print_progress,selected,indent+1); }
      else               { for(int i = 0; i < g->n; i++) print_seed(&g->childs[i],                                     print_progress,0,indent+1); }
      for(int i = 0; i < indent; i++) printf("  "); printf(")"); if(g->tag_u || g->tag_g) printf(">"); if(selected) printf("*"); printf("\n");
      break;
    case GROUP_TYPE_CHARS:
      for(int i = 0; i < indent; i++) printf("  "); if(selected) printf("*"); if(g->tag_u || g->tag_g) { printf("<"); if(g->tag_u) print_tag(g->tag_u,1); if(g->tag_g) print_tag(g->tag_g,0); } if(g->n) printf("\"%s\"",g->chars); else printf("-"); if(g->tag_u || g->tag_g) printf(">"); if(selected) printf("*"); printf("\n");
      break;
    default: //appease compiler
      break;
  }
  if(g->n_mods)
  {
    for(int i = 0; i < indent; i++) printf("  "); printf("[\n");
    for(int i = 0; i < g->n_mods; i++)
    {
      modification *m = &g->mods[i];
      for(int j = 0; j < indent+1; j++) printf("  ");
      if(m->n_injections+m->n_smart_substitutions+m->n_substitutions+m->n_deletions+m->n_copys == 0) printf("-\n");
      else
      {
        if(m->n_injections)          printf("i%d ",m->n_injections);
        if(m->n_smart_substitutions) printf("m%d ",m->n_smart_substitutions);
        if(m->n_substitutions)       printf("s%d ",m->n_substitutions);
        if(m->n_deletions)           printf("d%d ",m->n_deletions);
        if(m->n_copys)               printf("d%d ",m->n_copys+1);
        if(m->n) printf("\"%s\"",m->chars);
        printf("\n");
      }
    }
    for(int i = 0; i < indent; i++) printf("  "); printf("]\n");
  }
}

