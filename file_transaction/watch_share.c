#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <limits.h>
#include <sqlite3.h>
#include <syslog.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "../Global/global_definitions.h"
#include "../volume_management/file_assembly.h"
#include "../volume_management/file_mapping.h"
#include "../cache_access/cache_operation.h"
#include "watch_share.h"

#define MAX_EVENTS 1024 /*Max. number of events to process at one go*/
#define LEN_NAME 32 /*Assuming that the length of filename won't exceed 16 bytes*/
#define EVENT_SIZE ( sizeof (struct inotify_event) ) /*size of one event*/
#define BUF_LEN  ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )) /*buffer to store the data of events*/

#define MAX_WTD 200 //max is 200 watches, assumed

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
    //syslog(LOG_INFO, "FileTransaction: Query = %s\n", query);
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
    sqlite3_finalize(res);
    //syslog(LOG_INFO, "FileTransaction: Filename : %s || Fileloc : %s\n", filename, fileloc);
    update_target_size_delete(filename, fileloc);
    system(comm);
    syslog(LOG_INFO, "FileTransaction: File %s successfully deleted.\n", filename);

    sprintf(query, "DELETE from VolContent where filename = '%s';", filename);
    rc = sqlite3_exec(db, query, 0, 0, 0);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Delete Linear File Error!\n");
    }

    //sqlite3_finalize(res);
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
       syslog(LOG_INFO, "FileTransaction: File %s deleted from Cache.\n", filename);
    }

    //another code here: remove part1. in filename
    //this is only for the usage of like in query
    memmove(filename, filename + strlen("part1."), 1 + strlen(filename + strlen("part1.")));


    char percent = '%';
    String query, sql;
    sprintf(query, "SELECT  filename, fileloc from VolContent where filename like '%c%s';", percent, filename); // note to aidz meron ako pinalitan dito!!!!
    strcpy(sql, query);
    rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Delete Stripe File() Error\n");
       sqlite3_close(db);
       exit(1);
    }

    int count = 0;
    while(sqlite3_step(res) == SQLITE_ROW){
	count++;
    }

    String file_list[count];
    String fileloc_list[count];
    int x = 0;
    while (sqlite3_step(res) == SQLITE_ROW){
       //String comm;

       String file, fileloc;
       sprintf(file, "%s", sqlite3_column_text(res,0));
       sprintf(fileloc, "%s", sqlite3_column_text(res,1));
       strcpy(file_list[x], file);
       strcpy(fileloc_list[x], fileloc);
       x++;
       //update_target_size_delete(file, fileloc);

       //sprintf(comm, "rm '%s/%s'", sqlite3_column_text(res,1), sqlite3_column_text(res,0));
       //syslog(LOG_INFO, "FileTransaction: File %s has been successfully deleted.\n", sqlite3_column_text(res,0));
       //system(comm);

       //delete entry from volcontent
       /*sprintf(query, "DELETE from VolContent where filename = '%s';", sqlite3_column_text(res,0));
       rc = sqlite3_exec(db, query, 0, 0, 0);
       if (rc != SQLITE_OK){
          fprintf(stderr, "Delete Stripe File Entry() Error\n");
       }*/

       //delete entry from cachecontent; check if filename contains part1.
       //delete from cachecontent db: because only part1. have entry in table
       if (strstr(sqlite3_column_text(res,0), "part1.") != NULL){
          sprintf(query, "DELETE from CacheContent where filename = '%s';", sqlite3_column_text(res,0));
        //   syslog(LOG_INFO, "FileTransaction: CacheQuery = %s\n", query);
          rc = sqlite3_exec(db, query, 0, 0, 0);
          if (rc != SQLITE_OK){
             fprintf(stderr, "Delete Stripe File Cache Entry() Error\n");
          }
       }
    }

    sqlite3_finalize(res);

    int z = 0;
    for (z = 0; z < count; z++){
       String comm;
       sprintf(comm, "rm '%s/%s'", fileloc_list[z], file_list[z]);
       update_target_size_delete(file_list[z], fileloc_list[z]);
       system(comm);
       syslog(LOG_INFO, "FileTransaction: File Part %s successfully deleted.\n", file_list[z]);
    }

    rc = sqlite3_prepare_v2(db, sql, 1000, &res, &tail);
    while (sqlite3_step(res) == SQLITE_ROW){
	sprintf(query, "DELETE from VolContent where filename = '%s';", sqlite3_column_text(res,0));
    	rc = sqlite3_exec(db, query, 0, 0, 0);
	if (rc != SQLITE_OK){
	    syslog(LOG_INFO, "FileTransaction: Delete Stripe File() Error!\n");
        }
    }

    sqlite3_finalize(res);
    sqlite3_close(db);
}


