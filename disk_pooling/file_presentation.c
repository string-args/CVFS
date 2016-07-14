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
   int inCache = 0;

   sqlite3 *db;
   sqlite3_stmt *res;

   rc = sqlite3_open(DBNAME, &db);
   if (rc != SQLITE_OK){
     fprintf(stderr, "File Presentation: Can't open database: %s.\n", sqlite3_errmsg(db));
     sqlite3_close(db);
     exit(1);
   }

   sprintf(comm, "ls %s", CACHE_LOC);
   runCommand(comm, comm_out);
   char *pch = strtok(comm_out, "\n");

   sprintf(query, "SELECT filename, fileloc FROM VolContent;");

   rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);

   while (sqlite3_step(res) == SQLITE_ROW){

	while (pch != NULL){
		if (strcmp(sqlite3_column_text(res,0),pch) == 0){
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
				String part = "";
				strcpy(part, sqlite3_column_text(res,0));
				memmove(part, part + strlen("part1."), 1 + strlen(part + strlen("part1.")));
				sprintf(dest, "%s/%s", SHARE_LOC, part);
			} else {
				sprintf(dest, "%s/%s", SHARE_LOC, sqlite3_column_text(res,0));
			}

			if (symlink(source,dest) == 0){
				syslog(LOG_INFO, "File Presentation: Created Link: %s\n", dest);
			}else {	}
		}
	}
   }

   sqlite3_finalize(res);
   sqlite3_close(db);
}


void create_link_cache(String filename){
   // String ln;

   // sprintf(ln, "ln -s '%s/part1.%s' '%s/part1.%s'", CACHE_LOC, filename, SHARE_LOC, filename);
    String sors = "", dest = "";
    sprintf(sors, "%s/part1.%s", CACHE_LOC, filename);
    sprintf(dest, "%s/%s", SHARE_LOC, filename);
    if(symlink(sors, dest) == 0) {
        syslog(LOG_INFO, "DiskPooling: Link Created: '%s'\n", dest);
    } else {
        printf("!!! Error creating link %s", dest);
    }
   // syslog(LOG_INFO, "DiskPooling: ln(dest) := %s\n", dest);
   // system(ln);
   //syslog(LOG_INFO, "DiskPooling: Created Link for file in Cache: %s\n" , filename);
}


void update_link_cache(String filename, String fileloc){
    String ln, rm;

    //sprintf(rm, "rm '/mnt/Share/%s'", filename);
    //syslog(LOG_INFO, "DiskPooling: Rm = %s\n", rm);
    //system(rm);
    // sprintf(ln, "ln -s '%s/%s' '%s/%s'", fileloc, filename, SHARE_LOC, filename);
    //syslog(LOG_INFO, "DiskPooling: LN = %s\n", ln);
    // system(ln);
    String sors = "", dest = "";
	sprintf(sors, "'%s/%s'", fileloc, filename);
	sprintf(dest, "'%s/%s'", SHARE_LOC, filename);
    if(symlink(sors, dest) == 0) {
        syslog(LOG_INFO, "DiskPooling: Link Created: '%s'\n", dest);
    } else {
        printf("!!! Error creating link %s", dest);
    }

    //syslog(LOG_INFO, "DiskPooling: Update Link for file: %s\n", filename);
}
