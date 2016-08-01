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

   printf("query = %s\n", query);

   int good = 0;
   while (!good){
   	rc = sqlite3_exec(db, query, callback, 0, &errmsg);
   	if (rc != SQLITE_OK){
        //rc = sqlite3_open(DBNAME, &db);
        	//printf("Update Cache list: DB Error %s\n", sqlite3_errmsg(db));
   		//sqlite3_free(errmsg);
	} else {
		good = 1;
		fprintf(stdout, "Update Cache List successfully!\n");
	}
   }
   printf("done with update cache!\n");
   sqlite3_close(db);
   //syslog(LOG_INFO, "CacheAccess: Inserted Successfully!\n");
}

//this function checks if a file is in cache or not
int inCache(String *list, String file){
   int i;
   for (i = 0; i < 11; i++){
       //printf("CONTENTS[%d] := %s | file: %s\n", i, list[i], file);
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
/*
void refreshCache(){

	printf("REFRESH CACHE\n");

	String cache_contents[10];
	String contents_not_to_be_remove[10];
	String ls = "", ls_out = "", query = "";

	sqlite3 *db;
	sqlite3_stmt *res;
	const char *tail;
	int rc = 0;
	int counter = 0;
	int i = 0;

	//initialize arrays
	for (i = 0; i < 10; i++){
		strcpy(cache_contents[i], "");
		strcpy(contents_not_to_be_remove[i], "");
	}

	sprintf(ls, "ls %s", CACHE_LOC);
	runCommand(ls, ls_out);

	char *pch = strtok(ls_out, "\n");
	while (pch != NULL){
		strcpy(cache_contents[counter], pch);
		counter++;
		pch = strtok(NULL, "\n");
	}

	sprintf(query, "SELECT filename FROM CacheContent ORDER BY frequency DESC LIMIT %d", MAX_CACHE_SIZE);

	rc = sqlite3_open(DBNAME, &db);
	if (rc != SQLITE_OK){
		fprintf(stderr, "RefreshCache(): Can't open database %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	int good = 0;
	while (!good){
		rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
		if (rc != SQLITE_OK){ //database is still locked
			good = 0;
		} else {
			good = 1;
		}
	}
	counter = 0;
	while (sqlite3_step(res) == SQLITE_ROW){
		strcpy(contents_not_to_be_remove[counter], sqlite3_column_text(res,0));
		counter++;
	}

	sqlite3_finalize(res);

	i = 0;
	int j = 0;
	for (i = 0; i < 10; i++){
		for (j = 0; j < 10; j++){
			if (strcmp(cache_contents[i], contents_not_to_be_remove[j]) == 0){
				strcpy(cache_contents[i], "");
			}
		}
	}

	i = 0;
	for (i = 0; i < 10; i++){
		if (strcmp(cache_contents[i], "") != 0){
			//should be remove
			printf("%s removed from cache.\n", cache_contents[i]);
			String query = "", fileloc = "";
			sprintf(query, "SELECT mountpt FROM CacheContent where filename = '%s'", cache_contents[i]);
			rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
			if (sqlite3_step(res) == SQLITE_ROW){
				strcpy(fileloc, sqlite3_column_text(res,0));
			}
			sqlite3_finalize(res);
			String linkname = "", filename = "", source = "", dest = "";
			strcpy(filename, cache_contents[i]);
			memmove(filename,filename+strlen("part1."),1+strlen(filename+strlen("part1.")));
			sprintf(linkname, "%s/%s%s", SHARE_LOC, fileloc, filename);
			printf("LINKNAME := %s\n", linkname);
			String rm = "";
			sprintf(rm, "rm '%s'", linkname);
			system(rm);

			printf("RM := %s\n", rm);

			sprintf(rm, "rm '%s/%s'", CACHE_LOC, cache_contents[i]);
			system(rm);			printf("RM := %s\n", rm);

			create_link();
		}
	}

	sqlite3_close(db);
}*/


//refresh cache: follows least frequency
void* refreshCache(){
    sqlite3 *db;
    sqlite3_stmt *res;
    char *errmsg = 0;
    const char *tail;
    int rc, i;
   //printf("IN REFRESH CACHE!\n");
    // syslog(LOG_INFO, "CacheAccess: Refreshing Cache...\n");
    String comm = "", comm_out = "", file_list = "", query = "";
    String contents[CLISTSIZE];	// we list 20 files, assuming merong sobra
    String cache_files[CLISTSIZE];
    String cache_roots[CLISTSIZE];

    //printf("REFRESH CACHE\n");
    //get the files in CVFSCache folder

    //printf("\n\nLS CACHE_LOC\n");
    //system("ls /mnt/CVFSCache");
    //printf("\n\n");

   int flag = 1;
   FILE *fp = fopen("../file_transaction/flag.txt", "wb");
   fprintf(fp, "%d", flag);
   fclose(fp);

    //printf("COMM_OUT:=\n");
    strcpy(comm, "ls /mnt/CVFSCache");
    //sprintf(comm, "ls %s", CACHE_LOC);
    //syslog(LOG_INFO, "CacheAccess: Comm = %s\n", comm);

    //system(comm);
    //printf("\n\n");

    runCommand(comm, comm_out);
    //strcpy(file_list, comm_out);

    //printf("\nFiles in Cache: %s\n", file_list);


        for (i = 0; i < CLISTSIZE; i++){
           strcpy(contents[i], "");
	   strcpy(cache_files[i], "");
	   strcpy(cache_roots[i], "");
        }
      //printf("Initialize array done!\n");
      //printf("FILE LIST: %s\n", file_list);
	//syslog(LOG_INFO, "CacheAccess: Checking file_list...\n");
    //store the filenames in contents struct

	//printf("Files in Cache...\n %s\n\n\n", comm_out);
	char *pch = NULL;
    	pch = strtok(comm_out, "\n");
    	//syslog(LOG_INFO, "CacheAccess: Printing Cache Contents:\n");
    	int j = 0;

        while (pch != NULL){
       		strcpy(contents[j], pch);
       		//printf("PCH : CONTENTS[%d]: %s\n", j, contents[j]);
       		j++;
       		pch = strtok(NULL, "\n");
    	}
    	// j now contains the total number of files in the cache folder

    //opens database
    //syslog(LOG_INFO, "CacheAccess: RefreshCache(): Opening database %s\n", DBNAME);
    rc = sqlite3_open(DBNAME, &db);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Can't open database %s\n", sqlite3_errmsg(db));
       sqlite3_close(db);
       exit(1);
    }

    String pragma = "";
    strcpy(pragma, "PRAGMA journal_mode=WAL;");
    rc = sqlite3_exec(db, pragma, 0, 0, 0);
//while(1){
    //store all the files in cachecontent and get their mountpoints
    //this is another solution to the code below where you select mountpt where filename = filename
    //i did this because the database didn't unlock
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
    //syslog(LOG_INFO, "CacheAccess: Query := %s\n", query);

    // for each of the SELECTED files, transfer the appropriate files in the cache
    while (sqlite3_step(res) == SQLITE_ROW){
	   String filename = "", file = "";
	   String root = "";
       strcpy(filename, sqlite3_column_text(res,0));
       strcpy(file, sqlite3_column_text(res,0));
       strcpy(root, sqlite3_column_text(res,1));

//       	printf("FILENAME := %s\n", filename);
       	   //printf("Filename to be checked: %s In Cache or Not?\n", filename);
           if (inCache(contents, filename)){
              syslog(LOG_INFO, "CacheAccess: %s is already in cache.\n", filename);
           } else{
              syslog(LOG_INFO, "CacheAccess: %s not in cache, transfering...\n", filename);
              printf("Cache Access: Should be assembling file here...\n");
	      //assemble_cache_file(filename);	// AIDZ PA CHECK YUNG TAMANG CALL SA PAG ASSEMBLE
              String rm = "";
              // remove the part1, kasi sa share walang part1

	      //String file_to_be_assembled = "";
	      //sprintf(file_to_be_assembled, "%s%s", root, filename);
	      FILE *fp = fopen("is_assembling.txt", "w");
	      fprintf(fp, "1");
	      fclose(fp);
	      assemble_cache_file(filename, root);
              fp = fopen("is_assembling.txt", "w");
	      fprintf(fp, "0");
              fclose(fp);
   
	      String temp = "";
	      strcpy(temp, file);
              memmove(file, file+strlen("part1."), 1+strlen(file+strlen("part1.")));
	      //sprintf(rm, "rm '%s/%s%s'", SHARE_LOC, root, file);
	      //printf("cache: rm = %s\n", rm);	// might have error here if root does not have a / at the end, so we print to know
	      
              //printf("[-] %s: %s%s", SHARE_LOC, root, file);

	      String update = "";
	      sprintf(update, "ln -snf '%s/%s' '%s/%s%s'", CACHE_LOC, temp, SHARE_LOC, root, file);
	      printf("update link = %s\n", update);
	      system(update);
	      

		//printf("IN REFRESH CACHE!\n");
	      //system(rm);
		//exit(1);
	      // commented code below, since file_presentation should auto create link?
	      //create_link();		// uncommented already because its aidz code
              //create_link_cache(filename);
           }
    }
   //printf("IN CACHELOOP DONE!!\n");
    sqlite3_finalize(res);
    //sqlite3_close(db);
    String comm1 = "";
    //remove less frequent files


   // rc = sqlite3_open(DBNAME, &db);
  //  if (rc != SQLITE_OK){
//	fprintf(stderr, "RefreshCache() : Can't open database: %s\n", sqlite3_errmsg(db));
//	sqlite3_close(db);
	//exit(1);
   // }

	// check all the contents of cache folder (not necessarily of size MAX_CACHE_SIZE
    for (i = 0; i < CLISTSIZE; i++){
       //syslog(LOG_INFO, "CacheAccess: i = %d :: contents = %s\n", i, contents[i]);
	//printf("LOOP: contents[%d] := %s\n", i, contents[i]);
	//printf("CONTENTS[%d] := %s\n", i, contents[i]);
	  String filename = "";
	  strcpy(filename, contents[i]);

       if (strcmp(filename, "") != 0){
	  //printf("LAST LOOP : CONTENTS[%d] := %s\n", i, contents[i]);
          strcpy(comm, "");
          sprintf(comm, "rm -rf '%s/%s'", CACHE_LOC, filename);
	  printf("COMM (rm from cache) = %s\n", comm);

          int i = 0;
          String root = "";
          for (i = 0; i < counter; i++){
	     if (strcmp(cache_files[i], filename) == 0){
		strcpy(root, cache_roots[i]);
		break;
	     }
          }

	  //String comm1 = "";
	  //sprintf(comm1, "rm -rf '%s/%s%s'", SHARE_LOC,root,filename);
	  //printf("comm (remove from share) = %s\n", comm1);

	  //update link here...
	  sqlite3_stmt *res2;
	  String file = "", mountpt = "";
	  sprintf(file, "%s%s", root, filename);
          String sql = "";
	  sprintf(sql, "SELECT fileloc from volcontent where filename = '%s';", file);
	  printf("sql = %s\n", sql);
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
		printf("mountpt = %s\n", mountpt);
	  }
	  sqlite3_finalize(res2);

	  String ln = "";
	  String name1 = "";
	  strcpy(name1, file);
	  //strcpy(name1, replace_str(file,"part1.",""));
	  strcpy(name1, replace_str(name1,"part1.",""));
	  sprintf(ln, "ln -snf '%s/%s' '%s/%s'", mountpt, file, SHARE_LOC, name1);
	  printf("ln = %s\n", ln);
	  system(ln);

	  //system(comm1);
	  //create_link();
	  system(comm);

          syslog(LOG_INFO, "CacheAccess: Removing %s in Cache...\n", filename);
	  //system(comm1);
	  syslog(LOG_INFO, "CacheAccess: Removing %s in Share....\n", filename);
      }
  // }
   //printf("tapos na ang ref cache");    
 }
   //sqlite3_finalize(res);
    sqlite3_close(db);
    printf("tapos na ang ref cache");

      
   // exit(1);
}
