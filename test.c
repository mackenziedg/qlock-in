#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>

#include "tasks.h"
#include "task_utils.h"

struct test_results{
    int p;
    int n;
};

int clear_db(sqlite3 *tdb){
    sqlite3_stmt *stmt;
    int e;
    char *delete_info_table = "DELETE FROM task_info";
    char *delete_ts_table = "DELETE FROM task_ts";

    e = sqlite3_prepare_v2(tdb, delete_info_table, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, tdb);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, tdb);
        return e;
    }
    sqlite3_finalize(stmt);

    e = sqlite3_prepare_v2(tdb, delete_ts_table, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, tdb);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, tdb);
        return e;
    }
    sqlite3_finalize(stmt);

    return 0;
}

int eq(int a, int b){
    return (a == b);
}

int neq(int a, int b){
    return (a != b);
}

// test() is the general function for tests.
// sqlite3 *tdb -- Active database
// int (*cmp)(int, int) -- Pointer to a comparison function which compares a and b
// int a, b -- Values to be compared with cmp(a, b)
// struct test_results *tr -- Pointer to the active test results
// char *fmsg -- Message to print on failure (ie. what test is this?)
void test(sqlite3 *tdb, int (*cmp)(int, int), int a, int b, struct test_results *tr, char *fmsg){
    if((*cmp)(a, b)){
        fprintf(stderr, "\e[32m.");
        (*tr).p++;
    } else{
        fprintf(stderr, "\n\e[31mTest %d failed: %s. Should be %d, got %d\n", tr->n+1, fmsg, b, a);
    }
    (*tr).n++;
    fprintf(stderr, "\e[39m");
}

struct test_results test_tasksH(sqlite3 *tdb){
    struct test_results tr = {0, 0};

    // Test task creation
    test(tdb, neq, create_task(tdb, "task name", "task description"), -1, &tr, "Create a new task.");
    test(tdb, neq, create_task(tdb, "", ""), -1, &tr, "Create a task with empty name/desc.");
    test(tdb, neq, create_task(tdb, "\0", "\0"), -1, &tr, "Create a task with null name/desc");
    char *desc = calloc(1000000, 1);
    memset(desc, 'A', 70);
    test(tdb, neq, create_task(tdb, "Long description", desc), -1, &tr, "Create task with very long description"); // TODO: This test_eq passes but the description is not truncated. There's really no reason to actually truncate anything so probably should just remove that restriction anyway

    clear_db(tdb);
    // Test task starting/stopping
    create_task(tdb, "", "");
    test(tdb, eq, start_task(tdb, 1), TASK_OK, &tr, "Start the first task");
    test(tdb, eq, end_task(tdb, 1), TASK_OK, &tr, "Stop the first task");
    test(tdb, eq, start_task(tdb, -1), TASK_NOT_EXIST, &tr, "Start a nonexistant task");
    test(tdb, eq, end_task(tdb, -1), TASK_NOT_EXIST, &tr, "Stop a nonexistant task");
    test(tdb, eq, end_task(tdb, 1), TASK_WRONG_STATE, &tr, "Stop a task which has not been started");
    start_task(tdb, 1);
    test(tdb, eq, start_task(tdb, 1), TASK_WRONG_STATE, &tr, "Start a task which has already been started");

    clear_db(tdb);
    return tr;
}

struct test_results test_task_utilsH(sqlite3 *tdb){
    struct test_results tr = {0, 0};

    test(tdb, eq, get_max_id(tdb), 0, &tr, "Test if the max_id of an empty project is 0");
    create_task(tdb, "", "");
    test(tdb, eq, task_exists(tdb, 1), 1, &tr, "Test if a created task exists.");
    test(tdb, eq, task_exists(tdb, 2), 0, &tr, "Test if a nonexistant task exists.");
    test(tdb, eq, get_max_id(tdb), 1, &tr, "Test if the max_id is 1");
    test(tdb, eq, task_is_open(tdb, 1), 0, &tr, "Test if a closed task is open.");
    test(tdb, eq, get_num_timestamps(tdb, 1), 0, &tr, "Test if the number of timestamps of the task is 0");
    start_task(tdb, 1);
    test(tdb, eq, task_is_open(tdb, 1), 1, &tr, "Test if an open task is open.");
    test(tdb, eq, get_num_timestamps(tdb, 1), 1, &tr, "Test if the number of timestamps of the task is 1");
    end_task(tdb, 1);
    create_task(tdb, "", "");
    start_task(tdb, 2);
    end_task(tdb, 2);
    test(tdb, eq, get_elapsed_time(tdb, 2), 0, &tr, "Test if the elapsed time is 0");
    start_task(tdb, 1);
    start_task(tdb, 2);
    int *o;
    test(tdb, eq, get_open_tasks(tdb, &o), 2, &tr, "Test if two tasks are open");
    end_task(tdb, 1);
    end_task(tdb, 2);
    test(tdb, eq, get_open_tasks(tdb, &o), 0, &tr, "Test if no tasks are open");

    return tr;
}

int create_project(sqlite3 *db, char* name){
    char *dbpath;
    char *create_info_table = "CREATE TABLE IF NOT EXISTS task_info"
                              "(id INTEGER PRIMARY KEY,"
                              "name TEXT NOT NULL,"
                              "description TEXT);";
    char *create_ts_table = "CREATE TABLE IF NOT EXISTS task_ts"
                            "(id INTEGER NOT NULL,"
                            "timestamp INTEGER NOT NULL,"
                            "FOREIGN KEY(id) REFERENCES task_info(id));";
    sqlite3_stmt *stmt;
    int e;

    dbpath = malloc(sizeof(name)+3);
    sprintf(dbpath, "%s.db", name);

    if ((e = sqlite3_open(dbpath, &db)) != SQLITE_OK){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return e;
    }

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

int main(int argc, char **argv){
    sqlite3 *tdb;
    int e;
    struct test_results tr;
    struct test_results trt = {0, 0};
    char *tdb_path = "./.test.db";
    
    if ((e = sqlite3_open(tdb_path, &tdb)) != SQLITE_OK){
        fprintf(stderr, "Unable to open %s.\n", tdb_path);
        return 1;
    }
    if ((e = create_project(tdb, ".test")) != SQLITE_OK){
        fprintf(stderr, "Unable to open %s.\n", tdb_path);
        return 1;
    }

    clear_db(tdb);
    tr = test_tasksH(tdb);
    printf("\ntasks: %d of %d tests passed.\n", tr.p, tr.n);
    trt.n += tr.n;
    trt.p += tr.p;

    tr = test_task_utilsH(tdb);
    printf("\ntask_utils: %d of %d tests passed.\n", tr.p, tr.n);
    trt.n += tr.n;
    trt.p += tr.p;

    char lines[64];
    char tot_msg[64];
    sprintf(tot_msg, "| Total: %d of %d tests passed. |", trt.p, trt.n);
    memset(lines, '-', 64);
    lines[strlen(tot_msg)] = '\0';

    printf("\n%s", lines);
    printf("\n%s", tot_msg);
    printf("\n%s\n", lines);
    return 0;
}

