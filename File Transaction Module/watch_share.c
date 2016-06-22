#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <limits.h>
#include <sqlite3.h>

#include "../Global/global_definitions.h"
#include "../Volume Management Module/file_assembly.h"
#include "../Volume Management Module/file_mapping.h"
#include "../Cache Access Module/cache_operation.h"
#include "watch_share.h"

#define MAX_EVENTS 1024 /*Max. number of events to process at one go*/
#define LEN_NAME 16 /*Assuming that the length of filename won't exceed 16 bytes*/
#define EVENT_SIZE ( sizeof (struct inotify_event) ) /*size of one event*/
#define BUF_LEN  ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )) /*buffer to store the data of events*/

void delete_linear_file(String filename){
    sqlite3 *db;
    sqlite3_stmt *res;
    String comm, fileloc;

    int rc;
    const char *tail;

    rc = sqlite3_open(DBNAME, &db);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Can't open database\n");
       sqlite3_close(db);
       exit(1);
    }

    String query;
    sprintf(query, "SELECT fileloc FROM VolContent WHERE filename = '%s';", filename);
    printf("Query = %s\n", query);
    rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
    if (rc != SQLITE_OK){
      fprintf(stderr, "Delete Linear File() Error\n");
      sqlite3_close(db);
      exit(1);
    }

    if (sqlite3_step(res) == SQLITE_ROW){
       sprintf(fileloc, "%s", sqlite3_column_text(res,0));
       sprintf(comm, "rm '%s/%s'", sqlite3_column_text(res,0), filename);
    }

    printf("Filename : %s || Fileloc : %s\n", filename, fileloc);
    update_target_size_delete(filename, fileloc);
    system(comm);
    printf("File %s successfully deleted.\n", filename);
    
    sprintf(query, "DELETE from VolContent where filename = '%s';", filename);
    rc = sqlite3_exec(db, query, 0, 0, 0);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Delete Linear File Error!\n");
    }

    sqlite3_finalize(res);
    sqlite3_close(db);
}

void delete_stripe_file(String filename)
{
    sqlite3 *db;
    sqlite3_stmt *res;

    int rc;
    const char *tail;

    rc = sqlite3_open(DBNAME, &db);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Can't open database!\n");
       sqlite3_close(db);
       exit(1);
    }
    
    //another code here: check file if in cache, if yes, remove file
    String comm, comm_out;
    int inCache = 0;
    sprintf(comm, "ls %s", CACHE_LOC);
    runCommand(comm, comm_out);
    
    char *ptr = NULL;
    ptr = strtok(comm_out, "\n");
    while (ptr != NULL){
        if (strcmp(ptr, filename) == 0){
            inCache = 1;
            break;
        }
        ptr = strtok(NULL, "\n");
    }

    if (inCache){
       sprintf(comm, "rm '%s/%s'", CACHE_LOC, filename);
       system(comm);
       printf("File %s deleted from Cache.\n", filename);
    }

    //another code here: remove part1. in filename
    //this is only for the usage of like in query
    memmove(filename, filename + strlen("part1."), 1 + strlen(filename + strlen("part1.")));


    char percent = '%';
    String query;
    sprintf(query, "SELECT  filename, fileloc from VolContent where filename like '%c%s';", percent, filename, percent);
    rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Delete Stripe File() Error\n");
       sqlite3_close(db);
       exit(1);
    }

    while (sqlite3_step(res) == SQLITE_ROW){
       String comm;

       String file, fileloc;
       sprintf(file, "%s", sqlite3_column_text(res,0));
       sprintf(fileloc, "%s", sqlite3_column_text(res,1));
       update_target_size_delete(file, fileloc);

       sprintf(comm, "rm '%s/%s'", sqlite3_column_text(res,1), sqlite3_column_text(res,0));
       printf("File %s has been successfully deleted.\n", sqlite3_column_text(res,0));
       system(comm);

       //delete entry from volcontent
       sprintf(query, "DELETE from VolContent where filename = '%s';", sqlite3_column_text(res,0));
       rc = sqlite3_exec(db, query, 0, 0, 0);
       if (rc != SQLITE_OK){
          fprintf(stderr, "Delete Stripe File Entry() Error\n");
       }

       //delete entry from cachecontent; check if filename contains part1.
       //delete from cachecontent db: because only part1. have entry in table
       if (strstr(sqlite3_column_text(res,0), "part1.") != NULL){
          sprintf(query, "DELETE from CacheContent where filename = '%s';", sqlite3_column_text(res,0));
          rc = sqlite3_exec(db, query, 0, 0, 0);
          if (rc != SQLITE_OK){
             fprintf(stderr, "Delete Stripe File Cache Entry() Error\n");
          }
       }
    }

    sqlite3_finalize(res);
    sqlite3_close(db);
}

