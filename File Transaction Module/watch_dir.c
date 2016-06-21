#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <limits.h>

#include "../Global/global_definitions.h"
#include "../Volume Management Module/file_mapping.h"
#include "../Volume Management Module/file_assembly.h"
#include "../Disk Pooling Module/file_presentation.h"
#include "../Cache Access Module/cache_operation.h"
#include "../File Striping Module/file_striping.h"

#define MAX_EVENTS 1024 /*Max. number of events to process at one go*/
#define LEN_NAME 16 /*Assuming that the length of filename won't exceed 16 bytes*/
#define EVENT_SIZE ( sizeof (struct inotify_event) ) /*size of one event*/

#define BUF_LEN  ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )) /*buffer to store the data of events*/

void* watch_temp()

{
    int length, i = 0, wd;
    int fd;
    char buffer[BUF_LEN];

    /*Initialize inotify*/
    fd = inotify_init();
    if ( fd < 0 ) {
       perror( "Couldn't initialize inotify" );
    }

    /*add watch to directory*/
//    wd = inotify_add_watch(fd, TEMP_LOC, IN_ALL_EVENTS);
    wd = inotify_add_watch(fd, TEMP_LOC, IN_CLOSE_WRITE | IN_MOVED_TO);        

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

              if (event->mask & IN_CLOSE_WRITE){
                  if (event->mask & IN_ISDIR)
                      printf("The directory %s open for writing was closed.\n", event->name);
                  else {
                      
                      //printf("The file %s open for writing was closed.\n", event->name);
                      
                      String filepath;
                      FILE *fp;

                      sprintf(filepath, "/mnt/CVFSTemp/%s", event->name);
                      fp = fopen(filepath, "rb");
                      if (fp != NULL){
                        fseek(fp, 0L, SEEK_END);
                        long sz = ftell(fp);
                        rewind(fp);
                         //check if stripe file
                        if (sz > STRIPE_SIZE){
                           //before striping, check cache 
                           printf("%s will be striped.\n", event->name);
			   printf("Inserting into CacheContent...\n");
                           update_cache_list(event->name);
                           printf("Cache Count: %d\n", getCacheCount());
                           if (getCacheCount() < MAX_CACHE_SIZE) { //max cache size is 10
                              printf("%s will be put to cache.\n", event->name);
                              file_map_cache(event->name);
                              create_link_cache(event->name);
                           }
                           stripe(event->name);
                          // String file = "";
                          // strcpy(file, "part1.");
                          // strcat(file, event->name);
                           //update_cache_file_mountpt(event->name);
                           refreshCache();
                          
                        } else {
                           file_map(event->name);
                        }
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
