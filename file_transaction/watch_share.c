
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <limits.h>
#include <sqlite3.h>
#include <syslog.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>

#include "../Global/global_definitions.h"
#include "../volume_management/file_assembly.h"
#include "../volume_management/file_mapping.h"
#include "../cache_access/cache_operation.h"
#include "watch_share.h"

#define MAX_EVENTS 1024 /*Max. number of events to process at one go*/
#define LEN_NAME 32 /*Assuming that the length of filename won't exceed 16 bytes*/
#define EVENT_SIZE ( sizeof (struct inotify_event) ) /*size of one event*/
#define BUF_LEN  ( MAX_EVENTS * ( EVENT_SIZE + LEN_NAME )) /*buffer to store the data of events*/

#define MAX_WTD 200 //max is 200 watches, assumed
#define MAX_DEPTH 30
#define TARGET_NUM 30

#define MAX_PARTS 4096

//this function replace substring in a string
char *replace_str(char *str, char *orig, char *rep){
	static char buffer[4096];
	char *p;
	
	if (!(p = strstr(str, orig)))
	return str;

	strncpy(buffer, str, p-str);
	buffer[p-str] = '\0';

	sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

	return buffer;
}

//THIS FUNCTION ADD A WATCH TO ALL DIRECTORIES AND SUBDIRECTORIES
//THAT ARE ALREADY WRITTEN
void list_dir(String dir_to_read, int fd, int wds[], String dirs[], int counter){
    //counter++;
    struct dirent *de;

    DIR *dr = opendir(dir_to_read);

    if (dr == NULL){
	syslog(LOG_INFO, "FileTransaction: Could not open current directory");
    }

    while ((de = readdir(dr)) != NULL){
	struct stat s;
	String subdir = "";
	sprintf(subdir, "%s/%s", dir_to_read, de->d_name);
	stat(subdir, &s);

	switch(s.st_mode & S_IFMT){
		case S_IFDIR: 
		if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {

			//since its loop constantly checks the share
			//if a link of folder has been linked
			if (strstr(subdir, "/mnt/Share") != NULL){	
				int wd = inotify_add_watch(fd, subdir, IN_DELETE);
				//printf("subdir: %s\n", subdir);
				//printf("subdir: %s | counter: %d\n", subdir, counter);
				if (wd != -1){
					//printf("subdir: %s\n", subdir);
					wds[counter] = wd;
					strcpy(dirs[counter], subdir);
					counter++;
				}
			} else {

				int wd = inotify_add_watch(fd, subdir, IN_OPEN | IN_CLOSE | IN_CREATE);
				//wds[counter] = wd;
				//strcpy(dirs[counter], subdir);
				//counter++;

				if (wd == -1){
				}else{
					syslog(LOG_INFO, "FileTransaction: READ := Watching:: %s\n", subdir);
					wds[counter] = wd;
					strcpy(dirs[counter], subdir);
					counter++;
				}
				
			}
			list_dir(subdir, fd, wds, dirs, counter);
		}
		break;
	}
    }
    closedir(dr);
}

