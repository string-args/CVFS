// Decommissioning of targets
// take note: I am using the shared db variable in global_defs
// note again: this version assumes bytes yung naka store sa db.
// pero... sa tingin ko kelangan natin ata i kilobytes..?
// in that case papalitan ko lang naman yung toBytes na function?
// actually takot lang ako na baka di kasya pag punta sa program
// pero medyo nakakagulat nga na kasya pala sa long!!!! hmm pero ewan ko pa rin

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <syslog.h>

#include "../Global/global_definitions.h"
#include "../Utilities/cmd_exec.h"
#define DBNAME2	"../Database/cvfs_db"	// for some reason this should be full path

static int callback(void *used, int argc, char **argv, char **colname) {
	int i;
	String comm = "";
	printf("exec -1\n");
	long occsp = (long)used;
	printf("exec 0\n");
	printf("argc = %d\n", argc);
	printf("argv[0] = %s\n", argv[0]);
	if(argc != 1) {
		fprintf(stderr, "Error in database table: missing columns\n");
		printf("FAILED0");
		exit(1);
	} else {
		// compare if the available space in other targets can accomodate
		if (argv[0] == NULL) {
			printf("You cannot decommission, not enough space.\n");
			printf("FAILED1");
			exit(0);
		}
		// the occupied space in the leaving node
		long total_av = atol(argv[0]);
		printf("total available space = %ld\n", total_av);
		if (total_av >= occsp) {
			printf("you can decommission now!\n");

		} else {
			printf("You cannot decommission, not enough space.\n");
			printf("FAILED1");
			exit(0);
		}
	}

	return 0;
}

