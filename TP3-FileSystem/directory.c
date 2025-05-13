#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define DIRENT_NAME_LEN 14

int directory_findname(struct unixfilesystem *fs, const char *name, int dirinumber, struct direntv6 *dirEnt)
{
  struct inode dir_inode;
  if (inode_iget(fs, dirinumber, &dir_inode) < 0)
  {
    fprintf(stderr, "Error getting inode %d\n", dirinumber);
    return -1;
  }

  // Verificar que sea un directorio
  if ((dir_inode.i_mode & IFMT) != IFDIR)
  {
    fprintf(stderr, "Inode %d is not a directory\n", dirinumber);
    return -1;
  }

  int filesize = inode_getsize(&dir_inode);
  int num_blocks = (filesize + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;

  for (int b = 0; b < num_blocks; b++)
  {
    struct direntv6 dir_entries[DISKIMG_SECTOR_SIZE / sizeof(struct direntv6)];
    int bytes_read = file_getblock(fs, dirinumber, b, dir_entries);
    if (bytes_read <= 0)
      continue;

    int n_entries = bytes_read / sizeof(struct direntv6);
    for (int i = 0; i < n_entries; i++)
    {
      if (strncmp(name, dir_entries[i].d_name, DIRENT_NAME_LEN) == 0)
      {
        *dirEnt = dir_entries[i];
        return 0;
      }
    }
  }

  return -1;
}
