
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> 
#include <string.h> 
#include <unistd.h> 
#include "zkc.h" 



extern zkcInfo g_zkcInfo ;


void help(void)
{
  fprintf(stderr,
"\nStarts a zookeeper cluster. \n"
"%s <options>\n"
"options: \n"
"-C <entry name>: create znode struct\n"
"-R <entry name>: release znode struct\n"
"-c <config file>\n"
"-p <application file>\n"
"-H <zookeeper server address list>\n"
"-h: help message\n",
  "zkc"
  );
  exit(-1);
}

int deal_cmdline(int argc, char **argv)
{
  char ch = 0, *ep = 0;
  const char *shortopts = "c:p:e:H:hC:R:";  
  const struct option longopts[] = {  
    {"conf", required_argument, NULL, 'c'},  
    {"app", required_argument, NULL, 'p'},  
    {"hosts", required_argument, NULL, 'H'},  
    {"create", required_argument, NULL, 'C'},  
    {"release", required_argument, NULL, 'R'},  
    {0, 0, 0, 0},  
  };  

  while ((ch = getopt_long(argc,argv,shortopts,longopts,NULL))!=-1) {
    switch(ch) {
      case 'c':
        ep = stpncpy(g_zkcInfo.conf,optarg,PATH_MAX);
        break ;

      case 'p':
        ep = stpncpy(g_zkcInfo.app,optarg,PATH_MAX);
        break ;

      case 'H':
        ep = stpncpy(g_zkcInfo.zkhosts,optarg,1024);
        break ;

      case 'C':
        ep = stpncpy(g_zkcInfo.entry,optarg,256);
        g_zkcInfo.mode = 1;
        break ;

      case 'R':
        ep = stpncpy(g_zkcInfo.entry,optarg,256);
        g_zkcInfo.mode = 2;
        break ;

      case 'h':
        help();

      default:
        printf("unknown option '%c'\n",ch);
        return -1 ;
    }
    *ep= '\0';
  }

  return 0;
}


int main(int argc, char *argv[])
{
  bzero(&g_zkcInfo,sizeof(g_zkcInfo));

  if (deal_cmdline(argc,argv)) {
    return -1;
  }

  /* normal HA process */
  if (g_zkcInfo.mode==0) {

    if (init_zkc_ha()) 
      return -1;

    zkc_ha_process();
  } 

  /* maintainance mode: create znode struct */
  else if (g_zkcInfo.mode==1) {

    if (init_zkc_create()) 
      return -1;

    zkc_create_znode();
  }

  /* maintainance mode: release znode struct */
  else if (g_zkcInfo.mode==2) {

    if (init_zkc_release()) 
      return -1;

    zkc_release_znode();
  }

  while (1) {
    sleep(1);
  }

  return 0;
}


