#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void shell() {
  char buf[64];
  char cmd[64];
  char arg[2][64];

  byte cwd = FS_NODE_P_ROOT;

  while (true) {
    printString("MengOS:");
    printCWD(cwd);
    printString("$ ");
    readString(buf);
    parseCommand(buf, cmd, arg);

    if (strcmp(cmd, "cd")) cd(&cwd, arg[0]);
    else if (strcmp(cmd, "ls")) ls(cwd, arg[0]);
    else if (strcmp(cmd, "mv")) mv(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cp")) cp(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cat")) cat(cwd, arg[0]);
    else if (strcmp(cmd, "mkdir")) mkdir(cwd, arg[0]);
    else if (strcmp(cmd, "clear")) clearScreen();
    else printString("Invalid command\n");
  }
}

// TODO: 4. Implement printCWD function
void printCWD(byte cwd) {
  // 1. Deklarasi variabel dan buffer
  struct node_fs node_fs_buf;
  byte path_indices[FS_MAX_NODE];
  int path_depth = 0;
  byte current_node_index = cwd;
  int i;

  // 2. Handle kasus khusus jika CWD adalah root
  if (cwd == FS_NODE_P_ROOT) {
      printString("/");
      return;
  }

  // 3. Baca node table dari disk
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  // 4. Telusuri path dari CWD ke atas (ke root) dan simpan jejaknya
  while (current_node_index != FS_NODE_P_ROOT) {
      path_indices[path_depth] = current_node_index;
      path_depth++;
      current_node_index = node_fs_buf.nodes[current_node_index].parent_index;
  }

  // 5. Cetak path dalam urutan yang benar (dari atas ke bawah)
  printString("/");

  for (i = path_depth - 1; i >= 0; i--) {
      printString(node_fs_buf.nodes[path_indices[i]].node_name);
      if (i > 0) {
          printString("/");
      }
  }
}

// // TODO: 5. Implement parseCommand function
// void parseCommand(char* buf, char* cmd, char arg[2][64]) {
//   // 1. Deklarasi variabel
//   int i, j;
//   // 'part' melacak bagian mana yang sedang diparsing:
//   int part = 0;

//   // 2. Inisialisasi output agar bersih
//   cmd[0] = '\0';
//   arg[0][0] = '\0';
//   arg[1][0] = '\0';

//   // 3. Lewati spasi di awal string
//   i = 0;
//   while (buf[i] == ' ' && buf[i] != '\0') {
//       i++;
//   }

//   // 4. Mulai mem-parsing setiap bagian
//   j = 0; 
//   for (; buf[i] != '\0'; i++) {
//       if (buf[i] == ' ') {
//           part++;
//           j = 0; 

//           while (buf[i+1] == ' ' && buf[i+1] != '\0') {
//               i++;
//           }
//           if (part > 2) {
//               break;
//           }
//       } else {
//           if (part == 0) {
//               cmd[j] = buf[i];
//               cmd[j+1] = '\0'; 
//           } else if (part == 1) {
//               arg[0][j] = buf[i];
//               arg[0][j+1] = '\0';
//           } else if (part == 2) {
//               arg[1][j] = buf[i];
//               arg[1][j+1] = '\0';
//           }
//           j++; 
//       }
//   }
// }

void parseCommand(char* buf, char* cmd, char arg[2][64]) {
  int i, j;
  int part = 0;

  cmd[0] = '\0';
  arg[0][0] = '\0';
  arg[1][0] = '\0';

  i = 0;
  while (buf[i] == ' ' && buf[i] != '\0') {
      i++;
  }

  j = 0;
  for (; buf[i] != '\0'; i++) {
      if (buf[i] == ' ') {
          part++;
          j = 0;
          while (buf[i+1] == ' ' && buf[i+1] != '\0') {
              i++;
          }
          if (part > 2) {
              break;
          }
      } else {
          if (part == 0) {
              cmd[j] = buf[i];
              cmd[j+1] = '\0';
          } else if (part == 1) {
              arg[0][j] = buf[i];
              arg[0][j+1] = '\0';
          } else if (part == 2) {
              arg[1][j] = buf[i];
              arg[1][j+1] = '\0';
          }
          j++;
      }
  }
}

