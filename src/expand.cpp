#include "util.h"
#include "expand.h"

char smart_sub(int i, char key)
{
  char stub[2] = "a";
  const char *c;
  switch(key)
  {
    case 'a': c = "A4"; break;
    case 'b': c = "B"; break; //68
    case 'c': c = "C"; break;
    case 'd': c = "D"; break;
    case 'e': c = "E3"; break;
    case 'f': c = "F"; break;
    case 'g': c = "G9"; break;
    case 'h': c = "H"; break;
    case 'i': c = "I!1;"; break; //|lL
    case 'j': c = "J"; break;
    case 'k': c = "K"; break;
    case 'l': c = "L!71"; break; //|iI
    case 'm': c = "M"; break;
    case 'n': c = "N"; break;
    case 'o': c = "O0"; break;
    case 'p': c = "P"; break;
    case 'q': c = "Q"; break;
    case 'r': c = "R"; break; //2
    case 's': c = "S5$"; break; //zZ
    case 't': c = "T"; break;
    case 'u': c = "U"; break;
    case 'v': c = "V"; break;
    case 'w': c = "W"; break;
    case 'x': c = "X"; break;
    case 'y': c = "Y"; break;
    case 'z': c = "Z2"; break; //sS
    case 'A': c = "a4"; break;
    case 'B': c = "b"; break; //68
    case 'C': c = "c"; break;
    case 'D': c = "d"; break;
    case 'E': c = "e3"; break;
    case 'F': c = "f"; break;
    case 'G': c = "g9"; break;
    case 'H': c = "h"; break;
    case 'I': c = "i!1;"; break; //|lL
    case 'J': c = "j"; break;
    case 'K': c = "k"; break;
    case 'L': c = "l!71"; break; //|iI
    case 'M': c = "m"; break;
    case 'N': c = "n"; break;
    case 'O': c = "o0"; break;
    case 'P': c = "p"; break;
    case 'Q': c = "q"; break;
    case 'R': c = "r"; break; //2
    case 'S': c = "s5$"; break; //zZ
    case 'T': c = "t"; break;
    case 'U': c = "u"; break;
    case 'V': c = "v"; break;
    case 'W': c = "w"; break;
    case 'X': c = "x"; break;
    case 'Y': c = "y"; break;
    case 'Z': c = "z2"; break; //sS
    case '0': c = "oO"; break;
    case '1': c = "Ii!"; break; //|lL
    case '2': c = "zZ"; break; //R
    case '3': c = "eE"; break;
    case '4': c = "Aa"; break;
    case '5': c = "Ss"; break;
    //case '6': c = ""; break;
    case '7': c = "Ll"; break;
    //case '8': c = ""; break;
    case '9': c = "g"; break;
    default: stub[0] = key; c = stub; break;
  }
  return c[i];
}

void basic_smart_substitute(int i, int sub_i, char *s)
{
  s[i] = smart_sub(sub_i, s[i]);
}

tag stamp_tag(tag t, tag dst, int inc)
{
  if(inc) return dst |  t;
  else    return dst & ~t;
}

tag *clone_tag(tag src, tag *t)
{
  *t = src;
  return t;
}

void stamp_tag_map(tag t, tag_map *map, int inc)
{
  if(!t) return;
  if(inc)
  {
    tag carry = 0;
    for(int i = 0; t && i < max_tag_stack; i++)
    {
      carry = t & map->map[i];
      map->map[i] ^= t;
      t = carry;
    }
    if(t)
    {
      for(int i = 0; t && i < max_tag_stack; i++)
        map->map[i] |= t; //don't overflow; max out bits
    }
  }
  else
  {
    tag carry = 0;
    for(int i = 0; t && i < max_tag_stack; i++)
    {
      carry = t & ~map->map[i];
      map->map[i] ^= t;
      t = carry;
    }
    if(t)
    {
      for(int i = 0; t && i < max_tag_stack; i++)
        map->map[i] &= ~t; //don't underflow; zero out bits
    }
  }
}

tag collapse_tag_map(tag_map map)
{
  tag t = 0;
  for(int i = 0; i < max_tag_stack; i++)
    t |= map.map[i];
  return t;
}

