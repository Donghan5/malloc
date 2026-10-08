#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
static long measure(const char *library,const char *mode)
{
 pid_t pid=fork(); int status; struct rusage usage;
 if(pid<0) return -1;
 if(!pid) {
  struct rlimit limit={0,0}; setrlimit(RLIMIT_CORE,&limit);
  unsetenv("MALLOC_DEBUG"); unsetenv("MALLOC_SCRIBBLE");
  setenv("LD_PRELOAD",library,1); setenv("EXPECTED_MALLOC_LIBRARY",library,1);
  alarm(10);
  execl("./test/bin/integration/workload","workload",mode,(char *)NULL);
  _exit(127);
 }
 while(wait4(pid,&status,0,&usage)<0) if(errno!=EINTR) return -1;
 if(!WIFEXITED(status) || WEXITSTATUS(status)) {
  fprintf(stderr,"FAIL workload %s status=%d\n",mode,status); return -1;
 }
 return usage.ru_minflt;
}
int main(void)
{
 char library[PATH_MAX]; int failures=0;
 if(getenv("LD_PRELOAD")) { fprintf(stderr,"driver must run without LD_PRELOAD\n"); return 1; }
 if(!realpath("./libft_malloc.so",library)) { perror("library"); return 1; }
 printf("Local equivalent workload (로컬 동등 workload); wait4 ru_minflt; library=%s\n",library);
 for(int i=0;i<5;i++) {
  long baseline=measure(library,"baseline"), hold=measure(library,"hold"), release=measure(library,"free");
  int success=baseline>=0 && hold>=0 && release>=0;
  int efficiency=success && hold-baseline>=255 && hold-baseline<=272;
  int quality=success && release-baseline<=3;
  printf("repeat %d raw baseline=%ld hold=%ld free=%ld delta hold=%ld free=%ld execution=%s allocation-score=%s free-score=%s\n",i+1,baseline,hold,release,hold-baseline,release-baseline,success?"PASS":"FAIL",efficiency?"PASS":"FAIL",quality?"PASS":"FAIL");
  failures+=!success || !efficiency || !quality;
 }
 printf("Preload measurements: %d/5 passed; %d failed\n",5-failures,failures);
 return failures!=0;
}
