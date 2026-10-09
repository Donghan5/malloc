#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <errno.h>

static int run(const char *binary, const char *index, int enabled, int count)
{
 FILE *out = tmpfile();
 int status;
 pid_t pid;
 if (!out) return -1;
 fflush(NULL);
 pid = fork();
 if (pid < 0) { fclose(out); return -1; }
 if (!pid) {
  struct rlimit limit = {0,0};
  setrlimit(RLIMIT_CORE, &limit);
  unsetenv("LD_PRELOAD");
  if (enabled) { setenv("MALLOC_DEBUG","1",1); setenv("MALLOC_SCRIBBLE","1",1); }
  else { unsetenv("MALLOC_DEBUG"); unsetenv("MALLOC_SCRIBBLE"); }
  if (dup2(fileno(out),1)<0 || dup2(fileno(out),2)<0) _exit(125);
  alarm(180);
  if (index) execl(binary,binary,index,(char *)NULL);
  else execl(binary,binary,(char *)NULL);
  _exit(127);
 }
 while (waitpid(pid,&status,0)<0) { if(errno!=EINTR) { fclose(out); return -1; } }
 rewind(out);
 int result = WIFEXITED(status) ? WEXITSTATUS(status) : 128+WTERMSIG(status);
 if (count) {
  unsigned n; char extra;
  if(result || fscanf(out,"%u %c",&n,&extra)!=1 || !n || n>10000) result=-1;
  else result=(int)n;
 } else {
  char buf[4096]; size_t n;
  while((n=fread(buf,1,sizeof(buf),out))) fwrite(buf,1,n,stdout);
  printf("%s %s%s%s (status %d)\n",result?"FAIL":"PASS",binary,index?" case ":"",index?index:"",result);
 }
 fclose(out); return result;
}
int main(int argc,char **argv)
{
 const struct { const char *path, *group, *argument; int suite; } bins[]={
  {"./test/bin/mandatory/basic","mandatory",NULL,1},
  {"./test/bin/mandatory/zones_output","mandatory",NULL,1},
  {"./test/bin/mandatory/heap_edges","mandatory",NULL,1},
  {"./test/bin/mandatory/reclamation","mandatory",NULL,1},
  {"./test/bin/bonus/coalescing","bonus",NULL,1},
  {"./test/bin/bonus/coalescing_edges","bonus",NULL,1},
  {"./test/bin/bonus/diagnostics","bonus",NULL,1},
  {"./test/bin/bonus/diagnostics_edges","bonus",NULL,1},
  {"./test/bin/bonus/concurrency","bonus",NULL,1},
  {"./test/bin/bonus/concurrency_edges","bonus",NULL,1},
  {"./test/bin/integration/build_contracts","integration",NULL,0},
  {"./test/integration/run.sh","integration","eval",0}
 };
 unsigned pass=0,total=0;
 if(argc>2 || (argc==2 && strcmp(argv[1],"mandatory") && strcmp(argv[1],"bonus"))) return 2;
 for(size_t g=0;g<sizeof(bins)/sizeof(bins[0]);g++) {
  const char *path=bins[g].path;
  if(argc==2 && strcmp(bins[g].group,argv[1])) continue;
  if(!bins[g].suite) { ++total; pass+=run(path,bins[g].argument,0,0)==0; continue; }
  int n=run(path,NULL,0,1);
  if(n<0) { ++total; printf("FAIL suite count %s\n",path); continue; }
  for(int mode=0;mode<(!strcmp(bins[g].group,"mandatory")?2:1);mode++) for(int i=0;i<n;i++) {
   char index[32]; snprintf(index,sizeof(index),"%d",i);
   if(mode) printf("[debug/scribble enabled]\n");
   ++total; pass+=run(path,index,mode,0)==0;
  }
 }
 printf("Result: %u/%u passed; %u failed\n",pass,total,total-pass);
 return pass!=total;
}
