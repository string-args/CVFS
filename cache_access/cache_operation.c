#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include <unistd.h>

#include "../Global/global_definitions.h"
#include "../Utilities/cmd_exec.h"
#include "../disk_pooling/file_presentation.h"
#include "../volume_management/file_assembly.h"
#include "../file_transaction/watch_share.h"
#include "cache_operation.h"

#define CLISTSIZE	20 		// number of files in cache allowed to be listed

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   int i;
   for (i = 0; i < argc; i++){
	printf("%s = %s\n", azColName[i], argv[i]?argv[i] : "NULL");
   }
   printf("\n");
   return 0;
}

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

   String pragma = "";
   strcpy(pragma, "PRAGMA journal_mode=WAL;");
   rc = sqlite3_exec(db, pragma, 0, 0, 0);

   sprintf(query, "UPDATE CacheContent SET frequency = frequency + 1 WHERE filename = '%s';", filename);

  int good = 0;
  while(!good) {
	rc = sqlite3_exec(db, query, callback, 0, &errmsg);
	if (rc != SQLITE_OK) {
		printf("Increment frequency: DB Error %s\n", sqlite3_errmsg(db));
	} else {
		//printf("Increment Frequency for %s\n", filename);
		good = 1;
	}
   }

   sqlite3_close(db);
}

