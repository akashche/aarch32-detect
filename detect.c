
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>


static sigjmp_buf global_ill_jmp;

static int check_instr(int instr);

static int detect() {
    //int res_nop = check_instr(0xe1a00000);
    //int res_bxlr = -1; //check_instr(0xe12fff1e);
    int res_mov = check_instr(0xe3a01001);
    int res_udf = check_instr(0xe7f000f0);
    int res_ldrex = check_instr(0xe1901f9f);
    //if (1 != res_nop) {
    //    printf("ERROR_NOP_%d\n", res_nop);
    //}
    //if (-1 != res_bxlr) {
    //    printf("ERROR_BXLR_%d\n", res_bxlr);
    //}
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

static void ill_handler(int sig) {
    siglongjmp(global_ill_jmp, sig);
}

static int write_instr(int instr, void** mem_ptr_out) {
    size_t mem_len = 18 * sizeof(int);
	void* mem_ptr = NULL;
    int err_pm = posix_memalign(&mem_ptr, 4096, mem_len);
    if (err_pm) {
        puts("posix_memalign error");
        return err_pm;
    }
    int err_mp = mprotect(mem_ptr, mem_len, PROT_READ | PROT_WRITE | PROT_EXEC);
    if (err_mp) {
        puts("mprotect error");
        return errno;
    }
	
    int* mem_ptr_int = (int*) mem_ptr;
    // no need to save state - we'll return with siglongjmp
    // mem_ptr_int[0] = 0xe92d4ff0; // push {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    mem_ptr_int[0] = 0xf57ff05f; // dmb sy
    mem_ptr_int[1] = 0xf57ff04f; // dsb sy
    mem_ptr_int[2] = 0xf57ff06f; // isb sy
    mem_ptr_int[3] = 0xe3a01000; // mov r1, #0
    mem_ptr_int[4] = 0xe5801000; // str r1, [r0]
    mem_ptr_int[5] = 0xf57ff05f; // dmb sy
    mem_ptr_int[6] = 0xf57ff04f; // dsb sy
    mem_ptr_int[7] = 0xf57ff06f; // isb sy
    mem_ptr_int[8] = instr; // instr to check
    mem_ptr_int[9] = 0xf57ff05f; // dmb sy
    mem_ptr_int[10] = 0xf57ff04f; // dsb sy
    mem_ptr_int[11] = 0xf57ff06f; // isb sy
    mem_ptr_int[12] = 0xe3a01001; // mov r1, #1
    mem_ptr_int[13] = 0xe5801000; // str r1, [r0]
    mem_ptr_int[14] = 0xf57ff05f; // dmb sy
    mem_ptr_int[15] = 0xf57ff04f; // dsb sy
    mem_ptr_int[16] = 0xf57ff06f; // isb sy
    mem_ptr_int[17] = 0xe7f000f0; // udf #0
    // mem_ptr_int[4] = 0xe8bd4ff0; //pop {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    // mem_ptr_int[5] = 0xe12fff1e; //bxlr

	*mem_ptr_out = mem_ptr;
	return 0;
}

static int check_instr(int instr) {
	void* mem_ptr = NULL;
	int err_write = write_instr(instr, &mem_ptr);
	if (err_write) {
		puts("write_instr error");
		return -1;
	}
    volatile int out = -1;
	void (*mem_ptr_fun)(void*);
	mem_ptr_fun = (void (*)(void*)) mem_ptr;
	if (0 == sigsetjmp(global_ill_jmp, 1)) { 
		mem_ptr_fun((void*) &out);
	} else {
        free(mem_ptr);
        return out;
    }
    return -1;
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
