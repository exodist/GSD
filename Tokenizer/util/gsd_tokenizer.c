#include <errno.h>
#include "../src/include/gsd_tokenizer_api.h"

int main(int argc, char* argv[]) {
    token_set *ts = NULL;
    if (argc > 1) {
        ts = tokenize_file( argv[1] );
        if(ts->error) {
            if (ts->error == TOKEN_ERROR_FILE) {
                errno = ts->file_error;
                perror(argv[1]);
                return 1;
            }
            else {
                fprintf( stderr, "Error.\n" );
                return 1;
            }
        }
    }
    else {
        ts = tokenize_stream(stdin);
    }
    dump_token_set( ts );
    free_token_set( ts );
    return 0;
}

