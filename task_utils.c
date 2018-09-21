#include <stdlib.h>
#include <stdio.h>
#include <sqlite3.h>
#include <time.h>

#ifndef TASK_UTILS_H
#define TASK_UTILS_H
#include "task_utils.h"
#endif

// cleanup() frees the current statement and db and prints error messages in
// the case of an error
void cleanup(int e, sqlite3_stmt *stmt, sqlite3 *db){
    fprintf(stderr, "SQL error: Error code %d -- %s\n", e, sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// get_num_timestamps() returns the number of timestamps for a given task id.
// This is primarily used to determine if a task is currently open or not.
int get_num_timestamps(sqlite3 *db, int id){
    sqlite3_stmt *stmt;
    int e, i;
    int n = 0;
    
    char *statement = "SELECT COUNT(*) FROM task_ts WHERE id=@id;";
    e = sqlite3_prepare_v2(db, statement, -1, &stmt, NULL);
    i = sqlite3_bind_parameter_index(stmt, "@id");
    sqlite3_bind_int(stmt, i, id);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return -1;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        n = sqlite3_column_int(stmt, 0);
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, db);
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
    sqlite3_stmt *stmt;
    int e, i;
    int n = 0;

    char *statement = "SELECT COUNT(*) FROM task_info WHERE id=@id;";
    e = sqlite3_prepare_v2(db, statement, -1, &stmt, NULL);
    i = sqlite3_bind_parameter_index(stmt, "@id");
    sqlite3_bind_int(stmt, i, id);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return -1;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        n = sqlite3_column_int(stmt, 0);
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, db);
        return -1;
    }
    sqlite3_finalize(stmt);
    return n;
}

// get_max_id() returns the max task id in task_info.
int get_max_id(sqlite3 *db){
    char *statement = "SELECT MAX(id) FROM task_info;";
    sqlite3_stmt *stmt;
    int e;
    int n = -1;

    e = sqlite3_prepare_v2(db, statement, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        n = sqlite3_column_int(stmt, 0);
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, db);
        return -1;
    }
    sqlite3_finalize(stmt);
    return n;
}

// get_open_tasks() builds an array of the currently open tasks and returns the
// length of the array
int get_open_tasks(sqlite3 *db, int **o){
    int n = 0;
    int i = 0;

    for (int id = 0; id <= get_max_id(db); id++){
        if (task_is_open(db, id)){
            n++;
        }
    }
    *o = malloc(sizeof(int)*n);
    for (int id = 0; id <= get_max_id(db); id++){
        if (task_is_open(db, id)){
            (*o)[i] = id;
            i++;
        }
    }

    return n;
}

// get_elapsed_time() returns the current number of seconds the selected task has
// been active, or -1 if there is an error
int get_elapsed_time(sqlite3 *db, int id){
    char *statement = "SELECT * FROM task_ts WHERE id=@id;";
    sqlite3_stmt *stmt;
    int e, i;
    int t = 0;
    int c = -1; // We flip the sign of c every row to find the difference between starting and ending times

    if (!task_exists(db, id)){
        fprintf(stderr, "Task #%d does not exist.\n", id);
        return -1;
    }
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
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        t += c*sqlite3_column_int(stmt, 1);
        c *= -1;
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, db);
        return -1;
    }
    sqlite3_finalize(stmt);

    if (task_is_open(db, id) != 0){
        t += time(NULL);
    }

    return t;
}
