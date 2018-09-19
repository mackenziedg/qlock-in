#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include <time.h>
#include <unistd.h>

#include "tasks.h"
#include "task_utils.h"

#define MDB_PATH "./.mdb.db"
#define MAX_PROJ_NAME_SZ 32

// deactivate_projects() deactivates all projects before
// adding a new project
int deactivate_projects(){
    sqlite3 *mdb;
    char *zero_actives = "UPDATE proj_info SET active=0";
    sqlite3_stmt *stmt;
    int e;

    if ((e = sqlite3_open(MDB_PATH, &mdb)) != SQLITE_OK){
        fprintf(stderr, "Can't open master database: %s\n", sqlite3_errmsg(mdb));
        sqlite3_close(mdb);
        return e;
    }
    e = sqlite3_prepare_v2(mdb, zero_actives, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, mdb);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, mdb);
        return e;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(mdb);
    return 0;
}

// create_project() creates a new project database
int create_project(sqlite3 *db){
    sqlite3 *mdb;
    char name[MAX_PROJ_NAME_SZ];
    char *dbpath;
    char *create_info_table = "CREATE TABLE IF NOT EXISTS task_info"
                              "(id INTEGER PRIMARY KEY,"
                              "name TEXT NOT NULL,"
                              "description TEXT);";
    char *create_ts_table = "CREATE TABLE IF NOT EXISTS task_ts"
                            "(id INTEGER NOT NULL,"
                            "timestamp INTEGER NOT NULL,"
                            "FOREIGN KEY(id) REFERENCES task_info(id));";
    char *insert_proj_into_mdb = "INSERT INTO proj_info (name, active)"
                                 "VALUES (@name, 1);";
    sqlite3_stmt *stmt;
    int e, i, l;

    printf("Enter a name for the project: ");
    fgets(name, MAX_PROJ_NAME_SZ, stdin);
    sscanf(name, "%[^\n]s", name);
    dbpath = malloc(sizeof(name)+3);
    sprintf(dbpath, "%s.db", name);
    if (access(dbpath, F_OK) != -1){
        fprintf(stderr, "Project %s already exists.", name);
        return 1;
    }
    l = sizeof(name)/sizeof(char);

    // Add the project to the master db
    if ((e = deactivate_projects()) != SQLITE_OK){
        cleanup(e, NULL, NULL);
        return e;
    }
    if ((e = sqlite3_open(MDB_PATH, &mdb)) != SQLITE_OK){
        fprintf(stderr, "Can't open master database: %s\n", sqlite3_errmsg(mdb));
        sqlite3_close(mdb);
        return e;
    }
    e = sqlite3_prepare_v2(mdb, insert_proj_into_mdb, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, mdb);
        return e;
    }
    i = sqlite3_bind_parameter_index(stmt, "@name");
    e = sqlite3_bind_text(stmt, i, name, l, SQLITE_TRANSIENT);
    if (e != SQLITE_OK){
        cleanup(e, stmt, mdb);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, mdb);
        return e;
    }
    sqlite3_finalize(stmt);
    sqlite3_close(mdb);

    // Create the project db file if everything went succesfully
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

    printf("Created project %s.\n", name);
    printf("Activated project %s.\n", name);
    return 0;
}

// get_active_project_name() provides the currently active project name
char *get_active_project_name(){
    sqlite3 *mdb;
    char *zero_actives = "SELECT name FROM proj_info WHERE active=1;";
    sqlite3_stmt *stmt;
    int e, n;
    char *name;

    if ((e = sqlite3_open(MDB_PATH, &mdb)) != SQLITE_OK){
        fprintf(stderr, "Can't open master database: %s\n", sqlite3_errmsg(mdb));
        sqlite3_close(mdb);
        return "";
    }
    e = sqlite3_prepare_v2(mdb, zero_actives, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, mdb);
        return "";
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        sqlite3_column_text(stmt, 0);
        n = sqlite3_column_bytes(stmt, 0);
        name = malloc(n+1);
        memcpy(name, (char*)sqlite3_column_text(stmt, 0), n+1);
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, mdb);
        return "";
    }

    sqlite3_finalize(stmt);
    sqlite3_close(mdb);
    return name;
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
            if (strcmp(argv[1], "in") == 0){
                id = atoi(argv[2]);
                start_task(db, id);
            } else if (strcmp(argv[1], "out") == 0){
                id = atoi(argv[2]);
                end_task(db, id);
            } else if (strcmp(argv[1], "--elapsed")==0|strcmp(argv[1], "-e")==0){
                id = atoi(argv[2]);
                int total_secs = get_elapsed_time(db, id);
                if (total_secs >= 0){
                    int hr, min, sec;
                    hr = total_secs/3600;
                    min = (total_secs-hr*3600)/60;
                    sec = total_secs-(hr*3600+min*60);
                    printf("%d:%d:%d\n", hr, min, sec);
                }
            } else if (strcmp(argv[1], "new") == 0){
                if (strcmp(argv[2], "p") == 0|strcmp(argv[2], "project") == 0){
                    create_project(db);
                } else if (strcmp(argv[2], "t") == 0|strcmp(argv[2], "task") == 0){
                    create_task(db);
                }
            }
            break;
    }
}

// create_master_db() creates the master db of projects
int create_master_db(){
    sqlite3 *mdb;
    char *create_info_table = "CREATE TABLE IF NOT EXISTS proj_info (id INTEGER PRIMARY KEY, name TEXT NOT NULL, active INTEGER NOT NULL);";
    char *create_temp_table = "INSERT INTO proj_info (name, active) VALUES ('temp', 1);";
    sqlite3_stmt *stmt;
    int e;

    if ((e = sqlite3_open(MDB_PATH, &mdb)) != SQLITE_OK){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(mdb));
        sqlite3_close(mdb);
        return e;
    }

    e = sqlite3_prepare_v2(mdb, create_info_table, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, mdb);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        printf("Creating project info table.\n");
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, mdb);
        return e;
    }
    e = sqlite3_prepare_v2(mdb, create_temp_table, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, mdb);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        printf("Creating temp project.\n");
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, mdb);
        return e;
    }
    sqlite3_finalize(stmt);

    return 0;
}

int main(int argc, char **argv){
    sqlite3 *db;
    int e, id;
    char *name;
    char *dbpath;

    if (access(MDB_PATH, F_OK) == -1){
        printf("Building master db at %s\n", MDB_PATH);
        if ((e = create_master_db()) != SQLITE_OK){
            return e;
        }
    }
    name = get_active_project_name();
    if (e != SQLITE_OK){
        return e;
    }
    printf("Using project %s.\n", name);
    dbpath = malloc(sizeof(name)+3);
    sprintf(dbpath, "%s.db", name);
    free(name);

    if (argc < 2){
        fprintf(stderr, "Must pass at least one parameter.\n");
        return 1;
    }

    if ((e = sqlite3_open(dbpath, &db)) != SQLITE_OK){
        fprintf(stderr, "Could not open project %s at path %s.", name, dbpath);
        return 1;
    }
    handle_input(db, argc, argv);

    free(dbpath);
    if (db != NULL){
        sqlite3_close(db);
    }
    return 0;
}
