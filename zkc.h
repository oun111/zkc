
#ifndef __ZKC_H__
#define __ZKC_H__

#include <limits.h> 
#include <zookeeper.h> 
#include <semaphore.h>


typedef struct zkc_info {
  /* 0: normal, 
   * 1: create znode struct, 
   * 2: clear znode struct, 
   * 3: upload configs to znode only */
  int mode ;

  /* entry point in zookeeper */
  char entry[256];

  /* obsolute path of application */
  char app[PATH_MAX] ;
 
  /* obsolute path of application's config */
  char conf[PATH_MAX] ;

  char ipAddr[64];

  /* the '/master' znode */
  char masterNode[PATH_MAX];

  /* the '/conf' znode */
  char confNode[PATH_MAX];

  /* zookeeper handle */
  zhandle_t *zh ;

  char zkhosts[1024];

  sem_t s;

  int res_code ;

} zkcInfo ;



extern int init_zkc_ha(void);

extern int init_zkc_maintainance(bool isCreate);

extern int zkc_ha_process(void);

extern int zkc_create_znode(void);

extern int zkc_release_znode(void);

extern int zkc_upload_config(void);

#endif /* __ZKC_H__*/

