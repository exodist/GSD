#ifndef RESULT_RESULT_H
#define RESULT_RESULT_H
#include "../include/gsd_struct_result.h"

struct result {
    // If these are both 0 then the operation failed, but there was no error.
    // This happens when failure is an acceptable result. An example would be
    // if an insert into a dict fails because the key is already present, this
    // is a blocked transaction, but no error has actually occured, it is
    // working as designed.
    res_status_code status; // True on success
    res_error_code  error;  // True on error

    // Sometimes an error will come with a helpful message, this can be NULL.
    const char *message;

    enum {
        RESULT_ITEM_NONE = 0,
        RESULT_ITEM_REF,
        RESULT_ITEM_PTR,
        RESULT_ITEM_NUM
    } item_type;

    union {
        void    *ptr;
        ref     *ref;
        int64_t  num;
    } item;
};

#endif
