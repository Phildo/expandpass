#include "stdio.h"
#include "stdlib.h"
#include "string.h"

enum GROUP_TYPE
{
  GROUP_TYPE_NULL,
  GROUP_TYPE_SEQUENCE,
  GROUP_TYPE_OPTION,
  GROUP_TYPE_PERMUTE,
  GROUP_TYPE_CHARS,
  GROUP_TYPE_MODIFICATION, //special case
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
  int *injection_i;
  int *injection_sub_i;
  int *substitution_i;
  int *substitution_sub_i;
  int *smart_substitution_i;
  char *smart_substitution_i_c;
  int *smart_substitution_sub_i;
  int *deletion_i;
  int nothing_i;
};
void zero_modification(modification *m)
{
  m->chars = 0;
  m->n = 0;
  m->n_injections = 0;
  m->n_smart_substitutions = 0;
  m->n_substitutions = 0;
  m->n_deletions = 0;
  m->injection_i = 0;
  m->injection_sub_i = 0;
  m->smart_substitution_i = 0;
  m->smart_substitution_i_c = 0;
  m->smart_substitution_sub_i = 0;
  m->substitution_i = 0;
  m->substitution_sub_i = 0;
  m->deletion_i = 0;
  m->nothing_i = 0;
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
};
void zero_group(group *g)
{
  g->type = GROUP_TYPE_NULL;
  g->childs = 0;
  g->chars = 0;
  g->n = 0;
  g->i = 0;
  g->mods = 0;
  g->n_mods = 0;
  g->mod_i = 0;
}

struct parse_error
{
  int error;
  char *txt;
};

int buff_len = 1024*1024; //1MB
int max_pass_len = 300;
int max_read_line_len = 1024;
char *devnull;

void append_password(char *password);
char smart_sub(int i, char key);
void basic_smart_substitute(int i, int sub_i, char *s);
int parse_modification(FILE *fp, int *line_n, char *buff, char **b, modification *m, parse_error *e);
int parse_modifications(FILE *fp, int *line_n, char *buff, char **b, group *g, parse_error *e);
int parse_child(FILE *fp, int *line_n, char *buff, char **b, group *g, group *prev_g, parse_error *e);
int parse_childs(FILE *fp, int *line_n, char *buff, char **b, group *g, parse_error *e);
group *parse();
unsigned long long int estimate_group(group *g);
int approximate_length_group(group *g);
int sprint_group(group *g, int inert, char *lockholder, char **buff_p);
void checkpoint_group(group *g, FILE *fp);
void resume_group(group *g, FILE *fp);
void checkpoint_to_file(group *g);
void resume_from_file(group *g);
int parse_number(char *b, int *n);

int ***cache_permute_indices;
int n_cached_permute_indices;

FILE *fp;
char *buff;
int buff_i;

char *seed_file = "seed.txt";
char *password_file = 0;
char *progress_file = "seed.progress";
char *checkpoint_file = "seed.progress";
char checkpoint_file_bak[512];
int estimate = 0;
int resume = 0;
int validate = 0;
int checkpoint = 0;

