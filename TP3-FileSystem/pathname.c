#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int pathname_lookup(struct unixfilesystem *fs, const char *pathname)
{
    if (!fs || !pathname || pathname[0] != '/')
    {
        fprintf(stderr, "Invalid path\n");
        return -1;
    }

    // Caso especial: directorio raíz
    if (strcmp(pathname, "/") == 0)
    {
        return 1;
    }

    int curr_inumber = 1; // Inodo raíz
    char path_copy[strlen(pathname) + 1];
    strcpy(path_copy, pathname);

    char *token = strtok(path_copy, "/");
    while (token != NULL)
    {
        struct direntv6 entry;
        if (directory_findname(fs, token, curr_inumber, &entry) < 0)
        {
            fprintf(stderr, "Component not found: %s\n", token);
            return -1;
        }

        // Verificar que el inodo encontrado sea válido
        struct inode in;
        if (inode_iget(fs, entry.d_inumber, &in) < 0)
        {
            fprintf(stderr, "Invalid inode: %d\n", entry.d_inumber);
            return -1;
        }

        curr_inumber = entry.d_inumber;
        token = strtok(NULL, "/");
    }

    return curr_inumber;
}