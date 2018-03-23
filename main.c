
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> 
#include <string.h> 
#include <unistd.h> 
#include <stdbool.h> 
#include <signal.h>
#include "zkc.h" 



extern zkcInfo g_zkcInfo ;


void help(char **argv)
{
  fprintf(stderr,
"\nStarts a zookeeper client app cluster. \n"
"%s <options>\n"
"options: \n"
"-C <entry name>: create znode struct\n"
"-R <entry name>: release znode struct\n"
"-L <entry name>: upload config onto znode\n"
"-M <entry name>: fetch master address from znode\n"
"-F <entry name>: fetch config contents from znode\n"
"-c <config file>\n"
"-p <application file>\n"
"-H <zookeeper server address list>\n"
"-h: help message\n\n"
"normal HA mode:\n"
"%s -p ./app -c ./app.cfg -H 1.2.3.4:2181\n\n"
"create znode struct:\n"
"%s -C app -c ./app.cfg -H 1.2.3.4:2181\n\n"
"release znode struct:\n"
"%s -R app -H 1.2.3.4:2181\n\n"
"upload configs only:\n"
"%s -L app -c ./app/app.cfg -H 1.2.3.4:2181\n\n"
"fetch configs only:\n"
"%s -F app -H 1.2.3.4:2181\n\n"
"fetch master info:\n"
"%s -M app -H 1.2.3.4:2181\n\n",
  argv[0],argv[0],argv[0],argv[0],argv[0],argv[0],argv[0]
  );
  exit(-1);
}

int deal_cmdline(int argc, char **argv)
{
  char ch = 0, *ep = 0;
  const char *shortopts = "c:p:e:H:hC:R:L:F:M:";  
  const struct option longopts[] = {  
    {"conf", required_argument, NULL, 'c'},  
    {"app", required_argument, NULL, 'p'},  
    {"hosts", required_argument, NULL, 'H'},  
    {"create", required_argument, NULL, 'C'},  
    {"release", required_argument, NULL, 'R'},  
    {"upload", required_argument, NULL, 'L'},  
    {"fetch", required_argument, NULL, 'F'},  
    {"master", required_argument, NULL, 'M'},  
    {0, 0, 0, 0},  
  };  

  if (argc==1) {
    help(argv);
  }

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

      case 'L':
        ep = stpncpy(g_zkcInfo.entry,optarg,256);
        g_zkcInfo.mode = 3;
        break ;

      case 'F':
        ep = stpncpy(g_zkcInfo.entry,optarg,256);
        g_zkcInfo.mode = 4;
        break ;

      case 'M':
        ep = stpncpy(g_zkcInfo.entry,optarg,256);
        g_zkcInfo.mode = 5;
        break ;

      case 'h':
        help(argv);

      default:
        printf("unknown option '%c'\n",ch);
        return -1 ;
    }
    *ep= '\0';
  }

  return 0;
}

void sig_hdlr(int s)
{
  if (g_zkcInfo.p_child>0) {
    kill(g_zkcInfo.p_child,SIGKILL);
    exit(0);
  }
}

int main(int argc, char *argv[])
{
  bzero(&g_zkcInfo,sizeof(g_zkcInfo));

  if (deal_cmdline(argc,argv)) {
    return -1;
  }

  /* register signals */
  signal(SIGILL,sig_hdlr);
  signal(SIGSEGV,sig_hdlr);
  signal(SIGFPE,sig_hdlr);
  signal(SIGTERM,sig_hdlr);
  signal(SIGINT,sig_hdlr);

  /* normal HA process */
  if (g_zkcInfo.mode==0) {

    if (init_zkc_ha()) 
      return -1;

    zkc_ha_process();
  } 

  else {
    bool bCreate = g_zkcInfo.mode==1 ;

    if (init_zkc_maintainance(bCreate)) 
      return -1;
    
    /* maintainance mode: create znode struct */
    if (g_zkcInfo.mode==1)
      zkc_create_znode();

    /* maintainance mode: release znode struct */
    else if (g_zkcInfo.mode==2)
      zkc_release_znode();

    /* maintainance mode: upload configs to znode */
    else if (g_zkcInfo.mode==3) 
      zkc_upload_config();

    /* maintainance mode: fetch configs from znode */
    else if (g_zkcInfo.mode==4) 
      zkc_fetch_config();

    /* maintainance mode: fetch master address from znode */
    else if (g_zkcInfo.mode==5) 
      zkc_fetch_master_addr();
  }

  return 0;
}


