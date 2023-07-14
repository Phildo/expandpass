#ifndef EXPANSION_H
#define EXPANSION_H

#include "util.h"

const int version_maj = 0;
const int version_min = 27;

const int max_pass_len = 300;
const int max_tag_count = 16;
const int max_tag_stack = 4;

//tag = bitmask representing which tags present
//tag_map = vertical binary count of # of each tag present (eg: "how many [3] tags stamped?" = bitwise concatenation of 3rd bit of each level in the stack) //LOL
//justification for this nonsense: tag is most common representation; bitmask is simple and efficient. "edge case" is aggregation (slow)

typedef unsigned int tag;
typedef struct
{
  tag map[max_tag_stack];
} tag_map;

enum GROUP_TYPE
{
  GROUP_TYPE_NULL,
  GROUP_TYPE_SEQUENCE,
  GROUP_TYPE_OPTION,
  GROUP_TYPE_PERMUTE,
  GROUP_TYPE_CHARS,
  GROUP_TYPE_MODIFICATION, //special case, shouldn't exist in formed group
  GROUP_TYPE_COUNT,
};

struct modification
{
  char *chars;
  int n;
  int n_injections;
  int n_smart_substitutions;
  int n_substitutions;
  int n_deletions;
  int n_copys;
  int *injection_i;
  int *injection_sub_i;
  int *substitution_i;
  int *substitution_sub_i;
  int *smart_substitution_i;
  char *smart_substitution_i_c;
  int *smart_substitution_sub_i;
  int *deletion_i;
};
inline void zero_modification(modification *m)
{
  m->chars                    = 0;
  m->n                        = 0;
  m->n_injections             = 0;
  m->n_smart_substitutions    = 0;
  m->n_substitutions          = 0;
  m->n_deletions              = 0;
  m->n_copys                  = 0;
  m->injection_i              = 0;
  m->injection_sub_i          = 0;
  m->smart_substitution_i     = 0;
  m->smart_substitution_i_c   = 0;
  m->smart_substitution_sub_i = 0;
  m->substitution_i           = 0;
  m->substitution_sub_i       = 0;
  m->deletion_i               = 0;
}

struct group
{
  GROUP_TYPE type;
  group *childs;
  char *chars;
  int n;
  int i;
  modification *mods;
  int n_mods;
  int mod_i;
  tag tag_u;
  tag tag_g;
  tag child_tag_u;
  tag child_tag_g;
  tag zerod_sum_tag_u;
  tag zerod_sum_tag_g;
  tag sum_tag_u;
  tag sum_tag_g;
  int countable; //TODO: fix countability
  int estimate;
};
inline void zero_group(group *g)
{
  g->type            = GROUP_TYPE_NULL;
  g->childs          = 0;
  g->chars           = 0;
  g->n               = 0;
  g->i               = 0;
  g->mods            = 0;
  g->n_mods          = 0;
  g->mod_i           = 0;
  g->tag_u           = 0;
  g->tag_g           = 0;
  g->child_tag_u     = 0;
  g->child_tag_g     = 0;
  g->zerod_sum_tag_u = 0;
  g->zerod_sum_tag_g = 0;
  g->sum_tag_u       = 0;
  g->sum_tag_g       = 0;
  g->countable       = 0;
  g->estimate        = 0;
}

typedef long long int expand_iter;

void collapse_group(group *g, group *root, int handle_gamuts);
void prepare_group_iteration(group *g);
void unroll_group(group *g, int threshhold, char *devnull);
void propagate_countability(group *g);
float approximate_length_premodified_group(group *g);
float approximate_length_modified_group(group *g);
expand_iter estimate_group(group *g);
expand_iter state_from_countable_group(group *g);
void resume_countable_group(group *g, expand_iter state);
void free_group_contents(group *g);
void print_seed(group *g, int print_progress, int selected, int indent);

#endif //EXPANSION_H
