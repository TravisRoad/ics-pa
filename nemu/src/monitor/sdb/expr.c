#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_INT

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"\\-", '-'},
  {"\\*", '*'},
  {"/", '/'},
  {"\\(", '('},
  {"\\)", ')'},
  {"[0-9]+", TK_INT}
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

int eval(int p, int q, bool *success);
bool check_parentheses(int p, int q);

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            // i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        if(substr_len > 32) { // FIXME: stack overflow error
          assert(0);
          // panic("token too long");
        }

        switch (rules[i].token_type) {
          case TK_INT:
            tokens[nr_token].type = TK_INT;
            memcpy(tokens[nr_token].str, substr_start, substr_len + 1);
            nr_token++;
            break;
          case TK_NOTYPE:
            break;
          default:
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  // tokens[0] is the first token
  word_t val = eval(0, nr_token - 1, success);

  return val;
}

int eval(int p, int q, bool *success){
  // TODO:edge condition unconcerned
  if(p > q){
    *success = false;
    return 0; // TODO: bad expression
  }
  else if(p == q){
    switch(tokens[p].type){
      case TK_INT:
        *success = true;
        return atoi(tokens[p].str);
      default:
        return -1;
    }
  }
  else if(check_parentheses(p, q)){ // '(' and ')' are around the expression
    return eval(p+1, q-1, success);
  }
  else{
    // 首先要找到分界点，要求是不能是在括号当中，其次优先找优先度低的节点
    int div = p;
    int step = 0;
    for(int i = p; i <= q; ++i){
      if(tokens[i].type == '(') step++;
      if(tokens[i].type == ')') step--;
      if(step != 0) continue;

      if(tokens[i].type == '+' || tokens[i].type == '-'){
        div = i;
        continue;
      }

      if(tokens[i].type == '*' || tokens[i].type == '/'){
        if(tokens[div].type == '+' || tokens[div].type == '-'){
          continue;
        }
        div = i;
      }
    }

    bool left_success = false;
    bool right_success = false;
    int left = eval(p, div - 1, &left_success);
    int right = eval(div + 1, q, &right_success);

    *success = left_success && right_success;

    switch (tokens[div].type) {
      case '+':
        return left + right;
      case '-':
        return left - right;
      case '*':
        return left * right;
      case '/':
        if(right == 0){
          *success = false;
          return 0;
        }
        return left / right;
      default:
        *success = false;
        return 0;
    }
  }

  return 0;
}

bool check_parentheses(int p, int q){
  if(tokens[p].type == '('){
    int count = 1;
    int i;
    for(i = p + 1; i <= q; ++i){
      switch (tokens[i].type) {
        case '(':
          count++;
          break;
        case ')':
          count--;
          break;
        default:
          break;
      }
      if(count == 0) break;
    }
    return i == q;
  }
  return false;
}

void test_expr(){
  FILE *file = fopen("/tmp/test.txt", "r");

  word_t result;
  char str[128];
  while(fscanf(file, "%lu %s", &result, str)){
    if(strcmp(str,"eof") == 0) break;
    bool success;
    word_t val = expr(str, &success);
    if (!success || !(val == result)){
      ERROR("failed: %lu %s %lu\n", result, str, val);
      break;
    }

    // assert(success);
    // assert(val == result);
  }
  fclose(file);
}
