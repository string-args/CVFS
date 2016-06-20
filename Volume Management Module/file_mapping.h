/*
   Description:
     Definitions of File Mapping Submodule to be shared to other modules.
*/

void file_map(String filename);

void file_map_cache(String filename);

void update_target_size_delete(sqlite3 *db, String filename, String fileloc);
