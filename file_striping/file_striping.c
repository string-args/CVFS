/*
   Name: File Striping Module
   Description:
      Stripe files into smaller parts.
      File parts threshold = 124MB.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "../Global/global_definitions.h"
#include "../volume_management/file_mapping.h"
#include "file_striping.h"

#define MAX_PARTS   4096     // maximum number of stripe parts
// this can handle 20GB striped into 64MB of stripe size

void stripe(String dirpath, String fullpath, String filename) {
 
  FILE *file_to_open;

  //size_t threshold = 536870912; /*124MB*/
  size_t bytes_read;

  int part_count = 1;
  char *buffer = malloc(STRIPE_SIZE); /*buffer to put data on*/

  String part_name;
  //String full;
  String comm;

  syslog(LOG_INFO, "FileStriping: File %s will be striped.\n", filename);
  sprintf(comm, "rm '%s'", fullpath);

  file_to_open = fopen(fullpath, "rb");

  //printf("PATH : %s\n", fullpath);
  //printf("FILENAME : %s\n", filename);


  if (file_to_open == NULL){
    syslog(LOG_INFO, "FileStriping: fopen() unsuccessful!\n");
  }

  String path;
  sprintf(path, "%s/%s", TEMP_LOC, dirpath);


  memmove(filename, filename+strlen(dirpath), 1 +strlen(filename+strlen(dirpath)));

  int flag = 1;
  FILE *fp = fopen("../file_transaction/is_striping.txt", "w");
  fprintf(fp, "%d", flag);
  fclose(fp);
  String partnames[MAX_PARTS];
  String partfiles[MAX_PARTS];
  while ((bytes_read = fread(buffer, sizeof(char), STRIPE_SIZE, file_to_open)) > 0){
       sprintf(part_name, "%spart%d.%s", path, part_count, filename);
       FILE *file_to_write = fopen(part_name, "wb");
       fwrite(buffer, 1, bytes_read, file_to_write);
       fclose(file_to_write);
       String part_file = "";
       sprintf(part_file, "%s", part_name);
       memmove(part_file,part_file+strlen("/mnt/CVFSTemp/"),1+strlen(part_file+strlen("/mnt/CVFSTemp/")));

       strcpy(partnames[part_count - 1], part_name);
       strcpy(partfiles[part_count - 1], part_file);
       part_count++;
  }
  file_map_stripe(partnames, partfiles, part_count - 1);    // not so sure if -1

  free(buffer);
  system(comm);
  fclose(file_to_open);

  flag = 0;
  fp = fopen("../file_transaction/is_striping.txt", "w");
  fprintf(fp, "%d", flag);
  fclose(fp);
}
