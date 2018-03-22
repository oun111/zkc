
#ifndef __ZKC_H__
#define __ZKC_H__

#include <limits.h> 
#include <zookeeper.h> 
#include <semaphore.h>


typedef struct zkc_info {
  /* 0: normal, 1: create znode struct, 2: clear znode struct */
  int mode ;

  /* entry point in zookeeper */
  char entry[256];

  /* obsolute path of application */
  char app[PATH_MAX] ;
 
  /* obsolute path of application's config */
  char conf[PATH_MAX] ;

  char ipAddr[64];

  char masterNode[PATH_MAX];
  char confNode[PATH_MAX];

  /* zookeeper handle */
  zhandle_t *zh ;

  char zkhosts[1024];

  sem_t s;

  int res_code ;

} zkcInfo ;



extern int init_zkc_ha(void);

extern int init_zkc_create(void);

extern int init_zkc_release(void);

extern int zkc_ha_process(void);

extern int zkc_create_znode(void);

extern int zkc_release_znode(void);

#endif /* __ZKC_H__*/

