
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
	syslog(LOG_INFO, "FileTransaction: Could not open current directory %s", dir_to_read);
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
				if (wd != -1){
					wds[counter] = wd;
					strcpy(dirs[counter], subdir);
					counter++;
				}
			} else {
				int wd = inotify_add_watch(fd, subdir, IN_OPEN | IN_CLOSE | IN_CREATE);
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

void delete_from_cache(String file){
	sqlite3 *db;
	int rc;

	String rm = "";
	sprintf(rm, "rm -rf '%s/%s'", CACHE_LOC, file);
	system(rm);

	String sql = "";

	rc = sqlite3_open(DBNAME, &db);
	if (rc != SQLITE_OK){
		fprintf(stderr, "Can't open database %s!\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	sprintf(sql, "Delete from cachecontent where filename = '%s';", file);
	int good = 0;
	while (!good){
		rc = sqlite3_exec(db, sql, 0, 0, 0);
		if (rc != SQLITE_OK){
	
		} else {good = 1;}
	}
	sqlite3_close(db);
	
}

void delete_stripe(String file){
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
			system(rm);
		}

		//remove from target
		//String rm = "";
		sprintf(rm, "rm -rf '%s/%s'", fileloc[i], files[i]);
		system(rm);

		if (strstr(files[i], "part1.") != NULL){
			String name = "", rm = "";
			strcpy(name, files[i]);
			strcpy(name, replace_str(name, "part1.", ""));
			sprintf(rm, "rm -rf '%s/%s'", SHARE_LOC, name);
			system(rm);
		}
	

		sprintf(sql, "delete from volcontent where filename = '%s';", files[i]);
		good = 0;
		while (!good){
			rc = sqlite3_exec(db, sql, 0, 0, 0);
			if (rc == SQLITE_OK){
				good = 1;
			}
		}

		sprintf(sql2, "select avspace from target where mountpt = '%s';", fileloc[i]);
		good = 0;
		while (!good){
			rc = sqlite3_prepare_v2(db, sql2, 1000, &res2, &tail);
			if (rc != SQLITE_OK){
			}else {good = 1;}
		}
		if (sqlite3_step(res2) == SQLITE_ROW){
			//get avspace of target;
			avspace = sqlite3_column_double(res2,0);
		}
		sqlite3_finalize(res2);

		avspace = avspace + filesize[i];
		
		//update target size here
		sprintf(sql2, "Update target set avspace = %lf where mountpt = '%s';", avspace, fileloc[i]);
		good = 0;
		while (!good){
			rc = sqlite3_exec(db, sql2, 0, 0, 0);
			if (rc != SQLITE_OK){
			} else {good = 1;}
		}
	}

	sqlite3_close(db);
}

void delete_linear_file(String root, String file){

	String fullpath;
	sprintf(fullpath, "%s/%s", root, file);

	String rm = "";
	sqlite3 *db;
	sqlite3_stmt *res;
	const char *tail;
	String query = "", filename = "", fileloc = "";
	double filesize;
	int rc;

	if (strstr(fullpath, "/mnt/Share") != NULL){
		strcpy(fullpath, replace_str(fullpath, "/mnt/Share/", ""));
	}

	String file_to_check = "";
	sprintf(file_to_check, "rm -rf '%s/%s'", SHARE_LOC, fullpath);
	system(file_to_check);

	sprintf(rm, "rm -rf '%s/%s'", SHARE_LOC, fullpath);
	system(rm);
	
	while (doesFileExist(file_to_check)){
		system(rm);
	}

	//get file information in volcontent;
	sprintf(query, "SELECT filename, fileloc, filesize FROM VOLCONTENT WHERE filename = '%s';", fullpath);
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
	}
	sqlite3_finalize(res);

	sprintf(file_to_check, "%s/%s", fileloc, filename);
	int status = 0;
	status = remove(file_to_check);


	sprintf(query, "delete from volcontent where filename = '%s'\n", filename);
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
	double avspace = 0.0;
	good = 0;
	while (!good){
		rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
		if (rc != SQLITE_OK){
		} else {good = 1;}
	}

	if (sqlite3_step(res) == SQLITE_ROW){
		avspace = sqlite3_column_double(res,0);
	}
	sqlite3_finalize(res);
	//update avspace
	avspace = avspace + filesize;

	sprintf(query, "Update Target set avspace = %lf where mountpt = '%s';", avspace, fileloc);
	good = 0;
	while (!good){
		rc = sqlite3_exec(db, query, 0, 0, 0);
		if (rc != SQLITE_OK){
		} else {good = 1;}
	}
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
	String linear[4096];
	String stripe[4096];
	int lcounter = 0;
	int scounter = 0;	sqlite3 *db;
	sqlite3_stmt *res;

	String found = "";
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
				strcpy(stripe[scounter], temp);
				scounter++;
			}
		} else {
			//linear file deletion
			sprintf(dest, "%s/%s", SHARE_LOC, filename);
			if (!doesFileExist(dest)){

				int status = 0;
				status = remove(dest);
				if (status == 0)
					printf("[-] %s: %s\n", SHARE_LOC, dest);
                strcpy(linear[lcounter], filename);
				lcounter++;
			}
		}
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
		if (strstr(tempname, "/") != NULL){
			char *pch = strtok(tempname, "/");
			while(pch != NULL){
				strcpy(tempname, pch);
				pch = strtok(NULL, "/");
			}
		}
		if (inCaches(tempname)){
			delete_from_cache(tempname);
		}
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
    /*do it forever*/
    for(;;){
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
     			   if (wds[i] == event->wd){
				if (strstr(dirs[i], "/mnt/Share") == NULL){
				   
				   String add_watch_dir = "";
				   sprintf(add_watch_dir, "%s/%s", dirs[i], event->name);
				   int wd = inotify_add_watch(fd, add_watch_dir, IN_ALL_EVENTS);
				   if (wd == -1){

				   } else {
					syslog(LOG_INFO, "FileTransaction: Watching := %s\n", add_watch_dir);
				   }

				   wds[c] = wd;
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
             if (event->mask & IN_OPEN){
                  if (event->mask & IN_ISDIR){

                  }else {
                      if (strstr(event->name,"part1.") != NULL){

			int k = 0;
			for (k = 0; k < c; k++){
				if (wds[k] == event->wd){
					break;
				}
			}
			int flag, flag1, flag2;
            FILE *fp = fopen("is_striping.txt", "r");
			fscanf(fp,"%d",&flag);
			fclose(fp);
			fp = fopen("cache_assembling.txt", "r");
			fscanf(fp, "%d", &flag1);
			fclose(fp);
			fp = fopen("is_assembling.txt", "rb");
		      fscanf(fp, "%d", &flag2);
		      fclose(fp);
			if (flag == 0 && flag1 == 0 & flag2 == 0){ 
                        //done striping continue with open event
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
                
		FILE *fp1 = fopen("assembled.txt", "rb");
        String file = "";
		int check = 0;
		String line = "";
		strcpy(file, event->name);
                strcat(file, "\n");
                while (fgets(line, sizeof(file), fp1) != NULL){
			    if (strcmp(line, file) == 0){
				   check = 1;
				   break;
			    }
		}
		fclose(fp1);
		if (!check){
            FILE *fp = fopen("is_assembling.txt", "w");
            fprintf(fp, "1");
            fclose(fp);
			assemble(event->name);
			fp = fopen("is_assembling.txt", "w");
			fprintf(fp, "0");
			fclose(fp);
		}}}}}}

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
		      int flag, flag1, flag2;
		      FILE *fp = fopen("is_striping.txt", "rb");
		      fscanf(fp, "%d", &flag);
		      fclose(fp);
		      fp = fopen("cache_assembling.txt", "rb");
		      fscanf(fp, "%d", &flag1);
		      fclose(fp);
		      fp = fopen("is_assembling.txt", "rb");
		      fscanf(fp, "%d", &flag2);
		      fclose(fp);
			if (flag == 0 && flag1 == 0 && flag2 == 0) { //done striping and assembling
			
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
					disassemble(event->name);
				}
			}

                      	if (strstr(event->name, "part1.") != NULL){
				           refreshCache();
                      	}
		      }
                  }
              }

              p += EVENT_SIZE + event->len;

       }
}}

    /* Clean up */
    inotify_rm_watch(fd, wd);
    close(fd);
    //sqlite3_finalize(res);
    sqlite3_close(db);
}
