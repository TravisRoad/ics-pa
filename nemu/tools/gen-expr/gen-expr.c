#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"#include <stdint.h>\n"
"int main() { "
"  uint64_t result = %s; "
"  printf(\"%%lu\", result); "
"  return 0; "
"}";

static int idx = -1;
static int token_num = 0;

void gen(char c){
  buf[++idx] = c;
  token_num++;
}

void gen_num(){
  int num = rand() % 99 + 1;
  char str[3];
  sprintf(str, "%d", num);
  for(int i = 0; i < 3 && str[i] != '\0'; ++i){
    gen(str[i]);
  }
}

void gen_rand_op(){
  int op = rand() % 4;
  switch(op){
    case 0: gen('+'); break;
    case 1: gen('-'); break;
    case 2: gen('*'); break;
    case 3: gen('/'); break;
  }
}

static void gen_rand_expr() {
  int val = rand() % 100;
  if(val > 55){
    gen_num();
  }
  else if (val > 30){
    gen('('); gen_rand_expr(); gen(')');
  }
  else{
    gen_rand_expr(); gen_rand_op(); gen_rand_expr();
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 10;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    idx = -1;
    token_num = 0;
    gen_rand_expr();
    gen('\0');
    if (token_num >= 32){
      --i;
      continue;
    }

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    uint64_t result;
    fscanf(fp, "%lu", &result);
    pclose(fp);

    printf("%lu %s\n", result, buf);
  }
  return 0;
}
