#ifndef EXPAND_H
#define EXPAND_H

#include "expansion.h"

char smart_sub(int i, char key);
void basic_smart_substitute(int i, int sub_i, char *s);
unsigned long long int estimate_group(group *g);
tag stamp_tag(tag t, tag dst, int inc);
tag *clone_tag(tag src, tag *t);
void stamp_tag_map(tag t, tag_map *map, int inc);
tag collapse_tag_map(tag_map map);
void merge_tag_map(tag_map map, tag_map *dst); //slow if many conflicts
int tag_conflict(tag t, tag dst);
int tag_map_conflict(tag t, tag_map map);
int tag_maps_conflict(tag_map m1, tag_map m2);
int tag_map_overconflict(tag t, tag_map map);
int tag_map_overconflicted(tag_map map);
tag_map *zero_tag_map(tag_map *map);
void absorb_tags(group *dst, group *src);
void elevate_tags(group *g, group *parent, group *parent_option_child);
int gtag_required_option(group *g, tag tag_g, tag inv_tag_g, int *ri);
int sprint_group(group *g, int inert, int *utag_dirty, int *gtag_dirty, char *lockholder, char **buff_p, char *devnull);
int tags_coherent_with_selected_option(group *g, tag tag_u, tag tag_g, tag inv_tag_g);
int tags_coherent_with_children(group *g, tag *tag_u, tag *tag_g, tag *inv_tag_g);
void revoke_child_utags(group *g, tag *tag_u);
void aggregate_child_gtags(group *g, tag *tag_g, tag *inv_tag_g);
void merge_group_children_tag_maps(group *g, tag_map *map_u, tag_map *map_g, tag_map *inv_map_g);
int advance_group(group *g, tag current_tag_u, tag current_tag_g, tag current_inv_tag_g);
void zero_progress_group(group *g);
void zero_progress_modifications(group *g);

#endif //EXPAND_H

