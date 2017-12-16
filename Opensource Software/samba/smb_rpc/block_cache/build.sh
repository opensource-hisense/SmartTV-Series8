#!/bin/sh

rm ./b; gcc -g -Wall -Werror -D_GNU_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 bc_block_cache.c bc_hash.c bc_free_block.c bc_file_index.c bc_dlist.c atom_strdup.c -o b -lpthread -lrt; time ./b