void merge_tag_map(tag_map map, tag_map *dst) //slow if many conflicts
{
  if(map.map[0]) stamp_tag_map(map.map[0], dst, 1);
  int pow = 2;
  for(int i = 1; i < max_tag_stack; i++)
  {
    if(map.map[i]) for(int j = 0; j < pow; j++) stamp_tag_map(map.map[i], dst, 1);
    pow *= 2;
  }
}

int tag_conflict(tag t, tag dst)
{
  return t & dst;
}

int tag_map_conflict(tag t, tag_map map)
{
  for(int i = 0; i < max_tag_stack; i++)
    if(t & map.map[i]) return 1;
  return 0;
}

int tag_map_overconflict(tag t, tag_map map)
{
  for(int i = 1; i < max_tag_stack; i++)
    if(t & map.map[i]) return 1;
  return 0;
}

int tag_map_overconflicted(tag_map map)
{
  for(int i = 1; i < max_tag_stack; i++)
    if(map.map[i]) return 1;
  return 0;
}

tag_map *zero_tag_map(tag_map *map)
{
  memset(map,0,sizeof(tag_map));
  return map;
}

int gtag_required_option(group *g, tag tag_g, tag inv_tag_g, int *ri)
{
  if(!tag_g && !inv_tag_g) return -1;
  int forced_i = -1;
  for(int i = 0; i < g->n; i++)
  {
    if(tag_g & g->childs[i].tag_g)
    {
      if(forced_i != -1 || //overcommitted
        inv_tag_g & g->childs[i].tag_g) //simultaneously required and prohibited
      {
        *ri = -1;
        return 1;
      }
      forced_i = i;
    }
  }
  if(forced_i == -1) return 0;
  *ri = forced_i;
  return 1;
}

int tick_mod(modification *m)
{
  return 0;
}

