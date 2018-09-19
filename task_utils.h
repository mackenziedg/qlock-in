#include <sqlite3.h>

void cleanup(int e, sqlite3_stmt *stmt, sqlite3 *db);
int get_num_timestamps(sqlite3 *db, int id);
int task_is_open(sqlite3 *db, int id);
int task_exists(sqlite3 *db, int id);
int get_max_id(sqlite3 *db);
int print_open_tasks(sqlite3 *db);
