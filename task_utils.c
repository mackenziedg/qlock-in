#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

// TODO: Have these also return the names of the tasks
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
    if (n > 0){
        *o = malloc(sizeof(int)*n);
        for (int id = 0; id <= get_max_id(db); id++){
            if (task_is_open(db, id)){
                (*o)[i] = id;
                i++;
            }
        }
    }

    return n;
}

// TODO: Have these also return the names of the tasks
// get_all_tasks() builds an array of all tasks and returns the length of the array
int get_all_tasks(sqlite3 *db, int **o){
    int n = 0;
    int i = 0;

    for (int id = 0; id <= get_max_id(db); id++){
        if (task_exists(db, id)){
            n++;
        }
    }
    if (n > 0){
        *o = malloc(sizeof(int)*n);
        for (int id = 0; id <= get_max_id(db); id++){
            if (task_exists(db, id)){
                (*o)[i] = id;
                i++;
            }
        }
    }

    return n;
}

// is_same_day() returns true if the two times occur on the same year, month, and day
int is_same_day(struct tm a, struct tm b){
    return ((a.tm_year == b.tm_year) &&
            (a.tm_mon  == b.tm_mon)  &&
            (a.tm_mday == b.tm_mday));
}

// print_elapsed_breakdown() breaksdown the tracked time by day
// TODO: Change this to return the values to be printed elsewhere
int print_elapsed_breakdown(sqlite3 *db, int id){
    char *statement = "SELECT * FROM task_ts WHERE id=@id;";
    sqlite3_stmt *stmt;
    int e, i;
    time_t rawtime;
    struct tm *ltime;
    int hr, min, sec;
    int j = 0;
    int elapsed = 0;
    int total_elapsed = 0;
    int num_ts = get_num_timestamps(db, id);
    if (num_ts <= 0){
        return -1;
    }
    struct tm dates[num_ts];
    int timestamps[num_ts];

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
        rawtime = sqlite3_column_int(stmt, 1);
        ltime = localtime(&rawtime);
        dates[j] = *ltime;
        timestamps[j] = rawtime;
        j++;
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, db);
        return -1;
    }
    sqlite3_finalize(stmt);
    // If we didn't read any lines in, return
    if (j == 0){
        cleanup(1, NULL, db);
        return -1;
    }

    // FIXME: This will not accurately count if a task starts one day and runs through to the next day (ie. working over midnight)
    int n = 0;
    for (j = 0; j < num_ts; j++){
        if ((is_same_day(dates[j+1], dates[j])) && (j != num_ts)){
            elapsed += timestamps[j+1]-timestamps[j];
            n++;
        } else{
            if (n%2==0){
                // Mod check seems backwards but it's because we skip incrementing n the last time
                elapsed += time(NULL) - timestamps[j];
            }
            hr = elapsed/3600;
            min = (elapsed-hr*3600)/60;
            sec = elapsed-(hr*3600+min*60);
            printf("%d-%d-%d: %02d:%02d:%02d\n", dates[j].tm_year+1900, dates[j].tm_mon+1, dates[j].tm_mday, hr, min, sec);
            total_elapsed += elapsed;
            elapsed = 0;
            n = 0;
        }
    }
    hr = total_elapsed/3600;
    min = (total_elapsed-hr*3600)/60;
    sec = total_elapsed-(hr*3600+min*60);
    printf("-----------\nTotal: %02d:%02d:%02d\n", hr, min, sec);

    return 0;
}
