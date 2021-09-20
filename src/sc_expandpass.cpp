#ifndef SC_EXPANDPASS_H
#define SC_EXPANDPASS_H

struct sc_expandpass_ini
{
  int state;
  const char *seed_file;
};

void sc_expandpass_init(sc_expandpass_ini ini);
int sc_expandpass_keyspace();
void sc_expandpass_seek(int state);
void sc_expandpass_next();
void sc_expandpass_shutdown();

#endif //SC_EXPANDPASS_H

