#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <syslog.h>

#include "../Global/global_definitions.h"
#include "../Utilities/cmd_exec.h"
#include "../disk_pooling/file_presentation.h"
#include "../volume_management/file_assembly.h"
#include "cache_operation.h"

//increment the frequency of the file
void incrementFrequency(String filename){
   sqlite3 *db;

   //const char *tail;
   //sqlite3_stmt *res;

   char *errmsg = 0;
   int rc;

   String query;
   syslog(LOG_INFO, "CacheAccess: Increment frequency of file %s\n", filename);
   //syslog(LOG_INFO, "CacheAccess: Filename : %s :: Increment frequency by 1\n", filename);
   //syslog(LOG_INFO, "CacheAccess: incrementFrequency() :: Opening database %s\n", DBNAME);
   rc = sqlite3_open(DBNAME, &db);

   if (rc != SQLITE_OK){
      fprintf(stderr, "Can't open database :: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      exit(1);
   }

   sprintf(query, "UPDATE CacheContent SET frequency = frequency + 1 WHERE filename = '%s';", filename);
   //syslog(LOG_INFO, "CacheAccess: incFreq: query = %s\n", query);
   rc = sqlite3_exec(db, query, 0, 0, &errmsg);
    //syslog(LOG_INFO, "CacheAccess: incFreq: rc = %d\n", rc);
   //sqlite3_finalize(res);
   if (rc != SQLITE_OK){
     fprintf(stderr, "SQL Error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   //syslog(LOG_INFO, "CacheAccess: increment frequency is done\n");
   sqlite3_close(db);
}

//this function update the mountpoint of the file in cache
void update_cache_file_mountpt(String filename){
   sqlite3 *db;
   sqlite3_stmt *res;
   char *errmsg = 0;
   const char *tail;
   int rc;

   String query, query1;

   sprintf(query, "SELECT fileloc from VolContent where filename = 'part1.%s';", filename);

   //syslog(LOG_INFO, "CacheAccess: Query := %s\n", query);
   //sprintf(query, "UPDATE CacheContent set mountpt = '%s' where filename = '%s';", fileloc);
   rc = sqlite3_open(DBNAME, &db);
   if(rc != SQLITE_OK){
      fprintf(stderr, "Can't open database %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      exit(1);
   }

   rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
   if (rc != SQLITE_OK){
      fprintf(stderr, "Update Mounpt(): SQL Error: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      exit(1);
   }

   if (sqlite3_step(res) == SQLITE_ROW){
      sprintf(query1, "UPDATE CacheContent SET mountpt = '%s' WHERE filename = '%s';", sqlite3_column_text(res,0), filename);

    //   syslog(LOG_INFO, "CacheAccess: Filename :: %s : Mount Point : %s\n", filename, sqlite3_column_text(res,0));
   }
   sqlite3_finalize(res);

   //syslog(LOG_INFO, "CacheAccess: Query1 := %s\n", query1);
   rc = sqlite3_exec(db, query1, 0, 0, &errmsg);
   if (rc != SQLITE_OK){
      fprintf(stderr, "Update CacheContent Mountpt()... SQL Error: %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sqlite3_close(db);
   //syslog(LOG_INFO, "CacheAccess: Update Mountpt Success!\n");
}

//update cache content table of the newly entered file
void update_cache_list(String filename){
   sqlite3 *db;
   char *errmsg = 0;
   int rc;

   String query;

   syslog(LOG_INFO, "CacheAccess: Filename: %s :: Frequency 1\n", filename);

   sprintf(query, "INSERT INTO CacheContent (filename, frequency) VALUES ('part1.%s', 1);", filename);
   rc = sqlite3_open(DBNAME, &db);
   if (rc != SQLITE_OK){
      fprintf(stderr, "Can't open database %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      exit(1);
   }

   rc = sqlite3_exec(db, query, 0, 0, &errmsg);
   if (rc != SQLITE_OK){
      fprintf(stderr, "Insert Cache Content() :: SQL Error %s\n", errmsg);
      sqlite3_free(errmsg);
   }
   sqlite3_close(db);
   //syslog(LOG_INFO, "CacheAccess: Inserted Successfully!\n");
}

//this function checks if a file is in cache or not
int inCache(String *list, String file){
   int i;
   for (i = 0; i < MAX_CACHE_SIZE; i++){
       if (strcmp(list[i], file) == 0){
          strcpy(list[i], "");
          return 1;
       }
   }
   return 0;
}

//this function gets all the filecount in the cache folder
int getCacheCount(){
    //sqlite3 *db;
    //sqlite3_stmt *res;
    //const char *tail;
    //int rc;

    String comm = "", comm_out = "";
    int count = 0;

    strcpy(comm, "ls /mnt/CVFSCache");
    runCommand(comm, comm_out);

    char *pch = NULL;
    pch = strtok(comm_out, "\n");
    while (pch != NULL){
        count++;
        pch = strtok(NULL, "\n");
    }

    return count;
}

//refresh cache: follows least frequency
void refreshCache(){
    sqlite3 *db;
    sqlite3_stmt *res;
    char *errmsg = 0;
    const char *tail;
    int rc, i;

    // syslog(LOG_INFO, "CacheAccess: Refreshing Cache...\n");
    String comm = "", comm_out = "", file_list = "", query;
    String contents[MAX_CACHE_SIZE];

    //get the files in CVFSCache folder
    sprintf(comm, "ls %s", CACHE_LOC);
    //syslog(LOG_INFO, "CacheAccess: Comm = %s\n", comm);
    runCommand(comm, comm_out);
    strcpy(file_list, comm_out);

    //printf("\nFiles in Cache: %s\n", file_list);


        for (i = 0; i < MAX_CACHE_SIZE; i++){
           strcpy(contents[i], "");
        }
   // if (strcmp(file_list,"") != 0)
   // {
      //syslog(LOG_INFO, "CacheAccess: Checking file_list...\n");
    //store the filenames in contents struct
    	char *pch = NULL;
    	pch = strtok(file_list, "\n");
    	//syslog(LOG_INFO, "CacheAccess: Printing Cache Contents:\n");
    	int j = 0;
        while (pch != NULL){
       		strcpy(contents[j], pch);
       		//printf("%s\n", contents[j]);
       		j++;
       		pch = strtok(NULL, "\n");
    	}
   // }
    //opens database
    //syslog(LOG_INFO, "CacheAccess: RefreshCache(): Opening database %s\n", DBNAME);
    rc = sqlite3_open(DBNAME, &db);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Can't open database %s\n", sqlite3_errmsg(db));
       sqlite3_close(db);
       exit(1);
    }

    sprintf(query, "SELECT * FROM CacheContent ORDER BY frequency DESC LIMIT %d;", MAX_CACHE_SIZE);
    //syslog(LOG_INFO, "CacheAccess: Query := %s\n", query);
    rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
    if (rc != SQLITE_OK){
       fprintf(stderr, "SQL Error: %s\n", sqlite3_errmsg(db));
       sqlite3_close(db);
       exit(1);
    }


    while (sqlite3_step(res) == SQLITE_ROW){
       //for (i = 0; i < MAX_CACHE_SIZE; i++){
	   String filename = "";
           strcpy(filename, sqlite3_column_text(res,0));
           if (inCache(contents, filename)){
              syslog(LOG_INFO, "CacheAccess: %s is already in cache.\n", filename);
           } else{
              syslog(LOG_INFO, "CacheAccess: %s not in cache, transfering...\n", filename);
              assemble_cache_file(filename);
              String rm;
	      sprintf(rm, "rm '%s/part1.%s'", SHARE_LOC, filename);
	      system(rm);
              create_link_cache(filename);
           }
      // }
    }
    sqlite3_finalize(res);
    /* rc = sqlite3_exec(db, query, storeCont, contents, &errmsg);
    if (rc != SQLITE_OK){
       fprintf(stderr, "SQL Error: %s\n", errmsg);
       sqlite3_free(errmsg);
    }*/
    //sqlite3_close(db);
    String comm1 = "";
    //remove less frequent files
    //syslog(LOG_INFO, "CacheAccess: maxcahche size = %d\n", MAX_CACHE_SIZE);
    for (i = 0; i < MAX_CACHE_SIZE; i++){
       //syslog(LOG_INFO, "CacheAccess: i = %d :: contents = %s\n", i, contents[i]);
       if (strcmp(contents[i], "") != 0){
          strcpy(comm, "");
          sprintf(comm, "rm '%s/%s'", CACHE_LOC, contents[i]);
          sprintf(comm1, "rm '%s/%s'", SHARE_LOC, contents[i]);
          //syslog(LOG_INFO, "CacheAccess: comm = %s\n", comm);
          system(comm);
          syslog(LOG_INFO, "CacheAccess: Removing %s in Cache...\n", contents[i]);
	  system(comm1);
	  syslog(LOG_INFO, "CacheAccess: Removing %s in Share...\n", contents[i]);
          //update link cache here....

	  String query1;
	  sprintf(query1, "SELECT fileloc FROM VolContent where filename = '%s';", contents[i]);
	  rc = sqlite3_prepare_v2(db, query1, 1000, &res, &tail);
	  if (rc != SQLITE_OK){
		fprintf(stderr, "Error Prepare V2 in Update Link Cache!\n");
	  }
	  if (sqlite3_step(res) == SQLITE_ROW){
		String fileloc;
		sprintf(fileloc, "%s", sqlite3_column_text(res,0));
		update_link_cache(contents[i], fileloc);
	  }
	  sqlite3_finalize(res);
       }
    }
    sqlite3_close(db);
}
