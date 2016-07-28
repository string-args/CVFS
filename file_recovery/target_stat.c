// check target status if up or down
// can only check 1 target at a time

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <syslog.h>

#include "../Global/global_definitions.h"
#include "../Utilities/cmd_exec.h"

#define MAX_TARGETS 50
#define INTERVAL    10     // 5 mins

// untested
void kick_target(String mountpt, String iqn, String assocvol, String ipadd) {
    int rc;
    sqlite3 *db;
    sqlite3_stmt *res, *res2, *res3;
    const char *tail;
    char *errmsg = 0;
    String query = "";
    rc = sqlite3_open(DBNAME, &db);

    // kick out target from pool
    openlog("cvfs2", LOG_PID|LOG_CONS, LOG_USER);


    // not sure what other thing to do, but here's a draft:
    // delete from volcontent where mountpt = mounpt
    strcpy(query, "");

    sprintf(query, "SELECT filename, partno from Volcontent Where fileloc = '%s';", mountpt);
    int good = 0;
    while (!good){
	rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
	if (rc == SQLITE_OK){
		good = 1;
	}
    }

    while (sqlite3_step(res) == SQLITE_ROW){
        String filename = "";
        int partno = sqlite3_column_int(res, 1);
        strcpy(filename, sqlite3_column_text(res,0));
        printf("filename = %s\n", sqlite3_column_text(res,0));
        // if stripe delete from other targets:
        if (partno != 0) {
            // get filename without partX
            String root = "";
            String withoutroot = "";
            char* pch = NULL;
            String filename2 = "";
            strcpy(filename2, filename);
            pch = strtok(filename2, "/");
        	while (pch != NULL){
                strcat(root, pch);
                strcpy(withoutroot, pch);
                strcat(root, "/");
                pch = strtok(NULL, "/");
            }
            root[strlen(root) - (strlen(withoutroot) + 2)] = '\0';
            printf("check if root is correct = %s\n", root);

            // remove partX. from withoutroot
            String partx = "";
            sprintf(partx, "part%d.", partno);
            memmove(withoutroot, withoutroot + strlen(partx), 1 + strlen(withoutroot + strlen(partx)));
            printf("should not have partx now = %s\n", withoutroot);

            char percent = '%';
            sprintf(query, "SELECT filename, mountpt from Volcontent Where filename LIKE '%s/part%c.%s';", root, percent, withoutroot);
            int good = 0;
            while (!good){
        	rc = sqlite3_prepare_v2(db, query, 1000, &res2, &tail);
        	if (rc == SQLITE_OK){
        		good = 1;
        	}
            }
            while (sqlite3_step(res2) == SQLITE_ROW) {
                // delete it
                // delete it from table
                String fn2, mpt2 = "";
                strcpy(fn2, sqlite3_column_text(res2,0));   // filename
                strcpy(mpt2, sqlite3_column_text(res2,1));  // mountpt
                String rm = "";
                sprintf(rm, "rm %s/%s", mpt2, fn2);
                printf("kick: rm = %s\n", rm);
                system(rm);
                // delete it from volcontent
                sprintf(query, "DELETE FROM Volcontent Where filename = '%s';", fn2);
                do {
            		rc = sqlite3_exec(db, query, 0, 0, &errmsg);
            	} while (rc == SQLITE_BUSY);
                printf("kick deleted from voclontetn\n");
            }
        } else {
            // linear file, just delete
            String rm = "";
            sprintf(rm, "rm %s/%s", mountpt, filename);
            printf("kick: rm(linear) = %s\n", rm);
            system(rm);
            // delete it from volcontent
            sprintf(query, "DELETE FROM Volcontent Where filename = '%s';", filename);
            do {
                rc = sqlite3_exec(db, query, 0, 0, &errmsg);
            } while (rc == SQLITE_BUSY);
            printf("kick deleted from voclontetn\n");
        }


    }
    sqlite3_finalize(res);
/*
    // i think this one not needed
	sprintf(query, "DELETE FROM VolContent WHERE fileloc = '%s';", mountpt);
	good = 0;
	while (!good){
		rc = sqlite3_exec(db, query, 0, 0, &errmsg);
		if (rc == SQLITE_OK){
			good = 1;
		}
	}

/*
    // but this one is needed i thik?
    // delete from cache content
    strcpy(query, "");
	sprintf(query, "DELETE FROM CacheContent WHERE mountpt = '%s';", mountpt);
	good = 0;
	while (!good){
		rc = sqlite3_exec(db, query, 0, 0, &errmsg);
		if (rc == SQLITE_OK){
			good = 1;
		}
	}

    // delete from target table
   strcpy(query, "");
	sprintf(query, "DELETE FROM Target WHERE iqn = '%s';", iqn);

	good = 0;
	while (!good){
		rc = sqlite3_exec(db, query, 0, 0, &errmsg);
		if (rc == SQLITE_OK){
			good = 1;
		}
	}
*/
/*
    // deactivate volume
    //unmount
    String umount = "";
    sprintf(umount, "umount '%s'", mountpt);
    printf("umount = %s\n");
    system(umount);

    //logout
    String logout = "";
    sprintf(logout, "iscsiadm -m node -u -T %s %s:3260", iqn, ipadd);
    printf("logout = %s\n", logout);
    system(logout);

    //remove folder
    String rm = "";
    sprintf(rm, "rm -rf '%s'", mountpt);
    printf("rm = %s\n", rm);
    system(rm);

    //remove logical volume
    String lvremove = "";
    sprintf(lvremove, "lvremove -f '%s'", assocvol);
    printf("lvremove = %s\n", lvremove);
    system(lvremove);

    //remove volume group
    String vgremove = "", vgname = "";
    char *pch = NULL;
    int counter = 0;
    pch = strtok(assocvol, "/");
    while (pch != NULL){
        strcat(vgname, "/");
        strcat(vgname, pch);
        counter++;
        pch = strtok(NULL, "/");
        if (counter == 2)
        break;
    }
    sprintf(vgremove, "vgremove -f '%s'", vgname);
    printf("vgremove = %s\n", vgremove);
    system(vgremove);
*/
    syslog(LOG_INFO, "File Recovery: Target %s (%s) is removed from network due to inactivity.", iqn, ipadd);
    sqlite3_close(db);
}

