#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>

#ifndef TASKS_H
#define TASKS_H
#include "tasks.h"
#endif

#ifndef TASK_UTILS_H
#define TASK_UTILS_H
#include "task_utils.h"
#endif

// create_task() creates a new task with a generated id, no start or end time,
// and a user-set name and description
int create_task(sqlite3 *db, char *name, char *desc){
    sqlite3_stmt *stmt;
    char *statement = "INSERT INTO task_info (id, name, description) VALUES (@id, @name, @desc);";
    int e, i, l, id;

    id = get_max_id(db) + 1;

    e = sqlite3_prepare_v2(db, statement, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return -1;
    }
    i = sqlite3_bind_parameter_index(stmt, "@id");
    e = sqlite3_bind_int(stmt, i, id);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return -1;
    }
    i = sqlite3_bind_parameter_index(stmt, "@name");
    l = strlen(name);
    e = sqlite3_bind_text(stmt, i, name, l, SQLITE_TRANSIENT);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return -1;
    }
    i = sqlite3_bind_parameter_index(stmt, "@desc");
    l = strlen(desc);
    e = sqlite3_bind_text(stmt, i, desc, l, SQLITE_TRANSIENT);
    if (e != SQLITE_OK){
        cleanup(e, stmt, db);
        return -1;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, db);
        return -1;
    }
    sqlite3_finalize(stmt);
    return id;
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
        return TASK_NOT_EXIST;
    }
    if (task_is_open(db, id) != 0){
        return TASK_WRONG_STATE;
    }

    e = stamp_task(db, id);
    return e;
}

// end_task() ends a specified task. It will throw an error if the task is not
// currently active or does not exist.
int end_task(sqlite3 *db, int id){
    int e;

    if (task_exists(db, id) != 1){
        return TASK_NOT_EXIST;
    }
    if (task_is_open(db, id) != 1){
        return TASK_WRONG_STATE;
    }

    e = stamp_task(db, id);
    return e;
}