int smodify_modification(modification *m, int inert, char *lockholder, char *buff, char **buff_p)
{
  char *buff_e = *buff_p;
  int flen = buff_e-buff;
  for(int i = 0; i < flen; i++) lockholder[i] = 0;

  int open_remaining;
  int prev_i;
  int fi;

  fi = 0;
  for(int i = 0; i < m->n_injections; i++)
  {
    fi = m->injection_i[i]+i;
    char *o = *buff_p;
    while(o > (buff+fi))
    {
      *o = *(o-1);
      o--;
    }
    *buff_p = *buff_p+1;
    *(buff+fi) = m->chars[m->injection_sub_i[i]];

    lockholder[flen++] = 0;
    *(lockholder+fi) = 1;
  }

  open_remaining = 0;
  prev_i = -1;
  fi = 0;
  for(int i = 0; i < m->n_smart_substitutions; i++)
  {
    open_remaining = m->smart_substitution_i[i]-(prev_i+1);
    prev_i = m->smart_substitution_i[i];
    while(lockholder[fi]) fi++;
    while(open_remaining)
    {
      fi++;
      open_remaining--;
      while(lockholder[fi]) fi++;
    }

    if(buff+fi >= *buff_p) break;
    m->smart_substitution_i_c[i] = buff[fi];
    basic_smart_substitute(fi, m->smart_substitution_sub_i[i], buff);
    *(lockholder+fi) = 1;
    fi++;
  }

  open_remaining = 0;
  prev_i = -1;
  fi = 0;
  for(int i = 0; i < m->n_substitutions; i++)
  {
    open_remaining = m->substitution_i[i]-(prev_i+1);
    prev_i = m->substitution_i[i];
    while(lockholder[fi]) fi++;
    while(open_remaining)
    {
      fi++;
      open_remaining--;
      while(lockholder[fi]) fi++;
    }

    if(buff+fi >= *buff_p) break;
    *(buff+fi) = m->chars[m->substitution_sub_i[i]];
    *(lockholder+fi) = 1;
    fi++;
  }

  open_remaining = 0;
  prev_i = 0;
  fi = 0;
  for(int i = 0; i < m->n_deletions; i++)
  {
    open_remaining = m->deletion_i[i]-prev_i;
    prev_i = m->deletion_i[i];
    while(lockholder[fi]) fi++;
    while(open_remaining)
    {
      fi++;
      open_remaining--;
      while(lockholder[fi]) fi++;
    }

    if(buff+fi >= *buff_p) break;
    char *o = (buff+fi);
    while(o < *buff_p)
    {
      *o = *(o+1);
      o++;
    }
    *buff_p = *buff_p-1;
    flen--;
    for(int j = fi; j < flen; j++) lockholder[j] = lockholder[j+1];
  }

  char *s = buff;
  char *o = *buff_p;
  for(int i = 0; i < m->n_copys; i++)
  {
    for(int j = 0; j < flen; j++)
    {
      *o = *(s+j);
      o++;
    }
  }
  *buff_p = o;

  if(!inert)
  {
    int i;
    int flen;

    int n_injections = m->n_injections;
    flen = buff_e-buff;
    buff_e += n_injections;
    i = n_injections;
    while(i)
    {
      i--;
      m->injection_sub_i[i]++;
      if(m->injection_sub_i[i] == m->n) m->injection_sub_i[i] = 0;
      else return 1;
      m->injection_i[i]++;
      if(m->injection_i[i] > flen)
      {
        if(i == 0) m->injection_i[i] = 0;
        else m->injection_i[i] = m->injection_i[i-1];
      }
      else inert = 1;
      for(int j = i+1; j < n_injections; j++)
        m->injection_i[j] = m->injection_i[j-1];
      if(inert) return inert;
    }

    int n_smart_substitutions = m->n_smart_substitutions;
    flen = (buff_e-buff)-n_injections;
    if(n_smart_substitutions > flen) n_smart_substitutions = flen;
    i = n_smart_substitutions;
    while(i)
    {
      i--;
      m->smart_substitution_sub_i[i]++;
      if(smart_sub(m->smart_substitution_sub_i[i], m->smart_substitution_i_c[i]) == '\0') m->smart_substitution_sub_i[i] = 0;
      else return 1;

      m->smart_substitution_i[i]++;
      if(m->smart_substitution_i[i] >= flen-(n_smart_substitutions-i-1))
      {
        if(i == 0) m->smart_substitution_i[i] = 0;
        else m->smart_substitution_i[i] = m->smart_substitution_i[i-1]+1;
      }
      else inert = 1;
      for(int j = i+1; j < n_smart_substitutions; j++)
        m->smart_substitution_i[j] = m->smart_substitution_i[j-1]+1;
      if(inert) return inert;
    }

    int n_substitutions = m->n_substitutions;
    flen = (buff_e-buff)-n_injections-n_smart_substitutions;
    if(n_substitutions > flen) n_substitutions = flen;
    i = n_substitutions;
    while(i)
    {
      i--;
      m->substitution_sub_i[i]++;
      if(m->substitution_sub_i[i] == m->n) m->substitution_sub_i[i] = 0;
      else return 1;

      m->substitution_i[i]++;
      if(m->substitution_i[i] >= flen-(n_substitutions-i-1))
      {
        if(i == 0) m->substitution_i[i] = 0;
        else m->substitution_i[i] = m->substitution_i[i-1]+1;
      }
      else inert = 1;
      for(int j = i+1; j < n_substitutions; j++)
        m->substitution_i[j] = m->substitution_i[j-1]+1;
      if(inert) return inert;
    }

    int n_deletions = m->n_deletions;
    flen = (buff_e-buff)-n_injections-n_smart_substitutions-n_substitutions;
    if(n_deletions > flen) n_deletions = flen;
    i = n_deletions;
    buff_e -= i;
    while(i)
    {
      i--;
      m->deletion_i[i]++;
      if(m->deletion_i[i] >= flen-(n_deletions-1))
      {
        if(i == 0) m->deletion_i[i] = 0;
        else m->deletion_i[i] = m->deletion_i[i-1];
      }
      else inert = 1;
      for(int j = i+1; j < n_deletions; j++)
        m->deletion_i[j] = m->deletion_i[j-1];
      if(inert) return inert;
    }
  }
  return inert;
}

