
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h> 
#include <limits.h> 
#include <string.h> 
#include <unistd.h> 


typedef struct app_info {
  /* entry point in zookeeper */
  char entry[PATH_MAX];

  /* obsolute path of application */
  char app[PATH_MAX] ;
 
  /* obsolute path of application's config */
  char conf[PATH_MAX] ;

} appInfo ;

appInfo g_appInfo ;


void help(void)
{
  fprintf(stderr,
"\nStarts a zookeeper cluster. \n"
"%s <options>\n"
"options: \n"
"-c config file\n"
"-p application file\n"
"-e zookeeper entry\n",
  "zkc"
  );
  exit(-1);
}

int parse_cmdline(int argc, char **argv)
{
  char ch = 0, *ep = 0;
  const char *shortopts = "c:p:e:";  
  const struct option longopts[] = {  
    {"conf", required_argument, NULL, 'c'},  
    {"app", required_argument, NULL, 'p'},  
    {"entry", required_argument, NULL, 'e'},  
    {0, 0, 0, 0},  
  };  

  while ((ch = getopt_long(argc,argv,shortopts,longopts,NULL))!=-1) {
    switch(ch) {
      case 'c':
        ep = stpncpy(g_appInfo.conf,optarg,PATH_MAX);
        break ;

      case 'p':
        ep = stpncpy(g_appInfo.app,optarg,PATH_MAX);
        break ;

      case 'e':
        ep = stpncpy(g_appInfo.entry,optarg,PATH_MAX);
        break ;

      default:
        printf("unknown option '%c'\n",ch);
        return -1 ;
    }
    *ep= '\0';
  }
  return 0;
}

int init(void)
{
  bzero(&g_appInfo,sizeof(g_appInfo));

  return 0;
}

int check_params(void)
{
  if (!*g_appInfo.entry) {
    fprintf(stderr,"no zookeeper entry given!\n");
    help();
  }

  if (!*g_appInfo.app) {
    fprintf(stderr,"no application file given!\n");
    help();
  } else if (access(g_appInfo.app,F_OK|X_OK)) {
    fprintf(stderr,"can't access application file '%s'\n",g_appInfo.app);
    return -1;
  }

  if (!*g_appInfo.conf) {
    fprintf(stderr,"no config file given!\n");
    help();
  } else if (!access(g_appInfo.conf,F_OK|R_OK|W_OK)) {
    fprintf(stderr,"warning: the config file '%s' will be overwirtten!\n",g_appInfo.conf);
    return 0;
  }

  return 0;
}


int main(int argc, char *argv[])
{
  if (init()) {
    return -1;
  }

  if (parse_cmdline(argc,argv)) {
    return -1;
  }

  if (check_params()) {
    return -1;
  }



  return 0;
}