void *watch_share()
{
    int length, i = 0, wd;
    int fd;
    int rc;
    int access_count = 1;
    int r_count = 1;
    char *p;
    char buffer[BUF_LEN] __attribute__ ((aligned(8)));

    String query;

    sqlite3 *db;
    sqlite3_stmt *res;

    const char *tail;
   
   
    rc = sqlite3_open(DBNAME, &db);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Can't open database: %s.\n", sqlite3_errmsg(db));
       sqlite3_close(db);
       exit(1);
    }
    
    strcpy(query, "SELECT mountpt FROM Target;");
    rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Failed to retrieved data.\n");
       exit(1);
    }
   
    /*Initialize inotify*/
    fd = inotify_init();
    if ( fd < 0 ) {
       perror( "Couldn't initialize inotify" );
    }

    while (sqlite3_step(res) == SQLITE_ROW){
       wd = inotify_add_watch(fd, sqlite3_column_text(res,0), IN_OPEN | IN_CLOSE_NOWRITE);
       if (wd == -1){
          printf("Couldn't add watch to %s\n", sqlite3_column_text(res,0));
       } else {
          printf("Watching:: %s\n", sqlite3_column_text(res,0));
       }
    }

   wd = inotify_add_watch(fd, CACHE_LOC, IN_OPEN | IN_CLOSE_NOWRITE);
      
   if (wd != -1){
    	printf("Watching:: %s\n", CACHE_LOC);
   }

    /*do it forever*/
    for(;;){
       length = read(fd, buffer, BUF_LEN);
      
       if (length < 0){
          perror("read");
       }

     for(p = buffer; p < buffer + length;){
           struct inotify_event *event = (struct inotify_event *) p;

           //printf("AAA* event is %d\n", event->mask);
         //if (event->len){
              if (event->mask & IN_OPEN){
                  if (event->mask & IN_ISDIR){
                      //printf("The directory %s was opened.\n", event->name);
                  }else {
                      //only recognize if part1 of the file is opened
                      //check if its cache or not
                      //if its not, assemble the file
                      printf("%s was opened.\n", event->name);
                      //incrementFrequency(event->name);
                      if (strstr(event->name,"part1.") != NULL){
                      
                        incrementFrequency(event->name);
                     /*   String comm, comm_out;
                        int inCache = 0;
                        sprintf(comm, "ls %s", CACHE_LOC);
                        runCommand(comm, comm_out);

                        char *ptr = NULL;
                        ptr = strtok(comm_out, "\n");
                        while (ptr != NULL){
                             if (strcmp(ptr, event->name) == 0){
                                 inCache = 1;
                                 break;
                             }
                        }                        
     
                        if (!inCache){
                            assemble(event->name);
                        }*/
                      }
                  }
              }

              if (event->mask & IN_DELETE){
                  if (event->mask & IN_ISDIR){
		             //printf("The directory %s will be deleted.\n", event->name);
	          }else{
		             printf("The file %s will be deleted.\n", event->name);
                     	     if (strstr(event->name, "part1.") != NULL){
                         	delete_stripe_file(event->name);
                     	     } else {
                         	delete_linear_file(event->name);
                     	     }
                  }	
              }

              if (event->mask & IN_CLOSE_NOWRITE){
                  if (event->mask & IN_ISDIR){
                      //printf("The directory %s not open for writing was closed.\n", event->name);
                  }else{
                      printf("The file %s was closed.\n", event->name);
                  if (strstr(event->name, "part1.") != NULL){
                           refreshCache();
                      }             
                  }
              }
              
              p += EVENT_SIZE + event->len;
           //}
       }
}    

    /* Clean up */
    inotify_rm_watch(fd, wd);
    close(fd);
    sqlite3_finalize(res);
    sqlite3_close(db);
}