int smodify_group(group *g, int inert, char *lockholder, char *buff, char **buff_p)
{
  if(!g->n_mods) return inert;
  inert = smodify_modification(&g->mods[g->mod_i], inert, lockholder, buff, buff_p);
  if(!inert)
  {
    g->mod_i++;
    if(g->mod_i == g->n_mods) g->mod_i = 0;
    else inert = 1;
  }
  return inert;
}

//prints current state, and manages/advances to _next_ state
int sprint_group(group *g, int inert, int *utag_dirty, int *gtag_dirty, char *lockholder, char **buff_p, char *devnull) //"inert = 1" is the cue to stop incrementing as the rest of the password is written
{
  char *tdevnull;
  char *buff = *buff_p;
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
    {
      if(!inert && g->n_mods) //this is incredibly inefficient
      {
        for(int i = 0; i < g->n; i++)
          sprint_group(&g->childs[i], 1, utag_dirty, gtag_dirty, lockholder, buff_p, devnull); //dry run
        inert = smodify_group(g, inert, lockholder, buff, buff_p); //to let modifications get a chance to increment
        if(!inert) //still hot?
        {
          for(int i = 0; i < g->n; i++)
          {
            tdevnull = devnull;
            inert = sprint_group(&g->childs[i], inert, utag_dirty, gtag_dirty, lockholder, &tdevnull, devnull); //redo dry run, updating state (but printing to garbage)
          }
        }
      }
      else //inert OR no modifications means no dry run necessary
      {
        for(int i = 0; i < g->n; i++)
          inert = sprint_group(&g->childs[i], inert, utag_dirty, gtag_dirty, lockholder, buff_p, devnull);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
      }
    }
      break;
    case GROUP_TYPE_OPTION:
    {
      if(!inert && g->n_mods) //this is incredibly inefficient
      {
        sprint_group(&g->childs[g->i], 1, utag_dirty, gtag_dirty, lockholder, buff_p, devnull);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
        if(!inert)
        {
          tdevnull = devnull;
          inert = sprint_group(&g->childs[g->i], inert, utag_dirty, gtag_dirty, lockholder, &tdevnull, devnull);
          if(!inert)
          {
            //if(g->childs[g->i].sum_tag_u) *utag_dirty = 1; //"leaving a utag" should never cause an issue
            if(g->childs[g->i].sum_tag_g) *gtag_dirty = 1; //more conservative than "necessary" ("should" check against _current state's_ aggregate tag_u; not sum)
            g->i++;
            if(g->i == g->n) g->i = 0;
            else
            {
              inert = 1;
              if(g->childs[g->i].zerod_sum_tag_u) *utag_dirty = 1;
              if(g->childs[g->i].sum_tag_g) *gtag_dirty = 1; //more conservative than "necessary" ("should" check if advancing _changed_ the _current state_)
            }
          }
        }
      }
      else
      {
        inert = sprint_group(&g->childs[g->i], inert, utag_dirty, gtag_dirty, lockholder, buff_p, devnull);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
        if(!inert)
        {
          //if(g->childs[g->i].sum_tag_u) *utag_dirty = 1; //"leaving a utag" should never cause an issue
          if(g->childs[g->i].sum_tag_g) *gtag_dirty = 1;  //more conservative than "necessary" ("should" check against _current state's_ aggregate tag_u; not sum)
          g->i++;
          if(g->i == g->n) g->i = 0;
          else
          {
            inert = 1;
            if(g->childs[g->i].zerod_sum_tag_u) *utag_dirty = 1;
            if(g->childs[g->i].sum_tag_g) *gtag_dirty = 1; //more conservative than "necessary" ("should" check if advancing _changed_ the _current state_)
          }
        }
      }
    }
      break;
    case GROUP_TYPE_PERMUTE:
    {
      if(!inert && g->n_mods) //this is incredibly inefficient
      {
        for(int i = 0; i < g->n; i++)
          sprint_group(&g->childs[cache_permute_indices[g->n][g->i+1][i]], 1, utag_dirty, gtag_dirty, lockholder, buff_p, devnull);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
        if(!inert)
        {
          for(int i = 0; i < g->n; i++)
          {
            tdevnull = devnull;
            inert = sprint_group(&g->childs[cache_permute_indices[g->n][g->i+1][i]], inert, utag_dirty, gtag_dirty, lockholder, &tdevnull, devnull);
          }
          if(!inert)
          {
            g->i++;
            if(g->i == cache_permute_indices[g->n][0]-(int *)0) g->i = 0;
            else inert = 1;
          }
        }
      }
      else
      {
        for(int i = 0; i < g->n; i++)
          inert = sprint_group(&g->childs[cache_permute_indices[g->n][g->i+1][i]], inert, utag_dirty, gtag_dirty, lockholder, buff_p, devnull);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
        if(!inert)
        {
          g->i++;
          if(g->i == cache_permute_indices[g->n][0]-(int *)0) g->i = 0;
          else inert = 1;
        }
      }
    }
      break;
    case GROUP_TYPE_CHARS:
      safe_strcpy(buff,g->chars);
      *buff_p = buff+g->n;
      inert = smodify_group(g, inert, lockholder, buff, buff_p);
      break;
    default: //appease compiler
      break;
  }
  return inert;
}

