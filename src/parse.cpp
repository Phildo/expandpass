#include "parse.h"
#include "util.h"


int parse_child(FILE *fp, int unquoted, int *line_n, char *buff, char **b, group *g, group *prev_g, int depth, parse_error *er)
{
  g->countable = 1; //until proven otherwise
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
        er->error = ERROR_EOF;
        safe_sprintf(er->txt,"ERROR: EOF\nline %d\n",*line_n);
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
      g->chars = (char *)safe_malloc(sizeof(char)*g->n+1);
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
      g->chars = (char *)safe_malloc(sizeof(char)*g->n+1);
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
      er->error = ERROR_INVALID_LINE;
      if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
      safe_sprintf(er->txt,"ERROR: Invalid line\nline %d ( %s )\n",*line_n,buff);
      return 0;
    }

    if(g->type != GROUP_TYPE_CHARS)
    {
      while(*s == ' ' || *s == '\t') s++;
    }

    char *c;
    if(g->type == GROUP_TYPE_OPTION) g->countable = 0; //TODO: this limitation renders "countability" a farce
    switch(g->type)
    {
      case GROUP_TYPE_SEQUENCE:
      case GROUP_TYPE_OPTION:
      case GROUP_TYPE_PERMUTE:
        g->n = 0;
        *b = s;
        if(g->type == GROUP_TYPE_SEQUENCE)
        {
          if(!parse_tag(fp, line_n, buff, b, &g->tag_u, 1, er)) return 0;
          if(!parse_tag(fp, line_n, buff, b, &g->tag_g, 0, er)) return 0;
          if(g->tag_u) g->countable = 0;
          if(g->tag_g) g->countable = 0;
        }
        return parse_childs(fp, unquoted, line_n, buff, b, g, depth+1, er);
        break;
      case GROUP_TYPE_CHARS:
        c = s;
        while(*c != '"' && *c != '\n' && *c != '\0') { if(*c == '\\') c++; c++; }
        if(*c == '"')
        {
          g->n = (c-s);
          g->chars = (char *)safe_malloc(sizeof(char)*g->n+1);
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
            er->error = ERROR_UNTERMINATED_STRING;
            if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
            safe_sprintf(er->txt,"ERROR: Unterminated string\nline %d ( %s )\n",*line_n,buff);
            return 0;
          }
          s++;
          *c = '\0';
        }
        else
        {
          er->error = ERROR_INVALID_STRING;
          if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
          safe_sprintf(er->txt,"ERROR: Invalid characters after begining string\nline %d ( %s )\n",*line_n,buff);
          return 0;
        }
        break;
      case GROUP_TYPE_MODIFICATION: //special case
        if(!prev_g)
        {
          er->error = ERROR_UNPARENTED_MODIFICATION;
          if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
          safe_sprintf(er->txt,"ERROR: Modification with no previous group\nline %d ( %s )\n",*line_n,buff);
          return 0;
        }
        *b = s;
        return parse_modifications(fp, line_n, buff, b, prev_g, er);
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

int parse_tag(FILE *fp, int *line_n, char *buff, char **b, tag *t, int u, parse_error *er)
{
  assert(!*t);
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
        er->error = ERROR_BADEOF;
        safe_sprintf(er->txt,"ERROR: EOF parsing tag\nline %d\n",*line_n);
        return 0;
      }
      s = buff;
    }
    else if(*s == ' ' || *s == '\t') s++;
    else break;
  }

  int parsed = 0;
  int d;
  if((u && *s == 'U') || (!u && *s == 'G'))
  {
    s++;
    while((d = parse_number(s,&parsed)) != -1)
    {
      if(parsed < 1 || parsed >= max_tag_count)
      {
        er->error = ERROR_TAG_RANGE;
        if(u) safe_sprintf(er->txt,"ERROR: unique tag outside of valid range [1-%d]\nline %d\n",max_tag_count-1,*line_n);
        else  safe_sprintf(er->txt,"ERROR: group tag outside of valid range [1-%d]\nline %d\n",max_tag_count-1,*line_n);
        return 0;
      }
      s += d;
      *t |= 1 << (parsed-1);
      if(*s == ',') s++;
    }
    if(!*t)
    {
      er->error = ERROR_TAG_SPECIFY;
      if(u) safe_sprintf(er->txt,"ERROR: unique tag not specified\nline %d\n",*line_n);
      else  safe_sprintf(er->txt,"ERROR: group tag not specified\nline %d\n",*line_n);
      return 0;
    }
  }
  *b = s;
  return 1;
}

