
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> 
#include <string.h> 
#include <unistd.h> 
#include <stdbool.h> 
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>
#include "zkc.h"


zkcInfo g_zkcInfo ;



int get_master_addr(void)
{
  int i = 0;
  struct ifreq *ifr;
  struct ifconf ifc;
  char buf[1024];
  int fd = socket(AF_INET,SOCK_DGRAM,0);

  if (fd<0) {
    return -1;
  }

  /* iterate all local interfaces */
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = buf;
  if (ioctl(fd,SIOCGIFCONF,(char*)&ifc)<0) {
    /* XXX: error message here */
    return -1; 
  }

  for (i=ifc.ifc_len/sizeof(struct ifreq),
    ifr = ifc.ifc_req; --i>=0; ifr++) 
  {
    if (!strcmp(ifr->ifr_ifrn.ifrn_name,"lo")) {
      continue ;
    }

    char *ipaddr = inet_ntoa(((struct sockaddr_in*)
      &ifr->ifr_ifru.ifru_addr)->sin_addr);

    //printf("addr: %s\n",);
    strcpy(g_zkcInfo.ipAddr,ipaddr);
    break ;
  }

  return 0;
}

#if 1
static const char* state2String(int state){
  if (state == 0)
    return "CLOSED_STATE";
  if (state == ZOO_CONNECTING_STATE)
    return "CONNECTING_STATE";
  if (state == ZOO_ASSOCIATING_STATE)
    return "ASSOCIATING_STATE";
  if (state == ZOO_CONNECTED_STATE)
    return "CONNECTED_STATE";
  if (state == ZOO_EXPIRED_SESSION_STATE)
    return "EXPIRED_SESSION_STATE";
  if (state == ZOO_AUTH_FAILED_STATE)
    return "AUTH_FAILED_STATE";

  return "INVALID_STATE";
}

static const char* type2String(int state){
  if (state == ZOO_CREATED_EVENT)
    return "CREATED_EVENT";
  if (state == ZOO_DELETED_EVENT)
    return "DELETED_EVENT";
  if (state == ZOO_CHANGED_EVENT)
    return "CHANGED_EVENT";
  if (state == ZOO_CHILD_EVENT)
    return "CHILD_EVENT";
  if (state == ZOO_SESSION_EVENT)
    return "SESSION_EVENT";
  if (state == ZOO_NOTWATCHING_EVENT)
    return "NOTWATCHING_EVENT";

  return "UNKNOWN_EVENT_TYPE";
}
#endif

int get_result_code(void)
{
  return __sync_fetch_and_and(&g_zkcInfo.res_code,ZOK);
}

bool is_conf_node(char *data)
{
  return data && !strcmp(data,g_zkcInfo.confNode);
}

bool is_master_node(char *data)
{
  return data && !strcmp(data,g_zkcInfo.masterNode);
}

void create_complete(int rc, const char *name, const void *data) 
{
  /* completion creation of the znode */
  __sync_lock_test_and_set(&g_zkcInfo.res_code,rc);

  sem_post(&g_zkcInfo.s);

  if (data)
    free((void*)data);
}

void stat_complete(int rc, const struct Stat *stat, const void *data) 
{
  //fprintf(stderr,"rc=%d, data: %s\n",rc,(char*)data);

  if (data) 
    free((void*)data);
}

void get_complete(int rc, const char *value, int value_len, 
   const struct Stat *stat, const void *data) 
{
#if 0
	fprintf(stderr, "%s: rc = %d\n", (char*)data, rc);
	if (value) {
			fprintf(stderr, " value_len = %d\n", value_len);
			assert(write(2, value, value_len) == value_len);
	}
	fprintf(stderr, "\nStat:\n");
#endif

  /* get the 'conf' node ok */
  if (is_conf_node((char*)data)) {
    /* TODO: process the config contents */
    if (rc==ZOK && value_len>0) {
    }

    __sync_lock_test_and_set(&g_zkcInfo.res_code,rc);

    sem_post(&g_zkcInfo.s);
  }

  if (data)
    free((void*)data);
}