void cd(byte* cwd, char* dirname) {
  struct node_fs node_fs_buf;
  byte current_search_dir;
  char path_part[MAX_FILENAME];
  int i, j, k;
  int path_is_valid;
  int found_part;

  path_is_valid = 1; // 1 untuk true

  if (dirname[0] == '\0') {
      return;
  }

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  // PERBAIKAN: Disesuaikan dengan strcmp kustom
  if (strcmp(dirname, "/")) {
      *cwd = FS_NODE_P_ROOT;
      return;
  }

  if (dirname[0] == '/') {
      current_search_dir = FS_NODE_P_ROOT;
      i = 1;
  } else {
      current_search_dir = *cwd;
      i = 0;
  }

  while (dirname[i] != '\0' && path_is_valid) {
      j = 0;
      while (dirname[i] != '/' && dirname[i] != '\0') {
          path_part[j] = dirname[i];
          j++;
          i++;
      }
      path_part[j] = '\0';

      if (path_part[0] == '\0') {
          if (dirname[i] == '/') i++;
          continue;
      }

      // PERBAIKAN: Disesuaikan dengan strcmp kustom
      if (strcmp(path_part, ".")) {
          if (dirname[i] == '/') i++;
          continue;
      }
      if (strcmp(path_part, "..")) {
          if (current_search_dir != FS_NODE_P_ROOT) {
              current_search_dir = node_fs_buf.nodes[current_search_dir].parent_index;
          }
          if (dirname[i] == '/') i++;
          continue;
      }

      found_part = 0; // 0 untuk false
      for (k = 0; k < FS_MAX_NODE; ++k) {
          // PERBAIKAN: Disesuaikan dengan strcmp kustom
          if (node_fs_buf.nodes[k].parent_index == current_search_dir && strcmp(node_fs_buf.nodes[k].node_name, path_part)) {
              if (node_fs_buf.nodes[k].data_index == FS_NODE_D_DIR) {
                  current_search_dir = k;
                  found_part = 1;
              } else {
                  printString("cd: '"); printString(path_part); printString("': Not a directory\n");
                  path_is_valid = 0;
              }
              break;
          }
      }

      if (!found_part && path_is_valid) {
          printString("cd: '"); printString(path_part); printString("': No such file or directory\n");
          path_is_valid = 0;
      }
      
      if (dirname[i] == '/') {
          i++;
      }
  }

  if (path_is_valid) {
      *cwd = current_search_dir;
  }
}

// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {
  // Deklarasi variabel 
  struct node_fs node_fs_buf;
  byte target_dir_index;
  bool target_found = false;
  int i;
  bool first_item = true;

  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

  // Kasus 1: 'ls' atau 'ls .' -> Tampilkan isi CWD
  if (dirname[0] == '\0' || strcmp(dirname, ".") == 0) {
      target_dir_index = cwd;
      target_found = true;
  } else {
      // Kasus 2: 'ls <dirname>' -> Cari direktori dan tampilkan isinya
      for (i = 0; i < FS_MAX_NODE; ++i) {
          if (node_fs_buf.nodes[i].parent_index == cwd &&
              strcmp(node_fs_buf.nodes[i].node_name, dirname) == 0) {
              
              if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                  target_dir_index = i;
                  target_found = true;
              } else {
                  printString("ls: '");
                  printString(dirname);
                  printString("' is not a directory\n");
                  return; 
              }
              break; 
          }
      }
  }

  // Jika direktori target ditemukan, tampilkan isinya
  if (target_found) {
      for (i = 0; i < FS_MAX_NODE; ++i) {
          if (node_fs_buf.nodes[i].parent_index == target_dir_index) {
              if (!first_item) {
                  printString("  ");
              }
              printString(node_fs_buf.nodes[i].node_name);
              first_item = false;
          }
      }
      printString("\n"); 
  } else {
      printString("ls: cannot access '");
      printString(dirname);
      printString("': No such file or directory\n");
  }
}

