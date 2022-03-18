#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
void test_expr();

word_t return_val = 0;

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static int cmd_px(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "step n command and stop", cmd_si},
  { "info", "output infomation of register or watchpoint", cmd_info},
  { "x", "output continual N Bytes", cmd_x},
  { "p", "eval the value of expression", cmd_p},
  { "w", "watch val", cmd_w},
  { "d", "delete watch point", cmd_d},
  { "px", "print last val in hex", cmd_px},
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_si(char *args){
  int step = 1;
  if (args != NULL) {
    char * arg = strtok(args, " ");
    for(char * cptr = arg; *cptr != '\0'; cptr++) {
      if (!isdigit(*cptr)) {
        printf("si: invalid argument\n");
        return 0;
      }
    }
    step = atoi(arg);
  }
  printf("step n command and stop %d\n", step);
  cpu_exec(step);
  return 0;
}

int cmd_info(char * args){
  if (args == NULL) {
    printf("info: no argument\n");
    return 0;
  }
  char * arg = strtok(args, " ");
  if (strcmp(arg, "r") == 0) {
    isa_reg_display();
  }
  else if (strcmp(arg, "w") == 0) {
    // init_wp_pool();
    // printf("watchpoint pool:\n");
    // for (int i = 0; i < wp_pool_size; i++) {
    //   printf("%d: %s\n", i, wp_pool[i].expr);
    // }
  }
  else {
    bool success;
    word_t val = isa_reg_str2val(arg, &success);
    if(success) {
      printf("%s = 0x%016lx\n", arg, val);
    }
    else {
      printf("info: invalid argument\n");
    }
  }
  return 0;
}

int cmd_x(char *args){
  if (args == NULL) {
    printf("x: no argument\n");
    return 0;
  }
  char * arg = strtok(args, " ");
  //TODO: check if arg is valid

  for(char* i = arg; *i != '\0'; i++){
    if (!isdigit(*i)) {
      ERROR("x: invalid argument\n");
      return 0;
    }
  }

  int n;
  sscanf(arg, "%d", &n);

  bool success;
  paddr_t val = expr(arg + strlen(arg) + 1, &success);

  if (!success) {
    printf("x: invalid expression\n");
    return 0;
  }
  if ( val < CONFIG_MBASE){
    printf("x: invalid address\n");
    return 0;
  }

  uint8_t * memptr = guest_to_host(val);

  for(int i = 0; i < n; i++)
    printf("%02x ", memptr[i]);

  putchar('\n');
  return 0;
}

int cmd_d(char *args){
  return 0;
}

int cmd_w(char *args){
  return 0;
}

int cmd_p(char *args){
  if (args == NULL) {
    printf("p: no argument\n");
    return 0;
  }
  bool success;
  word_t val = expr(args, &success);

  return_val = val;

  if(success) {
    printf("%s = %lu\n", args, val);
  }
  else {
    printf("p: invalid argument\n");
  }

  return 0;
}

int cmd_px(char *args){

  printf("0x%016lx\n", return_val);

  return 0;
}


void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();
  test_expr();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
