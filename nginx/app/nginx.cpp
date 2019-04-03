#include "ngx_cpp_main.h"

using namespace std;
int main(int argc,char ** argv)
{
  size_t argv_length{0};
  int i{0};
  while(argv[i])
  {
    argv_length += strlen(argv[i++])+1;
  }  
  CNgx_cpp_main ngx_main(argv_length,argc,argv);
  return ngx_main.ngx_main();
}