int main(int argc, char **argv)
{
  int i = 1;
  while(i < argc)
  {
    if(strcmp(argv[i],"--help") == 0)
    {
      fprintf(stdout,"usage: expandpass [--help] [--estimate] [-i input_seed.txt] [-o output_passwords.txt] [-v #] [-c # checkpoint_seed.progress] [-r recovery_seed.progress]\n");
      fprintf(stdout,"--help shows this menu\n");
      fprintf(stdout,"--estimate shows a (crude) estimation of # of likely generatable passwords\n");
      fprintf(stdout,"-i specifies seed file (default \"seed.txt\" if blank)\n");
      fprintf(stdout,"   (see readme for seed syntax)\n");
      fprintf(stdout,"-o specifies output file (default stdout if blank)\n");
      fprintf(stdout,"-v specifies verification before output; # is minimum password length\n");
      fprintf(stdout,"   note: any verification > 0 will also verify that there is at least\n");
      fprintf(stdout,"         1+ char A-Z, 1+ a-z, 1+ 0-9, and 1+ other. (cmd args coming soon)\n");
      fprintf(stdout,"   (default is no verification)\n");
      fprintf(stdout,"-c specifies how often to checkpoint via progress file (default \"seed.progress\" if blank)\n");
      fprintf(stdout,"-r specifies to resume from specified (or not) progress file (default \"seed.progress\" if blank)\n");
      exit(0);
    }
    else if(strcmp(argv[i],"--estimate") == 0)
    {
      estimate = 1;
    }
    else if(strcmp(argv[i],"-o") == 0)
    {
      i++;
      password_file = argv[i];
    }
    else if(strcmp(argv[i],"-i") == 0)
    {
      i++;
      seed_file = argv[i];
    }
    else if(strcmp(argv[i],"-v") == 0)
    {
      i++;
      parse_number(argv[i], &validate);
    }
    else if(strcmp(argv[i],"-c") == 0)
    {
      i++;
      parse_number(argv[i], &checkpoint);
      if(i+1 < argc) { i++; checkpoint_file = argv[i]; }
    }
    else if(strcmp(argv[i],"-r") == 0)
    {
      resume = 1;
      if(i+1 < argc) { i++; progress_file = argv[i]; }
    }
    else
    {
      fprintf(stderr,"error: unrecognized argument \"%s\"\n",argv[i]);
    }
    i++;
  }
  sprintf(checkpoint_file_bak,"%s.bak",checkpoint_file);

  n_cached_permute_indices = 0;
  group *g = parse();

  if(estimate)
  {
    unsigned long long int e = estimate_group(g);
    fprintf(stdout,"estimated output for seed file (%s): %llu\n",seed_file,e);
    e /= 600000;
    fprintf(stdout,"%llus @ 600k/s\n",e);
    e /= 60; //minute
    e /= 60; //hour
    e /= 24; //day
    fprintf(stdout,"%llu days @ 600k/s\n",e);
    exit(0);
  }

  buff = (char *)malloc(sizeof(char)*buff_len);
  buff_i = 0;

  devnull = (char *)malloc(sizeof(char)*buff_len);

  char *passholder = (char *)malloc(sizeof(char)*max_pass_len);
  char *passholder_p = passholder;
  char *lockholder = (char *)malloc(sizeof(char)*max_pass_len);
  for(int i = 0; i < max_pass_len; i++) lockholder[i] = 0;

  if(resume)
    resume_from_file(g);

  if(password_file)
  {
    if(resume) fp = fopen(password_file, "a");
    else       fp = fopen(password_file, "w");
    if(!fp) { fprintf(stderr,"Error opening output file: %s\n",password_file); exit(1); }
  }
  else fp = stdout;

  if(validate)
  {
    int done = 0;
    long long int e = 0;
    while(!done)
    {
      done = !sprint_group(g, 0, lockholder, &passholder_p);
      e++;
      *passholder_p = '\0';
      if(passholder_p-passholder >= validate)
      {
        int has_upp = 0;
        int has_low = 0;
        int has_num = 0;
        int has_sym = 0;
        passholder_p = passholder;
        while(has_upp+has_low+has_num+has_sym < 4 && *passholder_p != '\0')
        {
               if(*passholder_p >= 'A' && *passholder_p <= 'Z') has_upp = 1;
          else if(*passholder_p >= 'a' && *passholder_p <= 'z') has_low = 1;
          else if(*passholder_p >= '0' && *passholder_p <= '9') has_num = 1;
          else                                                  has_sym = 1;
          passholder_p++;
        }
        if(has_upp+has_low+has_num+has_sym == 4) append_password(passholder);
      }
      passholder_p = passholder;
      if(checkpoint && e >= checkpoint)
      {
        checkpoint_to_file(g);
        e = 0;
      }
    }
  }
  else
  {
    int done = 0;
    long long int e = 0;
    while(!done)
    {
      done = !sprint_group(g, 0, lockholder, &passholder_p);
      e++;
      *passholder_p = '\0';
      append_password(passholder);
      passholder_p = passholder;
      if(checkpoint && e >= checkpoint)
      {
        checkpoint_to_file(g);
        e = 0;
      }
    }
  }

  fwrite(buff,sizeof(char),buff_i,fp);
  fclose(fp);
}

void append_password(char *password)
{
  int pass_i = 0;
  while(password[pass_i] != '\0')
  {
    buff[buff_i] = password[pass_i];
    buff_i++;
    pass_i++;
    if(buff_i >= buff_len)
    {
      fwrite(buff,sizeof(char),buff_len,fp);
      buff_i = 0;
    }
  }
  buff[buff_i] = '\n';
  buff_i++;
  if(buff_i >= buff_len)
  {
    fwrite(buff,sizeof(char),buff_len,fp);
    buff_i = 0;
  }
}

