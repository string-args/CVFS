#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <syslog.h>

#include "../Global/global_definitions.h"
#include "../Utilities/cmd_exec.h"
#include "file_presentation.h"

void* create_link(){
   int rc;
   const char *tail;

   String query = "", comm = "", comm_out = "";
   
   sqlite3 *db;
   sqlite3_stmt *res;

   rc = sqlite3_open(DBNAME, &db);
   if (rc != SQLITE_OK){
     fprintf(stderr, "File Presentation: Can't open database: %s.\n", sqlite3_errmsg(db));
     sqlite3_close(db);
     exit(1);
   }

   String pragma = "";
   strcpy(pragma, "PRAGMA journal_mode=WAL;");
   rc = sqlite3_exec(db, pragma, 0, 0,0);

   while(1){
       String comm = "", comm_out = "";
       sprintf(comm, "ls %s", CACHE_LOC);
       runCommand(comm, comm_out);

       sprintf(query, "SELECT filename, fileloc FROM VolContent;");
       int good = 0;
       while(!good) {
	                rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
	                if (rc != SQLITE_OK) {
		               //printf("db is locked\n");
                    } else {
		              good = 1;
	                }
       }
   
       while (sqlite3_step(res) == SQLITE_ROW){
	
	         int inCache = 0;
             String orig = "";
             sprintf(orig, "%s", comm_out);

	         String file = "";
	         strcpy(file, sqlite3_column_text(res,0));
	         char *pch1 = strtok(file,"/");
	         String x = "", dir = "";
	         int folder = 0;
	         while (pch1 != NULL){
		           strcat(dir, pch1);
                   strcat(dir, "/");
		           strcpy(x, pch1);
		           pch1 = strtok(NULL, "/");
		           folder = 1;
             }
	         dir[strlen(dir)-strlen(x) - 1] = '\0';

	         if (!folder){
		        strcpy(x,sqlite3_column_text(res,0));
	         } 

	         char *pch = strtok(orig, "\n");
	         while (pch != NULL){
		           if (strcmp(x,pch) == 0){
			          inCache = 1;
                      break;
                   }
		           pch = strtok(NULL, "\n");
             }
  
	         if (!inCache){
		
		        //if not in cache, link only part1 and linear file
		        if (strstr(sqlite3_column_text(res,0), "part1.") != NULL || strstr(sqlite3_column_text(res,0), "part") == NULL){
			       String source = "", dest = "";
			       sprintf(source, "%s/%s", sqlite3_column_text(res,1), sqlite3_column_text(res,0));
		
                   if (strstr(sqlite3_column_text(res,0), "part1.") != NULL){
                         memmove(x, x + strlen("part1."), 1 + strlen(x + strlen("part1.")));
				         sprintf(dest, "%s/%s%s", SHARE_LOC, dir, x);
                   } else {
			             sprintf(dest, "%s/%s", SHARE_LOC, sqlite3_column_text(res,0));
                   }
			
			       if  (access(source, F_OK) != -1){
		
                if (symlink(source,dest) == 0){
				   syslog(LOG_INFO, "File Presentation: Created Link: %s\n", dest);
				   printf("[+] %s: %s\n", SHARE_LOC, dest);
                }
			}
		}
	} else {
		if (strstr(sqlite3_column_text(res,0),"part1.") != NULL){

			String filename = "", source = "", dest = "";
			sprintf(source, "%s/%s", CACHE_LOC, x);
			memmove(x,x+strlen("part1."),1+strlen(x+strlen("part1.")));
			sprintf(dest, "%s/%s%s", SHARE_LOC, dir,x);
			if (access(source, F_OK) != -1){
			   if (symlink(source,dest) == 0){
				  syslog(LOG_INFO,"File Presentation: Created Link %s\n", dest);
				  printf("[+] %s: %s\n", SHARE_LOC, dest);
			   } 
			}
		}
	}
   }
   sqlite3_finalize(res);
 }
 sqlite3_close(db);
}

