
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <limits.h>
#include <dirent.h>
#include <errno.h>
#include <sqlite3.h>

#include "../Global/global_definitions.h"
#include "../volume_management/file_mapping.h"
#include "../volume_management/file_assembly.h"
#include "../disk_pooling/file_presentation.h"
#include "../cache_access/cache_operation.h"
#include "../file_striping/file_striping.h"

#define MAX_EVENTS 1024 /*Max. number of events to process at one go*/
#define LEN_NAME 32 /*Assuming that the length of filename won't exceed 16 bytes*/
#define EVENT_SIZE ( sizeof (struct inotify_event) ) /*size of one event*/

#define BUF_LEN  ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )) /*buffer to store the data of events*/

#define MAXDEPTH 30

void make_folder(String root){
	sqlite3 *db;
	sqlite3_stmt *res;
	int rc;
	const char *tail;
	rc = sqlite3_open(DBNAME, &db);

	String sql = "";
	strcpy(sql, "Select mountpt from Target;");

	if (rc != SQLITE_OK){
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	rc = sqlite3_prepare_v2(db, sql, 1000, &res, &tail);
	if (rc != SQLITE_OK){
		fprintf(stderr, "Can't execute prepare v2: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	//int counter = 0;
	while (sqlite3_step(res) == SQLITE_ROW){
		String mkdir;
		sprintf(mkdir, "mkdir '%s/%s'", sqlite3_column_text(res,0), root);
		//printf("MKDIR = %s\n", mkdir);
		system(mkdir);

		String chmod;
		sprintf(chmod, "chmod -R 777 '%s/%s'", sqlite3_column_text(res,0), root);
		system(chmod);

	    if (strcmp(sqlite3_column_text(res,0), "/mnt/lvsdb") == 0){
		printf("ROOT: %s\n", root);

		// String ln = "";
		// sprintf(ln, "ln -s '%s/%s' '%s/%s'", sqlite3_column_text(res,0), root, SHARE_LOC, root);

		String sors = "", dest = "";
		sprintf(sors, "%s/%s", sqlite3_column_text(res,0), root);
		sprintf(dest, "%s/%s", SHARE_LOC, root);
		if(symlink(sors, dest) == 0) {
            		printf("Link Created: '%s'\n", dest);
        	} else {
            	
	//printf("!!! Error creating link %s\n", dest);
        	}
	// printf("LN(dest) = %s\n", dest);
		//system(ln);
	    }

	}
	//counter = 0;
	sqlite3_finalize(res);
	sqlite3_close(db);
}

void get_root(int wds[], String dirs[], int counter, int wd, String arr[])
{
    int x;
    for (x = 0; x < counter; x++){
        if (wd == wds[x]){
	    strcpy(arr[x], dirs[x]);
            return get_root(wds, dirs, counter, x, arr);
	}
    }
    //return arr;
}

void* watch_temp()

{
    int length, i = 0, wd;
    int fd;
    char buffer[BUF_LEN];

    int wds[69];
    String dirs[69];
    int counter = 1;

    /*Initialize inotify*/
    fd = inotify_init();
    if ( fd < 0 ) {
       perror( "Couldn't initialize inotify" );
    }
    /*add watch to directory*/
//    wd = inotify_add_watch(fd, TEMP_LOC, IN_ALL_EVENTS);
    wd = inotify_add_watch(fd, TEMP_LOC, IN_ALL_EVENTS);
    wds[counter-1] = wd;
    strcpy(dirs[counter-1], TEMP_LOC);

    if (wd == -1){
        printf("Couldn't add watch to %s\n", TEMP_LOC);
    } else {
        printf("Watching:: %s\n", TEMP_LOC);
    }

    /*do it forever*/
    while (1){

       create_link();
 //      watch_share();

       i = 0;
       length = read(fd, buffer, BUF_LEN);

       if (length < 0){
          perror("read");
       }

       while( i < length ){
           struct inotify_event *event = (struct inotify_event *) &buffer[i];

           if (event->len){


              //printf("AAAA is %x\n", event->mask);
	      if (event->mask & IN_CREATE){
		if (event->mask & IN_ISDIR){
		     printf("%s is created.\n", event->name);
		     String dir_to_watch = "";
		     String root = "";
		     String arr[MAXDEPTH];
		     int d;
		    //Initialize array....
		     for (d = 0; d < 30; d++){
			strcpy(arr[d], "");
		     }

		     get_root(wds,dirs,counter,event->wd,arr);

		     for (d = 1; d < counter; d++){
			if(strcmp(arr[d], "") != 0) {
			    strcat(root, arr[d]);
			    strcat(root, "/");
			}
		     }

		     String mkdir = "", rm = "", ln = "";

		     if (wd == 1){
		        //sprintf(rm, "rm -rf '%s/%s'", SHARE_LOC, event->name);
			//system(rm);
			//sprintf(mkdir, "cp '%s/%s' '%s/%s'", TEMP_LOC, event->name, SHARE_LOC, event->name);
			//printf("CP: %s\n", mkdir);
			//system(mkdir);
			//String cp = "";
			//sprintf(cp, "cp -r '%s/%s' '%s/.'", TEMP_LOC, event->name, SHARE_LOC, event->name);
			//system(cp);
			//system(rsync);
			make_folder(event->name);
			//sprintf(mkdir, "mkdir -p '%s/%s'", SHARE_LOC, event->name);

			//system(mkdir);
		     }else{
			String x;
			sprintf(x, "%s%s", root, event->name);
			//String cp = "";
			//sprintf(cp, "cp -r '%s/%s' '%s/%s'", TEMP_LOC, root, SHARE_LOC, root);
			//system(cp);
			//sprintf(rm, "rm -rf '%s/%s'", SHARE_LOC, x);
			//system(rm);
			//sprintf(mkdir, "cp '%s/%s' '%s/%s'", TEMP_LOC, x, SHARE_LOC, x);
			//printf("CP :'%s'\n", mkdir);
			//system(mkdir);
			make_folder(x);
			//sprintf(mkdir, "mkdir -p '%s/%s'", SHARE_LOC, x);
			//system(mkdir);
		     }





		     //select mountpt from targets
		     //mkdir
		     //make_folder(root);


		     //printf("Root Directory: %s%s\n", root, event->name);
                     sprintf(dir_to_watch,"%s/%s%s", TEMP_LOC, root, event->name);
		     wd = inotify_add_watch(fd, dir_to_watch, IN_ALL_EVENTS);
                     wds[counter] = wd;
                     strcpy(dirs[counter], event->name);
		     counter++;
		}
              }

              if (event->mask & IN_CLOSE_WRITE){
		     String root = "";
		     String arr[MAXDEPTH];
		     int d;
		      //initialize the array...
		      for (d = 0; d < MAXDEPTH; d++){
			strcpy(arr[d], "");
		      }
		      get_root(wds, dirs, counter, event->wd, arr);
		      for (d = 1; d < counter; d++){
			if(strcmp(arr[d], "") != 0) {
			    strcat(root, arr[d]);
			    strcat(root, "/");
			}
		      }

                      String filepath = "";
		      String filename = "";
		      sprintf(filename, "%s%s", root, event->name);
                      FILE *fp;
                      sprintf(filepath, "%s/%s%s", TEMP_LOC, root, event->name);
		      //printf("hahahaa: filepath:%s\n\tfilename:%s", filepath, filename);
                      fp = fopen(filepath, "rb");
                      if (fp != NULL){
                        fseek(fp, 0L, SEEK_END);
                        long sz = ftell(fp);
                        rewind(fp);
                         //check if stripe file
                        if (sz > STRIPE_SIZE){
                           //before striping, check cache
                        /*   printf("%s will be striped.\n", event->name);
			   printf("Inserting into CacheContent...\n");
                           update_cache_list(event->name);
                           printf("Cache Count: %d\n", getCacheCount());
                           if (getCacheCount() < MAX_CACHE_SIZE) { //max cache size is 10
                              printf("%s will be put to cache.\n", event->name);
                              file_map_cache(event->name);
                              create_link_cache(event->name);
                           }
                           stripe(event->name);
                           refreshCache();
			*/
			   stripe(root, filepath, filename);
                        } else {
			   printf("Transferring %s to targets...\n", filename);
                           file_map(filepath, filename);
                        }

                  }
              }

              if (event->mask & IN_MOVED_TO){
                  if (event->mask & IN_ISDIR)
                      printf("The directory %s is transferring.\n", event->name);
                  else
                      printf("The file %s is transferring.\n", event->name);
              }



              i += EVENT_SIZE + event->len;
           }
       }
    }

    /* Clean up */
    inotify_rm_watch(fd, wd);
    close(fd);
}