void watcher(zhandle_t *zzh, int type, int state, const char *path, void* context)
{
  fprintf(stderr, "Watcher %s state = %s", type2String(type), state2String(state));
  
  /* the master node is deleted */
  if (is_master_node((char*)path) && type==ZOO_DELETED_EVENT) {

    __sync_lock_test_and_set(&g_zkcInfo.res_code,ZOK);

    sem_post(&g_zkcInfo.s);
  }

}

int init_zkc_ha(void)
{
  if (!*g_zkcInfo.zkhosts) {
    fprintf(stderr,"no zookeeper host list given!\n");
    return -1;
  }

  if (!*g_zkcInfo.app) {
    fprintf(stderr,"no application file given!\n");
    return -1;
  }

  if (!*g_zkcInfo.conf) {
    fprintf(stderr,"no config file given!\n");
    return -1;
  }

  if (access(g_zkcInfo.app,F_OK|X_OK)) {
    fprintf(stderr,"can't access application file '%s'\n",g_zkcInfo.app);
    return -1;
  }

  if (!access(g_zkcInfo.conf,F_OK|R_OK|W_OK)) {
    fprintf(stderr,"warning: the config file '%s' will be overwirtten!\n",g_zkcInfo.conf);
  }

  if (sem_init(&g_zkcInfo.s,0,0)) {
    fprintf(stderr,"sem init fail: %s",strerror(errno));
    return -1;
  }

  if (get_master_addr()) {
    return -1;
  }

  /* get entry name from application path */
  {
    char *p = strrchr(g_zkcInfo.app,'/');
    char *ep= stpncpy(g_zkcInfo.entry,p?(p+1):g_zkcInfo.app,256);
    *ep= '\0';
  }

  sprintf(g_zkcInfo.confNode,"/%s/conf",g_zkcInfo.entry);
  sprintf(g_zkcInfo.masterNode,"/%s/master",g_zkcInfo.entry);

  /* init the zookeeper client handle */
  g_zkcInfo.zh = zookeeper_init(g_zkcInfo.zkhosts, watcher, 30000, 0, 0, 0);
  if (!g_zkcInfo.zh) {
    fprintf(stderr,"init zookeeper client failed\n");
    return -1;
  }

  return 0;
}


/* the main zookeeper client process */
int zkc_ha_process(void)
{
  int ret = 0;
  const char *pMastNode = g_zkcInfo.masterNode ;
  const char *pConfNode = g_zkcInfo.confNode ;

  while (1) {

    /* 
     * create the $appname/master 
     */
    ret = zoo_acreate(g_zkcInfo.zh, pMastNode, g_zkcInfo.ipAddr, 
      strlen(g_zkcInfo.ipAddr), &ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL,
      create_complete, strdup(pMastNode));
    if (ret) {
      fprintf(stderr,"error create '%s'\n",pMastNode);
      return -1;
    }

    sem_wait(&g_zkcInfo.s);

    if ((ret=get_result_code())==ZOK) {
      fprintf(stderr,"\ncreate '%s' ok\n",pMastNode);
      break ;
    }

    /* node exists, wait for its delete event */
    if (ret==ZNODEEXISTS) {
      printf("\nwaiting for create '%s'...\n",pMastNode);

      /* add watch */
      ret = zoo_aexists(g_zkcInfo.zh, pMastNode, 1, stat_complete, 
        /*strdup(pMastNode)*/0);
      if (ret==-1) {
        fprintf(stderr,"error add watch on '%s'\n", pMastNode);
        return -1;
      }

      sem_wait(&g_zkcInfo.s);

      /* the master node is not existing now, 
       *  try to create it */
    } 
    else if (ret==ZNONODE) {
      fprintf(stderr,"the upper node of '%s' may not "
        "exists, create it first\n", pMastNode);
      return -1;
    }

  } // end while


  /* read config */
  ret = zoo_aget(g_zkcInfo.zh,pConfNode,1,get_complete,
      strdup(pConfNode));
  if (ret==-1) {
    fprintf(stderr,"error get '%s'\n",pConfNode);
    return -1;
  }

  sem_wait(&g_zkcInfo.s);

  if (get_result_code()!=ZOK) {
    fprintf(stderr,"get '%s' fail\n",pConfNode);
    return -1;
  }

  return 0;
}

