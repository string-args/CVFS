#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sqlite3.h>
#include <syslog.h>

#include "../Global/global_definitions.h"
#include "../disk_pooling/file_presentation.h"
#include "../cache_access/cache_operation.h"
#include "file_mapping.h"

void update_target_size_delete(String filename, String fileloc){
    int rc;

    char *err = 0;
    //sqlite3 *db;
    sqlite3_stmt *res;
    sqlite3_stmt *res1;
    sqlite3 *db;
    const char *tail;

    double avspace;
    String sql;

    rc = sqlite3_open(DBNAME, &db);
    if (rc != SQLITE_OK){
	fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    //syslog(LOG_INFO, "VolumeManagement: INSIDE UPDATE TARGET SIZE DELETE FUNCTION!\n");

    String query;
    sprintf(query, "SELECT avspace FROM target WHERE mountpt = '%s';", fileloc);
    //syslog(LOG_INFO, "VolumeManagement: QUERY = %s\n", query);
    rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
    if (rc != SQLITE_OK){
       syslog(LOG_INFO, "VolumeManagement: Error: Prepare V2!\n");
    }

    if (sqlite3_step(res) == SQLITE_ROW){

       //sqlite3_stmt *res1;
       String query1;
       sprintf(query1, "SELECT filesize FROM VolContent where filename = '%s' and fileloc = '%s';", filename,  fileloc);
       //syslog(LOG_INFO, "VolumeManagement: QUERY1 = %s\n", query1);
       rc = sqlite3_prepare_v2(db, query1, 1000, &res1, &tail);
       if (sqlite3_step(res1) == SQLITE_ROW){

       		//syslog(LOG_INFO, "VolumeManagement: File Size: %lf\n", sqlite3_column_double(res1,0));
       		avspace = sqlite3_column_double(res, 0);
       		avspace = avspace + sqlite3_column_double(res1,0);

       		sprintf(sql, "Update Target set avspace = %lf where mountpt = '%s';", avspace, fileloc);
       		//syslog(LOG_INFO, "VolumeManagement: SQL = %s\n", sql);
       	        rc = sqlite3_exec(db, sql, 0, 0, &err);
       		if (rc != SQLITE_OK){
          	   syslog(LOG_INFO, "VolumeManagement: Error: Didn't Update Target Size on Delete! %s\n", err);
       		}
	}
	//sqlite3_finalize(res1);
    }

    sqlite3_finalize(res);
    sqlite3_finalize(res1);

    //char *err;

    //rc = sqlite3_open(DBNAME, &db);
    //if (rc != SQLITE_OK){
    //   syslog(LOG_INFO, "VolumeManagement: Can't open database: %s\n", sqlite3_errmsg(db));
    //}
/*
    sprintf(sql, "Update Target set avspace = %lf where mountpt = '%s';", avspace, fileloc);
    syslog(LOG_INFO, "VolumeManagement: SQL = %s\n", sql);
    rc = sqlite3_exec(db, sql, 0, 0, &err);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Error: Didn't Update Target Size on Delete! %s\n", err);
    }*/

    //sqlite3_close(db);
}

void update_target_size(sqlite3 *db, String filename, const char* fileloc){
    int rc;

    char *err = 0;
    const char *tail;

    String file;
    String sql;

    sqlite3_stmt *res;

    strcpy(file,fileloc);
    strcat(file,"/");
    strcat(file,filename);

    //get file size of mountpt
    sprintf(sql, "SELECT avspace FROM Target WHERE mountpt = '%s'", fileloc);
    rc = sqlite3_prepare_v2(db, sql, 1000, &res, &tail);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Update(VolContent): SQL Error: %s\n", sqlite3_errmsg(db));
       sqlite3_close(db);
       exit(1);
    }

    if (sqlite3_step(res) == SQLITE_ROW){
       //do the operation here: you already retrieved the space of the mountpt
       double avspace = sqlite3_column_double(res,0);
       FILE *fp = fopen(file, "rb");
       fseek(fp, 0L, SEEK_END);
       double sz = ftell(fp); //get filesize of file
       rewind(fp);
       avspace = avspace - sz;
       sprintf(sql, "UPDATE Target SET avspace = %lf WHERE mountpt = '%s';", avspace, fileloc);
    }
    // sqlite3_finalize(res);
    rc = sqlite3_exec(db, sql, 0, 0, &err);
    //syslog(LOG_INFO, "VolumeManagement: sql for update size: %s\n", sql);
    if (rc != SQLITE_OK){
       fprintf(stderr, "SQL Error: %s\n", err);
       sqlite3_free(err);
    }
    sqlite3_finalize(res);
}


//this function insert a new entry to the volcontent table
void update_list(sqlite3 *db, String filename, const char* fileloc){
    int rc;

    char *err = 0;

    double sz;
    String file_to_open;
    sprintf(file_to_open, "%s/%s", fileloc, filename);
    //syslog(LOG_INFO, "VolumeManagement: FILE: %s |", file_to_open);
    FILE *fp = fopen(file_to_open, "rb");
    fseek(fp, 0L, SEEK_END);
    sz = ftell(fp);
    rewind(fp);
    fclose(fp);
    //syslog(LOG_INFO, "VolumeManagement: SIZE : %lf\n", sz);

    String sql;
    sprintf(sql, "INSERT INTO VolContent (filename, fileloc, filesize) VALUES ('%s','%s', %lf);", filename, fileloc, sz);

    /*Database is already open*/
    rc = sqlite3_exec(db, sql, 0, 0, &err);
    if (rc != SQLITE_OK){
       fprintf(stderr, "SQL Error: %s\n", err);
       sqlite3_free(err);
    }
    update_target_size(db, filename, fileloc);
}


