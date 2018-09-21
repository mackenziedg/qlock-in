#include <sqlite3.h>

typedef enum {TASK_OK, TASK_NOT_EXIST, TASK_WRONG_STATE} TASK_STATE;

int create_task(sqlite3 *db, char *name, char *desc);
int stamp_task(sqlite3 *db, int id);
int start_task(sqlite3 *db, int id);
int end_task(sqlite3 *db, int id);
