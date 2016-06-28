/*
    Manual ISCSI configuration
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../Global/global_definitions.h"
#include "../Utilities/cmd_exec.h"
#include "../volume_management/make_volumes.h"


int main(int argc, char *argv[]) {

    if(argc != 2){
		printf("FATAL: Program takes exactly 1 argument.\n");
		printf("Usage:\n");
		printf("\tadd_target <IP address of target>\n");
		printf("FAILED");
		exit(1);
	}

    String ipadd = "";
    strcpy(ipadd, argv[1]);
    printf("Target IP Address: %s\n", ipadd);

    String comm, out;
    sprintf(comm, "nmap %s -p3260 --open | grep open", ipadd);
    runCommand(comm, out);
    if (strcmp(out, "") == 0) {
        printf("Error: Invalid Target (%s)\n", ipadd);
        printf("FAILED");
        exit(1);
    }

    // target is valid, proceed to discovery
    sprintf(comm, "iscsiadm -m discover -t sendtargets -p %s", ipadd);
    system(comm);
    printf("\n\nLogging in to targets...");
    system("iscsiadm -m node --login");
    // sleep(5);

    // volume manager: initialize and create volumes
    makeVolume(1);
    printf("\n\nAvailable partitions written to file \"%s\"", AV_DISKS);
    /* overwrites previous, para pag next update, i exempt na yung updated disk */
    sprintf(comm, "cat /proc/partitions | awk '{print $4}' | grep 'sd[b-z] > %s", AV_DISKS);
    system(comm);

    printf("\n\nAdding target finished :)\n");
    printf("SUCCESS");
    return 0;
}


// check if valid target (discovery, check for 3260)
// initial config will check for available targets in the network
// automatic configuration of volumes
// update will be manual, admin will input IP / target IQN
