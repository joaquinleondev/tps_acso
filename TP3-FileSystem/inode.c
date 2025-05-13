#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "inode.h"
#include "diskimg.h"

#define INODES_PER_BLOCK (DISKIMG_SECTOR_SIZE / sizeof(struct inode))

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp)
{
    if (!fs || !inp || inumber <= 0)
    {
        return -1;
    }

    // Calcular bloque y offset del inodo
    int inode_block = INODE_START_SECTOR + (inumber - 1) / INODES_PER_BLOCK;
    int inode_offset = (inumber - 1) % INODES_PER_BLOCK;

    // Leer el bloque de inodos
    struct inode inodes[INODES_PER_BLOCK];
    if (diskimg_readsector(fs->dfd, inode_block, inodes) != DISKIMG_SECTOR_SIZE)
    {
        return -1;
    }

    // Copiar el inodo solicitado
    *inp = inodes[inode_offset];

    // Verificar si el inodo está asignado
    if ((inp->i_mode & IALLOC) == 0)
    {
        return -1;
    }

    return 0;
}

int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum)
{
    if (blockNum < 0)
        return -1;

    int pointers_per_block = DISKIMG_SECTOR_SIZE / sizeof(uint16_t);

    if ((inp->i_mode & ILARG) == 0)
    {
        // Archivo pequeño: bloques directos
        if (blockNum >= 8)
            return -1;
        return inp->i_addr[blockNum];
    }
    else
    {
        // Archivo grande: bloques indirectos o doblemente indirecto
        int max_indirect_blocks = 7 * pointers_per_block;

        if (blockNum < max_indirect_blocks)
        {
            // Indirectos simples
            int indirect_block = blockNum / pointers_per_block;
            int offset = blockNum % pointers_per_block;

            uint16_t blockno = inp->i_addr[indirect_block];
            if (blockno <= 0)
                return -1;

            uint16_t pointers[pointers_per_block];
            if (diskimg_readsector(fs->dfd, blockno, pointers) != DISKIMG_SECTOR_SIZE)
                return -1;

            return pointers[offset];
        }
        else
        {
            // Doble indirecto
            int double_indirect_index = blockNum - max_indirect_blocks;

            uint16_t double_indirect_blockno = inp->i_addr[7];
            if (double_indirect_blockno <= 0)
                return -1;

            uint16_t indirect_block_pointers[pointers_per_block];
            if (diskimg_readsector(fs->dfd, double_indirect_blockno, indirect_block_pointers) != DISKIMG_SECTOR_SIZE)
                return -1;

            int first_level_index = double_indirect_index / pointers_per_block;
            int second_level_index = double_indirect_index % pointers_per_block;

            if (first_level_index >= pointers_per_block)
                return -1;

            uint16_t indirect_block = indirect_block_pointers[first_level_index];
            if (indirect_block <= 0)
                return -1;

            uint16_t data_block_pointers[pointers_per_block];
            if (diskimg_readsector(fs->dfd, indirect_block, data_block_pointers) != DISKIMG_SECTOR_SIZE)
                return -1;

            return data_block_pointers[second_level_index];
        }
    }
}

int inode_getsize(struct inode *inp)
{
    return ((inp->i_size0 << 16) | inp->i_size1);
}