int main(int argc, char *argv[]) {
    char *errmsg = 0;
    int rc;
	long occsp;
    sqlite3 *db;

	openlog("cvfs2", LOG_PID|LOG_CONS, LOG_USER);

	if(argc != 5){
		printf("FATAL: Program takes exactly 3 arguments.\n");
		printf("Usage:\n");
		printf("\tdecomm <IQN> <mount point> <vg_name> <ip_add>\n");	// this is to limit db queries
		printf("FAILED2");
		exit(1);
	}

	// at this point, we assume that the IQN is valid
	String iqn = "", query = "", lvname = "", comm = "", comm_out = "", dmount = "", query2 = "", ipadd = "";
	strcpy(iqn, argv[1]);
	strcpy(dmount, argv[2]);	// mount point of leaving node
	strcpy(lvname, argv[3]); 	//logical volume group name
	strcpy(ipadd, argv[4]);
	syslog(LOG_INFO, "decomm: Decommissioning of target %s mounted at %s\n", iqn, dmount);
	printf("IQN = %s\n", iqn);
	//sizex = <occupied space in x>
	sprintf(comm, "du -s %s | awk '{print $1}'", argv[2]);
	runCommand(comm, comm_out);
	occsp = atol(comm_out) * 1000;	// convert to bytes
	printf("occupied space in target %s = %ld\n", iqn, occsp);

	// open database
	printf("decomm: Opening database %s\n", DBNAME2);
	rc = sqlite3_open(DBNAME2, &db);
	if(rc) {
		fprintf(stderr, "Can\'t open database %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		printf("FAILED3");
		exit(1);
	}

	sprintf(query, "SELECT sum(avspace) FROM Target WHERE iqn != '%s';", iqn);
	do {
		rc = sqlite3_exec(db, query, callback, (void *)occsp, &errmsg);
	} while (rc == SQLITE_BUSY);

	printf("done done!");
	// put decommissioning code here
	// removing node in the target db
	strcpy(query, "");
	sprintf(query, "DELETE FROM Target WHERE iqn = '%s';", iqn);
	printf("before delete\n");
	do {
		rc = sqlite3_exec(db, query, 0, 0, &errmsg);
	} while (rc == SQLITE_BUSY);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL Error: %s\n", errmsg);
		sqlite3_free(errmsg);
		printf("FAILED4");
		exit(1);
	}
	// since we cannot use file_map from file_mapping.c, we do another version of it here

	printf("selecting from volcontent\n");

	// select all files in dmount
	strcpy(query, "");
	sprintf(query, "SELECT filename FROM VolContent WHERE fileloc = '%s';", dmount);
	sqlite3_stmt *res, *res2;
	const char *tail;
	
    	do {
		rc = sqlite3_prepare_v2(db, query, 1000, &res, &tail);
	} while (rc == SQLITE_BUSY);

	String filename = "", newloc = "";
	String longname = "";	// long filename
	int newtid;		// tid of destinatation target
	double newavspace;	// available space at destination
    while (sqlite3_step(res) == SQLITE_ROW) {
		strcpy(filename, sqlite3_column_text(res, 0));	// file in dmount
		// select target(tid, mountpt) with most available space (same with file_map)
		String rm = "";
		sprintf(rm, "rm %s/%s", SHARE_LOC, filename);
		system(rm);


		strcpy(query2, "SELECT avspace, mountpt, tid FROM TARGET WHERE avspace = (SELECT max(avspace) from Target);");
		
	    do {
		rc = sqlite3_prepare_v2(db, query2, 1000, &res2, &tail);
	    } while (rc == SQLITE_BUSY);
	    if (sqlite3_step(res2) == SQLITE_ROW) {
			newavspace = sqlite3_column_double(res2,0);
			strcpy(newloc, sqlite3_column_text(res2, 1));
			newtid = sqlite3_column_int(res2, 2);
			printf("File %s is transfered to %s.\n", filename, newloc);
			strcpy(comm, "");
			sprintf(comm, "mv '%s/%s' '%s/%s'", dmount, filename, newloc, filename);
			system(comm);
		}

	    sqlite3_finalize(res2);

		// update volcontent
		strcpy(query2, "");
		sprintf(query2, "UPDATE VolContent SET fileloc = '%s' WHERE filename = '%s'", newloc, filename);
	do {
		rc = sqlite3_exec(db, query2, 0, 0, &errmsg);
	} while (rc == SQLITE_BUSY);


		// update cache content
		strcpy(query2, "");
		sprintf(query2, "UPDATE CacheContent SET mountpt = '%s' WHERE filename = '%s'", newloc, filename);
		
		do {
			rc = sqlite3_exec(db, query2, 0, 0, &errmsg);
		} while (rc == SQLITE_BUSY);

		// update target to decrease avspace
		strcpy(longname, "");	// di ko alam bakit pero takot ako lagi baka mag segmentation fault so ninunull ko nlng
		sprintf(longname, "%s/%s", newloc, filename);
		FILE *fp = fopen(longname, "rb");
		fseek(fp, 0L, SEEK_END);
		double sz = ftell(fp); //get filesize of file
		newavspace = newavspace - sz;
		printf("longname=%s\n", longname);
		printf("newavspace = %lf\n", newavspace);
		printf("size=%lf\n", sz);
		sprintf(query2, "UPDATE Target SET avspace = '%lf' WHERE tid = '%d';", newavspace, newtid);
		do {
			rc = sqlite3_exec(db, query2, 0, 0, &errmsg);
		} while (rc == SQLITE_BUSY);
	}



    sqlite3_finalize(res);
	// yehey tapos na?

	
	//unmount
   	String umount = "";
   	sprintf(umount, "umount -l '%s'", dmount);
	printf("umount = %s\n", umount);
   	system(umount);

	//logout of iscsitarget
	String logout = "";
	sprintf(logout, "iscsiadm -m node -u -T %s -p %s:3260", iqn, ipadd);
	printf("logout = %s\n", logout);
	system(logout);	

   	
   	//remove folder
   	String rm = "";
   	sprintf(rm, "rm -rf '%s'", dmount);
	printf("rm = %s\n", rm);
   	system(rm);
   	
   	//remove logical volume
   	String lvremove = "";
   	sprintf(lvremove, "lvremove -f '%s'", lvname);
	printf("lvrem = %s\n", lvremove);
  	system(lvremove);
  
  	//remove volume group
  	String vgremove = "", vgname = "";
  	char *pch = NULL;
  	int counter = 0;
  	pch = strtok(lvname, "/");
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
  	

	String pvremove = "";

	String newpt = "";
	strcpy(newpt, dmount);
	memmove(newpt, newpt + strlen("/mnt/lv"), 1 + strlen(newpt + strlen("/mnt/lv")));
	sprintf(pvremove, "pvremove -ff '%s'", newpt);
	printf("pvremove = %s\n", newpt);
	system(pvremove);  

	system("cat /proc/partitions > AvailableDisks.txt");

	// close db
	sqlite3_close(db);
	syslog(LOG_INFO, "decomm: Decommissioning successful!");
	printf("SUCCESS");

	//deactivate volume here
       


	return 0;
}
