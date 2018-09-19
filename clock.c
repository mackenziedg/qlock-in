#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include <time.h>

#include "task_utils.h"

#define MAX_TASK_NAME_SZ 64
#define MAX_TASK_DESC_SZ 256

// create_task() creates a new task with a generated id, no start or end time,
// and a user-set name and description
int create_task(sqlite3 *db){
    char name[MAX_TASK_NAME_SZ];
    char desc[MAX_TASK_DESC_SZ];
    char *zErrMsg;
    sqlite3_stmt *stmt;
    char *statement = "INSERT INTO task_info (id, name, description) VALUES (@id, @name, @desc);";
    int e, i, l, id;

    id = get_max_id(db) + 1;
    printf("Enter a name for the task: ");
    fgets(name, MAX_TASK_NAME_SZ, stdin);
    sscanf(name, "%[^\n]s", name);
    printf("Enter a description for the task: ");
    fgets(desc, MAX_TASK_DESC_SZ, stdin);
    sscanf(desc, "%[^\n]s", desc);

    e = sqlite3_prepare_v2(db, statement, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return e;
    }
    i = sqlite3_bind_parameter_index(stmt, "@id");
    e = sqlite3_bind_int(stmt, i, id);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return e;
    }
    i = sqlite3_bind_parameter_index(stmt, "@name");
    l = sizeof(name)/sizeof(char);
    e = sqlite3_bind_text(stmt, i, name, l, SQLITE_TRANSIENT);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return e;
    }
    i = sqlite3_bind_parameter_index(stmt, "@desc");
    l = sizeof(desc)/sizeof(char);
    e = sqlite3_bind_text(stmt, i, desc, l, SQLITE_TRANSIENT);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        printf("Creating new task.\n");
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, db);
        return e;
    }
    sqlite3_finalize(stmt);
    printf("Created task #%d.\n", id);
    return 0;
}

// stamp_task() adds a new timestamp for task #id into the task_ts table
int stamp_task(sqlite3 *db, int id){
    char *statement = "INSERT INTO task_ts (id, timestamp) VALUES (@id, @ts);";
    sqlite3_stmt *stmt;
    int e, i;

    e = sqlite3_prepare_v2(db, statement, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return e;
    }
    i = sqlite3_bind_parameter_index(stmt, "@id");
    e = sqlite3_bind_int(stmt, i, id);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return e;
    }
    i = sqlite3_bind_parameter_index(stmt, "@ts");
    e = sqlite3_bind_int(stmt, i, time(NULL));
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, db);
        return e;
    }
    sqlite3_finalize(stmt);
    return 0;
}

// start_task() starts a specified task. It will throw an error if the task is
// currently active or does not exist.
int start_task(sqlite3 *db, int id){
    int e;

    if (task_exists(db, id) != 1){
        fprintf(stderr, "Could not start task #%d as it does not exist.\n", id);
        return 1;
    }
    if (task_is_open(db, id) != 0){
        fprintf(stderr, "Could not start task #%d as it is currently active.\n", id);
        return 1;
    }

    e = stamp_task(db, id);
    if (e == SQLITE_OK){
        printf("Started task #%d.\n", id);
    }
    return e;
}

// end_task() ends a specified task. It will throw an error if the task is not
// currently active or does not exist.
int end_task(sqlite3 *db, int id){
    int e;

    if (task_exists(db, id) != 1){
        fprintf(stderr, "Could not end task #%d as it does not exist.\n", id);
        return 1;
    }
    if (task_is_open(db, id) != 1){
        fprintf(stderr, "Could not end task #%d as it is not currently active.\n", id);
        return 1;
    }

    e = stamp_task(db, id);
    if (e == SQLITE_OK){
        printf("Ended task #%d.\n", id);
    }
    return e;
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
        cleanup(e, stmt, db);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        printf("Creating task info table.\n");
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, db);
        return e;
    }
    sqlite3_finalize(stmt);

    e = sqlite3_prepare_v2(db, create_ts_table, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        printf("Creating task timestamps table.\n");
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, db);
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
            } else if (strcmp(argv[1], "--open")==0|strcmp(argv[1], "-o")==0){
                int *o;
                int n;
                n = get_open_tasks(db, &o);
                for (int i = 0; i < n; i++){
                    printf("%d\n", o[i]);
                }
                free(o);
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
            } else if (strcmp(argv[1], "--elapsed")==0|strcmp(argv[1], "-e")==0){
                int t = get_elapsed_time(db, id);
                printf("%d\n", t);
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
        sqlite3_close(db);
        return e;
    }

    handle_input(db, argc, argv);

    sqlite3_close(db);
    return 0;
}
