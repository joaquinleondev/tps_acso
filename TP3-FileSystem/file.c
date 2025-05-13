#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf)
{
    struct inode in;

    // Obtener el inodo
    if (inode_iget(fs, inumber, &in) < 0)
    {
        fprintf(stderr, "file_getblock: error al obtener inodo %d\n", inumber);
        return -1;
    }

    // Obtener bloque físico correspondiente al bloque lógico blockNum
    int phys_block = inode_indexlookup(fs, &in, blockNum);
    if (phys_block <= 0)
    {
        fprintf(stderr, "file_getblock: bloque físico inválido (%d) para blockNum %d\n", phys_block, blockNum);
        return -1;
    }

    // Leer bloque físico del disco
    int err = diskimg_readsector(fs->dfd, phys_block, buf);
    if (err == -1)
    {
        fprintf(stderr, "file_getblock: error leyendo sector %d\n", phys_block);
        return -1;
    }

    // Calcular el tamaño del archivo
    int filesize = (in.i_size0 << 16) | in.i_size1;

    // Calcular cuántos bytes válidos hay en este bloque
    int start_byte = blockNum * DISKIMG_SECTOR_SIZE;
    int remaining_bytes = filesize - start_byte;

    if (remaining_bytes >= DISKIMG_SECTOR_SIZE)
    {
        return DISKIMG_SECTOR_SIZE;
    }
    else if (remaining_bytes > 0)
    {
        return remaining_bytes;
    }
    else
    {
        // El bloque solicitado está fuera del tamaño del archivo
        return 0;
    }
}