int tags_coherent_with_selected_option(group *g, tag tag_u, tag tag_g, tag inv_tag_g)
{
  if(tag_conflict(g->childs[g->i].tag_u,tag_u)) return 0;
  if(tag_conflict(g->childs[g->i].tag_g,inv_tag_g)) return 0;
  for(int i = 0; i < g->n; i++)
    if(i != g->i) if(tag_conflict(g->childs[i].tag_g,tag_g)) return 0;
  return 1;
}

int tags_coherent_with_children(group *g, tag *tag_u, tag *tag_g, tag *inv_tag_g)
{
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
    case GROUP_TYPE_PERMUTE:
      for(int i = 0; i < g->n; i++)
        if(!tags_coherent_with_children(&g->childs[i], tag_u, tag_g, inv_tag_g)) return 0;
      break;
    case GROUP_TYPE_OPTION:
    {
      if(tag_conflict(g->childs[g->i].tag_u,*tag_u)) return 0;
      *tag_u = stamp_tag(g->childs[g->i].tag_u,*tag_u,1);
      *tag_g = stamp_tag(g->childs[g->i].tag_g,*tag_g,1);
      for(int i = 0; i < g->n; i++)
        if(i != g->i) *inv_tag_g = stamp_tag(g->childs[i].tag_g,*inv_tag_g,1);
      if(tag_conflict(*tag_g,*inv_tag_g)) return 0;
      if(!tags_coherent_with_children(&g->childs[g->i], tag_u, tag_g, inv_tag_g)) return 0;
    }
      break;
    case GROUP_TYPE_CHARS:
    default: //appease compiler
      break;
  }
  return 1;
}

void revoke_child_utags(group *g, tag *tag_u)
{
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
    case GROUP_TYPE_PERMUTE:
      for(int i = 0; i < g->n; i++)
        revoke_child_utags(&g->childs[i],tag_u);
      break;
    case GROUP_TYPE_OPTION:
      *tag_u = stamp_tag(g->childs[g->i].tag_u,*tag_u,0);
      revoke_child_utags(&g->childs[g->i],tag_u);
      break;
    case GROUP_TYPE_CHARS:
    default: //appease compiler
      break;
  }
}

void aggregate_child_gtags(group *g, tag *tag_g, tag *inv_tag_g)
{
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
    case GROUP_TYPE_PERMUTE:
      for(int i = 0; i < g->n; i++)
        aggregate_child_gtags(&g->childs[i],tag_g,inv_tag_g);
      break;
    case GROUP_TYPE_OPTION:
      *tag_g = stamp_tag(g->childs[g->i].tag_g,*tag_g,1);
      for(int i = 0; i < g->n; i++)
        if(i != g->i) *inv_tag_g = stamp_tag(g->childs[i].tag_g,*inv_tag_g,1);
      aggregate_child_gtags(&g->childs[g->i],tag_g,inv_tag_g);
      break;
    case GROUP_TYPE_CHARS:
    default: //appease compiler
      break;
  }
}