void delete_folder(String root, String foldname){
	
	sqlite3 *db;
	sqlite3_stmt *res;
	int rc;
	const char *tail;
	rc = sqlite3_open(DBNAME, &db);
	if (rc != SQLITE_OK){
		fprintf(stderr, "Can't open database %s!\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}
	
	String foldpath = "";
	//ex: if testfold is in share then foldpath = /mnt/Share/testfold
	//ex: if testfold is inside testfold2 then foldpath = /mnt/Share/testfold2/testfold
	sprintf(foldpath, "%s/%s", root, foldname);
	
	//we remove the "/mnt/Share/"
	strcpy(foldpath, replace_str(foldpath, "/mnt/Share/", ""));
	String tempfoldpath = "";
	strcpy(tempfoldpath, foldpath);
	//now we check if its a subdirectory, if its not then we don't have problem
	if (strstr(foldpath, "/") != NULL){ //its a subdirectory!
		String subdirname = "";
		char *ptr = strtok(foldpath, "/");
		while (ptr!= NULL){
			strcpy(subdirname, ptr);
			ptr = strtok(NULL, "/");
		}
		//first remove it from share
		String rm = "";
		sprintf(rm, "rm -rf '%s/%s'", SHARE_LOC, tempfoldpath);
		system(rm);
		printf("[-] %s: %s\n", SHARE_LOC, tempfoldpath);
		
		//remove it from CVFSFStorage
		String subdirpath = "";
		sprintf(subdirpath, "%s/%s", STORAGE_LOC, foldname);
		sprintf(rm, "rm -rf '%s'", subdirpath);
		system(rm);
		printf("[-] %s: %s\n", STORAGE_LOC, foldname);
		
		//remove the directory from all targets
		String sql = "";
		strcpy(sql, "SELECT mountpt from target;");
		int good = 0;
		while (!good){
			rc = sqlite3_prepare_v2(db, sql, 1000, &res, &tail);
			if (rc != SQLITE_OK){
			} else {good = 1;}
		}
		//we have deleted successfully the folder from the targets
		while (sqlite3_step(res) == SQLITE_ROW){
			String fold_to_delete = "";
			sprintf(fold_to_delete, "%s/%s", sqlite3_column_text(res,0), tempfoldpath);
			sprintf(rm, "rm -rf '%s'", fold_to_delete);
			system(rm);
			printf("[-] %s: %s\n", sqlite3_column_text(res,0), tempfoldpath);
		}
		sqlite3_finalize(res);
		
		//since the subdirectory been removed, we must update the target size 
		//for all the files there
		char percent = '%';
		sprintf(sql, "SELECT filename, filesize from VolContent where filename like '%s/%c'", tempfoldpath, percent);
		good = 0;
		while (!good){
			rc = sqlite3_prepare_v2(db, sql, 1000, &res, &tail);
			if (rc != SQLITE_OK){
			} else {good = 1;}
		}
		
		while (sqlite3_step(res) == SQLITE_ROW){
			//check if we retrieved correctly, test later
			printf("Filename: %s | Filesize: %lf\n", sqlite3_column_text(res,0), sqlite3_column_double(res,1));
		}
		sqlite3_finalize(res);
		
	} else {
		
	}
	sqlite3_close(db);
}

void delete_from_cache(String file){
	printf("in delete from cache!\n");
	printf("file = %s\n", file);
	sqlite3 *db;
	int rc;

	String rm = "";
	sprintf(rm, "rm -rf '%s/%s'", CACHE_LOC, file);
	printf("rm = %s\n", rm);
	system(rm);

	String sql = "";

	rc = sqlite3_open(DBNAME, &db);
	if (rc != SQLITE_OK){
		fprintf(stderr, "Can't open database %s!\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	sprintf(sql, "Delete from cachecontent where filename = '%s';", file);
	printf("sql = %s\n", sql);
	int good = 0;
	while (!good){
		rc = sqlite3_exec(db, sql, 0, 0, 0);
		if (rc != SQLITE_OK){
	
		} else {good = 1;}
	}
	sqlite3_close(db);
	
}

void delete_stripe(String file){
        printf("in delete stripe function!\n");
	sqlite3 *db;
	sqlite3_stmt *res;
	sqlite3_stmt *res2;
	double avspace;
	String sql2 = "";

	const char *tail;
	int rc;
	String sql = "";
	String name = "";	
	String files[1000]; //assuming 1000 files
        String fileloc[1000];
	double filesize[1000];
        int cnt = 0;

	rc = sqlite3_open(DBNAME, &db);
	if (rc != SQLITE_OK){
		fprintf(stderr, "Can't open database %s!\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	String pragma = "";
	strcpy(pragma, "PRAGMA journal_mode=WAL;");
	rc = sqlite3_exec(db, pragma, 0, 0, 0);

	//remove part1. change to part% for query purposes	
	strcpy(name, replace_str(file, "part1.", "part%."));

	//this query select all the parts since part%. na
	sprintf(sql, "SELECT filename, fileloc, filesize FROM VOLCONTENT where filename like '%s'", name);
	printf("sql1 = %s\n", sql);
	int good = 0;
	while (!good){
		rc = sqlite3_prepare_v2(db, sql, 1000, &res, &tail);
		if (rc != SQLITE_OK){

		} else {good = 1;}
	}	

        while (sqlite3_step(res) == SQLITE_ROW){
		strcpy(files[cnt], sqlite3_column_text(res,0));
		strcpy(fileloc[cnt], sqlite3_column_text(res,1));
		filesize[cnt] = sqlite3_column_double(res,2);
		cnt++;
	
        }
        sqlite3_finalize(res);
	int i =0;
	for (i = 0; i < cnt; i++){
		//res0 = filename, res1 = fileloc, res3 = filesize
		//strcpy(parts[counter], sqlite3_column_text(res,0));

		//remove from share
		String rm = "";
		if (strstr(files[i], "part1.") != NULL){
			String name = "";
			strcpy(name, files[i]);
			strcpy(name, replace_str(name, "part1.", ""));
			sprintf(rm, "rm -rf '%s/%s'", SHARE_LOC, name);
			printf("rm = %s\n", rm);
			system(rm);
		}

		//remove from target
		//String rm = "";
		sprintf(rm, "rm -rf '%s/%s'", fileloc[i], files[i]);
		printf("rm = %s\n", rm);
		system(rm);

		if (strstr(files[i], "part1.") != NULL){
			String name = "", rm = "";
			strcpy(name, files[i]);
			strcpy(name, replace_str(name, "part1.", ""));
			sprintf(rm, "rm -rf '%s/%s'", SHARE_LOC, name);
			printf("rm = %s\n", rm);
			system(rm);
		}
	

		sprintf(sql, "delete from volcontent where filename = '%s';", files[i]);
		printf("sql = %s\n", sql);
		good = 0;
		while (!good){
			rc = sqlite3_exec(db, sql, 0, 0, 0);
			if (rc == SQLITE_OK){
				good = 1;
			}
		}

		//select fileloc and update target size here
		//sqlite3_stmt *res2;
		//const char *tail2;
		//double avspace;
		//String sql2 = "";
		sprintf(sql2, "select avspace from target where mountpt = '%s';", fileloc[i]);
		printf("SQL2 := %s\n", sql2);
		good = 0;
		while (!good){
			rc = sqlite3_prepare_v2(db, sql2, 1000, &res2, &tail);
			if (rc != SQLITE_OK){
			}else {good = 1;}
		}
		if (sqlite3_step(res2) == SQLITE_ROW){
			//get avspace of target;
			avspace = sqlite3_column_double(res2,0);
			printf("avspace = %lf\n", avspace);
		}
		sqlite3_finalize(res2);

		printf("FILESIZE: %lf\n", filesize[i]);
		avspace = avspace + filesize[i];
		
		//update target size here
		sprintf(sql2, "Update target set avspace = %lf where mountpt = '%s';", avspace, fileloc[i]);
		printf("sql2 = %s\n", sql2);
		//printf("SQL3 = %s\n", sql2);
		good = 0;
		while (!good){
			rc = sqlite3_exec(db, sql2, 0, 0, 0);
			if (rc != SQLITE_OK){
				//printf("update target delete locked!\n");
			} else {good = 1;}
		}

		//remove it from target;
		//String rm = "";
		//String filepath = "";
		//sprintf(filepath, "%s/%s", fileloc[i], files[i]);
		//sprintf(rm, "rm -rf '%s'", filepath);
		//printf("rm = %s\n", rm);
		//system(rm);
		//printf("filepath = %s\n", filepath);
		//status = remove(filepath);
		//if (status == 0){
			//printf("[-] %s: %s\n", sqlite3_column_text(res,1), sqlite3_column_text(res,0));
		//}
		//counter++;
	}

	//sqlite3_finalize(res);

	//after deleting actual file delete entry from volcontent
/*	int i = 0;
	for (i = 0; i < MAX_PARTS; i++){
		if (strcmp(parts[i], "") != 0){
			sprintf(sql, "delete from volcontent where filename = '%s';", parts[i]);
			//printf("parts_sql = %s\n", sql);
			good = 0;
			while (!good){
				rc = sqlite3_exec(db, sql, 0, 0, 0);
				if (rc != SQLITE_OK){
				} else {good = 1;}
			}
		}
	}*/
	

	sqlite3_close(db);
}

void delete_linear_file(String root, String file){

	printf("in delete linear!\n");

	String fullpath;
	sprintf(fullpath, "%s/%s", root, file);

	String rm = "";
	sqlite3 *db;
	sqlite3_stmt *res;
	const char *tail;
	String query = "", filename = "", fileloc = "";
	double filesize;
	int rc;
	//printf("Full path: %s\n", fullpath);

	if (strstr(fullpath, "/mnt/Share") != NULL){
		strcpy(fullpath, replace_str(fullpath, "/mnt/Share/", ""));
	}
		//memmove(fullpath, fullpath + strlen("/mnt/Share/"), 1 + strlen(fullpath + strlen("/mnt/Share/")));

	//printf("fullpath = %s\n", fullpath);

	//int status = 1;
	String file_to_check = "";
	sprintf(file_to_check, "rm -rf '%s/%s'", SHARE_LOC, fullpath);
	printf("rm = %s\n", file_to_check);
	system(file_to_check);

	sprintf(rm, "rm -rf '%s/%s'", SHARE_LOC, fullpath);
	printf("rm = %s\n", rm);
	system(rm);
	
	while (doesFileExist(file_to_check)){
		system(rm);
	}
	//status = remove(rm);
	//if (status == 0){
	//	printf("[-] %s: %s\n", SHARE_LOC, rm);
	

	//printf("Fullpath after: %s\n", fullpath);

	//get file information in volcontent;
	sprintf(query, "SELECT filename, fileloc, filesize FROM VOLCONTENT WHERE filename = '%s';", fullpath);
        printf("query = %s\n", query);
	
	//printf("query1 = %s\n", query);

	rc = sqlite3_open(DBNAME, &db);
	if (rc != SQLITE_OK){
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	String pragma = "";
	strcpy(pragma, "PRAGMA journal_mode=WAL;");
	rc = sqlite3_exec(db, pragma, 0, 0, 0);

	int good = 0;
	while (!good){
		rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
		if (rc != SQLITE_OK){
		} else {good = 1;}
	}

	if (sqlite3_step(res) == SQLITE_ROW){
		strcpy(filename, sqlite3_column_text(res,0));
		strcpy(fileloc, sqlite3_column_text(res,1));
		filesize = sqlite3_column_double(res,2);

		//printf("filename: %s | fileloc %s | filesize %lf\n", filename, fileloc, filesize);
	}
	sqlite3_finalize(res);

	sprintf(file_to_check, "%s/%s", fileloc, filename);
	int status = 0;
	status = remove(file_to_check);


	sprintf(query, "delete from volcontent where filename = '%s'\n", filename);
        printf("query = %s\n", query);	
        good = 0;
	while(!good){
        rc = sqlite3_exec(db, query, 0, 0, 0);
        if (rc == SQLITE_OK){
		good = 1;
	}
	}

	//update target size here
	//sqlite3_stmt *res2;
	sprintf(query, "SELECT avspace FROM target where mountpt = '%s';", fileloc);
	printf("query2 = %s\n", query);
	double avspace = 0.0;
	good = 0;
	while (!good){
		rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
		if (rc != SQLITE_OK){
		} else {good = 1;}
	}

	if (sqlite3_step(res) == SQLITE_ROW){
		avspace = sqlite3_column_double(res,0);
	
		//printf("fileloc: %s | avspace: %lf\n", fileloc, sqlite3_column_double(res2,0));
	}
	sqlite3_finalize(res);

	//update avspace
	avspace = avspace + filesize;

	sprintf(query, "Update Target set avspace = %lf where mountpt = '%s';", avspace, fileloc);
	printf("query3 = %s\n", query);
	good = 0;
	while (!good){
		rc = sqlite3_exec(db, query, 0, 0, 0);
		if (rc != SQLITE_OK){
		} else {good = 1;}
	}

	//after updating avspace, delete it from volcontent table
/*	sprintf(query, "Delete from volcontent where filename = '%s';", filename);

	//printf("query4 = %s\n", query);

	good = 0;
	while (!good){
		rc = sqlite3_exec(db, query, 0, 0, 0);
		if (rc != SQLITE_OK){
		} else {good = 1;}
	}*/

	//delete the actual file from target;
	//String rm = "";
	//sprintf(file_to_check, "%s/%s", fileloc, filename);
	//sprintf(rm, "rm -rf '%s/%s'", fileloc, filename);
	//status = remove(rm);
	//printf("query5 = %s\n", rm);
	//system(rm);

	//while(doesFileExist(file_to_check)){
	//	system(rm);
	//}
	//close db
	sqlite3_close(db);
    
}

int doesFileExist(const char *filename){
	struct stat st;
	int result = lstat(filename, &st);
	return result == 0;
}

int inCaches(const char *filename){
	String comm = "", comm_out = "";
	int inCache = 0;

	strcpy(comm, "ls /mnt/CVFSCache");
	runCommand(comm, comm_out);

	char *pch = strtok(comm_out, "\n");
	while (pch != NULL){
		if (strcmp(pch, filename) == 0){
			inCache = 1;
			break;
		}
		pch = strtok(NULL, "\n");
	}

	return inCache;
}

void check_delete(){
	//printf("in check delete\n");

	String linear[4096];
	String stripe[4096];
	int lcounter = 0;
	int scounter = 0;
	//printf("after initialization!\n");
	sqlite3 *db;
	sqlite3_stmt *res;

	String found = "";

	//String files[4096]; //assuming 4096 files are transferred

	int rc;
	const char *tail;

	rc = sqlite3_open(DBNAME, &db);
	if (rc != SQLITE_OK){
		fprintf(stderr, "Can't open database! %s\n",sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1); 	
	}

	String sql = "";
	
	strcpy(sql, "SELECT filename, fileloc from VolContent;");
	int good = 0;
	while (!good){
		rc = sqlite3_prepare_v2(db, sql, 1000, &res, &tail);
		if(rc != SQLITE_OK){}
		else {good = 1;}
	}

	while (sqlite3_step(res) == SQLITE_ROW){
		//printf("file to check: %s\n", sqlite3_column_text(res,0));
		//check for stripe and linear files
		if (strstr(sqlite3_column_text(res,0), "part1.") != NULL || strstr(sqlite3_column_text(res,0), "part") == NULL){
		String filename = "";
		String tempname = "";
		String fileloc = "";
		String source = "", dest = "";
		strcpy(filename, sqlite3_column_text(res,0));
		strcpy(tempname, filename);
		strcpy(fileloc, sqlite3_column_text(res,1));
		sprintf(source, "%s/%s", fileloc, filename);

		if (strstr(filename, "part1.") != NULL){
			//stripe file deletion
			String temp = "";
			strcpy(temp, filename);
			strcpy(filename, replace_str(filename, "part1.", ""));
			sprintf(dest, "%s/%s", SHARE_LOC, filename);
			if (!doesFileExist(dest)){
				printf("file %s is deleted\n", filename);
				strcpy(stripe[scounter], temp);
				scounter++;
				printf("after strcpy!\n");
			}
		} else {
			//linear file deletion
			sprintf(dest, "%s/%s", SHARE_LOC, filename);
			if (!doesFileExist(dest)){

				int status = 0;
				status = remove(dest);
				if (status == 0)
					printf("[-] %s: %s\n", SHARE_LOC, dest);
				//printf("file %s deleted!\n", filename);
				//delete_linear_file(SHARE_LOC, filename);
				strcpy(linear[lcounter], filename);
				//int status = 0;
				//status = remove(dest);
				//if (status == 0)
				//	printf("[-] %s: %s\n", SHARE_LOC, dest);
				//printf("after strcpy");
				lcounter++;
			}
		}

		//memmove(filename, filename + strlen("part1."), pos + strlen(filename + strlen("part1.")));
		//sprintf(dest, "%s/%s", SHARE_LOC, filename);
		//printf("filename = %s\n", dest);
		//printf("SOURCE: %s | DEST: %s\n", source, dest);
		}
	}
	sqlite3_finalize(res);

	int i;
	for (i = 0; i < lcounter; i++){
		delete_linear_file(SHARE_LOC, linear[i]);
	}

	int j;
	for (j = 0; j < scounter; j++){
		//check if in cache
		String tempname = "";
		strcpy(tempname, stripe[j]);
		printf("in stripe loop!\n");
		if (strstr(tempname, "/") != NULL){
			printf("in here!\n");
			char *pch = strtok(tempname, "/");
			while(pch != NULL){
				strcpy(tempname, pch);
				pch = strtok(NULL, "/");
			}
		}
		//delete_stripe_file(tempname);
		printf("after strstr in loop!\n");
		printf("stripefile = %s\n",tempname); 
		if (inCaches(tempname)){
			printf("file in cache!\n");
			delete_from_cache(tempname);
			printf("after cache deletion!\n");
		}
		printf("after checking cahce!\n");	
		delete_stripe(stripe[j]);
	}

	sqlite3_close(db);
}

void *watch_share()
{
    int length, i = 0, wd;
    int fd;
    int rc;
    int access_count = 1;
    int r_count = 1;
    char *p;
    char buffer[BUF_LEN] __attribute__ ((aligned(8)));

    
    String targets[TARGET_NUM];
    int target_num = 0;

    int c = 0;
    int wds[MAX_WTD];
    String dirs[MAX_WTD];
    //int trigger[MAX_WTD];

    String query;

    sqlite3 *db;
    sqlite3_stmt *res;

    const char *tail;


    rc = sqlite3_open(DBNAME, &db);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Can't open database: %s.\n", sqlite3_errmsg(db));
       sqlite3_close(db);
       exit(1);
    }

    strcpy(query, "SELECT mountpt FROM Target;");
    rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
    if (rc != SQLITE_OK){
       fprintf(stderr, "Failed to retrieved data.\n");
       exit(1);
    }

    /*Initialize inotify*/
    fd = inotify_init();
    if ( fd < 0 ) {
       perror( "Couldn't initialize inotify" );
    }

   //list_dir(TEMP_LOC, fd, wds, dirs, counter); 
   //list_dir(SHARE_LOC, fd, wds, dirs, counter); //for delete function

   //printf("HELLO!!!\n");
    while (sqlite3_step(res) == SQLITE_ROW){
        strcpy(targets[target_num], sqlite3_column_text(res,0));
	target_num++;
    }
    sqlite3_finalize(res);

   int ti = 0;
   for (ti = 0; ti < target_num; ti++){
	wd = inotify_add_watch(fd, targets[ti], IN_CREATE | IN_OPEN | IN_CLOSE);
	wds[c] = wd;
	strcpy(dirs[c], targets[ti]);
	c++;
	list_dir(targets[ti], fd, wds, dirs, c);
   }

   wd = inotify_add_watch(fd, CACHE_LOC, IN_OPEN | IN_CLOSE);
   wds[c] = wd;
   strcpy(dirs[c], CACHE_LOC);
   c++;
   if (wd != -1){
    	syslog(LOG_INFO, "FileTransaction: Watching:: %s\n", CACHE_LOC);
   }

   wd = inotify_add_watch(fd, SHARE_LOC, IN_DELETE);
   wds[c] = wd;
   strcpy(dirs[c], SHARE_LOC);
   c++;
   if (wd != -1){
	syslog(LOG_INFO, "FileTransaction: Watching:: %s\n", SHARE_LOC);
   }

   //list_dir(TEMP_LOC, fd, wds, dirs, c);
   //c+=2;
   //list_dir(SHARE_LOC, fd, wds, dirs, c);

    /*do it forever*/
    for(;;){
       //check_delete();
       //create_link();
       //system("symlinks -dr /mnt/Share");
       length = read(fd, buffer, BUF_LEN);

       if (length < 0){
          perror("read");
       }

     list_dir(SHARE_LOC, fd, wds, dirs, c);

     for(p = buffer; p < buffer + length;){
           struct inotify_event *event = (struct inotify_event *) p;
	     if (event->mask & IN_CREATE){
	   	  if (event->mask & IN_ISDIR){
                      int size = sizeof(wds) / sizeof(wds[0]);
		      int i = 0;

      		      for (i = 0; i < size; i++){
			   //printf("WDS[%d]: %d | DIRS[%d]: %s\n", i, wds[i], i, dirs[i]);
     			   if (wds[i] == event->wd){
				if (strstr(dirs[i], "/mnt/Share") == NULL){
				   
				   String add_watch_dir = "";
				   sprintf(add_watch_dir, "%s/%s", dirs[i], event->name);
				   int wd = inotify_add_watch(fd, add_watch_dir, IN_ALL_EVENTS);
				   if (wd == -1){

				   } else {
					syslog(LOG_INFO, "FileTransaction: Watching := %s\n", add_watch_dir);
					//printf("READ := Watching := %s\n", add_watch_dir);
				   }

				   wds[c] = wd;
				   //trigger[c] = event->wd;
				   strcpy(dirs[c], add_watch_dir);
				   c++; 	
			        }
			      break;
			   }
		      }
		  } else {
		  }
	     }

	     if (event->mask & IN_DELETE){
		//printf("%s is deleted!\n", event->name);

		FILE *fp = fopen("deleting.txt", "w");
		fprintf(fp, "1");
		fclose(fp);
		check_delete();
		fp = fopen("deleting.txt", "w");
		fprintf(fp, "0");
		fclose(fp);
             }
              
             if (event->mask & IN_OPEN){
                  if (event->mask & IN_ISDIR){

                  }else {
			//printf("Linear file %s opened.\n", event->name);
                      if (strstr(event->name,"part1.") != NULL){

			int k = 0;
			for (k = 0; k < c; k++){
				if (wds[k] == event->wd){
					//printf("IN OPEN : %s | FILENAME : %s\n", dirs[k], event->name);
					break;
				}
			}

			//printf("FILENAME : %s opened.\n", event->name);
			int flag, flag1;
                        FILE *fp = fopen("random.txt", "r");
			fscanf(fp,"%d",&flag);
			fclose(fp);
			fp = fopen("is_assembling.txt", "r");
			fscanf(fp, "%d", &flag1);
			fclose(fp);
			printf("IN OPEN FLAG := %d %d: filename = %s \n", flag, flag1, event->name);
			if (flag == 0 && flag1 == 0){ //done striping continue with open event
                        //and done with assembling file
			incrementFrequency(event->name);
                        String comm = "", comm_out = "";
                        int inCache = 0;
                        sprintf(comm, "ls %s", CACHE_LOC);
                        runCommand(comm, comm_out);

                        char *ptr = NULL;
                        ptr = strtok(comm_out, "\n");
                        while (ptr != NULL){
                             if (strcmp(ptr, event->name) == 0){
                                 inCache = 1;
                                 break;
                             }
			     ptr = strtok(NULL, "\n");
                        }
                        if (!inCache){
			    //printf("Watch Share: Should be assembling file here....\n");
                
		FILE *fp1 = fopen("assembled.txt", "rb");
                String file = "";
		int check = 0;
		String line = "";
		strcpy(file, event->name);
                strcat(file, "\n");
                while (fgets(line, sizeof(file), fp1) != NULL){
			//printf("LINE := %s | FILENAME := %s\n", line, event->name);
			if (strcmp(line, file) == 0){
				//printf("SAME FILE := \n");
				check = 1;
				break;
			}
		}
		fclose(fp1);
		if (!check){
		// assemble the file by getting parts from volumes
                // take NOTE: assembly of file should only happen when all files are present
                // (or when no file striping is happening)
                // can this be determined with random.txt?
                	//fp = fopen("is_assembling.txt", "w");
			//fprintf(fp, "1");
			//fclose(fp);
			assemble(event->name);
			//fp = fopen("is_assembling.txt", "w");
			//fprintf(fp, "0");
			//fclose(fp);
		}
			    //String assembled_file = "";
			    //sprintf(assembled_file, "%s/%s", ASSEMBLY_LOC, event->name);


			    //printf("Assembled File: %s\n", assembled_file);
                            //assemble(event->name);
			    //printf("Checking if assembled file exist...\n");
			    //FILE *fp;
			    //fp = fopen(assembled_file, "r");
			    //if (fp == NULL){
				//printf("Assembled file does not exist. Assembling here...\n");
				//assemble(event->name);
			    //} else {
				//printf("Assembled file already exist. Don't assembled anymore..\n");
			    //}
			    //fclose(fp);
                        }
                      }
		    }
                  }
              }

              if (event->mask & IN_CLOSE){
                  if (event->mask & IN_ISDIR){

                  }else{
                      syslog(LOG_INFO, "FileTransaction: The file %s was closed.\n", event->name);
                      //printf("File %s closed.\n", event->name);
		      int k = 0;
		      for (k = 0; k < c; k++){
			if (wds[k] == event->wd){
				//printf("IN_CLOSE : %s | FILENAME : %s\n", dirs[k], event->name);
				break;
			}
		      }
		      //strcpy(COMMANDS[COUNTER], "");
		      //COUNTER++;
		      //String original_file = "";
		      //sprintf(original_file, "%s/%s", STORAGE_LOC, event->name);
		      //FILE *fp;
                    //  fp = fopen(original_file, "r");
		  //    if (fp == NULL){
		//	printf("Original file does not exist.. Do nothing..\n");
		  //    } else {
		//	printf("Original file exist. File closed. Disassembling file..\n");
		     //}
		      //fclose(fp);
		      int flag;
		      FILE *fp = fopen("random.txt", "rb");
		      fscanf(fp, "%d", &flag);
		      fclose(fp);
		      printf("IN CLOSE FLAG := %d : filename = %s\n", flag, event->name);
		      if (flag == 0) { //done striping
			
			String comm = "", comm_out = "";
			int inCache = 0;
			strcpy(comm, "ls /mnt/CVFSCache");
			runCommand(comm, comm_out);

			char *pch = strtok(comm_out, "\n");
			while (pch != NULL){
				if (strcmp(pch, event->name) == 0){
					inCache = 1;
					break;
				}
				pch = strtok(NULL, "\n");
			}

			if (!inCache){
				//check if file already assembled
				FILE *fp = fopen("assembled.txt", "rb");
				String line = "";
				String file = "";
				int assembled = 0;
				strcpy(file, event->name);
				strcat(file, "\n");
				while (fgets(line, sizeof(file), fp) != NULL){
					//printf("LINE := %s | FILE := %s\n", line, event->name);
					if (strcmp(line, file) == 0){
						assembled = 1;
						break;
					}
				}
				fclose(fp);

				if (assembled){
		    			printf("File has been closed\n");
					disassemble(event->name);
				}
			}

                      	if (strstr(event->name, "part1.") != NULL){
                        	 //printf("refresh cache here!\n");  
				 refreshCache();
                      	}
		      }
                  }
              }

              p += EVENT_SIZE + event->len;

       }
}

    /* Clean up */
    inotify_rm_watch(fd, wd);
    close(fd);
    //sqlite3_finalize(res);
    sqlite3_close(db);
}
