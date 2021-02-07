#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifndef _WIN32
#include <unistd.h> //for isatty()
#endif //!_WIN32

#ifdef _WIN32
size_t getline(char **lineptr, size_t *n, FILE *stream) //hack recreation of this c11 api
{
  const int bsize = 1024;
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
      strcpy(*lineptr,buffer);
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

static const int version_maj = 0;
static const int version_min = 17;

static const int ERROR_NULL                      = 0;
static const int ERROR_EOF                       = 1;
static const int ERROR_INVALID_LINE              = 2;
static const int ERROR_INVALID_STRING            = 3;
static const int ERROR_INVALID_MODIFICATION      = 4;
static const int ERROR_UNTERMINATED_STRING       = 5;
static const int ERROR_UNPARENTED_MODIFICATION   = 6;
static const int ERROR_INVALID_NULL_MODIFICATION = 7;
static const int ERROR_MODIFICATION_EMPTY_GAMUT  = 8;
static const int ERROR_NULL_CHILD                = 9;
static const int ERROR_UTAG_RANGE                = 10;

int buff_len = 1024*1024; //1MB
const int max_pass_len = 300;
const int max_read_line_len = 1024;
const int max_utag_count = 16;
const int max_utag_stack = 4;
char *devnull;
char *tdevnull;

typedef unsigned int utag;
typedef struct
{
  utag map[max_utag_stack];
} utag_map;

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
void zero_modification(modification *m)
{
  m->chars = 0;
  m->n = 0;
  m->n_injections = 0;
  m->n_smart_substitutions = 0;
  m->n_substitutions = 0;
  m->n_deletions = 0;
  m->n_copys = 0;
  m->injection_i = 0;
  m->injection_sub_i = 0;
  m->smart_substitution_i = 0;
  m->smart_substitution_i_c = 0;
  m->smart_substitution_sub_i = 0;
  m->substitution_i = 0;
  m->substitution_sub_i = 0;
  m->deletion_i = 0;
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
  utag tag;
  unsigned int child_tag;
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
  g->tag = 0;
  g->child_tag = 0;
}

struct parse_error
{
  int error;
  int force;
  char *txt;
};

void append_password(char *password);
char smart_sub(int i, char key);
void basic_smart_substitute(int i, int sub_i, char *s);
int parse_modification(FILE *fp, int *line_n, char *buff, char **b, modification *m, parse_error *e);
int parse_modifications(FILE *fp, int *line_n, char *buff, char **b, group *g, parse_error *e);
int parse_child(FILE *fp, int *line_n, char *buff, char **b, group *g, group *prev_g, parse_error *e);
int parse_childs(FILE *fp, int *line_n, char *buff, char **b, group *g, parse_error *e);
int parse_utag(FILE *fp, int *line_n, char *buff, char **b, unsigned int *utag, parse_error *e);
group *parse();
unsigned long long int estimate_group(group *g);
utag stamp_utag(utag tag, utag dst, int inc);
utag_map *stamp_utag_map(utag tag, utag_map *map, int inc);
void merge_utag_map(utag_map map, utag_map *dst); //slow if many conflicts
int utag_conflict(utag tag, utag dst);
int utag_map_conflict(utag tag, utag_map map);
int utag_map_overconflict(utag tag, utag_map map);
int utag_map_overconflicted(utag_map map);
utag_map *zero_utag_map(utag_map *map);
void absorb_utag(group *dst, group *src);
unsigned int elevate_utags(group *g, group *parent, group *parent_option_child);
void preprocess_group(group *g);
group *unroll_group(group *g);
void collapse_group(group *g, group *root, int handle_gamuts);
void print_seed(group *g, int print_progress, int selected, int indent);
float approximate_length_premodified_group(group *g);
float approximate_length_modified_group(group *g);
int sprint_group(group *g, int inert, int *utag_dirty, char *lockholder, char **buff_p);
utag_map *merge_group_children_utag_map(group *g, utag_map *map);
int advance_group(group *g, utag parent_tag);
void zero_progress_group(group *g);
void zero_progress_modifications(group *g);
void checkpoint_group(group *g, FILE *fp);
void resume_group(group *g, FILE *fp);
void checkpoint_to_file(group *g);
void resume_from_file(group *g);
int parse_number(char *b, int *n);
void print_number(int n);
void print_utag(utag tag);

int ***cache_permute_indices;
int n_cached_permute_indices;

FILE *fp;
char *buff;
int buff_i;

group *mg;

const char *seed_file = "seed.txt";
int seed_specified = 0;
char *password_file = 0;
const char *resume_file = "seed.progress";
const char *checkpoint_file = "seed.progress";
char checkpoint_file_bak[512];
int estimate = 0;
int estimate_rate = 600000;
int resume = 0;
int validate_alpha_e            = 0;
int validate_up_alpha_e         = 0;
int validate_low_alpha_e        = 0;
int validate_numeric_e          = 0;
int validate_alphanumeric_e     = 0;
int validate_non_alphanumeric_e = 0;
int validate_length_min = 0;
int validate_length_max = max_pass_len;
int checkpoint = 0;
int unroll = 1000;
int normalize = 0;
int unquoted = 0;

int main(int argc, char **argv)
{
  int i = 1;
  while(i < argc)
  {
    if(strcmp(argv[i],"--help") == 0)
    {
      fprintf(stdout,"usage: expandpass [--help] [--estimate [@#]] [-i input_seed.txt] [-o output_passwords.txt] [--unroll #] [--normalize] [-b #] [-f[aA|A|a|#|aA#|@|l] #] [-c # [checkpoint_seed.progress]] [-r [recovery_seed.progress]]\n");
      fprintf(stdout,"--help shows this menu\n");
      fprintf(stdout,"--version displays version (%d.%d)\n",version_maj,version_min);
      fprintf(stdout,"--estimate shows a (crude) estimation of # of likely generatable passwords\n");
      fprintf(stdout,"--unroll specifies cutoff group size, below which will be optimized (default 1000; 0 == no unrolling)\n");
      fprintf(stdout,"--normalize prints normalized/optimized seed (as used in actual gen)\n");
      fprintf(stdout,"--unquoted treats otherwise invalid characters as single-character strings\n");
      fprintf(stdout,"-i specifies seed file (default \"seed.txt\" if blank)\n");
      fprintf(stdout,"   (see readme for seed syntax)\n");
      fprintf(stdout,"-o specifies output file (default stdout if blank)\n");
      fprintf(stdout,"-b specifies buffer length between output\n");
      fprintf(stdout,"-f filter output, ensuring existence of:\n");
      fprintf(stdout,"   aA alphabetic character [a-zA-Z]\n");
      fprintf(stdout,"   A uppercase alphabetic character [A-Z]\n");
      fprintf(stdout,"   a lowercase alphabetic character [a-z]\n");
      fprintf(stdout,"   # numeric character [0-9]\n");
      fprintf(stdout,"   aA# alphanumeric character [a-zA-Z0-9]\n");
      fprintf(stdout,"   @ non-alphanumeric character [!a-zA-Z0-9]\n");
      fprintf(stdout,"   optional number specifies required amount in character set (default 1)\n");
      fprintf(stdout,"-flmin filter output ensuring specified minimum length (default 10)\n");
      fprintf(stdout,"-flmax filter output ensuring specified maximum length (default 20)\n");
      fprintf(stdout,"-c specifies how often to checkpoint via progress file (default \"seed.progress\" if blank)\n");
      fprintf(stdout,"-r specifies to resume from progress file (default \"seed.progress\" if blank)\n");
      exit(0);
    }
    else if(strcmp(argv[i],"--version") == 0)
    {
      fprintf(stdout,"expandpass version %d.%d\n",version_maj,version_min);
      exit(0);
    }
    else if(strcmp(argv[i],"--estimate") == 0)
    {
      estimate = 1;
      if(i+1 < argc) { i++; if(argv[i][0] == '@') parse_number(argv[i]+1, &estimate_rate); else i--; }
    }
    else if(strcmp(argv[i],"--unroll") == 0)
    {
      i++;
      if(i >= argc || parse_number(argv[i], &unroll) < 0)
      {
        fprintf(stderr,"error: no unroll threshhold specified with --unroll\n");
        exit(1);
      }
    }
    else if(strcmp(argv[i],"--normalize") == 0)
    {
      normalize = 1;
    }
    else if(strcmp(argv[i],"--unquoted") == 0)
    {
      unquoted = 1;
    }
    else if(strcmp(argv[i],"-o") == 0)
    {
      if(i+1 >= argc)
      {
        fprintf(stderr,"error: no output file specified with -o\n");
        exit(1);
      }
      i++;
      password_file = argv[i];
    }
    else if(strcmp(argv[i],"-i") == 0)
    {
      if(i+1 >= argc)
      {
        fprintf(stderr,"error: no input file specified with -i\n");
        exit(1);
      }
      i++;
      seed_file = argv[i];
      seed_specified = 1;
    }
    else if(strcmp(argv[i],"-b") == 0)
    {
      i++;
      if(i >= argc || parse_number(argv[i], &buff_len) < 0)
      {
        fprintf(stderr,"error: no buffer size specified with -b\n");
        exit(1);
      }
    }
    else if(strcmp(argv[i],"-faA") == 0)
    {
      validate_alpha_e = 1;
      if(i+1 < argc)
      {
        i++;
        if(parse_number(argv[i], &validate_alpha_e) < 0) i--;
      }
    }
    else if(strcmp(argv[i],"-fA") == 0)
    {
      validate_up_alpha_e = 1;
      if(i+1 < argc)
      {
        i++;
        if(parse_number(argv[i], &validate_up_alpha_e) < 0) i--;
      }
    }
    else if(strcmp(argv[i],"-fa") == 0)
    {
      validate_low_alpha_e = 1;
      if(i+1 < argc)
      {
        i++;
        if(parse_number(argv[i], &validate_low_alpha_e) < 0) i--;
      }
    }
    else if(strcmp(argv[i],"-f#") == 0)
    {
      validate_numeric_e = 1;
      if(i+1 < argc)
      {
        i++;
        if(parse_number(argv[i], &validate_numeric_e) < 0) i--;
      }
    }
    else if(strcmp(argv[i],"-faA#") == 0)
    {
      validate_alphanumeric_e = 1;
      if(i+1 < argc)
      {
        i++;
        if(parse_number(argv[i], &validate_alphanumeric_e) < 0) i--;
      }
    }
    else if(strcmp(argv[i],"-f@") == 0)
    {
      validate_non_alphanumeric_e = 1;
      if(i+1 < argc)
      {
        i++;
        if(parse_number(argv[i], &validate_non_alphanumeric_e) < 0) i--;
      }
    }
    else if(strcmp(argv[i],"-fl") == 0 || strcmp(argv[i],"-flmin") == 0)
    {
      validate_length_min = 10;
      if(i+1 < argc)
      {
        i++;
        if(parse_number(argv[i], &validate_length_min) < 0) i--;
      }
    }
    else if(strcmp(argv[i],"-flmax") == 0)
    {
      validate_length_max = 20;
      if(i+1 < argc)
      {
        i++;
        if(parse_number(argv[i], &validate_length_max) < 0) i--;
      }
    }
    else if(strcmp(argv[i],"-c") == 0)
    {
      checkpoint = 1000000;
      if(i+1 < argc)
      {
        i++;
        if(parse_number(argv[i], &checkpoint) < 0) i--;
      }
      if(i+1 < argc) { i++; checkpoint_file = argv[i]; }
    }
    else if(strcmp(argv[i],"-r") == 0)
    {
      resume = 1;
      if(i+1 < argc) { i++; resume_file = argv[i]; }
    }
    else
    {
      fprintf(stderr,"error: unrecognized argument \"%s\"\n",argv[i]);
      exit(1);
    }
    i++;
  }
  sprintf(checkpoint_file_bak,"%s.bak",checkpoint_file);

  n_cached_permute_indices = 0;

  buff = (char *)malloc(sizeof(char)*buff_len);
  buff_i = 0;

  devnull = (char *)malloc(sizeof(char)*buff_len);

  char *passholder = (char *)malloc(sizeof(char)*max_pass_len);
  char *passholder_p = passholder;
  char *lockholder = (char *)malloc(sizeof(char)*max_pass_len);
  for(int i = 0; i < max_pass_len; i++) lockholder[i] = 0;

  group *g = parse();

  collapse_group(g,g,0);
  preprocess_group(g);
  elevate_utags(g,0,0);

  if(unroll)
  {
    group *og = g;
    g = unroll_group(og);
    if(g != og) free(og);
  }

  collapse_group(g,g,1);

  if(normalize)
  {
    print_seed(g,0,0,0);
    exit(0);
  }

  if(estimate)
  {
    unsigned long long int e = estimate_group(g);
    unsigned long long int d = e;
    unsigned long long int h = e;
    unsigned long long int m = e;
    unsigned long long int s = e;
    fprintf(stdout,"estimated output for seed file (%s): %llu @ %d/s\n",seed_file,e,estimate_rate);
    d /= estimate_rate;
    h /= estimate_rate;
    m /= estimate_rate;
    s /= estimate_rate;
    d /= 60; //minute
    h /= 60;
    m /= 60;
    d /= 60; //hour
    h /= 60;
    d /= 24; //day
    if(d) { fprintf(stdout,"%llud ",d); h -= d*24; m -= d*24*60; s -= d*24*60*60; }
    if(h) { fprintf(stdout,"%lluh ",h);            m -= h*60;    s -= h*60*60; }
    if(m) { fprintf(stdout,"%llum ",m);                          s -= m*60; }
    if(s) { fprintf(stdout,"%llus ",s); ; }
    if(d+h+m+s <= 0) { fprintf(stdout,"0s "); }
    fprintf(stdout,"\n");
    exit(0);
  }

  if(resume)
    resume_from_file(g);

  if(password_file)
  {
    if(resume) fp = fopen(password_file, "a");
    else       fp = fopen(password_file, "w");
    if(!fp) { fprintf(stderr,"Error opening output file: %s\n",password_file); exit(1); }
  }
  else fp = stdout;

  mg = g; //set global for debugging purposes

  int done = 0;
  long long int e = 0;
  utag_map map;
  int utag_dirty = 1;
  while(!done)
  {
    int preskip = 0;
    //print_seed(g,1,1,0);
    if(g->child_tag)
    {
      if(utag_dirty)
      {
        merge_group_children_utag_map(g,zero_utag_map(&map));
        if(utag_map_overconflicted(map)) if(advance_group(g,0)) break; //advance_group returning 1 means done
      }
      done = !sprint_group(g, 0, &utag_dirty, lockholder, &passholder_p);
    }
    else done = !sprint_group(g, 0, &utag_dirty, lockholder, &passholder_p);
    e++;
    *passholder_p = '\0';
    int plength = passholder_p-passholder;
    if(!preskip && validate_length_min <= plength && validate_length_max >= plength)
    {
      int alpha_e = 0;
      int up_alpha_e = 0;
      int low_alpha_e = 0;
      int numeric_e = 0;
      int alphanumeric_e = 0;
      int non_alphanumeric_e = 0;
      passholder_p = passholder;
      while(*passholder_p != '\0')
      {
             if(*passholder_p >= 'A' && *passholder_p <= 'Z') { up_alpha_e++;  alpha_e++; alphanumeric_e++; }
        else if(*passholder_p >= 'a' && *passholder_p <= 'z') { low_alpha_e++; alpha_e++; alphanumeric_e++; }
        else if(*passholder_p >= '0' && *passholder_p <= '9') { numeric_e++;              alphanumeric_e++; }
        else                                                  { non_alphanumeric_e++;                       }
        passholder_p++;
      }
      if(
        alpha_e            >= validate_alpha_e &&
        up_alpha_e         >= validate_up_alpha_e &&
        low_alpha_e        >= validate_low_alpha_e &&
        numeric_e          >= validate_numeric_e &&
        alphanumeric_e     >= validate_alphanumeric_e &&
        non_alphanumeric_e >= validate_non_alphanumeric_e
      ) append_password(passholder);
    }
    passholder_p = passholder;
    if(checkpoint && e >= checkpoint)
    {
      checkpoint_to_file(g);
      e = 0;
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

int parse_child(FILE *fp, int *line_n, char *buff, char **b, group *g, group *prev_g, parse_error *e)
{
  int n_chars = 0; while(buff[n_chars] != '\0') n_chars++;
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
        e->error = ERROR_EOF;
        sprintf(e->txt,"ERROR: EOF\nline %d\n",*line_n);
        return 0;
      }
      s = buff;
    }
    while(*s == ' ' || *s == '\t') s++;
         if(*s == '<' ) { g->type = GROUP_TYPE_SEQUENCE;     s++; }
    else if(*s == '{' ) { g->type = GROUP_TYPE_OPTION;       s++; }
    else if(*s == '(' ) { g->type = GROUP_TYPE_PERMUTE;      s++; }
    else if(*s == '"' ) { g->type = GROUP_TYPE_CHARS;        s++; }
    else if(*s == '[' ) { g->type = GROUP_TYPE_MODIFICATION; s++; }
    else if(*s == '#' ) { g->type = GROUP_TYPE_NULL;         s++; *s = '\0'; }
    else if(*s == '\n') { g->type = GROUP_TYPE_NULL;         s++; *s = '\0'; }
    else if(*s == '\0') { g->type = GROUP_TYPE_NULL;         s++; *s = '\0'; }
    else if(*s == '-')
    {
      char *c;
      g->type = GROUP_TYPE_CHARS;
      g->n = 0;
      g->chars = (char *)malloc(sizeof(char)*g->n+1);
      c = g->chars;
      s++;
      *c = '\0';
      *b = s;
      return 1;
    }
    else if(unquoted && *s != '>' && *s != '}' && *s != ')' && *s != ']')
    {
      char *c;
      g->type = GROUP_TYPE_CHARS;
      if(*s == '\\') s++;
      g->n = 1;
      g->chars = (char *)malloc(sizeof(char)*g->n+1);
      c = g->chars;
      *c = *s;
      c++;
      s++;
      *c = '\0';
      *b = s;
      return 1;
    }
    else
    {
      *b = s;
      e->error = ERROR_INVALID_LINE;
      if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
      sprintf(e->txt,"ERROR: Invalid line\nline %d ( %s )\n",*line_n,buff);
      return 0;
    }

    while(*s == ' ' || *s == '\t') s++;

    char *c;
    switch(g->type)
    {
      case GROUP_TYPE_SEQUENCE:
      case GROUP_TYPE_OPTION:
      case GROUP_TYPE_PERMUTE:
        g->n = 0;
        *b = s;
        if(g->type == GROUP_TYPE_SEQUENCE && !parse_utag(fp, line_n, buff, b, &g->tag, e)) return 0;
        return parse_childs(fp, line_n, buff, b, g, e);
        break;
      case GROUP_TYPE_CHARS:
        c = s;
        while(*c != '"' && *c != '\n' && *c != '\0') { if(*c == '\\') c++; c++; }
        if(*c == '"')
        {
          g->n = (c-s);
          g->chars = (char *)malloc(sizeof(char)*g->n+1);
          c = g->chars;
          while(*s != '"' && *s != '\n' && *s != '\0')
          {
            if(*s == '\\') { s++; g->n--; }
            *c = *s;
            c++;
            s++;
          }
          if(*s != '"')
          {
            e->error = ERROR_UNTERMINATED_STRING;
            if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
            sprintf(e->txt,"ERROR: Unterminated string\nline %d ( %s )\n",*line_n,buff);
            return 0;
          }
          s++;
          *c = '\0';
        }
        else
        {
          e->error = ERROR_INVALID_STRING;
          if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
          sprintf(e->txt,"ERROR: Invalid characters after begining string\nline %d ( %s )\n",*line_n,buff);
          return 0;
        }
        break;
      case GROUP_TYPE_MODIFICATION: //special case
        if(!prev_g)
        {
          e->error = ERROR_UNPARENTED_MODIFICATION;
          if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
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

int parse_utag(FILE *fp, int *line_n, char *buff, char **b, unsigned int *utag, parse_error *e)
{
  int n_chars = 0; while(buff[n_chars] != '\0') n_chars++;
  size_t line_buff_len = max_read_line_len;
  char *s = *b;
  while(1)
  {
    if(*s == '\0')
    {
      n_chars = getline(&buff, &line_buff_len, fp);
      *line_n = *line_n+1;
      if(n_chars <= 0)
      {
        *buff = '\0';
        e->error = ERROR_EOF;
        sprintf(e->txt,"ERROR: EOF\nline %d\n",*line_n);
        return 0;
      }
      s = buff;
    }
    else if(*s == ' ' || *s == '\t') s++;
    else break;
  }

  int u = 0;
  int d;
  while((d = parse_number(s,&u)) != -1)
  {
    if(u < 1 || u >= max_utag_count)
    {
      e->error = ERROR_UTAG_RANGE;
      sprintf(e->txt,"ERROR: unique tag outside of valid range [1-%d]\nline %d\n",max_utag_count-1,*line_n);
      return 0;
    }
    s += d;
    *utag |= 1 << (u-1);
    if(*s == ',') s++;
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
void print_number(int n)
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
void print_utag(utag tag)
{
  int first = 1;
  for(int i = 0; i < max_utag_count; i++)
  {
    if(tag &= (1 << i))
    {
      if(!first)printf(",");first = 0;
      print_number(i+1);
    }
  }
}

int parse_modification(FILE *fp, int *line_n, char *buff, char **b, modification *m, parse_error *e)
{
  int n_chars = 0; while(buff[n_chars] != '\0') n_chars++;
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
        e->error = ERROR_EOF;
        sprintf(e->txt,"ERROR: EOF\nline %d\n",*line_n);
        return 0;
      }
      s = buff;
    }

    int d = 1;
    while(d > 0)
    {
      while(*s == ' ' || *s == '\t') s++;
           if(*s == 'c' ) { s++; d = parse_number(s,&m->n_copys); m->n_copys -= 1; } //m->n_copys = 0 means "just do it once" (ie default)
      else if(*s == 'd' ) { s++; d = parse_number(s,&m->n_deletions);           }
      else if(*s == 's' ) { s++; d = parse_number(s,&m->n_substitutions);       }
      else if(*s == 'i' ) { s++; d = parse_number(s,&m->n_injections);          }
      else if(*s == 'm' ) { s++; d = parse_number(s,&m->n_smart_substitutions); }
      else if(*s == '-' )
      {
        s++; *b = s;
        if(m->n_injections+m->n_smart_substitutions+m->n_substitutions+m->n_deletions+m->n_copys == 0) return 1;
        else d = -1;
      }
      else break;

      if(d < 0)
      {
        e->error = ERROR_INVALID_NULL_MODIFICATION;
        if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
        sprintf(e->txt,"ERROR: Invalid null modification\nline %d ( %s )\n",*line_n,buff);
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
        e->error = ERROR_INVALID_MODIFICATION;
        if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
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
        while(*c != '"' && *c != '\n' && *c != '\0') { if(*c == '\\') c++; c++; }
        if(*c == '"')
        {
          m->n = (c-s);
          m->chars = (char *)malloc(sizeof(char)*m->n+1);
          c = m->chars;
          while(*s != '"' && *s != '\n' && *s != '\0')
          {
            if(*s == '\\') { s++; m->n--; }
            *c = *s;
            c++;
            s++;
          }
          if(*s != '"')
          {
            e->error = ERROR_UNTERMINATED_STRING;
            if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
            sprintf(e->txt,"ERROR: Unterminated modification gamut\nline %d ( %s )\n",*line_n,buff);
            return 0;
          }
          s++;
          *c = '\0';
        }
        else
        {
          e->error = ERROR_INVALID_STRING;
          if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
          sprintf(e->txt,"ERROR: Invalid characters after begining gamut\nline %d ( %s )\n",*line_n,buff);
          return 0;
        }
      }
    }
    if(m->n_substitutions+m->n_injections > 0 && !m->n)
    {
      e->error = ERROR_MODIFICATION_EMPTY_GAMUT;
      if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
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

  if(e->error == ERROR_EOF)                  return 1; //close everything
  if(e->error != ERROR_INVALID_MODIFICATION) return 0; //let "invalid modification" attempt parse as "end modification"
  if(e->force)                               return 0; //if error force, don't allow passthrough ("unexpected parse" should only bubble up one level)

  s = *b;
  if(*s != ']')
  {
    //e->error; //retain error from stack
    int n_chars = 0; while(buff[n_chars] != '\0') n_chars++;
    if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
    sprintf(e->txt,"ERROR: Invalid characters within modifiers\nline %d ( %s )\n",*line_n,buff);
    e->force = 1;
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
        e->error = ERROR_NULL_CHILD;
        int n_chars = 0; while(buff[n_chars] != '\0') n_chars++;
        if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
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

  if(e->error == ERROR_EOF)          return 1; //close everything
  if(e->error != ERROR_INVALID_LINE) return 0; //let "invalid line" attempt parse as "end line"
  if(e->force)                       return 0; //if error force, don't allow passthrough ("unexpected parse" should only bubble up one level)

  s = *b;
       if(*s == '>' && g->type == GROUP_TYPE_SEQUENCE) s++; //great
  else if(*s == '}' && g->type == GROUP_TYPE_OPTION)   s++; //great
  else if(*s == ')' && g->type == GROUP_TYPE_PERMUTE)  s++; //great
  else
  {
    //e->error; //retain error from stack
    int n_chars = 0; while(buff[n_chars] != '\0') n_chars++;
    if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
    if(g->type == GROUP_TYPE_SEQUENCE)
    sprintf(e->txt,"ERROR: Invalid characters within sequence\nline %d ( %s )\n",*line_n,buff);
    if(g->type == GROUP_TYPE_OPTION)
    sprintf(e->txt,"ERROR: Invalid characters within option\nline %d ( %s )\n",*line_n,buff);
    if(g->type == GROUP_TYPE_PERMUTE)
    sprintf(e->txt,"ERROR: Invalid characters within permute\nline %d ( %s )\n",*line_n,buff);
    e->force = 1;
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

utag stamp_utag(utag tag, utag dst, int inc)
{
  if(inc) return dst |  tag;
  else    return dst & ~tag;
}

utag_map *stamp_utag_map(utag tag, utag_map *map, int inc)
{
  if(inc)
  {
    utag carry = 0;
    for(int i = 0; tag && i < max_utag_stack; i++)
    {
      carry = tag & map->map[i];
      map->map[i] ^= tag;
      tag = carry;
    }
    if(tag)
    {
      for(int i = 0; tag && i < max_utag_stack; i++)
        map->map[i] |= tag; //don't overflow; max out bits
    }
  }
  else
  {
    utag carry = 0;
    for(int i = 0; tag && i < max_utag_stack; i++)
    {
      carry = tag & ~map->map[i];
      map->map[i] ^= tag;
      tag = carry;
    }
    if(tag)
    {
      for(int i = 0; tag && i < max_utag_stack; i++)
        map->map[i] &= ~tag; //don't underflow; zero out bits
    }
  }
  return map;
}

void merge_utag_map(utag_map map, utag_map *dst) //slow if many conflicts
{
  if(map.map[0]) stamp_utag_map(map.map[0], dst, 1);
  int pow = 2;
  for(int i = 1; i < max_utag_stack; i++)
  {
    if(map.map[i]) for(int j = 0; j < pow; j++) stamp_utag_map(map.map[i], dst, 1);
    pow *= 2;
  }
}

int utag_conflict(utag tag, utag dst)
{
  return tag & dst;
}

int utag_map_conflict(utag tag, utag_map map)
{
  for(int i = 0; i < max_utag_stack; i++)
    if(tag & map.map[i]) return 1;
  return 0;
}

int utag_map_overconflict(utag tag, utag_map map)
{
  for(int i = 1; i < max_utag_stack; i++)
    if(tag & map.map[i]) return 1;
  return 0;
}

int utag_map_overconflicted(utag_map map)
{
  for(int i = 1; i < max_utag_stack; i++)
    if(map.map[i]) return 1;
  return 0;
}

utag_map *zero_utag_map(utag_map *map)
{
  memset(map,0,sizeof(utag_map));
  return map;
}
void absorb_utag(group *dst, group *src)
{
  dst->tag |= src->tag;
  src->tag = 0;
}

unsigned int elevate_utags(group *g, group *parent, group *parent_option_child)
{
  if(parent && parent->type == GROUP_TYPE_OPTION) parent_option_child = g;
  else if(parent_option_child) absorb_utag(parent_option_child,g);
  if(g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION || g->type == GROUP_TYPE_PERMUTE)
  {
    for(int i = 0; i < g->n; i++)
      g->child_tag |= (elevate_utags(&g->childs[i],g,parent_option_child) || g->child_tag);
  }
  return g->tag | g->child_tag;
}

void preprocess_group(group *g)
{
  //alloc/init modifications
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

  //collapse single-or-less-child, no modification groups
  if(g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION || g->type == GROUP_TYPE_PERMUTE)
  {
    for(int i = 0; i < g->n; i++)
    {
      group *c = &g->childs[i];
      if(c->type == GROUP_TYPE_SEQUENCE || c->type == GROUP_TYPE_OPTION || c->type == GROUP_TYPE_PERMUTE)
      {
        if(c->n == 1 && c->n_mods == 0)
        {
          absorb_utag(&c->childs[0],c); //childs[0] will become c
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
        if(g->type == GROUP_TYPE_OPTION && c->tag) continue;
        absorb_utag(g,c);
        g->childs = (group *)realloc(g->childs,sizeof(group)*(g->n-1+c->n));
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

group *parse()
{
  FILE *fp;
  char *buff;

  #ifdef _WIN32
  if(false) ;
  #else
  if(!seed_specified && !isatty(fileno(stdin))) fp = stdin;
  #endif
  else
  {
    fp = fopen(seed_file, "r");
    if(!fp) { fprintf(stderr,"Error opening seed file: %s\n",seed_file); exit(1); }
  }
  buff = (char *)malloc(sizeof(char)*max_read_line_len);

  parse_error e;
  e.error = ERROR_NULL;
  e.force = 0;
  e.txt = (char *)malloc(sizeof(char)*max_read_line_len);
  int line_n = 0;

  group *g = (group *)malloc(sizeof(group));
  zero_group(g);
  g->type = GROUP_TYPE_SEQUENCE;
  char *b = buff;
  *b = '\0';
  if(!parse_childs(fp, &line_n, buff, &b, g, &e))
  {
    fprintf(stderr, "%s", e.txt); exit(1);
  }
  while(g->n == 1 && g->n_mods == 0 && (g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION || g->type == GROUP_TYPE_PERMUTE))
  {
    group *og = g->childs;
    *g = g->childs[0];
    free(og);
  }

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
int sprint_group(group *g, int inert, int *utag_dirty, char *lockholder, char **buff_p) //"inert = 1" is the cue to stop incrementing as the rest of the password is written
{
  char *buff = *buff_p;
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
    {
      if(!inert && g->n_mods) //this is incredibly inefficient
      {
        for(int i = 0; i < g->n; i++)
          sprint_group(&g->childs[i], 1, utag_dirty, lockholder, buff_p); //dry run
        inert = smodify_group(g, inert, lockholder, buff, buff_p); //to let modifications get a chance to increment
        if(!inert) //still hot?
        {
          for(int i = 0; i < g->n; i++)
          {
            tdevnull = devnull;
            inert = sprint_group(&g->childs[i], inert, utag_dirty, lockholder, &tdevnull); //redo dry run, updating state (but printing to garbage)
          }
        }
      }
      else //inert OR no modifications means no dry run necessary
      {
        for(int i = 0; i < g->n; i++)
          inert = sprint_group(&g->childs[i], inert, utag_dirty, lockholder, buff_p);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
      }
    }
      break;
    case GROUP_TYPE_OPTION:
    {
      if(!inert && g->n_mods) //this is incredibly inefficient
      {
        sprint_group(&g->childs[g->i], 1, utag_dirty, lockholder, buff_p);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
        if(!inert)
        {
          tdevnull = devnull;
          inert = sprint_group(&g->childs[g->i], inert, utag_dirty, lockholder, &tdevnull);
          if(!inert)
          {
            if(g->childs[g->i].tag) *utag_dirty = 1;
            g->i++;
            if(g->i == g->n) g->i = 0;
            else inert = 1;
            if(g->childs[g->i].tag) *utag_dirty = 1;
          }
        }
      }
      else
      {
        inert = sprint_group(&g->childs[g->i], inert, utag_dirty, lockholder, buff_p);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
        if(!inert)
        {
          if(g->childs[g->i].tag) *utag_dirty = 1;
          g->i++;
          if(g->i == g->n) g->i = 0;
          else inert = 1;
          if(g->childs[g->i].tag) *utag_dirty = 1;
        }
      }
    }
      break;
    case GROUP_TYPE_PERMUTE:
    {
      if(!inert && g->n_mods) //this is incredibly inefficient
      {
        for(int i = 0; i < g->n; i++)
          sprint_group(&g->childs[cache_permute_indices[g->n][g->i+1][i]], 1, utag_dirty, lockholder, buff_p);
        inert = smodify_group(g, inert, lockholder, buff, buff_p);
        if(!inert)
        {
          for(int i = 0; i < g->n; i++)
          {
            tdevnull = devnull;
            inert = sprint_group(&g->childs[cache_permute_indices[g->n][g->i+1][i]], inert, utag_dirty, lockholder, &tdevnull);
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
          inert = sprint_group(&g->childs[cache_permute_indices[g->n][g->i+1][i]], inert, utag_dirty, lockholder, buff_p);
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
      strcpy(buff,g->chars);
      *buff_p = buff+g->n;
      inert = smodify_group(g, inert, lockholder, buff, buff_p);
      break;
    default: //appease compiler
      break;
  }
  return inert;
}

utag_map *merge_group_children_utag_map(group *g, utag_map *map)
{
  utag tag = g->tag;
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
    case GROUP_TYPE_PERMUTE:
      for(int i = 0; i < g->n; i++)
        merge_group_children_utag_map(&g->childs[i], map);
      break;
    case GROUP_TYPE_OPTION:
    {
      stamp_utag_map(g->childs[g->i].tag,map,1); //only need to stamp here, as the only things with utags should be children of options
      merge_group_children_utag_map(&g->childs[g->i], map);
    }
      break;
    case GROUP_TYPE_CHARS:
    default: //appease compiler
      break;
  }
  return map;
}

int advance_group(group *g, utag parent_tag)
{
  int carry = 0;
  utag_map child_map;
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
          carry = advance_group(&g->childs[i], parent_tag);
        }
        if(!carry) merge_group_children_utag_map(&g->childs[i], zero_utag_map(&child_map));
        while(!carry && (utag_map_overconflicted(child_map) || utag_map_conflict(parent_tag,child_map)))
        {
          advanced = 1;
          newly_advanced = 1;
          carry = advance_group(&g->childs[i], parent_tag);
          if(!carry) merge_group_children_utag_map(&g->childs[i], zero_utag_map(&child_map));
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
            return 1;
          }
          else
          {
            utag untag = merge_group_children_utag_map(&g->childs[i],zero_utag_map(&child_map))->map[0];
            parent_tag = stamp_utag(untag,parent_tag,0);
          }
        }
        else
        {
          parent_tag = stamp_utag(child_map.map[0], parent_tag, 1);
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
        while(carry || utag_conflict(g->childs[g->i].tag,parent_tag))
        {
          g->i++;
          advanced = 1;
          carry = 0;
          if(g->i == g->n)
          {
            g->i = 0;
            zero_progress_group(g);
            return 1;
          }
        }
        parent_tag = stamp_utag(g->childs[g->i].tag,parent_tag,1);
        if(!advanced) carry = advance_group(&g->childs[g->i], parent_tag);
        advanced = 1;
        if(!carry) merge_group_children_utag_map(&g->childs[g->i], zero_utag_map(&child_map));
        while(!carry && (utag_map_overconflicted(child_map) || utag_map_conflict(parent_tag,child_map)))
        {
          carry = advance_group(&g->childs[g->i], parent_tag);
          if(!carry) merge_group_children_utag_map(&g->childs[g->i], zero_utag_map(&child_map));
        }
        if(carry) parent_tag = stamp_utag(g->childs[g->i].tag,parent_tag,0);
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
          carry = advance_group(&g->childs[pi], parent_tag);
        }
        if(!carry) merge_group_children_utag_map(&g->childs[pi], zero_utag_map(&child_map));
        while(!carry && (utag_map_overconflicted(child_map) || utag_map_conflict(parent_tag,child_map)))
        {
          advanced = 1;
          newly_advanced = 1;
          carry = advance_group(&g->childs[pi], parent_tag);
          if(!carry) merge_group_children_utag_map(&g->childs[pi], zero_utag_map(&child_map));
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
            utag untag = merge_group_children_utag_map(&g->childs[pi],zero_utag_map(&child_map))->map[0];
            parent_tag = stamp_utag(untag,parent_tag,0);
          }
        }
        else
        {
          parent_tag = stamp_utag(child_map.map[0], parent_tag, 1);
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
    float accrue_e = 0;
    int e = 0;
    for(int i = 0; i < g->n_mods; i++)
    {
      modification *m = &g->mods[i];
      //if(l > 0                                             && m->n_smart_substitutions > 0) ; //do nothing
      if(l+m->n_injections > 0                             && m->n_injections          > 0) e += m->n_injections;
      //if(l-m->n_smart_substitutions > 0                    && m->n_substitutions       > 0) ; //do nothing
      if(l-m->n_smart_substitutions-m->n_substitutions > 0 && m->n_deletions           > 0) e -= m->n_deletions;
      accrue_e += e;
      e = 0;
    }
    l += accrue_e/g->n_mods;
  }

  return l;
}

unsigned long long int ppow(int choose, int option)
{
  unsigned long long int t = option;
  for(int i = 0; i < choose-1; i++)
    t *= option;
  return t;
}

unsigned long long int permcomb(int len, int choose, int option)
{
  if(choose == 1) return option*len;
  if(len == choose) return ppow(choose,option);

  unsigned long long int t = 0;
  for(int len_i = len-1; len_i >= (choose-1); len_i--)
    t += permcomb(len_i, (choose-1), option)*option;

  return t;
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
    int l = approximate_length_premodified_group(g);
    unsigned long long int accrue_e = 0;
    unsigned long long int base_e = e;
    e = 1;
    for(int i = 0; i < g->n_mods; i++)
    {
      modification *m = &g->mods[i];
      if(l > 0                                             && m->n_smart_substitutions > 0) e *= permcomb(l,                                             m->n_smart_substitutions, 2); //assuming 2 is average smart sub
      if(l+m->n_injections > 0                             && m->n_injections          > 0) e *= permcomb(l+m->n_injections,                             m->n_injections,          m->n);
      if(l-m->n_smart_substitutions > 0                    && m->n_substitutions       > 0) e *= permcomb(l-m->n_smart_substitutions,                    m->n_substitutions,       m->n);
      if(l-m->n_smart_substitutions-m->n_substitutions > 0 && m->n_deletions           > 0) e *= permcomb(l-m->n_smart_substitutions-m->n_substitutions, m->n_deletions,           1);
      accrue_e += e*base_e;
      e = 1;
    }
    e = accrue_e;
  }
  return e;
}

group *unroll_group(group *g)
{
  int doit = 0;
  if(g->child_tag) return g;
  if(estimate_group(g) < unroll)
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
      char *passholder = (char *)malloc(sizeof(char)*max_pass_len);
      char *passholder_p = passholder;
      char *lockholder = (char *)malloc(sizeof(char)*max_pass_len);
      for(int i = 0; i < max_pass_len; i++) lockholder[i] = 0;

      int done = 0;
      group *ng = (group *)malloc(sizeof(group));
      zero_group(ng);
      ng->type = GROUP_TYPE_OPTION;
      absorb_utag(ng,g);
      int utag_dirty = 0; //irrelevant
      group *cg;
      while(!done)
      {
        done = !sprint_group(g, 0, &utag_dirty, lockholder, &passholder_p);
        *passholder_p = '\0';
        ng->n++;
        if(ng->n == 1) ng->childs = (group *)malloc(sizeof(group));
        else           ng->childs = (group *)realloc(ng->childs,sizeof(group)*ng->n);
        cg = &ng->childs[ng->n-1];
        zero_group(cg);
        cg->type = GROUP_TYPE_CHARS;
        cg->n = passholder_p-passholder;
        cg->chars = (char *)malloc(sizeof(char)*(cg->n+1));
        strcpy(cg->chars,passholder);
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
      ng = unroll_group(og);
      if(og != ng)
      {
        *og = *ng;
        free(ng);
      }
    }
  }
  return g;
}

void print_seed(group *g, int print_progress, int selected, int indent)
{
  switch(g->type)
  {
    case GROUP_TYPE_SEQUENCE:
      for(int i = 0; i < indent; i++) printf("  "); if(selected) printf("*"); printf("<"); if(g->tag) print_utag(g->tag); printf("\n");
      for(int i = 0; i < g->n; i++) print_seed(&g->childs[i],print_progress,selected,indent+1);
      for(int i = 0; i < indent; i++) printf("  "); printf(">"); if(selected) printf("*"); printf("\n");
      break;
    case GROUP_TYPE_OPTION:
      for(int i = 0; i < indent; i++) printf("  "); if(selected) printf("*"); if(g->tag) { printf("<"); print_utag(g->tag); } printf("{"); printf("\n");
      for(int i = 0; i < g->n; i++) { print_seed(&g->childs[i],print_progress,print_progress && selected && i == g->i,indent+1); }
      for(int i = 0; i < indent; i++) printf("  "); printf("}"); if(g->tag) printf(">"); if(selected) printf("*"); printf("\n");
      break;
    case GROUP_TYPE_PERMUTE:
      for(int i = 0; i < indent; i++) printf("  "); if(selected) printf("*"); if(g->tag) { printf("<"); print_utag(g->tag); } printf("("); printf("\n");
      if(print_progress) { for(int i = 0; i < g->n; i++) print_seed(&g->childs[cache_permute_indices[g->n][g->i+1][i]],print_progress,selected,indent+1); }
      else               { for(int i = 0; i < g->n; i++) print_seed(&g->childs[i],                                     print_progress,0,indent+1); }
      for(int i = 0; i < indent; i++) printf("  "); printf(")"); if(g->tag) printf(">"); if(selected) printf("*"); printf("\n");
      break;
    case GROUP_TYPE_CHARS:
      for(int i = 0; i < indent; i++) printf("  "); if(selected) printf("*"); if(g->tag) { printf("<"); print_utag(g->tag); } printf("\"%s\"",g->chars); if(g->tag) printf(">"); if(selected) printf("*"); printf("\n");
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
  }
}

void resume_from_file(group *g)
{
  FILE *fp;
  fp = fopen(resume_file, "r");
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
  }
}