int parse_modification(FILE *fp, int *line_n, char *buff, char **b, modification *m, parse_error *er)
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
        er->error = ERROR_BADEOF;
        safe_sprintf(er->txt,"ERROR: EOF parsing modification\nline %d\n",*line_n);
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
        er->error = ERROR_INVALID_NULL_MODIFICATION;
        if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
        safe_sprintf(er->txt,"ERROR: Invalid null modification\nline %d ( %s )\n",*line_n,buff);
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
        er->error = ERROR_INVALID_MODIFICATION;
        if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
        safe_sprintf(er->txt,"ERROR: Invalid line in modification\nline %d ( %s )\n",*line_n,buff);
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
          m->chars = (char *)safe_malloc(sizeof(char)*m->n+1);
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
            er->error = ERROR_UNTERMINATED_STRING;
            if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
            safe_sprintf(er->txt,"ERROR: Unterminated modification gamut\nline %d ( %s )\n",*line_n,buff);
            return 0;
          }
          s++;
          *c = '\0';
        }
        else
        {
          er->error = ERROR_INVALID_STRING;
          if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
          safe_sprintf(er->txt,"ERROR: Invalid characters after begining gamut\nline %d ( %s )\n",*line_n,buff);
          return 0;
        }
      }
    }
    if(m->n_substitutions+m->n_injections > 0 && !m->n)
    {
      er->error = ERROR_MODIFICATION_EMPTY_GAMUT;
      if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
      safe_sprintf(er->txt,"ERROR: gamut required with substitution or injection\nline %d ( %s )\n",*line_n,buff);
      return 0;
    }
  }
  *b = s;
  return 1;
}

int parse_modifications(FILE *fp, int *line_n, char *buff, char **b, group *g, parse_error *er)
{
  char *s;
  int valid_modification = 1;
  modification *m = (modification *)safe_malloc(sizeof(modification));
  while(valid_modification)
  {
    zero_modification(m);

    valid_modification = parse_modification(fp,line_n,buff,b,m,er);

    if(valid_modification)
    {
      if(m->n_smart_substitutions) g->countable = 0;
      g->n_mods++;
      if(g->n_mods == 1) g->mods = (modification *)safe_malloc(sizeof(modification));
      else               g->mods = (modification *)safe_realloc(g->mods,sizeof(modification)*g->n_mods);
      g->mods[g->n_mods-1] = *m;
    }
  }
  free(m);

  if(er->error == ERROR_BADEOF)               return 0; //close everything
  if(er->error != ERROR_INVALID_MODIFICATION) return 0; //let "invalid modification" attempt parse as "end modification"
  if(er->force)                               return 0; //if error force, don't allow passthrough ("unexpected parse" should only bubble up one level)

  s = *b;
  if(*s != ']')
  {
    //er->error; //retain error from stack
    int n_chars = 0; while(buff[n_chars] != '\0') n_chars++;
    if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
    safe_sprintf(er->txt,"ERROR: Invalid characters within modifiers\nline %d ( %s )\n",*line_n,buff);
    er->force = 1;
    return 0;
  }
  s++;
  *b = s;
  return 1;
}

