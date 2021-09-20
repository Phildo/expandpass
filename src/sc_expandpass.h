#include "sc_expandpass.h"

#include "util.h"
#include "expansion.h"
#include "parse.h"
#include "expand.h"

group *g;
char *devnull = 0;

// should this take in a filename? or a file pointer? or a buffer?
void sc_expandpass_init(sc_expandpass_ini ini)
{
  FILE *fp = safe_fopen(seed_file, "r");
  if(!fp) { fprintf(stderr,"Error opening seed file: %s\n",seed_file); exit(1); }
  g = parse(fp, unquoted);
  fclose(fp);

  if(!devnull) devnull = (char *)safe_malloc(sizeof(char)*1024*1024);
}

int sc_expandpass_keyspace()
{
  return estimate_group(g);
}

void sc_expandpass_seek(int state)
{
  resume_linear_group(g,state);
}

//do I print to stdout? a filepointer? a buffer?
void sc_expandpass_next()
{
  char passholder[max_pass_len];
  char *passholder_p = passholder;
  char lockholder[max_pass_len];
  memset(lockholder,0,sizeof(char)*max_pass_len);

  int utag_dirty = 0; //guaranteed (tags not allowed w/ hashcat)
  int gtag_dirty = 0; //guaranteed (tags not allowed w/ hashcat)
  sprint_group(g, 0, &utag_dirty, &gtag_dirty, lockholder, &passholder_p, devnull);
}

void sc_expandpass_shutdown()
{
  clean_group_contents(g);
  free(g);
}

