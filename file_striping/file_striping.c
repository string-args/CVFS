/*
   Name: File Striping Module
   Description:
      Stripe files into smaller parts.
      File parts threshold = 124MB.

*/

#include <stdio.h>
#include <stdlib.h>

#include "../Global/global_definitions.h"
#include "../volume_management/file_mapping.h"
#include "file_striping.h"

void stripe(String filename){

  FILE *file_to_open;

  //size_t threshold = 536870912; /*124MB*/
  size_t bytes_read;

  int part_count = 1;
  char *buffer = malloc(STRIPE_SIZE); /*buffer to put data on*/

  String part_name;
  String full;
  String comm;

  printf("File %s will be striped.\n", filename);
  sprintf(full, "/mnt/CVFSTemp/%s", filename);
  sprintf(comm, "rm '%s'", full);

  file_to_open = fopen(full, "rb");

  if (file_to_open == NULL){
    printf("fopen() unsuccessful!\n");
  }

  while ((bytes_read = fread(buffer, sizeof(char), STRIPE_SIZE, file_to_open)) > 0){
       sprintf(part_name, "/mnt/CVFSTemp/part%d.%s", part_count, filename);
       FILE *file_to_write = fopen(part_name, "wb");
       fwrite(buffer, 1, bytes_read, file_to_write);
       fclose(file_to_write);
       String mapfile;
       sprintf(mapfile, "part%d.%s", part_count, filename);
       file_map(mapfile, "AIDZWHATTOPUT??");
       part_count++;
  }
  free(buffer);
  system(comm);
  fclose(file_to_open);

}