//update cache content table of the newly entered file
void update_cache_list(String filename, String root){
   sqlite3 *db;
   char *errmsg = 0;
   int rc;

   String query;

   syslog(LOG_INFO, "CacheAccess: Filename: %s :: Frequency 1\n", filename);

   sprintf(query, "INSERT INTO CacheContent (filename, mountpt, frequency) VALUES ('part1.%s', '%s', 1);", filename, root);
   rc = sqlite3_open(DBNAME, &db);
   if (rc){
      fprintf(stderr, "Can't open database %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      exit(1);
   } else {
      fprintf(stderr, "UpdateCacheList(): Opened database successfully!\n");
   }

   String pragma = "";
   strcpy(pragma, "PRAGMA journal_mode=WAL;");
   rc = sqlite3_exec(db, pragma, callback, 0, &errmsg);

   int good = 0;
   while (!good){
   	rc = sqlite3_exec(db, query, callback, 0, &errmsg);
   	if (rc != SQLITE_OK){
        	//printf("Update Cache list: DB Error %s\n", sqlite3_errmsg(db));
	} else {
		good = 1;
		fprintf(stdout, "Update Cache List successfully!\n");
	}
   }
   sqlite3_close(db);
}

//this function checks if a file is in cache or not
int inCache(String *list, String file){
   int i;
   for (i = 0; i < CLISTSIZE; i++){
       if (strcmp(list[i], file) == 0){
          strcpy(list[i], "");
          return 1;
       }
   }
   return 0;
}

//refresh cache: follows least frequency
void* refreshCache(){
    printf("Refreshing Cache here...\n");
    sqlite3 *db;
    sqlite3_stmt *res;
    char *errmsg = 0;
    const char *tail;
    int rc, i;
  
    String comm = "", comm_out = "", file_list = "", query = "";
    String contents[CLISTSIZE];	// we list 20 files, assuming merong sobra
    String cache_files[CLISTSIZE];
    String cache_roots[CLISTSIZE];

    strcpy(comm, "ls /mnt/CVFSCache");

    runCommand(comm, comm_out);

    for (i = 0; i < CLISTSIZE; i++){
       strcpy(contents[i], "");
	   strcpy(cache_files[i], "");
	   strcpy(cache_roots[i], "");
    }
	char *pch = NULL;
	pch = strtok(comm_out, "\n");
	int j = 0;

    while (pch != NULL){
  		strcpy(contents[j], pch);
  		j++;
  		pch = strtok(NULL, "\n");
   	}
 
    rc = sqlite3_open(DBNAME, &db);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Can't open database %s\n", sqlite3_errmsg(db));
       sqlite3_close(db);
       exit(1);
    }

    String pragma = "";
    strcpy(pragma, "PRAGMA journal_mode=WAL;");
    rc = sqlite3_exec(db, pragma, 0, 0, 0);

    int good = 0;
    sprintf(query, "SELECT filename, mountpt FROM CacheContent;");
    while (!good){
	      rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
	      if (rc != SQLITE_OK){
		     //printf("DB Error %s\n", sqlite3_errmsg(db));
	      } else {
		    good = 1;
	      }
    }

    int counter = 0;
    while (sqlite3_step(res) == SQLITE_ROW){
	      strcpy(cache_files[counter], sqlite3_column_text(res,0));
	      strcpy(cache_roots[counter], sqlite3_column_text(res,1));
	      counter++;
    }
    sqlite3_finalize(res);
	// get the files that SHOULD be in the cache folder
    sprintf(query, "SELECT * FROM CacheContent ORDER BY frequency DESC LIMIT %d;", MAX_CACHE_SIZE);
    good = 0;
    while(!good) {
          rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
	      if (rc != SQLITE_OK) {
		     //printf("DB Error %s\n", sqlite3_errmsg(db));
	      } else {
		    good = 1;
	      }
    }

    // for each of the SELECTED files, transfer the appropriate files in the cache
    while (sqlite3_step(res) == SQLITE_ROW){
	   String filename = "", file = "";
	   String root = "";
       strcpy(filename, sqlite3_column_text(res,0));
       strcpy(file, sqlite3_column_text(res,0));
       strcpy(root, sqlite3_column_text(res,1));

       if (inCache(contents, filename)){
              syslog(LOG_INFO, "CacheAccess: %s is already in cache.\n", filename);
       } else{
              syslog(LOG_INFO, "CacheAccess: %s not in cache, transfering...\n", filename);
              printf("Cache Access: Should be assembling file here...\n");
              String rm = "";
	          FILE *fp = fopen("cache_assembling.txt", "w");
	          fprintf(fp, "1");
    	      fclose(fp);
	          assemble_cache_file(filename, root);
              fp = fopen("cache_assembling.txt", "w");
	          fprintf(fp, "0");
              fclose(fp);
   
	          String temp = "";
	          strcpy(temp, file);
              memmove(file, file+strlen("part1."), 1+strlen(file+strlen("part1.")));

	          String update = "";
    	      sprintf(update, "ln -snf '%s/%s' '%s/%s%s'", CACHE_LOC, temp, SHARE_LOC, root, file);
	          system(update);

        }
    }

    sqlite3_finalize(res);
    String comm1 = "";
    //remove less frequent files
	// check all the contents of cache folder (not necessarily of size MAX_CACHE_SIZE
    for (i = 0; i < CLISTSIZE; i++){
	  String filename = "";
	  strcpy(filename, contents[i]);
      if (strcmp(filename, "") != 0){
	  
          strcpy(comm, "");
          sprintf(comm, "rm -rf '%s/%s'", CACHE_LOC, filename);

          int i = 0;
          String root = "";
          for (i = 0; i < counter; i++){
	      if (strcmp(cache_files[i], filename) == 0){
             strcpy(root, cache_roots[i]);
		     break;
          }
     }
	  //update link here...
	  sqlite3_stmt *res2;
	  String file = "", mountpt = "";
	  sprintf(file, "%s%s", root, filename);
      String sql = "";
	  sprintf(sql, "SELECT fileloc from volcontent where filename = '%s';", file);
	  good = 0;
	  while (!good){
	  	rc = sqlite3_prepare_v2(db, sql, 1000, &res2, &tail);
	  	if (rc == SQLITE_OK){
			good = 1;
		} else {
			//printf("locked!\n");
		}
	  }

	  if (sqlite3_step(res2) == SQLITE_ROW){
		strcpy(mountpt, sqlite3_column_text(res2,0));
	  }
	  sqlite3_finalize(res2);

	  String ln = "";
	  String name1 = "";
	  strcpy(name1, file);
	  strcpy(name1, replace_str(name1,"part1.",""));
	  
	  String source = "", dest = "";
	  sprintf(source, "%s/%s", mountpt, file);
	  sprintf(dest, "%s/%s", SHARE_LOC, name1);

      sprintf(ln, "ln -snf '%s' '%s'", source, dest);
	  system(ln);
	  system(ln);
	  system(comm);

      syslog(LOG_INFO, "CacheAccess: Removing %s in Cache...\n", filename);
	  syslog(LOG_INFO, "CacheAccess: Removing %s in Share....\n", filename);
      }
 }
   
    sqlite3_close(db);
    printf("Done Refreshing Cache.\n");

}
