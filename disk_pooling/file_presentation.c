#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <syslog.h>

#include "../Global/global_definitions.h"
#include "../Utilities/cmd_exec.h"
#include "file_presentation.h"

static int callback(void *notUsed, int argc, char **argv, char **colname){
   int i;
   String comm;

   if (argc != 2){
      fprintf(stderr, "Error in database table: missing columns\n");
      exit(1);
   } else {
      //link only the part1

/*

	String filename = "";
	String usename = "";
	strcpy(filename, argv[0]);
	char *p;
	p = strtok(filename, "/");
	if(p == NULL) {
	    strcpy(usename, argv[0]);
	} else {
	    strcpy(usename, p);
	}
	//syslog(LOG_INFO, "DiskPooling: usename = %s\n", usename);

	int exist = 0;
	String ls = "", ls_out = "";
	strcpy(ls, "ls /mnt/Share/");
	runCommand(ls,ls_out);
	char *ptr1 = strtok(ls_out, "\n");
	while (ptr1 != NULL){
		if (strcmp(ptr1,usename) == 0){
			exist = 1;
			break;
		}
		ptr1 = strtok(NULL, "\n");
	}
	//strcpy(argv[0], usename);
	//syslog(LOG_INFO, "DiskPooling: exist = %d\n", exist);
	if (!exist){
      if (strstr(usename,"part1.") != NULL){
        // sprintf(comm, "ln -s '%s/%s' '%s/%s'", argv[1], usename, SHARE_LOC, usename);

        String sors = "", dest = "";
		sprintf(sors, "'%s/%s'", argv[1], usename);
		sprintf(dest, "'%s/%s'", SHARE_LOC, usename);
        if(symlink(sors, dest) == 0) {
            syslog(LOG_INFO, "DiskPooling: Link Created: '%s'\n", dest);
        } else {
            printf("!!! Error creating link %s", dest);;
        }
        // system(comm);
      } else if (strstr(usename, "part1.") == NULL){ //link linear files
        // sprintf(comm, "ln -s '%s/%s' '%s/%s'", argv[1], usename, SHARE_LOC, usename);
        // syslog(LOG_INFO, "DiskPooling: Link Created: '/mnt/Share/%s'\n", usename);
        String sors = "", dest = "";
        sprintf(sors, "'%s/%s'", argv[1], usename);
		sprintf(dest, "'%s/%s'", SHARE_LOC, usename);
        if(symlink(sors, dest) == 0) {
            syslog(LOG_INFO, "DiskPooling: Link Created: '%s'\n", dest);
        } else {
            printf("!!! Error creating link %s", dest);
        }
        // syslog(LOG_INFO, "DiskPooling: COMM = %s\n", comm);
    	// system(comm);
      } }
      //sprintf(comm, "ln -s '%s/%s' '%s/%s'", argv[1], argv[0], SHARE_LOC, argv[0]);
      //syslog(LOG_INFO, "DiskPooling: Link Created: '/mnt/Share/%s'\n", argv[0]);
      //syslog(LOG_INFO, "DiskPooling: comm = %s\n", comm);
      //system(comm);*/
   }
   return 0;
}

void* create_link(){
   int tcount = 0;
   int rc;

   char *err = 0;

   String file_list, comm, query, comm_out;

   sqlite3 *db;


   rc = sqlite3_open(DBNAME, &db);
   if (rc != SQLITE_OK){
     fprintf(stderr, "File Presentation: Can't open database: %s.\n", sqlite3_errmsg(db));
     sqlite3_close(db);
     exit(1);
   }

   /*get current contents of share folder*/
   sprintf(comm, "ls %s", SHARE_LOC);

   strcpy(file_list, "");
   strcpy(comm_out, "");
   //syslog(LOG_INFO, "DiskPooling: File Presentation:");
   runCommand(comm, comm_out);

   char *ptr = strtok(comm_out, "\n");

   while (ptr != NULL){
      strcat(file_list,"\"");
      strcat(file_list,ptr);
      strcat(file_list,"\"");
      ptr = strtok(NULL, "\n");
      if (ptr != NULL){
        strcat(file_list,",");
      }
   }

   //sprintf(file_list, "%s", comm_out);
   //syslog(LOG_INFO, "DiskPooling: File List: %s\n", file_list);

   sprintf(query, "SELECT filename, fileloc FROM VolContent WHERE filename NOT IN (%s);", file_list);

   //syslog(LOG_INFO, "DiskPooling: Query = %s\n", query);


   rc = sqlite3_exec(db, query, callback, 0, &err);
   if (rc != SQLITE_OK){
      fprintf(stderr, "SQL Error: %s.\n", err);
      sqlite3_free(err);
   }

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