void merge_group_children_tag_maps(group *g, tag_map *map_u, tag_map *map_g, tag_map *inv_map_g)
{
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
    case GROUP_TYPE_PERMUTE:
      for(int i = 0; i < g->n; i++)
        merge_group_children_tag_maps(&g->childs[i], map_u, map_g, inv_map_g);
      break;
    case GROUP_TYPE_OPTION:
    {
      //only need to aggregate here, as the only things with tags should be children of options
      stamp_tag_map(g->childs[g->i].tag_u,map_u,1);
      stamp_tag_map(g->childs[g->i].tag_g,map_g,1);
      for(int i = 0; i < g->n; i++)
        if(i != g->i) stamp_tag_map(g->childs[i].tag_g,inv_map_g,1);
      merge_group_children_tag_maps(&g->childs[g->i], map_u, map_g, inv_map_g);
    }
      break;
    case GROUP_TYPE_CHARS:
    default: //appease compiler
      break;
  }
}

int advance_group(group *g, tag current_tag_u, tag current_tag_g, tag current_inv_tag_g)
{
  int carry = 0;
  tag og_tag_g = current_tag_g;
  tag og_inv_tag_g = current_inv_tag_g;
  tag proposed_tag_u;
  tag proposed_tag_g;
  tag proposed_inv_tag_g;
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
    {
      int i = g->n-1;
      int advanced = 0;
      while(i >= 0)
      {
        int newly_advanced = 0;
        if(carry || (i == 0 && !advanced))
        {
          advanced = 1;
          newly_advanced = 1;
          carry = advance_group(&g->childs[i], current_tag_u, current_tag_g, current_inv_tag_g);
        }
        while(!carry && !tags_coherent_with_children(&g->childs[i],clone_tag(current_tag_u,&proposed_tag_u),clone_tag(current_tag_g,&proposed_tag_g),clone_tag(current_inv_tag_g,&proposed_inv_tag_g))) //to get out of this loop, we're either carrying or succeeding in populating proposed_*
        {
          advanced = 1;
          newly_advanced = 1;
          carry = advance_group(&g->childs[i], current_tag_u, current_tag_g, current_inv_tag_g); //assume success || carry
        }
        if(newly_advanced)
        {
          for(int j = i-1; j >= 0; j--)
            zero_progress_group(&g->childs[j]);
        }
        if(carry)
        {
          i++;
          if(i == g->n) //tried to pop too far: impossible group
          {
            zero_progress_group(g);
            return 1; //"carry"
          }
          else
          {
            //going to advance this next child on the carry; remove its influence
            revoke_child_utags(&g->childs[i],&current_tag_u); //because utags don't stack, it's easy to undo

            //rather than managing stack of gtags, just flush and re-aggregate
            clone_tag(og_tag_g,&current_tag_g);
            clone_tag(og_inv_tag_g,&current_inv_tag_g);
            for(int j = g->n-1; j > i; j--)
              aggregate_child_gtags(&g->childs[j],&current_tag_g,&current_inv_tag_g);
          }
        }
        else
        {
          current_tag_u = proposed_tag_u;
          current_tag_g = proposed_tag_g;
          current_inv_tag_g = proposed_inv_tag_g;
          i--;
        }
      }
    }
      break;
    case GROUP_TYPE_OPTION:
    {
      int advanced = 0;
      carry = 0;
      while(!advanced || carry)
      {
        while(carry || !tags_coherent_with_selected_option(g, current_tag_u, current_tag_g, current_inv_tag_g))
        {
          int i = 0;
          int forced = 0;
          if(current_tag_g) forced = gtag_required_option(g,current_tag_g,current_inv_tag_g,&i);
          if(!forced) i = g->i+1;
          if(i == -1 || i <= g->i || i == g->n) //over committed || can't advance (already correctly locked) || can't advance further
          {
            zero_progress_group(g);
            return 1;
          }

          g->i = i;
          advanced = 1;
          carry = 0;
        }

        if(!advanced) carry = advance_group(&g->childs[g->i], current_tag_u, current_tag_g, current_inv_tag_g);
        advanced = 1;
        while(!carry && !tags_coherent_with_children(&g->childs[g->i],clone_tag(current_tag_u,&proposed_tag_u),clone_tag(current_tag_g,&proposed_tag_g),clone_tag(current_inv_tag_g,&proposed_inv_tag_g))) //to get out of this loop, we're either carrying or succeeding in populating proposed_*
        {
          carry = advance_group(&g->childs[g->i], current_tag_u, current_tag_g, current_inv_tag_g);
        }
        if(carry)
        {
          current_tag_u = stamp_tag(g->childs[g->i].tag_u,current_tag_u,0);
          current_tag_g = og_tag_g;
          current_inv_tag_g = og_inv_tag_g;
        }
      }
    }
      break;
    case GROUP_TYPE_PERMUTE:
    {
      int i = g->n-1;
      int advanced = 0;
      while(i >= 0)
      {
        int pi = cache_permute_indices[g->n][g->i+1][i];
        int newly_advanced = 0;
        if(carry || (i == 0 && !advanced))
        {
          advanced = 1;
          newly_advanced = 1;
          carry = advance_group(&g->childs[pi], current_tag_u, current_tag_g, current_inv_tag_g);
        }
        while(!carry && !tags_coherent_with_children(&g->childs[i],clone_tag(current_tag_u,&proposed_tag_u),clone_tag(current_tag_g,&proposed_tag_g),clone_tag(current_inv_tag_g,&proposed_inv_tag_g))) //to get out of this loop, we're either carrying or succeeding in populating proposed_*
        {
          advanced = 1;
          newly_advanced = 1;
          carry = advance_group(&g->childs[pi], current_tag_u, current_tag_g, current_inv_tag_g);
        }
        if(newly_advanced)
        {
          for(int j = i-1; j >= 0; j--)
          {
            int pj = cache_permute_indices[g->n][g->i+1][j];
            zero_progress_group(&g->childs[pj]);
          }
        }
        if(carry)
        {
          i++;
          if(i == g->n) //tried to pop too far: impossible group
          {
            zero_progress_group(g);
            return 1;
          }
          else
          {
            pi = cache_permute_indices[g->n][g->i+1][i];
            //going to advance this next child on the carry; remove its influence
            revoke_child_utags(&g->childs[i],&current_tag_u); //because utags don't stack, it's easy to undo

            //rather than managing stack of gtags, just flush and re-aggregate
            clone_tag(og_tag_g,&current_tag_g);
            clone_tag(og_inv_tag_g,&current_inv_tag_g);
            for(int j = g->n-1; j > i; j--)
              aggregate_child_gtags(&g->childs[j],&current_tag_g,&current_inv_tag_g);
          }
        }
        else
        {
          current_tag_u = proposed_tag_u;
          current_tag_g = proposed_tag_g;
          current_inv_tag_g = proposed_inv_tag_g;
          i--;
        }
      }
    }
      break;
    case GROUP_TYPE_CHARS:
      carry = 1;
      break;
    default: //appease compiler
      break;
  }
  zero_progress_modifications(g);
  return carry;
}