int parse_childs(FILE *fp, int unquoted, int *line_n, char *buff, char **b, group *g, int depth, parse_error *er)
{
  char *s;
  int valid_kid = 1;
  group *prev_c = 0;
  group *c = (group *)safe_malloc(sizeof(group));
  while(valid_kid)
  {
    zero_group(c);
    valid_kid = parse_child(fp, unquoted, line_n, buff, b, c, prev_c, depth+1, er);

    if(valid_kid)
    {
      if(c->type == GROUP_TYPE_MODIFICATION)
      {
        //ok
      }
      else if(c->type == GROUP_TYPE_NULL)
      {
        er->error = ERROR_NULL_CHILD;
        int n_chars = 0; while(buff[n_chars] != '\0') n_chars++;
        if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
        safe_sprintf(er->txt,"Null child type found\nline %d ( %s )\n",*line_n,buff);
      }
      else
      {
        g->n++;
        if(g->n == 1) g->childs = (group *)safe_malloc(sizeof(group));
        else          g->childs = (group *)safe_realloc(g->childs,sizeof(group)*g->n);
        g->childs[g->n-1] = *c;
        prev_c = &g->childs[g->n-1];
      }
    }
  }
  free(c);

  if(er->error == ERROR_EOF)
  {
    if(depth == 0) return 1; //close everything
    else
    {
      er->error = ERROR_BADEOF; //upgrade error
      switch(g->type)
      {
        case GROUP_TYPE_SEQUENCE: safe_sprintf(er->txt,"ERROR: EOF unclosed sequence\nline %d\n",*line_n); break;
        case GROUP_TYPE_OPTION:   safe_sprintf(er->txt,"ERROR: EOF unclosed option\nline %d\n",*line_n); break;
        case GROUP_TYPE_PERMUTE:  safe_sprintf(er->txt,"ERROR: EOF unclosed permute\nline %d\n",*line_n); break;
        default: break; //appease compiler
      }
      return 0;
    }
  }
  if(er->error != ERROR_INVALID_LINE) return 0; //let "invalid line" attempt parse as "end line"
  if(er->force)                       return 0; //if error force, don't allow passthrough ("unexpected parse" should only bubble up one level)

  s = *b;
       if(*s == '>' && g->type == GROUP_TYPE_SEQUENCE) s++; //great
  else if(*s == '}' && g->type == GROUP_TYPE_OPTION)   s++; //great
  else if(*s == ')' && g->type == GROUP_TYPE_PERMUTE)  s++; //great
  else
  {
    //er->error; //retain error from stack
    int n_chars = 0; while(buff[n_chars] != '\0') n_chars++;
    if(buff[n_chars-1] == '\n') n_chars--; buff[n_chars] = '\0';
    if(g->type == GROUP_TYPE_SEQUENCE)
    safe_sprintf(er->txt,"ERROR: Invalid characters within sequence\nline %d ( %s )\n",*line_n,buff);
    if(g->type == GROUP_TYPE_OPTION)
    safe_sprintf(er->txt,"ERROR: Invalid characters within option\nline %d ( %s )\n",*line_n,buff);
    if(g->type == GROUP_TYPE_PERMUTE)
    safe_sprintf(er->txt,"ERROR: Invalid characters within permute\nline %d ( %s )\n",*line_n,buff);
    er->force = 1;
    return 0;
  }

  *b = s;
  return 1;
}

group *parse(FILE *fp, int unquoted)
{
  char *buff;

  buff = (char *)safe_malloc(sizeof(char)*max_read_line_len);

  parse_error er;
  er.error = ERROR_NULL;
  er.force = 0;
  er.txt = (char *)safe_malloc(sizeof(char)*max_sprintf_len);
  int line_n = 0;

  group *g = (group *)safe_malloc(sizeof(group));
  zero_group(g);
  g->type = GROUP_TYPE_SEQUENCE;
  char *b = buff;
  *b = '\0';
  if(!parse_childs(fp, unquoted, &line_n, buff, &b, g, 0, &er))
  {
    fprintf(stderr, "%s", er.txt); exit(1);
  }
  while(g->n == 1 && g->n_mods == 0 && (g->type == GROUP_TYPE_SEQUENCE || g->type == GROUP_TYPE_OPTION || g->type == GROUP_TYPE_PERMUTE))
  {
    group *og = g->childs;
    *g = g->childs[0];
    free(og);
  }

  free(buff);
  free(er.txt);

  return g;
}

