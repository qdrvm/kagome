#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  const char *suffix = ".dylib";
  char *dylib = (char *)malloc(strlen(argv[0]) + strlen(suffix) + 1);
  strcpy(dylib, argv[0]);
  strcpy(dylib + strlen(argv[0]), suffix);

  void *lib = dlopen(dylib, RTLD_NOW);
  if (lib == NULL) {
    printf("dlopen: %s\n", dlerror());
    return -1;
  }
  void *sym = dlsym(lib, "main");
  if (sym == NULL) {
    printf("dlsym: %s\n", dlerror());
    return -1;
  }
  return ((int (*)(int, char **))sym)(argc, argv);
}
