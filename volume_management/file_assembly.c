#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <syslog.h>

#include "../Global/global_definitions.h"
#include "file_assembly.h"

#define ASSEMBLY_LOC "/mnt/"
#define STORAGE_LOC "/mnt/CVFStore/"

void assemble_cache_file(String filename){
   char *err = 0;

   String files, comm, query, tempname;
   String assfile;

   sqlite3 *db;
   sqlite3_stmt *res;

   char percent = '%';
   const char *tail;
   int rc;

   syslog(LOG_INFO, "VolumeManagement: Assembling file to be put in cache...\n");
   strcpy(tempname, filename);
   if (strstr(filename, "part1.") != NULL){
       memmove(filename, filename + strlen("part1."), 1 + strlen(filename + strlen("part1.")));
   }

    sprintf(query, "SELECT filename, fileloc FROM VolContent WHERE filename LIKE 'part%c.%s'", percent,filename);
   // syslog(LOG_INFO, "VolumeManagement: query = %s\n", query);

   rc = sqlite3_open(DBNAME, &db);
   if (rc != SQLITE_OK){
      fprintf(stderr, "File Assembly: Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      exit(1);
   }

   rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
   if (rc != SQLITE_OK){
      fprintf(stderr, "Didn't get any data.\n");
      exit(1);
   }

   strcpy(files, "");
   strcpy(assfile, "");
   while (sqlite3_step(res) == SQLITE_ROW){

       if (strstr(sqlite3_column_text(res,0), "part1.") != NULL){
          //if file contains part1
          strcpy(assfile, sqlite3_column_text(res,0)); //will be the filename
       }

       String name;
       sprintf(name, "\"%s/%s\"", sqlite3_column_text(res,1), sqlite3_column_text(res,0));
       strcat(files, name);
       strcat(files, " ");
   }
   sqlite3_finalize(res);
   sqlite3_close(db);
   sprintf(comm, "cat %s > '%s%s'", files, ASSEMBLY_LOC, assfile);
   //syslog(LOG_INFO, "VolumeManagement: comm = %s\n", comm);

  //syslog(LOG_INFO, "VolumeManagement: PART1 OF THE FILE: %s/%s", ftemploc,ftemp);
   system(comm);
   syslog(LOG_INFO, "VolumeManagement: Assembled File: %s%s\n", ASSEMBLY_LOC, assfile);

   String put_cache;
   sprintf(put_cache, "mv '%s%s' '%s/%s'", ASSEMBLY_LOC, assfile, CACHE_LOC, assfile);
   //syslog(LOG_INFO, "VolumeManagement: MV = %s\n", put_cache);
   system(put_cache);
   printf("%s was put in cache.\n", assfile);
}

void assemble(String filename){
   char *err = 0;

   String files, comm, query, tempname;
   String assfile;

   int rc;

   sqlite3 *db;
   sqlite3_stmt *res;

   char percent = '%';
   const char *tail;

   strcpy(tempname, filename);
   /*Assumes only part1 of the file will be linked to share*/
   /*The part1 name in the share would be part1.<filename>*/
   /*All the fileparts in the database would be saved as part<number>.<filename>*/
   /*This checks if part1 of a certain file is open*/
   /*remove part1. in the filename*/
   /*ex: part1.DOTS.mkv = DOTS.mkv*/
   if (strstr(filename, "part1.") != NULL){
     memmove(filename, filename + strlen("part1."), 1 + strlen(filename + strlen("part1.")));
   }

   /*Look for all the fileparts in the database. part<number>.<filename>*/
   sprintf(query, "SELECT filename, fileloc FROM VolContent WHERE filename LIKE 'part%c.%s'", percent,filename);
   // syslog(LOG_INFO, "VolumeManagement: query = %s\n", query);

   rc = sqlite3_open(DBNAME, &db);
   if (rc != SQLITE_OK){
      fprintf(stderr, "File Assembly: Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      exit(1);
   }

   rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
   if (rc != SQLITE_OK){
      fprintf(stderr, "Didn't get any data.\n");
      exit(1);
   }

   strcpy(files, "");
   strcpy(assfile, "");
   while (sqlite3_step(res) == SQLITE_ROW){

       if (strstr(sqlite3_column_text(res,0), "part1.") != NULL){
          //if file contains part1
          strcpy(assfile, sqlite3_column_text(res,0)); //will be the filename
       }

       String name;
       sprintf(name, "\"%s/%s\"", sqlite3_column_text(res,1), sqlite3_column_text(res,0));
       strcat(files, name);
       strcat(files, " ");
   }
   sqlite3_finalize(res);

   sprintf(comm, "cat %s > '%s%s'", files, ASSEMBLY_LOC, assfile);
   // syslog(LOG_INFO, "VolumeManagement: comm = %s\n", comm);

  //syslog(LOG_INFO, "VolumeManagement: PART1 OF THE FILE: %s/%s", ftemploc,ftemp);
   system(comm);
   syslog(LOG_INFO, "VolumeManagement: Assembled File: %s%s\n", ASSEMBLY_LOC, assfile);

   //file is already assembled
   //move original part1 of the file to CVFStore folder
   String sql;
   sprintf(sql, "SELECT fileloc FROM VolContent WHERE filename = '%s'", tempname);
   // syslog(LOG_INFO, "VolumeManagement: SQL = %s", sql);

   rc = sqlite3_prepare_v2(db, sql, 1000, &res, &tail);
   if (rc != SQLITE_OK){
     fprintf(stderr, "Didn't get any data!\n");
     exit(1);
   }

   if (sqlite3_step(res) == SQLITE_ROW){
      String mv, cp;
      //copy original part1 of file to CVFStore folder
      sprintf(cp, "cp '%s/%s' '%s%s'", sqlite3_column_text(res,0), tempname, STORAGE_LOC, tempname);
      //move assembled file
      sprintf(mv, "mv '%s%s' '%s/%s'", ASSEMBLY_LOC, tempname, sqlite3_column_text(res,0), tempname);
      printf("\nCP = %s\n", cp);
      printf("\nMV = %s\n", mv);
      system(cp);
      system(mv);
   }
   sqlite3_finalize(res);
  //String comm1 = "";
  //sprintf(comm1, "ln -snf '%s%s' '%s/%s'", ASSEMBLY_LOC, filename, SHARE_LOC, ftemp);
  //syslog(LOG_INFO, "VolumeManagement: comm1 = %s\n", comm1);

  // system(comm1);

   sqlite3_close(db);
}
