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

void test_eq(sqlite3 *tdb, int a, int b, struct test_results *tr, char *fmsg){
    char *output;

    if(a != b){
        output = "failed";
        fprintf(stderr, "Failure: %s\nShould be %d, got %d\n", fmsg, b, a);
    } else{
        output = "succeeded";
        (*tr).p++;
    }
    fprintf(stderr, "Test %d %s.\n", ++((*tr).n), output);
}

void test_neq(sqlite3 *tdb, int a, int b, struct test_results *tr, char *fmsg){
    char *output;

    if(a == b){
        output = "failed";
        fprintf(stderr, "Failure: %s\nShould be %d, got %d\n", fmsg, b, a);
    } else{
        output = "succeeded";
        (*tr).p++;
    }
    fprintf(stderr, "Test %d %s.\n", ++((*tr).n), output);
}

struct test_results test_tasksH(sqlite3 *tdb){
    struct test_results tr = {0, 0};

    // Test task creation
    test_neq(tdb, create_task(tdb, "task name", "task description"), -1, &tr, "Create a new task.");
    test_neq(tdb, create_task(tdb, "", ""), -1, &tr, "Create a task with empty name/desc.");
    test_neq(tdb, create_task(tdb, "\0", "\0"), -1, &tr, "Create a task with null name/desc");
    char *desc = calloc(1000000, 1);
    memset(desc, 'A', 70);
    test_neq(tdb, create_task(tdb, "Long description", desc), -1, &tr, "Create task with very long description"); // TODO: This test_eq passes but the description is not truncated. There's really no reason to actually truncate anything so probably should just remove that restriction anyway

    // Test task starting/stopping
    test_eq(tdb, start_task(tdb, 1), TASK_OK, &tr, "Start the first task");
    test_eq(tdb, end_task(tdb, 1), TASK_OK, &tr, "Stop the first task");
    test_eq(tdb, start_task(tdb, -1), TASK_NOT_EXIST, &tr, "Start a nonexistant task");
    test_eq(tdb, end_task(tdb, -1), TASK_NOT_EXIST, &tr, "Stop a nonexistant task");
    test_eq(tdb, end_task(tdb, 1), TASK_WRONG_STATE, &tr, "Stop a task which has not been started");
    start_task(tdb, 1);
    test_eq(tdb, start_task(tdb, 1), TASK_WRONG_STATE, &tr, "Start a task which has already been started");

    return tr;
}

struct test_results test_task_utilsH(sqlite3 *tdb){
    struct test_results tr = {0, 0};

    test_eq(tdb, get_max_id(tdb), 0, &tr, "Test if the max_id of an empty project is 0");
    create_task(tdb, "", "");
    test_eq(tdb, task_exists(tdb, 1), 1, &tr, "Test if a created task exists.");
    test_eq(tdb, task_exists(tdb, 2), 0, &tr, "Test if a nonexistant task exists.");
    test_eq(tdb, get_max_id(tdb), 1, &tr, "Test if the max_id is 1");
    test_eq(tdb, task_is_open(tdb, 1), 0, &tr, "Test if a closed task is open.");
    test_eq(tdb, get_num_timestamps(tdb, 1), 0, &tr, "Test if the number of timestamps of the task is 0");
    start_task(tdb, 1);
    test_eq(tdb, task_is_open(tdb, 1), 1, &tr, "Test if an open task is open.");
    test_eq(tdb, get_num_timestamps(tdb, 1), 1, &tr, "Test if the number of timestamps of the task is 0");
    // TODO: Need to add a way to test the open tasks func
    create_task(tdb, "", "");
    start_task(tdb, 2);
    end_task(tdb, 2);
    test_eq(tdb, get_elapsed_time(tdb, 2), 0, &tr, "Test if the elapsed time is 0"); // This may not e zero possibly?

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
    printf("tasks: %d of %d tests passed.\n\n", tr.p, tr.n);
    trt.n += tr.n;
    trt.p += tr.p;

    clear_db(tdb);
    tr = test_task_utilsH(tdb);
    printf("task_utils: %d of %d tests passed.\n\n", tr.p, tr.n);
    trt.n += tr.n;
    trt.p += tr.p;

    printf("Total: %d of %d tests passed.\n", trt.p, trt.n);
    return 0;
}