void zero_progress_group(group *g)
{
  if(g->type == GROUP_TYPE_OPTION)
    zero_progress_group(&g->childs[g->i]);
  else if(g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_PERMUTE)
  {
    for(int i = 0; i < g->n; i++)
      zero_progress_group(&g->childs[i]);
  }
  g->i = 0;
}

void zero_progress_modifications(group *g)
{
  modification *m;
  for(int i = 0; i < g->n_mods; i++)
  {
    m = &g->mods[i];

    if(m->n_injections)          { for(int j = 0; j < m->n_injections;          j++) m->injection_i[j]          = 0; }
    if(m->n_smart_substitutions) { for(int j = 0; j < m->n_smart_substitutions; j++) m->smart_substitution_i[j] = j; }
    if(m->n_substitutions)       { for(int j = 0; j < m->n_substitutions;       j++) m->substitution_i[j]       = j; }
    if(m->n_deletions)           { for(int j = 0; j < m->n_deletions;           j++) m->deletion_i[j]           = 0; }

    if(m->n_injections)          { for(int j = 0; j < m->n_injections;          j++) m->injection_sub_i[j]          = 0; }
    if(m->n_smart_substitutions) { for(int j = 0; j < m->n_smart_substitutions; j++) m->smart_substitution_sub_i[j] = 0; }
    if(m->n_substitutions)       { for(int j = 0; j < m->n_substitutions;       j++) m->substitution_sub_i[j]       = 0; }
  }
}

