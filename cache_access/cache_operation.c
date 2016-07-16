#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>

#include "../Global/global_definitions.h"
#include "../Utilities/cmd_exec.h"
#include "../disk_pooling/file_presentation.h"
#include "../volume_management/file_assembly.h"
#include "cache_operation.h"

//increment the frequency of the file
void incrementFrequency(String filename){
   sqlite3 *db;

   char *errmsg = 0;
   int rc;

   String query;
   syslog(LOG_INFO, "CacheAccess: Increment frequency of file %s\n", filename);
   rc = sqlite3_open(DBNAME, &db);

   if (rc != SQLITE_OK){
      fprintf(stderr, "Can't open database :: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      exit(1);
   }

   sprintf(query, "UPDATE CacheContent SET frequency = frequency + 1 WHERE filename = '%s';", filename);

  int good = 0;
  while(!good) {
	rc = sqlite3_exec(db, query, 0, 0, &errmsg);
	if (rc != SQLITE_OK) {
		//printf("db is locked\n");
	} else {
		printf("Increment Frequency for %s\n", filename);
		good = 1;
	}
   }

   sqlite3_close(db);
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

   int good = 0;
   while (!good){
   	rc = sqlite3_exec(db, query, 0, 0, &errmsg);
   	if (rc != SQLITE_OK){
      		good = 0;
   	} else {
		good = 1;
	}
   }
   sqlite3_close(db);
   //syslog(LOG_INFO, "CacheAccess: Inserted Successfully!\n");
}

//this function checks if a file is in cache or not
int inCache(String *list, String file){
   int i;
   for (i = 0; i < MAX_CACHE_SIZE; i++){
       printf("CONTENTS[%d] := %s | file: %s\n", i, list[i], file);
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
   printf("IN REFRESH CACHE!\n");
    // syslog(LOG_INFO, "CacheAccess: Refreshing Cache...\n");
    String comm = "", comm_out = "", file_list = "", query = "";
    String contents[20];

    //get the files in CVFSCache folder
    sprintf(comm, "ls %s", CACHE_LOC);
    //syslog(LOG_INFO, "CacheAccess: Comm = %s\n", comm);
    runCommand(comm, comm_out);
    strcpy(file_list, comm_out);

    //printf("\nFiles in Cache: %s\n", file_list);


        for (i = 0; i < 20; i++){
           strcpy(contents[i], "");
        }
  printf("Initialize array done!\n");
   // if (strcmp(file_list,"") != 0)
   // {
      printf("FILE LIST: %s\n", file_list);
	//syslog(LOG_INFO, "CacheAccess: Checking file_list...\n");
    //store the filenames in contents struct
    	char *pch = NULL;
    	pch = strtok(file_list, "\n");
    	//syslog(LOG_INFO, "CacheAccess: Printing Cache Contents:\n");
    	int j = 0;
      
        while (pch != NULL){
       		strcpy(contents[j], pch);
		
       		printf("PCH : CONTENTS[%d]: %s\n", j, contents[j]);
       		j++;
       		pch = strtok(NULL, "\n");
    	}
    printf("STRTOK DONE!!!\n");
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
    int good = 0;
     while(!good) {
	rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
	if (rc != SQLITE_OK) {
		printf("db is locked\n");
	} else {
		good = 1;
	}
      }
    //syslog(LOG_INFO, "CacheAccess: Query := %s\n", query);

   printf("LOCKED DONE!!!\n");
    while (sqlite3_step(res) == SQLITE_ROW){
	   String filename = "";
           strcpy(filename, sqlite3_column_text(res,0));
     
//       	printf("FILENAME := %s\n", filename);
       
           if (inCache(contents, filename)){
              syslog(LOG_INFO, "CacheAccess: %s is already in cache.\n", filename);
           } else{
              syslog(LOG_INFO, "CacheAccess: %s not in cache, transfering...\n", filename);
              printf("Cache Access: Should be assembling file here...\n");
	      //assemble_cache_file(filename);
              //String rm = "";
	      //sprintf(rm, "rm '%s/%s'", SHARE_LOC, filename);
	      //printf("cache: rm = %s\n", rm);
	      //system(rm);
              //create_link_cache(filename);
           }
    }
   printf("IN CACHELOOP DONE!!\n");
    //sqlite3_finalize(res);
    String comm1 = "";
    //remove less frequent files

    for (i = 0; i < 20; i++){
       //syslog(LOG_INFO, "CacheAccess: i = %d :: contents = %s\n", i, contents[i]);
	printf("LOOP: contents[%d] := %s\n", i, contents[i]);
       if (strcmp(contents[i], "") != 0){

	  printf("LAST LOOP : CONTENTS[%d] := %s\n", i, contents[i]);
          strcpy(comm, "");
          sprintf(comm, "rm '%s/%s'", CACHE_LOC, contents[i]);
	  printf("COMM = %s\n", comm);
          sprintf(comm1, "rm '%s/%s'", SHARE_LOC, contents[i]);
          printf("COMM1 = %s\n", comm1);
          //syslog(LOG_INFO, "CacheAccess: comm = %s\n", comm);
          system(comm);
          syslog(LOG_INFO, "CacheAccess: Removing %s in Cache...\n", contents[i]);
	  system(comm1);
	  syslog(LOG_INFO, "CacheAccess: Removing %s in Share...\n", contents[i]);
          //update link cache here....
		/*
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
	  sqlite3_finalize(res);*/
       }
    }
    sqlite3_finalize(res);
    sqlite3_close(db);
    printf("tapos na ang ref cache");
}
