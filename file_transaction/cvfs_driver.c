#include <pthread.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>

#include "../Global/global_definitions.h"
#include "../disk_pooling/file_presentation.h"
#include "../disk_pooling/initial_configurations.h"
#include "watch_share.h"
#include "watch_dir.h"

#define THREADCNT 2

int main(int argc, char **argv)
{
   pthread_t t[THREADCNT];
   int i;
   // open logging
   openlog("cvfs2", LOG_PID|LOG_CONS, LOG_USER);
   system("clear");

   if (argc == 2){
	if (strcmp(argv[1], "init") == 0){
		initialize();
	}
   }

 //  initialize();
   while(1){
      //pthread_create(&t[0], NULL, create_link, NULL);
     pthread_create(&t[0], NULL, watch_temp, NULL);
      pthread_create(&t[1], NULL, watch_share, NULL);
      // pthread_create(&t[2], NULL, create_link, NULL);

      for (i = 0; i < THREADCNT; i++){
         pthread_join(t[i], NULL);
      }
   }


}
