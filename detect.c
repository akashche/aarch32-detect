
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>


static sigjmp_buf global_ill_jmp;

static int check_instr(int instr);

static int detect() {
    printf("%d: mov, r0, r0\n",    check_instr(0xe1a00000));
    printf("%d: invalid\n",        check_instr(0xffffffff));
    printf("%d: mov, r0, #1\n",    check_instr(0xe3a00000));
    printf("%d: ldrex r0, [r0]\n", check_instr(0xe1900f9f));
    
	return 0;
}

static void ill_handler(int sig) {
    siglongjmp(global_ill_jmp, sig);
}

static int write_instr(int instr, void** mem_ptr_out) {
    size_t mem_len = 4 * sizeof(int);
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
    // push {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    mem_ptr_int[0] = 0xe92d4ff0;
    // instr to check
    mem_ptr_int[1] = instr;
    // pop {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    mem_ptr_int[2] = 0xe8bd4ff0;
    // bxlr
    mem_ptr_int[3] = 0xe12fff1e;

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
    int tmp = 0;
	void (*mem_ptr_fun)(void*);
	mem_ptr_fun = (void (*)(void*)) mem_ptr;
    int success = 0;
	if (0 == sigsetjmp(global_ill_jmp, 1)) { 
		mem_ptr_fun((void*) &tmp);
        success = 1;
	} 
    free(mem_ptr);
    return success;
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
