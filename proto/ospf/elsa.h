/*
 * $Id: elsa.h $
 *
 * Author: Markus Stenberg <fingon@iki.fi>
 *
 * Created:       Wed Aug  1 13:31:21 2012 mstenber
 * Last modified: Thu Aug  2 16:27:21 2012 mstenber
 * Edit time:     44 min
 *
 */

/* TODO:
 *
 * - consider the LSA body+data handling semantics - non-copying
 * ones might be nice to have.
 */

#ifndef ELSA_H
#define ELSA_H

/*
 * Public bidirectional interface to the external-LSA code.
 *
 * elsa_* calls are called by the client application.
 *
 * elsai_* calls are called by ELSA code which wants to do something.
 *
 * General design criteria is that the data in network format should
 * be usable directly; what that means, is that whatever we use _has_
 * to be in network order (e.g. big endian), and therefore conversions
 * should be done in the platform functionality. Also, the LSA data is
 * assumed to be big endian.
 */

/* Whoever includes this file should provide the appropriate
 * definitions for the structures involved. The ELSA code treats them
 * as opaque. */
#include "elsa_platform.h"
/* e.g.
   typedef .. my stuff *elsa_client;
   typedef .. *elsa_lsa;
   typedef .. *elsa_if;
   ( typesafety is mandatory).
 */


/* Only externally visible pointer elsa itself provides. */
typedef struct elsa_struct *elsa;
typedef unsigned short elsa_lsatype;

/******************************************* outside world -> ELSA interface */

/* Create an ELSA instance. NULL is returned in case of an error. */
elsa elsa_create(elsa_client client);

/* Change notifications */
void elsa_lsa_changed(elsa e, elsa_lsatype lsatype);
void elsa_lsa_deleted(elsa e, elsa_lsatype lsatype);

/* Whether ELSA supports given LSAtype. */
bool elsa_supports_lsatype(elsa_lsatype lsatype);

/* Dispatch ELSA action - should be called once a second (or so). */
void elsa_dispatch(elsa e);

/* Destroy an ELSA instance. */
void elsa_destroy(elsa e);

/******************************************* ELSA -> outside world interface */

/* Allocate a block of memory. The memory should be zeroed. */
void *elsai_calloc(elsa_client client, size_t size);

/* Free a block of memory. */
void elsai_free(elsa_client client, void *ptr);


/* Get current router ID. */
uint32_t elsai_get_rid(elsa_client client);

/* (Try to) change the router ID of the router. */
void elsai_change_rid(elsa_client client);

/* Get first LSA by type. */
elsa_lsa elsai_get_lsa_by_type(elsa_client client, elsa_lsatype lsatype);

/* Get next LSA by type. */
elsa_lsa elsai_get_lsa_by_type_next(elsa_client client, elsa_lsa lsa);

/* Get interface */
elsa_if elsai_if_get(elsa_client client);

/* Get next interface */
elsa_if elsai_if_get_next(elsa_client client, elsa_if ifp);


/**************************************************** LSA handling interface */

/* Originate LSA.

   rid is implicitly own router ID.
   age/area is implicitly zero.
*/
void elsai_lsa_originate(elsa_client client,
                         elsa_lsatype lsatype,
                         uint32_t lsid,
                         uint32_t sn,
                         void *body, size_t body_len);

/* Getters */
elsa_lsatype elsai_lsa_get_type(elsa_lsa lsa);
uint32_t elsai_lsa_get_rid(elsa_lsa lsa);
uint32_t elsai_lsa_get_lsid(elsa_lsa lsa);
void elsai_lsa_get_body(elsa_lsa lsa, unsigned char **body, size_t *body_len);
void elsai_lsa_get_elsa_data(elsa_lsa lsa, unsigned char **data, size_t *data_len);

/******************************************************** Interface handling */

const char * elsai_if_get_name(elsa_client client, elsa_if i);
uint32_t elsai_if_get_index(elsa_client client, elsa_if i);
uint32_t elsai_if_get_neigh_iface_id(elsa_client client, elsa_if i, uint32_t rid);
uint8_t elsai_if_get_priority(elsa_client client, elsa_if i);

#endif /* ELSA_H */