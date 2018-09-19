#include <sqlite3.h>

int create_task(sqlite3 *db);
int stamp_task(sqlite3 *db, int id);
int start_task(sqlite3 *db, int id);
int end_task(sqlite3 *db, int id);
