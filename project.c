#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>

#ifndef PROJECT_H
#define PROJECT_H
#include "project.h"
#endif
#include "task_utils.h"

// deactivate_projects() deactivates all projects before
// adding a new project
int deactivate_projects(sqlite3 *mdb){
    char *zero_actives = "UPDATE proj_info SET active=0";
    sqlite3_stmt *stmt;
    int e;

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
int create_project(sqlite3 *db, sqlite3 *mdb, char* name){
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

    dbpath = malloc(sizeof(name)+3);
    sprintf(dbpath, "%s.db", name);
    if (access(dbpath, F_OK) != -1){
        fprintf(stderr, "Project %s already exists.\n", name);
        return 1;
    }
    l = sizeof(name)/sizeof(char);

    // Add the project to the master db
    if ((e = deactivate_projects(mdb)) != SQLITE_OK){
        cleanup(e, NULL, NULL);
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
char *get_active_project_name(sqlite3 *mdb){
    char *zero_actives = "SELECT name FROM proj_info WHERE active=1;";
    sqlite3_stmt *stmt;
    int e, n;
    char *name = malloc(1);

    e = sqlite3_prepare_v2(mdb, zero_actives, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, mdb);
        return "";
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        sqlite3_column_text(stmt, 0);
        n = sqlite3_column_bytes(stmt, 0);
        name = malloc(n+1);
        strcpy(name, (char*)sqlite3_column_text(stmt, 0));
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, mdb);
        return "";
    }

    sqlite3_finalize(stmt);
    sqlite3_close(mdb);
    return name;
}

// create_master_db() creates the master db of projects
int create_master_db(sqlite3 **mdb, char *mdb_path){
    char *create_info_table = "CREATE TABLE IF NOT EXISTS proj_info (id INTEGER PRIMARY KEY, name TEXT UNIQUE NOT NULL, active INTEGER NOT NULL);";
    char *create_temp_table = "INSERT INTO proj_info (name, active) VALUES ('temp', 1);";
    sqlite3_stmt *stmt;
    int e;

    if ((e = sqlite3_open(mdb_path, mdb)) != SQLITE_OK){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(*mdb));
        sqlite3_close(*mdb);
        return e;
    }

    e = sqlite3_prepare_v2(*mdb, create_info_table, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, *mdb);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, *mdb);
        return e;
    }
    e = sqlite3_prepare_v2(*mdb, create_temp_table, -1, &stmt, NULL);
    if (e != SQLITE_OK){
        cleanup(e, stmt, *mdb);
        return e;
    }
    while ((e = sqlite3_step(stmt)) == SQLITE_ROW){
        printf("Creating temp project.\n");
    }
    if (e != SQLITE_DONE){
        cleanup(e, stmt, *mdb);
        return e;
    }
    sqlite3_finalize(stmt);

    return 0;
}
