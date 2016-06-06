
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

static sigjmp_buf global_ill_jmp;

// declare probes
int probe_mov(void*);
int probe_udf(void*);
int probe_ldrex(void*);
int probe_movt(void*);

static void ill_handler(int sig) {
    siglongjmp(global_ill_jmp, sig);
}

static int safe_probe(int (*fun)(void*)) {
	if (0 == sigsetjmp(global_ill_jmp, 1)) { 
        volatile int tmp = 1;
        fun((void*) &tmp);
        return 1;
    }
    return 0;
}

static int detect() {
    int res_mov = safe_probe(probe_mov);
    int res_udf = safe_probe(probe_udf);
    int res_ldrex = safe_probe(probe_ldrex);
    int res_movt = safe_probe(probe_movt);

    printf("%d: mov probe\n", res_mov);
    printf("%d: udf probe\n", res_udf);
    printf("%d: ldrex probe\n", res_ldrex);
    printf("%d: movt probe\n", res_movt);

    return 0;
}

int main() {
    // signal handler
	struct sigaction ill_oact, ill_act;
    sigset_t all_masked, oset;
	sigfillset(&all_masked);
    sigdelset(&all_masked, SIGILL);
    sigdelset(&all_masked, SIGTRAP);
    sigdelset(&all_masked, SIGFPE);
    sigdelset(&all_masked, SIGBUS);
    sigdelset(&all_masked, SIGSEGV);

	memset(&ill_act, 0, sizeof(ill_act));
    ill_act.sa_handler = ill_handler;
    ill_act.sa_mask = all_masked;

    sigprocmask(SIG_SETMASK, &ill_act.sa_mask, &oset);
    sigaction(SIGILL, &ill_act, &ill_oact);

    // run detection
    int res = detect();

    // cleanup
	sigaction(SIGILL, &ill_oact, NULL);
    sigprocmask(SIG_SETMASK, &oset, NULL);

    return res;
}
