#include "content.h"
#include <iostream>

using namespace std;
using namespace minosys;

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("Usage: minosysscr <file>\n");
    return 0;
  }
  FILE *f = fopen(argv[1], "r");
  if (f) {
    LexFile lex(f);
    ContentTop top;
    Content *t = top.yylex(&lex);
    top.top = t;
    string s = top.toString();
    cout << s << endl;
  }
  fclose(f);
  return 0;
}

