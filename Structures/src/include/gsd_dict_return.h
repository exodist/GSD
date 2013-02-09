#ifndef GSD_DICT_ERROR_H
#define GSD_DICT_ERROR_H

#include <stdint.h>
#include <stdlib.h>

/* Return codes:
 * CODE < 0 - Fatal, cannot trust the dictionary after one of these
 * CODE = 0 - Success, nothing wrong
 * CODE > 0 - Action failed, but otherwise dictionary is fine
\*/

//        Error Name   Code  Fatal? Description
// These are explosions
#define DICT_INT_ERROR  -2 /* Y   Internal Error                              */
#define DICT_API_ERROR  -1 /* Y   API was used incorrectly                    */
// These are varying levels of success
#define DICT_NO_ERROR    0 /* N   Success, no error                           */
#define DICT_PATHO_ERROR 1 /* N   Operation would result in pathological tree */
// These are graceful failures 
#define DICT_MEM_ERROR   2 /* N   Out of memory,                              */
#define DICT_TRANS_FAIL  3 /* N   Transaction failed                          */
#define DICT_UNIMP_ERROR 4 /* N   Operation not implemented                   */

/* PATHO error is returned either when rebalancing a tree doesn't actually
 * balance it, or if one tree is significantly taller than its neighbors. Both
 * cases imply pathological data is being fed to your dictionary. A PATHO error
 * does NOT mean your operation failed.
 */

/* If the action triggered a rebalance this will be added to the return code,
 * if the return code is >= 100 it means that your action succeded, but
 * triggered a rebalance, the return code for the rebalance is
 *    (code - DICT_RBAL)
 *
 * If the rebalance fails your dictionary is uneffected and will still work
 * properly, it just won't be balanced until another insert triggers the
 * rebalance over again.
 *
 * NOTE1: It is still a good idea to abort on a fatal error (1000 > c > 100)
 * NOTE2: Do not ignore DICT_PATHO_ERROR, doing so will have drastic effects on
 *        performance. It occurs when rebalancing still results in an
 *        unbalanced tree, which means rebalances will happen frequently.
 *        Someone is probably exploiting a flaw in your program.
 */
#define DICT_RBAL 1000

#endif