char smart_sub(int i, char key)
{
  static char stub[2] = "a";
  char *c;
  switch(key)
  {
    case 'a': c = "A4"; break;
    case 'b': c = "B"; break;
    case 'c': c = "C"; break;
    case 'd': c = "D"; break;
    case 'e': c = "E3"; break;
    case 'f': c = "F"; break;
    case 'g': c = "G9"; break;
    case 'h': c = "H"; break;
    case 'i': c = "I!1"; break;
    case 'j': c = "J"; break;
    case 'k': c = "K"; break;
    case 'l': c = "L!71"; break;
    case 'm': c = "M"; break;
    case 'n': c = "N"; break;
    case 'o': c = "O0"; break;
    case 'p': c = "P"; break;
    case 'q': c = "Q"; break;
    case 'r': c = "R"; break;
    case 's': c = "S5"; break;
    case 't': c = "T"; break;
    case 'u': c = "U"; break;
    case 'v': c = "V"; break;
    case 'w': c = "W"; break;
    case 'x': c = "X"; break;
    case 'y': c = "Y"; break;
    case 'z': c = "Z2"; break;
    case 'A': c = "a4"; break;
    case 'B': c = "b"; break;
    case 'C': c = "c"; break;
    case 'D': c = "d"; break;
    case 'E': c = "e3"; break;
    case 'F': c = "f"; break;
    case 'G': c = "g9"; break;
    case 'H': c = "h"; break;
    case 'I': c = "i!1"; break;
    case 'J': c = "j"; break;
    case 'K': c = "k"; break;
    case 'L': c = "l!71"; break;
    case 'M': c = "m"; break;
    case 'N': c = "n"; break;
    case 'O': c = "o0"; break;
    case 'P': c = "p"; break;
    case 'Q': c = "q"; break;
    case 'R': c = "r"; break;
    case 'S': c = "s5"; break;
    case 'T': c = "t"; break;
    case 'U': c = "u"; break;
    case 'V': c = "v"; break;
    case 'W': c = "w"; break;
    case 'X': c = "x"; break;
    case 'Y': c = "y"; break;
    case 'Z': c = "z2"; break;
    case '0': c = "oO"; break;
    case '1': c = "Ii!"; break;
    case '2': c = "zZ"; break;
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

int parse_child(FILE *fp, int *line_n, char *buff, char **b, group *g, group *prev_g, parse_error *e)
{
  int n_chars = 0;
  size_t line_buff_len = max_read_line_len;
  char *s = *b;
  while(g->type == GROUP_TYPE_NULL)
  {
    if(*s == '\0')
    {
      n_chars = getline(&buff, &line_buff_len, fp);
      *line_n = *line_n+1;
      if(n_chars <= 0)
      {
        *buff = '\0';
        e->error = 7;
        sprintf(e->txt,"ERROR: EOF\nline %d\n",*line_n);
        return 0;
      }
      s = buff;
    }
    while(*s == ' ') s++;
         if(*s == '<' ) { g->type = GROUP_TYPE_SEQUENCE;     s++; }
    else if(*s == '{' ) { g->type = GROUP_TYPE_OPTION;       s++; }
    else if(*s == '(' ) { g->type = GROUP_TYPE_PERMUTE;      s++; }
    else if(*s == '"' ) { g->type = GROUP_TYPE_CHARS;        s++; }
    else if(*s == '[' ) { g->type = GROUP_TYPE_MODIFICATION; s++; }
    else if(*s == '#' ) { g->type = GROUP_TYPE_NULL;         s++; *s = '\0'; }
    else if(*s == '\n') { g->type = GROUP_TYPE_NULL;         s++; *s = '\0'; }
    else if(*s == '\0') { g->type = GROUP_TYPE_NULL;         s++; *s = '\0'; }
    else
    {
      *b = s;
      e->error = 1;
      if(buff[n_chars-1] == '\n') buff[n_chars-1] = '\0'; else buff[n_chars] = '\0';
      sprintf(e->txt,"ERROR: Invalid line\nline %d ( %s )\n",*line_n,buff);
      return 0;
    }

    char *c;
    switch(g->type)
    {
      case GROUP_TYPE_SEQUENCE:
      case GROUP_TYPE_OPTION:
      case GROUP_TYPE_PERMUTE:
        g->n = 0;
        *b = s;
        return parse_childs(fp, line_n, buff, b, g, e);
        break;
      case GROUP_TYPE_CHARS:
        c = s;
        while(*c != '"' && *c != '\n' && *c != '\0') c++;
        if(*c == '"')
        {
          g->n = (c-s);
          g->chars = (char *)malloc(sizeof(char)*g->n+1);
          c = g->chars;
          while(*s != '"')
          {
            *c = *s;
            c++;
            s++;
          }
          s++;
          *c = '\0';
        }
        else
        {
          e->error = 4;
          if(buff[n_chars-1] == '\n') buff[n_chars-1] = '\0'; else buff[n_chars] = '\0';
          sprintf(e->txt,"ERROR: Invalid characters after begining string\nline %d ( %s )\n",*line_n,buff);
          return 0;
        }
        break;
      case GROUP_TYPE_MODIFICATION: //special case
        if(!prev_g)
        {
          e->error = 8;
          if(buff[n_chars-1] == '\n') buff[n_chars-1] = '\0'; else buff[n_chars] = '\0';
          sprintf(e->txt,"ERROR: Modification with no previous group\nline %d ( %s )\n",*line_n,buff);
          return 0;
        }
        *b = s;
        return parse_modifications(fp, line_n, buff, b, prev_g, e);
        break;
      case GROUP_TYPE_NULL:
        ;
        break;
      default: //appease compiler
        break;
    }
  }
  *b = s;
  return 1;
}

int parse_number(char *b, int *n)
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

int parse_modification(FILE *fp, int *line_n, char *buff, char **b, modification *m, parse_error *e)
{
  int n_chars = 0;
  size_t line_buff_len = max_read_line_len;
  char *s = *b;
  int found = 0;
  while(!found)
  {
    if(*s == '\0')
    {
      n_chars = getline(&buff, &line_buff_len, fp);
      *line_n = *line_n+1;
      if(n_chars <= 0)
      {
        *buff = '\0';
        e->error = 7;
        sprintf(e->txt,"ERROR: EOF\nline %d\n",*line_n);
        return 0;
      }
      s = buff;
    }

    int d = 1;
    while(d > 0)
    {
      while(*s == ' ') s++;
           if(*s == 'd' ) { s++; d = parse_number(s,&m->n_deletions);           }
      else if(*s == 's' ) { s++; d = parse_number(s,&m->n_substitutions);       }
      else if(*s == 'i' ) { s++; d = parse_number(s,&m->n_injections);          }
      else if(*s == 'm' ) { s++; d = parse_number(s,&m->n_smart_substitutions); }
      else if(*s == '-' )
      {
        s++; *b = s;
        if(m->n_injections+m->n_smart_substitutions+m->n_substitutions+m->n_deletions == 0) return 1;
        else d = -1;
      }
      else break;

      if(d < 0)
      {
        e->error = 999;
        if(buff[n_chars-1] == '\n') buff[n_chars-1] = '\0'; else buff[n_chars] = '\0';
        sprintf(e->txt,"ERROR: Invalid line in modification\nline %d ( %s )\n",*line_n,buff);
        return 0;
      }
      else
      {
        s += d;
        found = 1;
      }
    }

    if(!found)
    {
           if(*s == '#' ) { s++; *s = '\0'; }
      else if(*s == '\n') { s++; *s = '\0'; }
      else if(*s == '\0') { s++; *s = '\0'; }
      else
      {
        *b = s;
        e->error = 10;
        if(buff[n_chars-1] == '\n') buff[n_chars-1] = '\0'; else buff[n_chars] = '\0';
        sprintf(e->txt,"ERROR: Invalid line in modification\nline %d ( %s )\n",*line_n,buff);
        return 0;
      }
    }
    else
    {
           if(*s == '#' ) { s++; *s = '\0'; }
      else if(*s == '\n') { s++; *s = '\0'; }
      else if(*s == '\0') { s++; *s = '\0'; }
      else if(*s == '"' ) //get chars
      {
        s++;
        char *c = s;
        while(*c != '"' && *c != '\n' && *c != '\0') c++;
        if(*c == '"')
        {
          m->n = (c-s);
          m->chars = (char *)malloc(sizeof(char)*m->n+1);
          c = m->chars;
          while(*s != '"')
          {
            *c = *s;
            c++;
            s++;
          }
          s++;
          *c = '\0';
        }
        else
        {
          e->error = 4;
          if(buff[n_chars-1] == '\n') buff[n_chars-1] = '\0'; else buff[n_chars] = '\0';
          sprintf(e->txt,"ERROR: Invalid characters after begining gamut\nline %d ( %s )\n",*line_n,buff);
          return 0;
        }
      }
    }
    if(m->n_substitutions+m->n_injections > 0 && !m->n)
    {
      e->error = 9;
      if(buff[n_chars-1] == '\n') buff[n_chars-1] = '\0'; else buff[n_chars] = '\0';
      sprintf(e->txt,"ERROR: gamut required with substitution or injection\nline %d ( %s )\n",*line_n,buff);
      return 0;
    }
  }
  *b = s;
  return 1;
}

int parse_modifications(FILE *fp, int *line_n, char *buff, char **b, group *g, parse_error *e)
{
  char *s;
  int valid_modification = 1;
  modification *m = (modification *)malloc(sizeof(modification));
  while(valid_modification)
  {
    zero_modification(m);

    valid_modification = parse_modification(fp,line_n,buff,b,m,e);

    if(valid_modification)
    {
      g->n_mods++;
      if(g->n_mods == 1) g->mods = (modification *)malloc(sizeof(modification));
      else               g->mods = (modification *)realloc(g->mods,sizeof(modification)*g->n_mods);
      g->mods[g->n_mods-1] = *m;
    }
  }
  free(m);

  if(e->error == 7)  return 1; //EOF- close everything
  if(e->error != 10) return 0; //wut

  s = *b;
  if(*s != ']')
  {
    e->error = 5;
    int n_chars = 0; while(buff[n_chars] != '\n' && buff[n_chars] != '\0') n_chars++; n_chars++; buff[n_chars-1] = '\0';
    sprintf(e->txt,"ERROR: Invalid characters within modifiers\nline %d ( %s )\n",*line_n,buff);
    return 0;
  }
  s++;
  *b = s;
  return 1;
}

int parse_childs(FILE *fp, int *line_n, char *buff, char **b, group *g, parse_error *e)
{
  char *s;
  int valid_kid = 1;
  group *prev_c = 0;
  group *c = (group *)malloc(sizeof(group));
  while(valid_kid)
  {
    zero_group(c);
    valid_kid = parse_child(fp, line_n, buff, b, c, prev_c, e);

    if(valid_kid)
    {
      if(c->type == GROUP_TYPE_MODIFICATION)
      {
        //ok
      }
      else if(c->type == GROUP_TYPE_NULL)
      {
        e->error = 888;
        int n_chars = 0; while(buff[n_chars] != '\n' && buff[n_chars] != '\0') n_chars++; n_chars++; buff[n_chars-1] = '\0';
        sprintf(e->txt,"Null child type found\nline %d ( %s )\n",*line_n,buff);
      }
      else
      {
        g->n++;
        if(g->n == 1) g->childs = (group *)malloc(sizeof(group));
        else          g->childs = (group *)realloc(g->childs,sizeof(group)*g->n);
        g->childs[g->n-1] = *c;
        prev_c = &g->childs[g->n-1];
      }
    }
  }
  free(c);

  if(e->error == 7) return 1; //EOF- close everything
  if(e->error != 1) return 0; //wut

  s = *b;
       if(*s == '>' && g->type == GROUP_TYPE_SEQUENCE) s++; //great
  else if(*s == '}' && g->type == GROUP_TYPE_OPTION)   s++; //great
  else if(*s == ')' && g->type == GROUP_TYPE_PERMUTE)  s++; //great
  else
  {
    e->error = 5;
    int n_chars = 0;
    while(buff[n_chars] != '\n' && buff[n_chars] != '\0') n_chars++; n_chars++; buff[n_chars-1] = '\0';
    if(g->type == GROUP_TYPE_SEQUENCE)
    sprintf(e->txt,"ERROR: Invalid characters within sequence\nline %d ( %s )\n",*line_n,buff);
    if(g->type == GROUP_TYPE_OPTION)
    sprintf(e->txt,"ERROR: Invalid characters within option\nline %d ( %s )\n",*line_n,buff);
    if(g->type == GROUP_TYPE_PERMUTE)
    sprintf(e->txt,"ERROR: Invalid characters within permute\nline %d ( %s )\n",*line_n,buff);
    return 0;
  }

  *b = s;
  return 1;
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

void preprocess_group(group *g, group *root)
{
  //remove null modifications
  if(g->n_mods == 1 && g->mods[0].n_injections+g->mods[0].n_smart_substitutions+g->mods[0].n_substitutions+g->mods[0].n_deletions == 0)
  {
    g->n_mods = 0;
    free(g->mods);
  }

  //collapse single-child, no modification groups
  if(g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION || g->type == GROUP_TYPE_PERMUTE)
  {
    for(int i = 0; i < g->n; i++)
    {
      group *c = &g->childs[i];
      if(c->n == 1 && c->n_mods == 0 && (c->type == GROUP_TYPE_SEQUENCE || c->type == GROUP_TYPE_OPTION || c->type == GROUP_TYPE_PERMUTE))
      {
        group *oc = c->childs;
        *c = c->childs[0];
        free(oc);
        i--;
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
        g->childs = (group *)realloc(g->childs,sizeof(group)*(g->n-1+c->n));
        c = &g->childs[i]; //NEED TO RE-ASSIGN C, AS G HAS BEEN REALLOCED
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

  //alloc/init modifications; collapse gamuts
  modification *m;
  for(int i = 0; i < g->n_mods; i++)
  {
    m = &g->mods[i];

    if(m->n_injections)          { m->injection_i          = (int *)malloc(sizeof(int)*m->n_injections);          for(int j = 0; j < m->n_injections;          j++) m->injection_i[j]          = 0; }
    if(m->n_smart_substitutions) { m->smart_substitution_i = (int *)malloc(sizeof(int)*m->n_smart_substitutions); for(int j = 0; j < m->n_smart_substitutions; j++) m->smart_substitution_i[j] = j; m->smart_substitution_i_c = (char *)malloc(sizeof(char)*m->n_smart_substitutions); }
    if(m->n_substitutions)       { m->substitution_i       = (int *)malloc(sizeof(int)*m->n_substitutions);       for(int j = 0; j < m->n_substitutions;       j++) m->substitution_i[j]       = j; }
    if(m->n_deletions)           { m->deletion_i           = (int *)malloc(sizeof(int)*m->n_deletions);           for(int j = 0; j < m->n_deletions;           j++) m->deletion_i[j]           = 0; }

    if(m->n_injections)          { m->injection_sub_i          = (int *)malloc(sizeof(int)*m->n_injections);          for(int j = 0; j < m->n_injections;          j++) m->injection_sub_i[j]          = 0; }
    if(m->n_smart_substitutions) { m->smart_substitution_sub_i = (int *)malloc(sizeof(int)*m->n_smart_substitutions); for(int j = 0; j < m->n_smart_substitutions; j++) m->smart_substitution_sub_i[j] = 0; }
    if(m->n_substitutions)       { m->substitution_sub_i       = (int *)malloc(sizeof(int)*m->n_substitutions);       for(int j = 0; j < m->n_substitutions;       j++) m->substitution_sub_i[j]       = 0; }

    if(m->n > 0)
      absorb_gamuts(m, root);
  }

  //ensure permutations cached
  if(g->type == GROUP_TYPE_PERMUTE)
  {
    if(n_cached_permute_indices <= g->n)
    {
      if(!n_cached_permute_indices) cache_permute_indices = (int ***)malloc(sizeof(int **)*(g->n+1));
      else                          cache_permute_indices = (int ***)realloc(cache_permute_indices,sizeof(int **)*(g->n+1));
      for(int i = n_cached_permute_indices; i < g->n+1; i++) cache_permute_indices[i] = 0;
      n_cached_permute_indices = g->n+1;
    }
    if(cache_permute_indices[g->n] == 0)
    {
      int f = g->n;
      for(int i = g->n-1; i > 1; i--) f *= i;
      cache_permute_indices[g->n] = (int **)malloc(sizeof(int *)*(f+1));
      cache_permute_indices[g->n][0] = 0;
      cache_permute_indices[g->n][0] += f;
      for(int i = 0; i < f; i++)
        cache_permute_indices[g->n][i+1] = (int *)malloc(sizeof(int)*g->n);

      //heap's algo- don't really understand
      int *c = (int *)malloc(sizeof(int)*g->n);
      for(int i = 0; i < g->n; i++) c[i] = 0;
      for(int i = 0; i < g->n; i++) cache_permute_indices[g->n][1][i] = i;
      if(f > 1) for(int i = 0; i < g->n; i++) cache_permute_indices[g->n][2][i] = i;
      int i = 0;
      int j = 2;
      while(i < g->n)
      {
        if(c[i] < i)
        {
          if(i%2 == 0) { int t = cache_permute_indices[g->n][j][0   ]; cache_permute_indices[g->n][j][0   ] = cache_permute_indices[g->n][j][i]; cache_permute_indices[g->n][j][i] = t; }
          else         { int t = cache_permute_indices[g->n][j][c[i]]; cache_permute_indices[g->n][j][c[i]] = cache_permute_indices[g->n][j][i]; cache_permute_indices[g->n][j][i] = t; }
          j++;
          if(j < f+1) for(int i = 0; i < g->n; i++) cache_permute_indices[g->n][j][i] = cache_permute_indices[g->n][j-1][i];
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
        for(int j = 0; j < g->n; j++)
        {
          printf("%d",cache_permute_indices[g->n][i+1][j]);
        }
        printf("\n");
      }
      */
    }
  }

  //recurse
  if(g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION || g->type == GROUP_TYPE_PERMUTE)
  {
    for(int i = 0; i < g->n; i++)
      preprocess_group(&g->childs[i], root);
  }
}

group *parse()
{
  FILE *fp;
  char *buff;

  fp = fopen(seed_file, "r");
  if(!fp) { fprintf(stderr,"Error opening seed file: %s\n",seed_file); exit(1); }
  buff = (char *)malloc(sizeof(char)*max_read_line_len);

  parse_error e;
  e.error = 0;
  e.txt = (char *)malloc(sizeof(char)*max_read_line_len);
  int line_n = -1;

  group *g = (group *)malloc(sizeof(group));
  zero_group(g);
  g->type = GROUP_TYPE_SEQUENCE;
  char *b = buff;
  *b = '\0';
  if(!parse_childs(fp, &line_n, buff, &b, g, &e))
    fprintf(stderr, "%s", e.txt);
  while(g->n == 1 && g->n_mods == 0 && (g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION || g->type == GROUP_TYPE_PERMUTE))
  {
    group *og = g->childs;
    *g = g->childs[0];
    free(og);
  }
  preprocess_group(g,g);

  free(buff);
  free(e.txt);

  return g;
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

int sprint_group(group *g, int inert, char *lockholder, char **buff_p)
{
  char *buff = *buff_p;
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
      if(!inert && g->n_mods) //this is incredibly inefficient
      {
        for(int i = 0; i < g->n; i++)
          sprint_group(&g->childs[i], 1, lockholder, buff_p);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
        if(!inert)
        {
          char *garbage = devnull;
          for(int i = 0; i < g->n; i++)
            inert = sprint_group(&g->childs[i], inert, lockholder, &garbage);
        }
      }
      else
      {
        for(int i = 0; i < g->n; i++)
          inert = sprint_group(&g->childs[i], inert, lockholder, buff_p);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
      }
      break;
    case GROUP_TYPE_OPTION:
      if(!inert && g->n_mods) //this is incredibly inefficient
      {
        sprint_group(&g->childs[g->i], 1, lockholder, buff_p);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
        if(!inert)
        {
          char *garbage = devnull;
          inert = sprint_group(&g->childs[g->i], inert, lockholder, &garbage);
          if(!inert)
          {
            g->i++;
            if(g->i == g->n) g->i = 0;
            else inert = 1;
          }
        }
      }
      else
      {
        inert = sprint_group(&g->childs[g->i], inert, lockholder, buff_p);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
        if(!inert)
        {
          g->i++;
          if(g->i == g->n) g->i = 0;
          else inert = 1;
        }
      }
      break;
    case GROUP_TYPE_PERMUTE:
      if(!inert && g->n_mods) //this is incredibly inefficient
      {
        for(int i = 0; i < g->n; i++)
          sprint_group(&g->childs[cache_permute_indices[g->n][g->i+1][i]], 1, lockholder, buff_p);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
        if(!inert)
        {
          char *garbage = devnull;
          for(int i = 0; i < g->n; i++)
            inert = sprint_group(&g->childs[cache_permute_indices[g->n][g->i+1][i]], inert, lockholder, &garbage);
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
          inert = sprint_group(&g->childs[cache_permute_indices[g->n][g->i+1][i]], inert, lockholder, buff_p);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
        if(!inert)
        {
          g->i++;
          if(g->i == cache_permute_indices[g->n][0]-(int *)0) g->i = 0;
          else inert = 1;
        }
      }
      break;
    case GROUP_TYPE_CHARS:
      strcpy(buff,g->chars);
      *buff_p = buff+g->n;
      inert = smodify_group(g, inert, lockholder, buff, buff_p);
      break;
    default: //appease compiler
      break;
  }
  return inert;
}

int approximate_length_group(group *g)
{
  int l = 0;
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
      l = 0;
      for(int i = 0; i < g->n; i++)
        l += approximate_length_group(&g->childs[i]);
      break;
    case GROUP_TYPE_OPTION:
      l = 0;
      for(int i = 0; i < g->n; i++)
        l += approximate_length_group(&g->childs[i]);
      l /= g->n;
      break;
    case GROUP_TYPE_PERMUTE:
      l = 0;
      for(int i = 0; i < g->n; i++)
        l += approximate_length_group(&g->childs[i]);
      break;
    case GROUP_TYPE_CHARS:
      l = g->n;
      break;
    default: //appease compile
      break;
  }
  return l;
}

unsigned long long int estimate_group(group *g)
{
  unsigned long long int e = 1;
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
      e = 1;
      for(int i = 0; i < g->n; i++)
        e *= estimate_group(&g->childs[i]);
      break;
    case GROUP_TYPE_OPTION:
      e = 0;
      for(int i = 0; i < g->n; i++)
        e += estimate_group(&g->childs[i]);
      break;
    case GROUP_TYPE_PERMUTE:
      e = 1;
      for(int i = 0; i < g->n; i++)
      {
        e *= estimate_group(&g->childs[i]);
        e *= (i+1);
      }
      break;
    case GROUP_TYPE_CHARS:
      e = 1;
      break;
    default: //appease compile
      break;
  }
  if(g->n_mods)
  {
    int l = approximate_length_group(g);
    unsigned long long int accrue_e = 0;
    unsigned long long int base_e = e;
    e = 1;
    for(int i = 0; i < g->n_mods; i++)
    {
      modification *m = &g->mods[i];
      for(int j = 0; j < m->n_smart_substitutions; j++)
        if(l+1-j > 0) e *= (l-j)*3; //assuming 3 is average smart sub
      for(int j = 0; j < m->n_injections; j++)
        if(l+1-j-m->n_smart_substitutions > 0) e *= (l+1-j)*m->n;
      for(int j = 0; j < m->n_substitutions; j++)
        if(l-j-m->n_smart_substitutions-m->n_injections > 0) e *= (l-m->n_smart_substitutions-j)*m->n;
      for(int j = 0; j < m->n_deletions; j++)
        if(l-j-m->n_smart_substitutions-m->n_injections-m->n_substitutions > 0) e *= l-m->n_smart_substitutions-m->n_substitutions-j;
      accrue_e += e*base_e;
      e = 1;
    }
    e = accrue_e;
  }
  return e;
}

void checkpoint_to_file(group *g)
{
  FILE *fp;
  fp = fopen(checkpoint_file_bak, "w");
  if(!fp) { fprintf(stderr,"Error opening checkpoint bak file: %s\n",password_file); exit(1); }
  checkpoint_group(g,fp);
  fclose(fp);

  fp = fopen(checkpoint_file, "w");
  if(!fp) { fprintf(stderr,"Error opening checkpoint file: %s\n",password_file); exit(1); }
  checkpoint_group(g,fp);
  fclose(fp);
}

void checkpoint_group(group *g, FILE *fp)
{
  fprintf(fp,"%d\n",g->i);
  fprintf(fp,"%d\n",g->mod_i);
  if(g->type != GROUP_TYPE_CHARS)
  {
    for(int i = 0; i < g->n; i++)
      checkpoint_group(&g->childs[i],fp);
  }
  for(int i = 0; i < g->n_mods; i++)
  {
    modification *m = &g->mods[i];
    for(int j = 0; j < m->n_injections; j++)
    {
      fprintf(fp,"%d\n",m->injection_i[j]);
      fprintf(fp,"%d\n",m->injection_sub_i[j]);
    }
    for(int j = 0; j < m->n_smart_substitutions; j++)
    {
      fprintf(fp,"%d\n",m->smart_substitution_i[j]);
      fprintf(fp,"%c\n",m->smart_substitution_i_c[j]);
      fprintf(fp,"%d\n",m->smart_substitution_sub_i[j]);
    }
    for(int j = 0; j < m->n_substitutions; j++)
    {
      fprintf(fp,"%d\n",m->substitution_i[j]);
      fprintf(fp,"%d\n",m->substitution_sub_i[j]);
    }
    for(int j = 0; j < m->n_deletions; j++)
    {
      fprintf(fp,"%d\n",m->deletion_i[j]);
    }
    fprintf(fp,"%d\n",m->nothing_i);
  }
}

void resume_from_file(group *g)
{
  FILE *fp;
  fp = fopen(progress_file, "r");
  if(!fp) { fprintf(stderr,"Error opening progress file: %s\n",password_file); exit(1); }
  resume_group(g,fp);
  fclose(fp);
}

void resume_group(group *g, FILE *fp)
{
  fscanf(fp,"%d\n",&g->i);
  fscanf(fp,"%d\n",&g->mod_i);
  if(g->type != GROUP_TYPE_CHARS)
  {
    for(int i = 0; i < g->n; i++)
      resume_group(&g->childs[i],fp);
  }
  for(int i = 0; i < g->n_mods; i++)
  {
    modification *m = &g->mods[i];
    for(int j = 0; j < m->n_injections; j++)
    {
      fscanf(fp,"%d\n",&m->injection_i[j]);
      fscanf(fp,"%d\n",&m->injection_sub_i[j]);
    }
    for(int j = 0; j < m->n_smart_substitutions; j++)
    {
      fscanf(fp,"%d\n",&m->smart_substitution_i[j]);
      fscanf(fp,"%c\n",&m->smart_substitution_i_c[j]);
      fscanf(fp,"%d\n",&m->smart_substitution_sub_i[j]);
    }
    for(int j = 0; j < m->n_substitutions; j++)
    {
      fscanf(fp,"%d\n",&m->substitution_i[j]);
      fscanf(fp,"%d\n",&m->substitution_sub_i[j]);
    }
    for(int j = 0; j < m->n_deletions; j++)
    {
      fscanf(fp,"%d\n",&m->deletion_i[j]);
    }
    fscanf(fp,"%d\n",&m->nothing_i);
  }
}

