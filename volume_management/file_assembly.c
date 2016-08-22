#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <syslog.h>

#include "../Global/global_definitions.h"
#include "file_assembly.h"

void assemble_cache_file(String filename, String root){
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

    sprintf(query, "SELECT filename, fileloc FROM VolContent WHERE filename LIKE '%spart%c.%s'", root, percent,filename);
   // syslog(LOG_INFO, "VolumeManagement: query = %s\n", query);

   rc = sqlite3_open(DBNAME, &db);
   if (rc != SQLITE_OK){
      fprintf(stderr, "File Assembly: Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      exit(1);
   }

   int good = 0;
   while (!good){
   	rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
   	if (rc != SQLITE_OK){
    		good = 0;
   	} else {good = 1;}
   }

   strcpy(files, "");
   strcpy(assfile, "");
   while (sqlite3_step(res) == SQLITE_ROW){
       String name;
       sprintf(name, "\"%s/%s\"", sqlite3_column_text(res,1), sqlite3_column_text(res,0));
       strcat(files, name);
       strcat(files, " ");
   }
   sqlite3_finalize(res);
   sqlite3_close(db);
   sprintf(comm, "cat %s > '%s/part1.%s'", files, ASSEMBLY_LOC, filename);
   system(comm);

   syslog(LOG_INFO, "VolumeManagement: Assembled File: %s/%s\n", ASSEMBLY_LOC, assfile);

   String put_cache = "";
   sprintf(put_cache, "mv '%s/part1.%s' '%s/part1.%s'", ASSEMBLY_LOC, filename, CACHE_LOC, filename);
   system(put_cache);
   printf("[+] Cache: part1.%s\n", filename);
   syslog(LOG_INFO, "VolumeManagement: part1.%s was put in cache.\n", filename);
}

// filename passed here has part1.xxxx
// we remove part1 and select the root from cachecontent
// then we assemble by checking from volcontent the locations of the parts
// that is root/part%.filename
// file is placed in assembly loc
// orig file is placed in cvfsstore
// should we update link in share or is that auto?
void assemble(String filename){
   char *err = 0;

   String comm, query, tempname;

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
   // remove part1 from filename
   //if (strstr(filename, "part1.") != NULL){
   //  memmove(filename, filename + strlen("part1."), 1 + strlen(filename + strlen("part1.")));
   //}

   // get the root (meron nang / sa dulo yun!!)
   rc = sqlite3_open(DBNAME, &db);
   if (rc != SQLITE_OK){
      fprintf(stderr, "File Assembly: Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      exit(1);
   }
   sprintf(query, "SELECT mountpt from CacheContent WHERE filename='%s'", filename);
   rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
   int good = 0;
   while (!good){
        rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
        if (rc == SQLITE_BUSY){
            // db is locked?
        } else { good = 1; }
    }
    String root = "";
    if (sqlite3_step(res) == SQLITE_ROW) {
        strcpy(root, sqlite3_column_text(res,0));
    }
    sqlite3_finalize(res);

    if (strstr(filename, "part1.") != NULL){
	memmove(filename, filename+strlen("part1."), 1+strlen(filename+strlen("part1.")));
    }

   /*Look for all the fileparts in the database. part<number>.<filename>*/
   sprintf(query, "SELECT filename, fileloc FROM VolContent WHERE filename LIKE '%spart%c.%s'", root, percent, filename);
 
   rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
   good = 0;
   while (!good) {
        rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
        if (rc == SQLITE_BUSY){
            // db is locked?
        } else { good = 1; }
    }

    String files = "";
    String assfile = "";
    String volname = "";   // for moving to store
    while (sqlite3_step(res) == SQLITE_ROW){
        if (strstr(sqlite3_column_text(res,0), "part1.") != NULL){
        //if file contains part1
            strcpy(assfile, sqlite3_column_text(res,0)); //will be the filename
            sprintf(volname, "%s/%s", sqlite3_column_text(res,1), sqlite3_column_text(res,0));
        }

        String name = "";
        sprintf(name, "\"%s/%s\"", sqlite3_column_text(res,1), sqlite3_column_text(res,0));
        strcat(files, name);
        strcat(files, " ");
    }
    sqlite3_finalize(res);

   sprintf(comm, "cat %s > '%s/part1.%s'", files, ASSEMBLY_LOC, filename);
   system(comm);
   syslog(LOG_INFO, "VolumeManagement: Assembled File: %s/%s\n", ASSEMBLY_LOC, assfile);
   // put orig file in store
   String mv = "", cp = "";
   //copy original part1 of file to CVFStore folder
   sprintf(cp, "cp '%s' '%s/%s'", volname, STORAGE_LOC, tempname);
   //move assembled file from assembly to volume with part1
   sprintf(mv, "mv '%s/part1.%s' '%s'", ASSEMBLY_LOC, filename, volname);
   system(cp);
   system(mv);

   printf("[+] %s: %s\n", STORAGE_LOC, tempname);

   FILE *fp1 = fopen("../file_transaction/assembled.txt", "a");
   fprintf(fp1, "%s\n", tempname);
   fclose(fp1);
   sqlite3_close(db);
}

void disassemble(String filename){

	sqlite3 *db;
	sqlite3_stmt *res;
	const char *tail;
	int rc;

        //printf("IN DISASSEMBLE!\n");
	rc = sqlite3_open(DBNAME, &db);
	if (rc != SQLITE_OK){
		fprintf(stderr, "Can't open database %s!\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	String query = "", root = "";
	sprintf(query, "Select mountpt from CacheContent where filename = '%s';", filename);
	int good = 0;
	while (!good){
		rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
		if (rc != SQLITE_OK){
			//printf("Disassemble: SQL Error: %s\n", sqlite3_errmsg(db));
		}else {good = 1;}
	}
	if (sqlite3_step(res) == SQLITE_ROW){
		strcpy(root, sqlite3_column_text(res,0));
	}
	sqlite3_finalize(res);

	String volume = "";
	sprintf(query, "Select fileloc from VolContent where filename = '%s%s'", root, filename);
	good = 0;
  
	while (!good){
		rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
		if (rc != SQLITE_OK){
			
		} else {good = 1;}
	}
	if (sqlite3_step(res) == SQLITE_ROW){
		strcpy(volume, sqlite3_column_text(res,0));
	}
        
	sqlite3_finalize(res);

	String mv = "";
	sprintf(mv, "mv '%s/%s' '%s/%s%s'", STORAGE_LOC, filename, volume, root, filename);
	system(mv);
	sqlite3_close(db);

	printf("[-] %s: %s\n", STORAGE_LOC, filename);

	FILE *fp1 = fopen("../file_transaction/assembled.txt", "rb");
        FILE *fp2 = fopen("../file_transaction/disassembled.txt", "ab");
        String line = "";
        while (fgets(line, sizeof(line), fp1) != NULL){
		String compare = "";
		strcpy(compare, filename);
		strcat(compare, "\n");
		if (strcmp(compare, line) != 0){ //remove assembled file from assembled txt
			fprintf(fp2, "%s", line);
		}
	}
	fclose(fp1);
	fclose(fp2);
	system("mv '../file_transaction/disassembled.txt' '../file_transaction/assembled.txt'");
}