// tested: working
int is_target_up(String ipadd) {
    String ping = "", out = "";
    sprintf(ping, "ping -c 4 %s | grep -oP \'\\d+(?=%% packet loss)\'", ipadd);
    runCommand(ping, out);
    if(strcmp(out, "100") == 0)    // 100 % packet loss = down
        return 0;
    return 1;
}

// run this in separate thread
// untested
void* check_target() {
    sqlite3 *db;
	sqlite3_stmt *res;
	int rc;
	const char *tail;
	rc = sqlite3_open(DBNAME, &db);

	String targets[MAX_TARGETS];
        int counter = 0;
	int i = 0;

    	if (rc != SQLITE_OK){
    		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    		sqlite3_close(db);
    		exit(1);
    	}

    String iqn = "", mountpt = "", assocvol = "", prevadd = "";

   printf("check target here...\n");
    while (1) {
    	String query = "";
    	strcpy(query, "select ipadd, iqn, mountpt, assocvol from Target;");

	int good = 0;
    	while (!good){
		rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
		if (rc == SQLITE_OK){
			printf("Success prepare v2!\n");
			good = 1;
		}
	}

    	// for each target, we check if its up
    	while (sqlite3_step(res) == SQLITE_ROW) {
            String curradd = "";
            strcpy(curradd, sqlite3_column_text(res,0));
            strcpy(iqn, sqlite3_column_text(res,1));
            strcpy(mountpt, sqlite3_column_text(res,2));
            strcpy(assocvol, sqlite3_column_text(res,3));
	    printf("curradd to check = %s\n", curradd);
            if(!is_target_up(curradd)) {  // if down,

		for (i = 0; i < MAX_TARGETS; i++){
// down for 10 mins?! get outta my network!
			if(strcmp(curradd, targets[i]) == 0){
			printf("curradd = %s | prevadd = %s\n", curradd, targets[i]);
				      kick_target(mountpt, iqn, assocvol, curradd);

			}
		}

		strcpy(targets[counter], curradd);
                //strcpy(prevadd, curradd);
		counter++;
            } else {
		for (i = 0; i < MAX_TARGETS; i++){
			if (strcmp(curradd, targets[i]) == 0){
				strcpy(targets[i], "");
			}
		}
		}

    	}
	sqlite3_finalize(res);
	counter = 0;

        sleep(INTERVAL);
    }
	//counter = 0;

sqlite3_close(db);
}


/*
int main() {
    check_target();
    return 0;

}*/
