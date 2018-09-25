#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>
#include <unistd.h>

#include "project.h"
#include "tasks.h"
#include "task_utils.h"

#define MDB_PATH "./.mdb.db"
#define MAX_PROJ_NAME_SZ 32
#define MAX_TASK_NAME_SZ 64
#define MAX_TASK_DESC_SZ 256

// handle_input() proccesses the command-line input and passes it to the
// correct function.
int handle_input(sqlite3 *db, sqlite3 *mdb, int argc, char **argv){
    int e, id;
    int *o;
    char **s;
    char *name;
    int n = 0;

    switch (argc) {
        case 2:
            if (strcmp(argv[1], "active")==0){
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
                e = start_task(db, id);
                if (e == TASK_NOT_EXIST){
                    fprintf(stderr, "Could not start task #%d as it does not exist.\n", id);
                } else if (e == TASK_WRONG_STATE){
                    fprintf(stderr, "Could not start task #%d as it has already been started.\n", id);
                } else if (e == TASK_OK) {
                    printf("Started task #%d.\n", id);
                }
            } else if (strcmp(argv[1], "out") == 0){
                id = atoi(argv[2]);
                e = end_task(db, id);
                if (e == TASK_NOT_EXIST){
                    fprintf(stderr, "Could not end task #%d as it does not exist.\n", id);
                } else if (e == TASK_WRONG_STATE){
                    fprintf(stderr, "Could not end task #%d as it has not been started.\n", id);
                } else if (e == TASK_OK) {
                    printf("Ended task #%d.\n", id);
                }
            } else if (strcmp(argv[1], "elapsed")==0){
                id = atoi(argv[2]);
                print_elapsed_breakdown(db, id);
            } else if (strcmp(argv[1], "new") == 0){
                if ((strcmp(argv[2], "p")==0)|(strcmp(argv[2], "project")==0)){
                    char name[MAX_PROJ_NAME_SZ];
                    printf("Enter a name for the project: ");
                    fgets(name, MAX_PROJ_NAME_SZ, stdin);
                    sscanf(name, "%[^\n]s", name);
                    if ((e = create_project(db, mdb, name)) == 0){
                        printf("Created project %s.\n", name);
                        printf("Activated project %s.\n", name);
                    } else{
                        switch (e){
                            case -1:
                                fprintf(stderr, "Must provide a name for the project.\n");

                                break;
                            case -2:
                                fprintf(stderr, "Project %s already exists.\n", name);
                                break;
                        }
                    }
                } else if ((strcmp(argv[2], "t")==0)|(strcmp(argv[2], "task")==0)){
                    char name[MAX_TASK_NAME_SZ];
                    char desc[MAX_TASK_DESC_SZ];
                    int id;

                    printf("Enter a name for the task: ");
                    fgets(name, MAX_TASK_NAME_SZ, stdin);
                    sscanf(name, "%[^\n]s", name);
                    printf("Enter a description for the task: ");
                    fgets(desc, MAX_TASK_DESC_SZ, stdin);
                    sscanf(desc, "%[^\n]s", desc);
                    id = create_task(db, name, desc);

                    printf("Created task #%d.\n", id);
                }
            } else if (strcmp(argv[1], "switch") == 0){
                name = malloc(strlen(argv[2]));
                strcpy(name, argv[2]);
                if ((e = switch_active_project(mdb, name)) != 0){
                    fprintf(stderr, "Failed to switch to project '%s'--Error %d.\n", name, e);
                }
                free(name);
            } else if (strcmp(argv[1], "list") == 0){
                if ((strcmp(argv[2], "p")==0)|(strcmp(argv[2], "project")==0)){
                    if ((n = get_all_projects(mdb, &s)) > 0){
                        for (int i = 0; i < n; i++){
                            printf("%s\n", s[i]);
                        }
                        free(s);
                    }
                } else if ((strcmp(argv[2], "t")==0)|(strcmp(argv[2], "task")==0)){
                    if ((n = get_all_tasks(db, &o)) > 0){
                        for (int i = 0; i < n; i++){
                            printf("%d\n", o[i]);
                        }
                        free(o);
                    }
                }
            }
            break;
    }
    return 0;
}

int main(int argc, char **argv){
    sqlite3 *db = NULL;
    sqlite3 *mdb = NULL;
    int e;
    char *name;
    char *dbpath;

    if (access(MDB_PATH, F_OK) == -1){
        printf("Building master db at %s\n", MDB_PATH);
        if ((e = create_master_db(&mdb, MDB_PATH)) != SQLITE_OK){
            return e;
        }
    } else{
        if ((e = sqlite3_open(MDB_PATH, &mdb)) != SQLITE_OK){
            return e;
        }
    }
    name = get_active_project_name(mdb);
    dbpath = malloc(sizeof(name)+3);
    sprintf(dbpath, "%s.db", name);

    if (argc < 2){
        fprintf(stderr, "Must pass at least one parameter.\n");
        free(dbpath);
        free(name);
        sqlite3_close(db);
        sqlite3_close(mdb);
        return 1;
    }

    if ((e = sqlite3_open(dbpath, &db)) != SQLITE_OK){
        fprintf(stderr, "Could not open project %s at path %s.", name, dbpath);
        free(dbpath);
        free(name);
        sqlite3_close(db);
        sqlite3_close(mdb);
        return 1;
    }
    handle_input(db, mdb, argc, argv);

    free(dbpath);
    if (db != NULL){
        sqlite3_close(db);
    }
    if (mdb != NULL){
        sqlite3_close(mdb);
    }
    return 0;
}
