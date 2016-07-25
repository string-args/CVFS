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

				int wd = inotify_add_watch(fd, subdir, IN_ALL_EVENTS);
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

void delete_linear_file(String root, String file){

	String fullpath;
	sprintf(fullpath, "%s/%s", root, file);

	String rm = "";

	//printf("Full path: %s\n", fullpath);

	if (strstr(fullpath, "/mnt/Share") != NULL)
		memmove(fullpath, fullpath + strlen("/mnt/Share/"), 1 + strlen(fullpath + strlen("/mnt/Share/")));

	int status = 1;
	sprintf(rm, "%s/%s", SHARE_LOC, fullpath);
	//status = remove(rm);
	if (status == 0){
		printf("[-] %s: %s\n", SHARE_LOC, rm);
	}

	//printf("Fullpath after: %s\n", fullpath);

	//get file information in volcontent;
	sqlite3 *db;
	sqlite3_stmt *res;
	const char *tail;
	int rc;
	String query = "", filename = "", fileloc = "";
	double filesize;
	sprintf(query, "SELECT filename, fileloc, filesize FROM VOLCONTENT WHERE filename = '%s';", fullpath);
	
	//printf("query1 = %s\n", query);

	rc = sqlite3_open(DBNAME, &db);
	if (rc != SQLITE_OK){
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

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

	//update target size here
	sqlite3_stmt *res2;
	sprintf(query, "SELECT avspace FROM target where mountpt = '%s';", fileloc);
	//printf("query2 = %s\n", query);
	double avspace = 0.0;
	good = 0;
	while (!good){
		rc = sqlite3_prepare_v2(db, query, 1000, &res2, &tail);
		if (rc != SQLITE_OK){
		} else {good = 1;}
	}

	if (sqlite3_step(res2) == SQLITE_ROW){
		avspace = sqlite3_column_double(res2,0);
	
		//printf("fileloc: %s | avspace: %lf\n", fileloc, sqlite3_column_double(res2,0));
	}
	sqlite3_finalize(res2);

	//update avspace
	avspace = avspace + filesize;

	sprintf(query, "Update Target set avspace = %lf where mountpt = '%s';", avspace, fileloc);
	
	//printf("query3 = %s\n", query);

	good = 0;
	while (!good){
		rc = sqlite3_exec(db, query, 0, 0, 0);
		if (rc != SQLITE_OK){
		} else {good = 1;}
	}

	//after updating avspace, delete it from volcontent table
	sprintf(query, "Delete from volcontent where filename = '%s';", filename);

	//printf("query4 = %s\n", query);

	good = 0;
	while (!good){
		rc = sqlite3_exec(db, query, 0, 0, 0);
		if (rc != SQLITE_OK){
		} else {good = 1;}
	}

	//delete the actual file from target;
	//String rm = "";
	sprintf(rm, "%s/%s", fileloc, filename);
	status = remove(rm);

	//printf("query5 = %s\n", rm);

	if (status == 0)
        	printf("[-] %s: %s\n", fileloc, filename);

	//close db
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
	create_link();
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
		if (event->mask & IN_ISDIR){

		} else {
			//printf("Delete event: %s\n", event->name);
			//String root = "";
			//String arr[MAX_DEPTH];
			//int n = sizeof(wds) / sizeof(wds[0]);
			int d, i, rooti;
			
			//for (d = 0; d < MAX_DEPTH; d++){
			//	strcpy(arr[d], "");
			//}

			//get_roots(wds, trigger, dirs, c, event->wd, arr);
			/*for (d = 1; d < c; d++){
				if (strcmp(arr[d], "") != 0){
					strcat(root, arr[d]);
					strcat(root, "/");
				}
			}*/
			printf("%s\n", event->name);
			for (d = 0; d < MAX_WTD; d++){
				if (wds[d] == event->wd){
			
					delete_linear_file(dirs[d], event->name);
					printf("DELETE LINEAR FILE!\n");
					break;
				}
			}
			//break;
		}
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
			int flag;
                        FILE *fp = fopen("random.txt", "r");
			fscanf(fp,"%d",&flag);
			fclose(fp);
			//printf("IN OPEN FLAG := %d\n", flag);
			if (flag == 0){ //done striping continue with open event
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
                //assemble(event->name);}
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
		      //printf("IN CLOSE FLAG := %d\n", flag);
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
