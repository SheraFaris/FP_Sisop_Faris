#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void fsInit() {
  struct map_fs map_fs_buf;
  int i = 0;

  readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  for (i = 0; i < 16; i++) map_fs_buf.is_used[i] = true;
  for (i = 256; i < 512; i++) map_fs_buf.is_used[i] = true;
  writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
}

// TODO: 2. Implement fsRead function
void fsRead(struct file_metadata* metadata, enum fs_return* status) {
  // Deklarasi buffer
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;

  int i;
  int node_index = -1;
  struct node_item current_node; // Deklarasi saja
  struct data_item current_data; // Deklarasi saja

  // Langkah 1: Membaca filesystem dari disk ke memory.
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

  // Langkah 2: Iterasi setiap item node untuk mencari node yang sesuai.
  for (i = 0; i < FS_MAX_NODE; ++i) {
      if (node_fs_buf.nodes[i].parent_index == metadata->parent_index &&
          strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0) {
          node_index = i;
          break;
      }
  }

  // Langkah 3: Jika node yang dicari tidak ditemukan.
  if (node_index == -1) {
      *status = FS_R_NODE_NOT_FOUND;
      return;
  }

  current_node = node_fs_buf.nodes[node_index];

  // Langkah 4: Jika node yang ditemukan adalah direktori.
  if (current_node.data_index == FS_NODE_D_DIR) {
      *status = FS_R_TYPE_IS_DIRECTORY;
      return;
  }

  // Langkah 5: Jika node yang ditemukan adalah file.
  current_data = data_fs_buf.datas[current_node.data_index];

  metadata->filesize = 0;
  for (i = 0; i < FS_MAX_SECTOR; ++i) {
      if (current_data.sectors[i] == 0x00) {
          break;
      }
      readSector(metadata->buffer + i * SECTOR_SIZE, current_data.sectors[i]);
      metadata->filesize += SECTOR_SIZE;
  }

  // Langkah 6: Set status sukses.
  *status = FS_SUCCESS;
}

// TODO: 3. Implement fsWrite function
void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
  struct map_fs map_fs_buf;
  struct node_fs node_fs_buf;
  struct data_fs data_fs_buf;
  // Buffer untuk menampung data dari filesystem

  // Deklarasi semua variabel di atas (Kompabilitas C89)
  int i, j;
  int free_node_index = -1;
  int free_data_index = -1;
  int free_blocks_count = 0;
  unsigned int sectors_needed;

  // Langkah 1: Membaca filesystem dari disk ke memory.
  readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

  // Langkah 2: Cek apakah node dengan nama dan parent yang sama sudah ada.
  for (i = 0; i < FS_MAX_NODE; ++i) {
      if (node_fs_buf.nodes[i].parent_index == metadata->parent_index &&
          strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name) == 0) {
          // Dalam enum, nama statusnya adalah FS_W_NODE_ALREADY_EXISTS
          *status = FS_W_NODE_ALREADY_EXISTS;
          return;
      }
  }

  // Langkah 3: Cari node kosong.
  for (i = 0; i < FS_MAX_NODE; ++i) {
      if (node_fs_buf.nodes[i].node_name[0] == '\0') {
          free_node_index = i;
          break;
      }
  }
  if (free_node_index == -1) {
      *status = FS_W_NO_FREE_NODE;
      return;
  }

  // Penanganan berbeda untuk file dan direktori
  if (metadata->filesize > 0) { // Proses penulisan FILE
      // Hitung jumlah sektor yang dibutuhkan
      sectors_needed = (metadata->filesize + SECTOR_SIZE - 1) / SECTOR_SIZE;

      // Langkah 4: Cari data entry yang kosong.
      for (i = 0; i < FS_MAX_DATA; ++i) {
          if (data_fs_buf.datas[i].sectors[0] == 0x00) {
              free_data_index = i;
              break;
          }
      }
      if (free_data_index == -1) {
          *status = FS_W_NO_FREE_DATA;
          return;
      }

      // Langkah 5: Hitung blok kosong pada map.
      for (i = 0; i < SECTOR_SIZE; ++i) {
          if (!map_fs_buf.is_used[i]) {
              free_blocks_count++;
          }
      }
      if (free_blocks_count < sectors_needed) {
          *status = FS_W_NOT_ENOUGH_SPACE;
          return;
      }

      // Langkah 6 (sebagian): Set data index untuk file.
      node_fs_buf.nodes[free_node_index].data_index = free_data_index;

      // Langkah 7: Lakukan penulisan data.
      j = 0; // j adalah counter sektor yang telah ditulis
      for (i = 0; i < SECTOR_SIZE && j < sectors_needed; ++i) {
          if (!map_fs_buf.is_used[i]) {
              // Tulis data dari buffer ke sektor disk ke-i
              writeSector(metadata->buffer + j * SECTOR_SIZE, i);

              // Update map: tandai sektor ke-i sebagai terpakai
              map_fs_buf.is_used[i] = true;

              // Update data entry: catat bahwa sektor ke-i digunakan
              data_fs_buf.datas[free_data_index].sectors[j] = i;

              j++; // Inkrementasi counter sektor tertulis
          }
      }

  } else { // Proses pembuatan DIREKTORI
      // Langkah 6 (sebagian): Set data index untuk direktori.
      node_fs_buf.nodes[free_node_index].data_index = FS_NODE_D_DIR;
  }

  // Langkah 6 (lanjutan): Set nama dan parent index pada node.
  strcpy(node_fs_buf.nodes[free_node_index].node_name, metadata->node_name);
  node_fs_buf.nodes[free_node_index].parent_index = metadata->parent_index;

  // Langkah 8: Tulis kembali filesystem yang telah diubah ke disk.
  writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
  writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
  writeSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

  // Langkah 9: Set status sukses.
  // Berdasarkan enum, status sukses umum adalah FS_SUCCESS
  *status = FS_SUCCESS;
}
