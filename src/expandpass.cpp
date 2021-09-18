#include "util.h"
#include "expansion.h"
#include "parse.h"
#include "expand.h"

void print_seed(group *g, int print_progress, int selected, int indent);
void checkpoint_group(group *g, FILE *fp);
void resume_group(group *g, FILE *fp);

int main(int argc, char **argv)
{
  int i = 1;

  const char *seed_file = "seed.txt";
  int seed_specified = 0;
  char *password_file = 0;
  int resume = 0;
  const char *resume_file = "seed.progress";
  int checkpoint = 0;
  const char *checkpoint_file = "seed.progress";
  char checkpoint_file_bak[max_sprintf_len];
  checkpoint_file_bak[0] = '\0';
  int normalize = 0;

  int estimate      = 0;
  int estimate_rate = 600000;
  int unroll        = 1000;
  int unquoted      = 0;

  int buff_len                    = 1024*1024; //1MB
  int validate_alpha_e            = 0;
  int validate_up_alpha_e         = 0;
  int validate_low_alpha_e        = 0;
  int validate_numeric_e          = 0;
  int validate_alphanumeric_e     = 0;
  int validate_non_alphanumeric_e = 0;
  int validate_length_min         = 0;
  int validate_length_max         = max_pass_len;

  while(i < argc)
  {
    if(strcmp(argv[i],"--help") == 0)
    {
      fprintf(stdout,"usage: expandpass [--help] [--estimate [@#]] [-i input_seed.txt] [-o output_passwords.txt] [--normalize] [--unroll #] [-b #] [-f[aA|A|a|#|aA#|@|l] #] [-c # [checkpoint_seed.progress]] [-r [recovery_seed.progress]]\n");
      fprintf(stdout,"--help shows this menu\n");
      fprintf(stdout,"--version displays version (%d.%d)\n",version_maj,version_min);
      fprintf(stdout,"--estimate shows a (crude) estimation of # of likely generatable passwords\n");
      fprintf(stdout,"--normalize prints normalized/optimized seed (as used in actual gen)\n");
      fprintf(stdout,"--unquoted treats otherwise invalid characters as single-character strings\n");
      fprintf(stdout,"--unroll specifies cutoff group size, below which will be optimized (default 1000; 0 == no unrolling)\n");
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
  safe_sprintf(checkpoint_file_bak,"%s.bak",checkpoint_file);

  FILE *fp;
  char *buff    = (char *)safe_malloc(sizeof(char)*buff_len);
  char *devnull = (char *)safe_malloc(sizeof(char)*buff_len);

  char *passholder = (char *)safe_malloc(sizeof(char)*max_pass_len);
  char *passholder_p = passholder;
  char *lockholder = (char *)safe_malloc(sizeof(char)*max_pass_len);
  memset(lockholder,0,sizeof(char)*max_pass_len);

  int buff_i = 0;

  #ifdef _WIN32
  if(0) ;
  #else
  if(!seed_specified && !isatty(fileno(stdin))) fp = stdin;
  #endif
  else
  {
    fp = safe_fopen(seed_file, "r");
    if(!fp) { fprintf(stderr,"Error opening seed file: %s\n",seed_file); exit(1); }
  }
  group *g = parse(fp, unquoted);
  #ifdef _WIN32
  if(0) ;
  #else
  if(!seed_specified && !isatty(fileno(stdin))) ; //nothing needs closing
  #endif
  else
  {
    fclose(fp);
    fp = 0;
  }

  collapse_group(g,g,0);
  elevate_tags(g,0,0);
  preprocess_group(g);

  if(unroll)
  {
    group *og = g;
    g = unroll_group(og, unroll, devnull);
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
    unsigned long long int a = estimate_group(g);
    unsigned long long int d;
    unsigned long long int h;
    unsigned long long int m;
    unsigned long long int s;
    fprintf(stdout,"estimated output for seed file (%s): %llu @ %d/s\n",seed_file,a,estimate_rate);
    s = a/estimate_rate;
    m = s/60; //minute
    h = m/60; //hour
    d = h/24; //day
    if(d) { fprintf(stdout,"%llud ",d); h -= d*24; m -= d*24*60; s -= d*24*60*60; }
    if(h) { fprintf(stdout,"%lluh ",h);            m -= h*60;    s -= h*60*60; }
    if(m) { fprintf(stdout,"%llum ",m);                          s -= m*60; }
    if(s) { fprintf(stdout,"%llus ",s); ; }
    if(d+h+m+s <= 0) { fprintf(stdout,"0s "); }
    fprintf(stdout,"\n");
    exit(0);
  }

  if(resume)
  {
    FILE *fp;
    fp = safe_fopen(resume_file, "r");
    if(!fp) { fprintf(stderr,"Error opening progress file: %s\n",resume_file); exit(1); }
    resume_group(g,fp);
    fclose(fp);
  }

  if(password_file)
  {
    if(resume) fp = safe_fopen(password_file, "a");
    else       fp = safe_fopen(password_file, "w");
    if(!fp) { fprintf(stderr,"Error opening output file: %s\n",password_file); exit(1); }
  }
  else fp = stdout;

  int done = 0;
  long long int a = 0;
  tag tag_u = 0;
  tag tag_g = 0;
  tag inv_tag_g = 0;
  int utag_dirty = g->zerod_sum_tag_u;
  int gtag_dirty = g->zerod_sum_tag_g;
  while(!done)
  {
    int preskip = 0;
    //print_seed(g,1,1,0);
    if(utag_dirty || gtag_dirty)
    {
      if(!tags_coherent_with_children(g,clone_tag(0,&tag_u),clone_tag(0,&tag_g),clone_tag(0,&inv_tag_g))) if(advance_group(g,0,0,0)) break; //advance_group returning 1 means done
      utag_dirty = 0;
      gtag_dirty = 0;
    }
    done = !sprint_group(g, 0, &utag_dirty, &gtag_dirty, lockholder, &passholder_p, devnull);

    a++;
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
      )
      { //append password
        int pass_i = 0;
        while(passholder[pass_i] != '\0')
        {
          buff[buff_i] = passholder[pass_i];
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
    }
    passholder_p = passholder;
    if(checkpoint && a >= checkpoint)
    {
      FILE *fp;
      fp = safe_fopen(checkpoint_file_bak, "w");
      if(!fp) { fprintf(stderr,"Error opening checkpoint bak file: %s\n",password_file); exit(1); }
      checkpoint_group(g,fp);
      fclose(fp);

      fp = safe_fopen(checkpoint_file, "w");
      if(!fp) { fprintf(stderr,"Error opening checkpoint file: %s\n",password_file); exit(1); }
      checkpoint_group(g,fp);
      fclose(fp);

      a = 0;
    }
  }

  fwrite(buff,sizeof(char),buff_i,fp);
  fclose(fp);
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

void resume_group(group *g, FILE *fp)
{
  safe_fscanf(fp,"%d\n",&g->i);
  safe_fscanf(fp,"%d\n",&g->mod_i);
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
      safe_fscanf(fp,"%d\n",&m->injection_i[j]);
      safe_fscanf(fp,"%d\n",&m->injection_sub_i[j]);
    }
    for(int j = 0; j < m->n_smart_substitutions; j++)
    {
      safe_fscanf(fp,"%d\n",&m->smart_substitution_i[j]);
      safe_fscanf(fp,"%c\n",&m->smart_substitution_i_c[j]);
      safe_fscanf(fp,"%d\n",&m->smart_substitution_sub_i[j]);
    }
    for(int j = 0; j < m->n_substitutions; j++)
    {
      safe_fscanf(fp,"%d\n",&m->substitution_i[j]);
      safe_fscanf(fp,"%d\n",&m->substitution_sub_i[j]);
    }
    for(int j = 0; j < m->n_deletions; j++)
    {
      safe_fscanf(fp,"%d\n",&m->deletion_i[j]);
    }
  }
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

