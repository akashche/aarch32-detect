
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

static sigjmp_buf global_ill_jmp;

int run_mov();
int run_udf();
int run_ldrex(void*);

static void ill_handler(int sig) {
    siglongjmp(global_ill_jmp, sig);
}

static int detect_mov() {
	if (0 == sigsetjmp(global_ill_jmp, 1)) { 
        run_mov();
        return 1;
    }
    return 0;
}

static int detect_udf() {
	if (0 == sigsetjmp(global_ill_jmp, 1)) { 
        run_udf();
        return 1;
    }
    return 0;
}

static int detect_ldrex() {
	if (0 == sigsetjmp(global_ill_jmp, 1)) { 
        volatile int tmp = 1;
        run_ldrex((void*) &tmp);
        return 1;
    }
    return 0;
}

static int detect() {
    int res_mov = detect_mov();
    int res_udf = detect_udf();
    int res_ldrex = detect_ldrex();

    if (1 != res_mov) {
        printf("ERROR_MOV_%d\n", res_mov);
    }
    if (0 != res_udf) {
        printf("ERROR_UDF_%d\n", res_udf);
    }
    if (1 != res_ldrex) {
        printf("ERROR_LDREX_%d\n", res_ldrex);
    }
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
