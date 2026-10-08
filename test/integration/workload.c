#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>
/* Identical startup and symbol verification in all measured processes. */
int main(int argc,char **argv)
{
 Dl_info info;
 struct stat expected, actual;
 const char *library=getenv("EXPECTED_MALLOC_LIBRARY");
 void *symbol=dlsym(RTLD_DEFAULT,"malloc");
 void *p[1024];
 if(argc!=2 || !library || !symbol || symbol != (void *)malloc || !dladdr(symbol,&info)
  || stat(library,&expected) || stat(info.dli_fname,&actual)
  || expected.st_dev!=actual.st_dev || expected.st_ino!=actual.st_ino) {
  write(2,"malloc injection verification failed\n",37); return 2;
 }
 if(!strcmp(argv[1],"baseline")) return 0;
 int release=!strcmp(argv[1],"free");
 if(!release && strcmp(argv[1],"hold")) return 2;
 for(size_t i=0;i<1024;i++) {
  p[i]=malloc(1024);
  if(!p[i]) return 3;
  volatile unsigned char *bytes=p[i];
  for(size_t j=0;j<1024;j++) bytes[j]=(unsigned char)(i+j);
  if(release) free(p[i]);
 }
 /* Volatile writes and reads keep the retained allocations observable. */
 if(!release) {
  volatile unsigned long sum=0;
  for(size_t i=0;i<1024;i++) sum+=*(volatile unsigned char *)p[i];
  (void)sum;
 }
 return 0;
}
