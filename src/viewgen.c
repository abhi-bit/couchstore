#include "config.h"
#include <libcouchstore/couch_index.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static int usage() {
    fprintf(stderr, "Usage: couch_viewgen [flag] <input file> [[flag] <input file> ...] <output file>\n"
            "       flags: --reduce=count\n"
            "              --reduce=sum\n"
            "              --reduce=stats\n");
    return EXIT_FAILURE;
}


int main(int argc, char** argv)
{
    if(argc < 3)
        return usage();

    couchstore_error_t errcode;
    CouchStoreIndex* index = NULL;

    const char* indexPath = argv[argc - 1];
    errcode = couchstore_create_index(indexPath, &index);
    if (errcode) {
        fprintf(stderr, "Couldn't open database %s: %s\n", indexPath, couchstore_strerror(errcode));
        goto cleanup;
    }

    const char* reduceName = NULL;
    couchstore_json_reducer reducer = COUCHSTORE_REDUCE_NONE;
    
    for (int i = 1; i < argc - 1; ++i) {
        const char* inputPath = argv[i];
        if (strncmp(inputPath, "--reduce=", 9) == 0) {
            reduceName = inputPath + 9;
            if (strcmp(reduceName, "count") == 0)
                reducer = COUCHSTORE_REDUCE_COUNT;
            else if (strcmp(reduceName, "sum") == 0)
                reducer = COUCHSTORE_REDUCE_SUM;
            else if (strcmp(reduceName, "stats") == 0)
                reducer = COUCHSTORE_REDUCE_STATS;
            else {
                fprintf(stderr, "Unknown reduce function '%s'\n", reduceName);
                return usage();
            }
        } else {
            printf("Adding %s to %s", inputPath, indexPath);
            if (reduceName)
                printf(", reducing by %s", reduceName);
            printf(" ...\n");
            errcode = couchstore_index_add(inputPath, reducer, index);
            if (errcode < 0) {
                fprintf(stderr, "Error adding %s: %s\n", inputPath, couchstore_strerror(errcode));
                goto cleanup;
            }
            reduceName = NULL;
            reducer = COUCHSTORE_REDUCE_NONE;
        }
    }
    printf("Done!");

cleanup:
    if (index) {
        couchstore_close_index(index);
        if (errcode < 0) {
            unlink(indexPath);
        }
    }
    return errcode ? EXIT_FAILURE : EXIT_SUCCESS;
}
