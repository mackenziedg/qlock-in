// FIXME: The SQL statements in this are all trivially open to SQL-injection attacks
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include <time.h>

#define MAX_TASK_NAME_SZ 64
#define MAX_TASK_DESC_SZ 256
#define MAX_STATEMENT_SIZE 512

// get_num_timestamps() returns the number of timestamps for a given task id.
// This is primarily used to determine if a task is currently open or not.
int get_num_timestamps(sqlite3 *db, int id){
    char statement[MAX_STATEMENT_SIZE]; 
    sqlite3_stmt *stmt;
    int e, n;

    sprintf(statement, "SELECT COUNT(*) FROM task_ts WHERE id=%d;", id);

    e = sqlite3_prepare_v2(db, statement, -1, &stmt, NULL);
    if (e != SQLITE_OK){
      fprintf(stderr, "SQL error: Error code %d\n", e);
      fprintf(stderr, "SQL error: Returned %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return -1;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        n = sqlite3_column_int(stmt, 0);
    }
    if (e != SQLITE_DONE){
      fprintf(stderr, "SQL error: Error code %d\n", e);
      fprintf(stderr, "SQL error: Returned %d\n", e);
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return -1;
    }
    sqlite3_finalize(stmt);
    return n;
}

// task_is_open() returns 1 if the task is currently open
int task_is_open(sqlite3 *db, int id){
    return (get_num_timestamps(db, id) % 2 != 0);
}

// task_exists() returns 1 if a given task exists
int task_exists(sqlite3 *db, int id){
    char statement[MAX_STATEMENT_SIZE]; 
    sqlite3_stmt *stmt;
    int e, n;

    sprintf(statement, "SELECT COUNT(*) FROM task_info WHERE id=%d;", id);

    e = sqlite3_prepare_v2(db, statement, -1, &stmt, NULL);
    if (e != SQLITE_OK){
      fprintf(stderr, "SQL error: Error code %d\n", e);
      fprintf(stderr, "SQL error: Returned %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return -1;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        n = sqlite3_column_int(stmt, 0);
    }
    if (e != SQLITE_DONE){
      fprintf(stderr, "SQL error: Error code %d\n", e);
      fprintf(stderr, "SQL error: Returned %d\n", e);
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return -1;
    }
    sqlite3_finalize(stmt);
    return n;
}

// get_max_id() returns the max task id in task_info.
int get_max_id(sqlite3 *db){
    char *statement = "SELECT MAX(id) FROM task_info;";
    sqlite3_stmt *stmt;
    int e, n;

    e = sqlite3_prepare_v2(db, statement, -1, &stmt, NULL);
    if (e != SQLITE_OK){
      fprintf(stderr, "SQL error: Error code %d\n", e);
      fprintf(stderr, "SQL error: Returned %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        n = sqlite3_column_int(stmt, 0);
    }
    if (e != SQLITE_DONE){
      fprintf(stderr, "SQL error: Error code %d\n", e);
      fprintf(stderr, "SQL error: Returned %d\n", e);
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return -1;
    }
    sqlite3_finalize(stmt);
    return n;
}

// create_task() creates a new task with a generated id, no start or end time,
// and a user-set name and description
int create_task(sqlite3 *db){
    char name[MAX_TASK_NAME_SZ];
    char desc[MAX_TASK_DESC_SZ];
    char *zErrMsg;
    char statement[MAX_STATEMENT_SIZE];
    sqlite3_stmt *stmt;
    int e, id;

    id = get_max_id(db) + 1;
    printf("Enter a name for the task: ");
    fgets(name, MAX_TASK_NAME_SZ, stdin);
    sscanf(name, "%[^\n]s", name);
    printf("Enter a description for the task: ");
    fgets(desc, MAX_TASK_DESC_SZ, stdin);
    sscanf(desc, "%[^\n]s", desc);

    sprintf(statement, "INSERT INTO task_info (id, name, description) VALUES (%d, '%s', '%s');", id, name, desc);

    e = sqlite3_prepare_v2(db, statement, -1, &stmt, NULL);
    if (e != SQLITE_OK){
      fprintf(stderr, "SQL error: Error code %d\n", e);
      fprintf(stderr, "SQL error: Returned %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        printf("Creating new task.\n");
    }
    if (e != SQLITE_DONE){
      fprintf(stderr, "SQL error: Error code %d\n", e);
      fprintf(stderr, "SQL error: Returned %d\n", e);
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return e;
    }
    sqlite3_finalize(stmt);
    printf("Created task #%d.\n", id);
    return 0;
}

// start_task() starts a specified task. It will throw an error if the task is
// currently active.
int start_task(sqlite3 *db, int id){
    char statement[MAX_STATEMENT_SIZE];
    sqlite3_stmt *stmt;
    int e;

    if (!task_exists(db, id)){
        fprintf(stderr, "Could not start task #%d as it does not exist.\n", id);
        return 1;
    }
    if (task_is_open(db, id)){
        fprintf(stderr, "Could not start task #%d as it is currently active.\n", id);
        return 1;
    }

    sprintf(statement, "INSERT INTO task_ts (id, timestamp) VALUES (%d, %d)", id, time(NULL));
    e = sqlite3_prepare_v2(db, statement, -1, &stmt, NULL);
    if (e != SQLITE_OK){
      fprintf(stderr, "SQL error: Error code %d\n", e);
      fprintf(stderr, "SQL error: Returned %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
    }
    if (e != SQLITE_DONE){
      fprintf(stderr, "SQL error: Error code %d\n", e);
      fprintf(stderr, "SQL error: Returned %d\n", e);
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return e;
    }
    sqlite3_finalize(stmt);
    printf("Started task #%d.\n", id);
    return 0;
}

// end_task() ends a specified task. It will throw an error if the task is not
// currently active.
int end_task(sqlite3 *db, int id){
    char statement[MAX_STATEMENT_SIZE];
    sqlite3_stmt *stmt;
    int e;

    if (!task_exists(db, id)){
        fprintf(stderr, "Could not end task #%d as it does not exist.\n", id);
        return 1;
    }
    if (!task_is_open(db, id)){
        fprintf(stderr, "Could not end task #%d as it is not currently active.\n", id);
        return 1;
    }

    sprintf(statement, "INSERT INTO task_ts (id, timestamp) VALUES (%d, %d)", id, time(NULL));
    e = sqlite3_prepare_v2(db, statement, -1, &stmt, NULL);
    if (e != SQLITE_OK){
      fprintf(stderr, "SQL error: Error code %d\n", e);
      fprintf(stderr, "SQL error: Returned %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
    }
    if (e != SQLITE_DONE){
      fprintf(stderr, "SQL error: Error code %d\n", e);
      fprintf(stderr, "SQL error: Returned %d\n", e);
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return e;
    }
    sqlite3_finalize(stmt);
    printf("Ended task #%d.\n", id);
    return 0;
}

// Create a new db at dbpath if one does not exist already. Populate the db
// with the necessary tables (task_info, task_ts)
int setup_db(sqlite3 *db){
    char *create_info_table = "CREATE TABLE IF NOT EXISTS task_info (id INTEGER PRIMARY KEY, name TEXT NOT NULL, description TEXT);";
    char *create_ts_table = "CREATE TABLE IF NOT EXISTS task_ts (id INTEGER NOT NULL, timestamp INTEGER NOT NULL, FOREIGN KEY(id) REFERENCES task_info(id));";
    sqlite3_stmt *stmt;
    int e;

    e = sqlite3_prepare_v2(db, create_info_table, -1, &stmt, NULL);
    if (e != SQLITE_OK){
      fprintf(stderr, "SQL error: Returned %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        printf("Creating task info table.\n");
    }
    if (e != SQLITE_DONE){
      fprintf(stderr, "SQL error: Returned %d\n", e);
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return e;
    }
    sqlite3_finalize(stmt);

    e = sqlite3_prepare_v2(db, create_ts_table, -1, &stmt, NULL);
    if (e != SQLITE_OK){
      fprintf(stderr, "SQL error: Returned %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        printf("Creating task timestamps table.\n");
    }
    if (e != SQLITE_DONE){
      fprintf(stderr, "SQL error: Returned %d\n", e);
      sqlite3_finalize(stmt);
      sqlite3_close(db);
      return e;
    }
    sqlite3_finalize(stmt);
    return 0;
}

// handle_input() proccesses the command-line input and passes it to the
// correct function.
int handle_input(sqlite3 *db, int argc, char **argv){
    int id;

    switch (argc) {
        case 2:
            if (strcmp(argv[1], "new") == 0){
                create_task(db);
            } else {
                fprintf(stderr, "Input 'clock %s' not correctly formatted.\n", argv[1]);
            }
            break;
        case 3:
            id = atoi(argv[2]);
            if (strcmp(argv[1], "in") == 0){
                start_task(db, id);
            } else if (strcmp(argv[1], "out") == 0){
                end_task(db, id);
            }
            break;
    }
}

int main(int argc, char **argv){
    sqlite3 *db;
    char *dbpath = "./clock.db";
    int e, id;

    if (argc < 2){
        fprintf(stderr, "Must pass at least one parameter.\n");
        return 1;
    }

    if ((e = sqlite3_open(dbpath, &db)) != SQLITE_OK){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return e;
    }
    if ((e = setup_db(db)) != 0){
        return e;
    }

    handle_input(db, argc, argv);

    sqlite3_close(db);
    return 0;
}