void parsePath(char* fullpath, char* dir_path, char* filename) {
  int i;
  int len = 0;
  int last_slash_idx = -1;

  for (len = 0; fullpath[len] != '\0'; ++len) {
      if (fullpath[len] == '/') {
          last_slash_idx = len;
      }
  }

  if (last_slash_idx == -1) {
      strcpy(dir_path, ".");
      strcpy(filename, fullpath);
  } else {
      strcpy(filename, &fullpath[last_slash_idx + 1]);
      
      if (last_slash_idx == 0) {
          strcpy(dir_path, "/"); 
      } else {
          for (i = 0; i < last_slash_idx; ++i) {
              dir_path[i] = fullpath[i];
          }
          dir_path[last_slash_idx] = '\0';
      }
  }
}

// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst) {
    // --- Deklarasi Variabel (C89 Style) ---
    struct node_fs node_fs_buf;
    char dest_dir_path[64];
    char new_filename[MAX_FILENAME];
    int src_node_index = -1;
    byte dest_dir_index = 0; // Inisialisasi
    bool dest_dir_found = false;
    int i;

    // --- Langkah 1: Baca filesystem ---
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // --- Langkah 2: Parsing path tujuan ---
    parsePath(dst, dest_dir_path, new_filename);
    if (new_filename[0] == '\0') {
        printString("mv: missing destination file operand\n");
        return;
    }

    // --- Langkah 3: Cari file sumber ---
    for (i = 0; i < FS_MAX_NODE; ++i) {
        if (node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, src) == 0) {
            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                printString("mv: cannot move directories\n");
                return;
            }
            src_node_index = i;
            break;
        }
    }
    if (src_node_index == -1) {
        printString("mv: file not found\n");
        return;
    }

    // --- Langkah 4: Cari direktori tujuan ---
    if (strcmp(dest_dir_path, "/") == 0) {
        dest_dir_index = FS_NODE_P_ROOT;
        dest_dir_found = true;
    } else if (strcmp(dest_dir_path, "..") == 0) {
        dest_dir_index = (cwd == FS_NODE_P_ROOT) ? FS_NODE_P_ROOT : node_fs_buf.nodes[cwd].parent_index;
        dest_dir_found = true;
    } else if (strcmp(dest_dir_path, ".") == 0) {
        dest_dir_index = cwd;
        dest_dir_found = true;
    } else { // Cari sub-direktori
        for (i = 0; i < FS_MAX_NODE; ++i) {
            if (node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, dest_dir_path) == 0) {
                if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                    dest_dir_index = i;
                    dest_dir_found = true;
                }
                break;
            }
        }
    }
    if (!dest_dir_found) {
        printString("mv: destination directory not found\n");
        return;
    }

    // --- Langkah 5: Cek konflik nama di tujuan ---
    for (i = 0; i < FS_MAX_NODE; ++i) {
        if (i != src_node_index && node_fs_buf.nodes[i].parent_index == dest_dir_index && strcmp(node_fs_buf.nodes[i].node_name, new_filename) == 0) {
            printString("mv: destination file already exists\n");
            return;
        }
    }

    // --- Langkah 6: Lakukan pemindahan/penggantian nama ---
    node_fs_buf.nodes[src_node_index].parent_index = dest_dir_index;
    strcpy(node_fs_buf.nodes[src_node_index].node_name, new_filename);

    // --- Langkah 7: Tulis kembali ke disk ---
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {
    // --- Deklarasi Variabel ---
    struct file_metadata src_meta, dst_meta;
    enum fs_return status;
    char dest_dir_path[64];
    char new_filename[MAX_FILENAME];
    byte dest_dir_index;
    bool dest_dir_found = false;
    struct node_fs node_fs_buf;
    int i;

    // --- Langkah 1: Baca file sumber ---
    src_meta.parent_index = cwd;
    strcpy(src_meta.node_name, src);
    fsRead(&src_meta, &status);

    if (status == FS_R_NODE_NOT_FOUND) {
        printString("cp: file not found\n");
        return;
    }
    if (status == FS_R_TYPE_IS_DIRECTORY) {
        printString("cp: cannot copy directories\n");
        return;
    }
    if (status != FS_SUCCESS) {
        printString("cp: error reading source file\n");
        return;
    }

    // --- Langkah 2: Parsing path tujuan dan cari direktori tujuan ---
    parsePath(dst, dest_dir_path, new_filename);
    if (new_filename[0] == '\0') {
        printString("cp: missing destination file operand\n");
        return;
    }
    
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    if (strcmp(dest_dir_path, "/") == 0) {
        dest_dir_index = FS_NODE_P_ROOT;
        dest_dir_found = true;
    } else if (strcmp(dest_dir_path, "..") == 0) {
        dest_dir_index = (cwd == FS_NODE_P_ROOT) ? FS_NODE_P_ROOT : node_fs_buf.nodes[cwd].parent_index;
        dest_dir_found = true;
    } else if (strcmp(dest_dir_path, ".") == 0) {
        dest_dir_index = cwd;
        dest_dir_found = true;
    } else { // Cari sub-direktori
        for (i = 0; i < FS_MAX_NODE; ++i) {
            if (node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, dest_dir_path) == 0) {
                if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                    dest_dir_index = i;
                    dest_dir_found = true;
                }
                break;
            }
        }
    }
    if (!dest_dir_found) {
        printString("cp: destination directory not found\n");
        return;
    }

    // --- Langkah 3: Siapkan metadata untuk file tujuan dan tulis ---
    dst_meta.parent_index = dest_dir_index;
    strcpy(dst_meta.node_name, new_filename);
    dst_meta.filesize = src_meta.filesize;

    // Salin isi buffer dari source ke destination
    for (i = 0; i < src_meta.filesize; ++i) {
        dst_meta.buffer[i] = src_meta.buffer[i];
    }
    
    fsWrite(&dst_meta, &status);

    // --- Langkah 4: Handle status dari proses penulisan ---
    if (status == FS_W_NODE_ALREADY_EXISTS) {
        printString("cp: destination file already exists\n");
    } else if (status == FS_W_NOT_ENOUGH_SPACE) {
        printString("cp: not enough space on device\n");
    } else if (status != FS_SUCCESS) {
        printString("cp: error writing destination file\n");
    }
    // Jika sukses, tidak ada pesan yang ditampilkan.
}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {
  // --- Deklarasi Variabel ---
  struct file_metadata metadata;
  enum fs_return status;
  int i;
  char single_char_buffer[2];


  // --- Langkah 1: Siapkan metadata dan panggil fsRead ---
  metadata.parent_index = cwd;
  strcpy(metadata.node_name, filename);

  fsRead(&metadata, &status);

  // --- Langkah 2: Periksa hasil dari fsRead ---
  if (status == FS_R_NODE_NOT_FOUND) {
      printString("cat: '");
      printString(filename);
      printString("': No such file or directory\n");
      return;
  }
  
  if (status == FS_R_TYPE_IS_DIRECTORY) {
      printString("cat: '");
      printString(filename);
      printString("': Is a directory\n");
      return;
  }

  if (status == FS_SUCCESS) {
      // --- Langkah 3: Cetak isi buffer jika sukses ---

      single_char_buffer[1] = '\0';
      for (i = 0; i < metadata.filesize; ++i) {
          single_char_buffer[0] = metadata.buffer[i];
          printString(single_char_buffer);
      }
      printString("\n"); 
  }
}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
  struct file_metadata metadata;
  enum fs_return status;

  if (dirname[0] == '\0') {
      printString("mkdir: missing operand\n");
      return;
  }

  metadata.parent_index = cwd;
  strcpy(metadata.node_name, dirname);
  metadata.filesize = 0;

  fsWrite(&metadata, &status);

  if (status == FS_W_NODE_ALREADY_EXISTS) {
      printString("mkdir: cannot create directory '");
      printString(dirname);
      printString("': File exists\n");
  } else if (status == FS_W_NO_FREE_NODE) {
      printString("mkdir: filesystem has no free nodes\n");
  } else if (status != FS_SUCCESS) {
      printString("mkdir: failed to create directory\n");
  }
}