int init_zkc_create(void)
{
  if (!*g_zkcInfo.zkhosts) {
    fprintf(stderr,"no zookeeper host list given!\n");
    return -1;
  }

  if (!*g_zkcInfo.entry) {
    fprintf(stderr,"no zookeeper entry given!\n");
    return -1;
  }

  if (!*g_zkcInfo.conf) {
    fprintf(stderr,"no config file given!\n");
    return -1;
  }

  if (access(g_zkcInfo.conf,F_OK|R_OK)) {
    fprintf(stderr,"the config file '%s' not exists!\n",g_zkcInfo.conf);
    return -1;
  }

  if (sem_init(&g_zkcInfo.s,0,0)) {
    fprintf(stderr,"sem init fail: %s",strerror(errno));
    return -1;
  }

  /* init the zookeeper client handle */
  g_zkcInfo.zh = zookeeper_init(g_zkcInfo.zkhosts, watcher, 30000, 0, 0, 0);
  if (!g_zkcInfo.zh) {
    fprintf(stderr,"init zookeeper client failed\n");
    return -1;
  }

  return 0;
}

int zkc_create_znode(void)
{
  int ret = 0;
  char zpath[PATH_MAX] = "";
  char contents[1<<20] = "";
  ssize_t szCont = 0L;

  snprintf(zpath,PATH_MAX,"/%s",g_zkcInfo.entry);

  while (1) {
    /* 
     * create the /$appname znode
     */
    ret = zoo_acreate(g_zkcInfo.zh, zpath, "new", 
      3, &ZOO_OPEN_ACL_UNSAFE, 0,
      create_complete, strdup(zpath));
    if (ret) {
      fprintf(stderr,"error create '%s'\n",zpath);
      return -1;
    }

    sem_wait(&g_zkcInfo.s);

    if ((ret=get_result_code())!=ZOK && ret!=ZNODEEXISTS) {
      fprintf(stderr,"\ncreate '%s' fail: %d\n",
        zpath,ret);
      return -1;
    }

    if (strstr(zpath,"/conf")) {
      fprintf(stderr,"done\n");
      break ;
    }

    /* 
     * create the /$appname/conf znode 
     */
    strcat(zpath,"/conf");
  }

  /* read configs */
  {
    int fd = open(g_zkcInfo.conf,0,0777);
    /* the default storage size of a znode is 1M */

    if (fd<0) {
      fprintf(stderr,"error open config file '%s'\n",g_zkcInfo.conf);
      return -1;
    }

    szCont = read(fd,contents,1<<20);
    close(fd);

    if (szCont==-1) {
      fprintf(stderr,"error read config file '%s': %s\n",
        g_zkcInfo.conf,strerror(errno));
      return -1;
    }
  }

  /* write config into /$appname/conf */
  ret = zoo_aset(g_zkcInfo.zh, zpath, contents, szCont, -1, stat_complete,
      /*strdup(zpath)*/0);
  if (ret) {
    fprintf(stderr,"error write '%s'\n",zpath);
    return -1;
  }

  fprintf(stderr,"refresh config done\n");

  return 0;
}

int init_zkc_release(void)
{
  if (!*g_zkcInfo.zkhosts) {
    fprintf(stderr,"no zookeeper host list given!\n");
    return -1;
  }

  if (!*g_zkcInfo.entry) {
    fprintf(stderr,"no zookeeper entry given!\n");
    return -1;
  }

  if (sem_init(&g_zkcInfo.s,0,0)) {
    fprintf(stderr,"sem init fail: %s",strerror(errno));
    return -1;
  }

  /* init the zookeeper client handle */
  g_zkcInfo.zh = zookeeper_init(g_zkcInfo.zkhosts, watcher, 30000, 0, 0, 0);
  if (!g_zkcInfo.zh) {
    fprintf(stderr,"init zookeeper client failed\n");
    return -1;
  }

  return 0;
}

void delete_complete(int rc, const void *data) 
{
  fprintf(stderr, "%s: rc = %d\n", (char*)data, rc);

  if (data)
    free((void*)data);
}

int zkc_release_znode(void)
{
  int ret = 0;
  char zpath[PATH_MAX] = "";
  
  /* try to delete the '/conf' node */
  sprintf(zpath,"/%s/conf",g_zkcInfo.entry);
  ret = zoo_adelete(g_zkcInfo.zh, zpath, -1, delete_complete, 
      strdup(zpath));



  return 0;
}