void file_map_share(String filename){
    int rc;
    String sql, comm;

    sqlite3 *db;
    sqlite3_stmt *res;

    char *err = 0;
    const char *tail;

    sprintf(sql, "SELECT avspace, mountpt FROM TARGET WHERE avspace = (SELECT max(avspace) FROM Target);");
    rc = sqlite3_open(DBNAME, &db);

    if (rc != SQLITE_OK){
	fprintf(stderr, "File Mapping Share: Can't open database: %s\n", sqlite3_errmsg(db));
	sqlite3_close(db);
	exit(1);
    }

    rc = sqlite3_prepare_v2(db, sql, 1000, &res, &tail);
    if (rc != SQLITE_OK){
	fprintf(stderr, "SQL Error: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    if (sqlite3_step(res) == SQLITE_ROW){
	syslog(LOG_INFO, "VolumeManagement: File %s redirected to %s\n", filename, sqlite3_column_text(res,1));
        sprintf(comm, "mv -f '%s/%s' '%s/%s'", SHARE_LOC, filename, sqlite3_column_text(res,1), filename);
	system(comm);
	update_list(db, filename, sqlite3_column_text(res,1));
    }

    sqlite3_finalize(res);
    sqlite3_close(db);
}

//this function redirects the file to targets
void file_map(String fullpath, String filename){
    int rc;

    String sql;
    String comm;

    sqlite3 *db;
    sqlite3_stmt *res;

    char *err = 0;
    const char *tail;

    strcpy(sql, "SELECT avspace, mountpt FROM TARGET WHERE avspace = (SELECT max(avspace) from Target);");

    rc = sqlite3_open(DBNAME, &db); /*Open database*/

    if (rc != SQLITE_OK){
       fprintf(stderr, "File Mapping: Can't open database: %s\n", sqlite3_errmsg(db));
       sqlite3_close(db);
       exit(1);
    }

    rc = sqlite3_prepare_v2(db, sql, 1000, &res, &tail);
    if (rc != SQLITE_OK){
       fprintf(stderr, "SQL Error: %s.\n", sqlite3_errmsg(db));
       sqlite3_close(db);
       exit(1);
    }

    if(sqlite3_step(res) == SQLITE_ROW){

       //if (getCacheCount() <= MAX_CACHE_SIZE){ //max cache size is 10
       //   file_map_cache(filename);
       //}

       syslog(LOG_INFO, "VolumeManagement: File %s is redirected to %s.\n", filename, sqlite3_column_text(res,1));
       sprintf(comm, "mv '%s' '%s/%s'", fullpath, sqlite3_column_text(res,1), filename);
       //syslog(LOG_INFO, "VolumeManagement: mv = %s\n", comm);
       system(comm);
	//this part update the entry of the file to volcontent table
       update_list(db, filename, sqlite3_column_text(res,1));
       //String ln = "";
       //sprintf(ln, "ln -s '%s/%s' '%s/%s'", sqlite3_column_text(res,1), filename, SHARE_LOC, filename);

       String sors = "", dest = "";
       if (strstr(filename, "part1.") != NULL || strstr(filename, "part") == NULL){ //for stripe file
		printf("LINKING!!!\n");
		int in_cache = 0;
		String ls = "", ls_out = "";
		sprintf(ls, "ls %s", CACHE_LOC);
		runCommand(ls,ls_out);
		char *pch = NULL;
		pch = strtok(ls_out, "\n");
		while (pch != NULL){
			if (strcmp(pch, filename) == 0){
				in_cache = 1;
				break;
			}
			pch = strtok(NULL, "\n");
		}
	
		if (!in_cache){
       		sprintf(sors, "%s/%s", sqlite3_column_text(res,1), filename);

		printf("SORS := %s\n", sors);
       		
		if (strstr(filename, "part1.") != NULL){
		        memmove(filename,filename+strlen("part1."), 1+strlen(filename + strlen("part1.")));
			 	
		}
		sprintf(dest, "%s/%s", SHARE_LOC, filename);
		
       		if(symlink(sors, dest) == 0) {
           		syslog(LOG_INFO, "VolumeManagement: Link Created: '%s'\n", dest);
       		} else {
           		syslog(LOG_INFO, "VolumeManagement: Error creating link %s\n", dest);
       		}
		}
       }
       //system(ln);
    }


    sqlite3_finalize(res);
    sqlite3_close(db);
}

void file_map_cache(String filename, String event_name){
    String cp;

    //update_cache_list(filename);
    syslog(LOG_INFO, "VolumeManagement: Copying file to cache...\n");
    sprintf(cp, "cp '%s/%s' '%s/part1.%s'", TEMP_LOC, filename, CACHE_LOC, event_name);
    system(cp); //copy file to cache
    //sprintf(rm, "rm '/mnt/CVFSTemp/%s'", filename);
    //system(rm);
}
