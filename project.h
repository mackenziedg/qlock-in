#include <sqlite3.h>

int deactivate_projects(sqlite3 *mdb);
int create_project(sqlite3 *db, sqlite3 *mdb, char* name);
char *get_active_project_name(sqlite3 *mdb);
int create_master_db(sqlite3 **mdb, char *mdb_path);
