#define _GNU_SOURCE
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <sys/time.h>

#ifndef iterations
#define iterations 30
#endif

#define stringize(x) stringize_(x)
#define stringize_(x) #x

#ifndef instrnum
#define instrnum 0
#endif

#define instr0 "nop"
#define instr1 "std"
#define instr2 "cld"
#define instr3 "stc"
#define instr4 "clc"
#define instr5 "cmc"
#define instr6 "inc %eax"
#define instr(n) instr_(n)
#define instr_(n) instr##n

void loop(void);
void endloop(void);
asm(".align 64\n"
    ".global loop\n"
    "loop:\n"
    ".rept " stringize(iterations) "\n"
    instr(instrnum) "\n"
    ".endr\n"
    ".global endloop\n"
    "endloop:\n"
    "jmp loop\n"
    );

void handler(int sig, siginfo_t *info, void *ctx) {
  static int rips[iterations];
  const int instrsize = ((long)endloop - (long)loop)/iterations;
  if (sig == SIGALRM) {
    ucontext_t* uc = ctx;
    long rip = uc->uc_mcontext.gregs[REG_RIP];
    if (rip >= (long) loop && rip < (long)endloop)
      rips[(uc->uc_mcontext.gregs[REG_RIP] - (long)loop)/instrsize]++;
  }
  else {
    puts("");
    printf("instrnum = %d\n%s\n", instrnum, instr(instrnum));
    printf("   hits | pc\n");
    printf("--------+-----------\n");
    for (int i = 0; i < sizeof rips/sizeof *rips; i++)
      printf("%7d | %p\n", rips[i], (char *)loop+i*instrsize);
    exit(0);
  }
}

int main() {
  struct itimerval t = {
    .it_interval = { .tv_sec = 0, .tv_usec = 1000 },
    .it_value    = { .tv_sec = 0, .tv_usec = 1000 }
  };
  setitimer(ITIMER_REAL, &t, 0);
  struct sigaction sa = {
    .sa_sigaction = handler,
    .sa_flags = SA_SIGINFO
  };
  sigaction(SIGALRM, &sa, 0);
  sigaction(SIGINT, &sa, 0);
  sigaction(SIGTERM, &sa, 0);
  loop();
}
