
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>

#include "../Global/global_definitions.h"
#include "../disk_pooling/file_presentation.h"
#include "../disk_pooling/initial_configurations.h"
#include "../file_recovery/target_stat.h"
#include "../cache_access/cache_operation.h"
#include "watch_share.h"
#include "watch_dir.h"

#define THREADCNT 3

// declare globals
int MAX_CACHE_SIZE;
long STRIPE_SIZE;

// shows help message and exit
void show_help() {
    printf("Usage:\n");
    printf("\tcvfs_driver [init]\n");
    printf("use init to initialize system");
    exit(1);
}

void configure() {
    int flag = 0; //initial value of flag
    FILE *fp = fopen("random.txt", "w");
    fprintf(fp, "%d", flag);
    fclose(fp);
    fp = fopen("is_assembling.txt", "w");
    fprintf(fp, "%d", flag);
    fclose(fp);
    // read configuration files of cache size and stripe size
    fp = fopen(CACHE_CONF, "r");
    if (fp == NULL) {
        printf("Cannot open file %s\n", CACHE_CONF);
        exit(1);
    }
    fscanf(fp, "%d", &MAX_CACHE_SIZE);
    fclose(fp);
    fp = fopen(STRIPE_CONF, "r");
    if (fp == NULL) {
        printf("Cannot open file %s\n", STRIPE_CONF);
        exit(1);
    }
    fscanf(fp, "%ld", &STRIPE_SIZE);
    fclose(fp);
    //strcpy(DBNAME, "../Database/cvfs_db");
    //sprintf(DBNAME, "%s", "../Database/cvfs_db"); 
    
}

int main(int argc, char *argv[]) {
    system("clear");
    // open logging
    openlog("cvfs2", LOG_PID|LOG_CONS, LOG_USER);

    if(argc > 2){
		printf("FATAL: Too much arguments.\n");
        show_help();
	} else if(argc == 2) {
        if(strcmp(argv[1], "init") == 0) {      // initialize and exit
            initialize();
            exit(0);
        } else {
            printf("FATAL: Wrong argument.\n");
    		show_help();
        }
    }

    pthread_t t[THREADCNT];
    int i;

    configure();

    while(1) {
        //pthread_create(&t[0], NULL, create_link, NULL);
        pthread_create(&t[0], NULL, watch_temp, NULL);
        pthread_create(&t[1], NULL, watch_share, NULL);
        
//	FILE *fp = fopen("deleting.txt", "r");
//	int flag;
//	fscanf(fp, "%d", &flag);
//	if (flag == 0){
        	pthread_create(&t[2], NULL, create_link, NULL);
//	}
	//pthread_create(&t[3], NULL, refreshCache, NULL);

        for (i = 0; i < THREADCNT; i++) {
            pthread_join(t[i], NULL);
        }
    }

}