//THIS FUNCTION ADD A WATCH TO ALL DIRECTORIES AND SUBDIRECTORIES
//THAT ARE ALREADY WRITTEN
void list_dir(String dir_to_read, int fd, int wds[], String dirs[], int counter){
    struct dirent *de;

    DIR *dr = opendir(dir_to_read);

    if (dr == NULL){
	printf("Could not open current directory");
    }     

    while ((de = readdir(dr)) != NULL){
	struct stat s;
	String subdir = "";
	sprintf(subdir, "%s/%s", dir_to_read, de->d_name);
	stat(subdir, &s);
	if (S_ISDIR(s.st_mode)){
		if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
			int wd = inotify_add_watch(fd, subdir, IN_ALL_EVENTS);
			wds[counter] = wd;
			strcpy(dirs[counter], subdir);
			counter++;
			printf("Watching:: %s\n", subdir);
			list_dir(subdir,fd, wds, dirs, counter);
		}
	}
    }
    closedir(dr);
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
    int counter = 0;
    int wds[MAX_WTD];
    String dirs[MAX_WTD];


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
       wds[counter] = wd;
       strcpy(dirs[counter], sqlite3_column_text(res,0));
       counter++;
       if (wd == -1){
          syslog(LOG_INFO, "FileTransaction: Couldn't add watch to %s\n", sqlite3_column_text(res,0));
       } else {
          syslog(LOG_INFO, "FileTransaction: Watching:: %s\n", sqlite3_column_text(res,0));
       }

       //Check each target for directory
       String dir_to_read = "";
       strcpy(dir_to_read, sqlite3_column_text(res,0));
       list_dir(dir_to_read, fd, wds, dirs, counter);

    }

   wd = inotify_add_watch(fd, CACHE_LOC, IN_ALL_EVENTS);
   wds[counter] = wd;
   strcpy(dirs[counter], CACHE_LOC);
   counter++;
   if (wd != -1){
    	syslog(LOG_INFO, "FileTransaction: Watching:: %s\n", CACHE_LOC);
   }

   wd = inotify_add_watch(fd, SHARE_LOC, IN_ALL_EVENTS);
   wds[counter] = wd;
   strcpy(dirs[counter], SHARE_LOC);
   counter++;
   if (wd != -1){
	syslog(LOG_INFO, "FileTransaction: Watching:: %s\n", SHARE_LOC);
   }

    /*do it forever*/
    for(;;){
	create_link();
       length = read(fd, buffer, BUF_LEN);

       if (length < 0){
          perror("read");
       }

     for(p = buffer; p < buffer + length;){
           struct inotify_event *event = (struct inotify_event *) p;

           //syslog(LOG_INFO, "FileTransaction: AAA* event is %d\n", event->mask);
         //if (event->len){
	     if (event->mask & IN_CREATE){
	   	  if (event->mask & IN_ISDIR){
		      //check if folder is created by make_folder() in write part
		      //in targets
                      int size = sizeof(wds) / sizeof(wds[0]); 
		      int i = 0;
		
      		      for (i = 0; i < size; i++){
     			   if (wds[i] == event->wd){
				if (strstr(dirs[i], "/mnt/Share") == NULL){
				//this means that the folder that trigger the event is a target
				   String add_watch_dir = "";
				   sprintf(add_watch_dir, "%s/%s", dirs[i], event->name);
				   //add to inotify watch
				   int wd = inotify_add_watch(fd, add_watch_dir, IN_ALL_EVENTS);
				   wds[counter] = wd;
				   strcpy(dirs[counter], add_watch_dir);
				   counter++;
			        }
			   }
		      }

		      printf("Directory %s created.\n", event->name);
		  } else {
		      //file_map_share(event->name);
		  }
	     }

             if (event->mask & IN_MODIFY){
		  if (event->mask & IN_ISDIR){
			//do nothing
		  } else {
		     syslog(LOG_INFO, "FileTransaction: File %s was modified.\n", event->name);
		  }
	     }

              if (event->mask & IN_OPEN){
                  if (event->mask & IN_ISDIR){
                      //syslog(LOG_INFO, "FileTransaction: The directory %s was opened.\n", event->name);
                  }else {
                      //only recognize if part1 of the file is opened
                      //check if its cache or not
                      //if its not, assemble the file
                      printf("%s was opened.\n", event->name);
                      //incrementFrequency(event->name);
                      if (strstr(event->name,"part1.") != NULL){

                        incrementFrequency(event->name);
                      /*  String comm, comm_out;
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
			     ptr = strtok(NULL, "\n");
                        }

                        if (!inCache){
                            assemble(event->name);
                        }*/
                      }
                  }
              }

              if (event->mask & IN_DELETE){
                  if (event->mask & IN_ISDIR){
		             //syslog(LOG_INFO, "FileTransaction: The directory %s will be deleted.\n", event->name);
	          }else{
		             syslog(LOG_INFO, "FileTransaction: The file %s will be deleted.\n", event->name);
                     	     if (strstr(event->name, "part1.") != NULL){
                         	delete_stripe_file(event->name);
                     	     } else {
                         	delete_linear_file(event->name);
                     	     }
                  }
              }

              if (event->mask & IN_CLOSE){
                  if (event->mask & IN_ISDIR){
                      //syslog(LOG_INFO, "FileTransaction: The directory %s not open for writing was closed.\n", event->name);
                  }else{
                      syslog(LOG_INFO, "FileTransaction: The file %s was closed.\n", event->name);
                  if (strstr(event->name, "part1.") != NULL){
                           //refreshCache();
                      }
                  }
              }

	      if (event->mask & IN_MOVED_TO){
		printf("IN_MOVED_TO := %s\n", event->name);
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
