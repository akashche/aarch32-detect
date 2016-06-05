
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define ALIGNED_MEMSIZE 64
// push {r4, r5, r6, r7, r8, r9, r10, r11, lr}
#define INSTR_PUSH 0xe92d4ff0
// mov r0, r0
#define INSTR_NOP 0xe1a00000
// mov r0, #0
#define INSTR_RET0 0xe3a00000
// mov r0, #1
#define INSTR_RET1 0xe3a00001
// dmb sy
#define INSTR_DMB 0xf57ff05f
// dsb sy
#define INSTR_DSB 0xf57ff04f
// isb sy
#define INSTR_ISB 0xf57ff06f 
// pop {r4, r5, r6, r7, r8, r9, r10, r11, lr}
#define INSTR_POP 0xe8bd4ff0
// bxlr
#define INSTR_BXLR 0xe12fff1e

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int* global_mem_ptr_int;

void full_sync() {
    asm("dmb sy");
    asm("dsb sy");
    asm("isb sy");
}

int create_test_instrs(int** instrs_out, char*** instr_names_out) {
    // update len accordingly on test list change
    int len = 9;
    int* instrs = malloc(len * sizeof(int));
    char** instr_names = malloc(len * sizeof(char*));
    int i = 0;

    // test list
    instrs[i] = 0xe1a00000; instr_names[i++] = "mov, r0, r0";
    instrs[i] = 0xffffffff; instr_names[i++] = "invalid";
    instrs[i] = 0xe1900f9f; instr_names[i++] = "ldrex r0, [r0]";
    instrs[i] = 0xe1a00000; instr_names[i++] = "mov, r0, r0";
    instrs[i] = 0xffffffff; instr_names[i++] = "invalid";
    instrs[i] = 0xe1900f9f; instr_names[i++] = "ldrex r0, [r0]";
    instrs[i] = 0xe1a00000; instr_names[i++] = "mov, r0, r0";
    instrs[i] = 0xffffffff; instr_names[i++] = "invalid";
    instrs[i] = 0xe1900f9f; instr_names[i++] = "ldrex r0, [r0]";
    
    // check and return
    if (i != len) {
        return -1;
    }
    *instrs_out = instrs;
    *instr_names_out = instr_names;
    return i;
}

void handle_signal(int signal) {
    full_sync();
    pthread_mutex_lock(&mutex);
    // rewrite illegal instruction
    global_mem_ptr_int[0] = INSTR_NOP;
    // return 1 to caller
    global_mem_ptr_int[1] = INSTR_RET1;
    pthread_mutex_unlock(&mutex);
    full_sync();
}

void detect(int len, int* instrs, char** instr_names, int (*mem_ptr_fun)(void*)) {
    volatile int tmp = 0;
    for (int i = 0; i < len; i++) {
        full_sync();
        pthread_mutex_lock(&mutex);
        global_mem_ptr_int[0] = instrs[i];
        global_mem_ptr_int[1] = INSTR_RET0;
        pthread_mutex_unlock(&mutex);
        full_sync();
        printf("%d %x %s\n", mem_ptr_fun((void*) &tmp), instrs[i], instr_names[i]);
    }
}

int main() {
    // aligned memory
    void* mem_ptr = NULL;
    int err_pm = posix_memalign(&mem_ptr, 4096, ALIGNED_MEMSIZE);
    if (err_pm) {
        puts("posix_memalign error");
        return err_pm;
    }
    int err_mp = mprotect(mem_ptr, ALIGNED_MEMSIZE, PROT_READ | PROT_WRITE | PROT_EXEC);
    if (err_mp) {
        puts("mprotect error");
        return errno;
    }
    int* mem_ptr_int = (int*) mem_ptr;
    mem_ptr_int[0] = INSTR_PUSH;
    mem_ptr_int[1] = INSTR_DMB;
    mem_ptr_int[2] = INSTR_DSB;
    mem_ptr_int[3] = INSTR_ISB;
    mem_ptr_int[4] = INSTR_NOP;
    mem_ptr_int[5] = INSTR_RET0;
    mem_ptr_int[6] = INSTR_DMB;
    mem_ptr_int[7] = INSTR_DSB;
    mem_ptr_int[8] = INSTR_ISB;
    mem_ptr_int[9] = INSTR_POP;
    mem_ptr_int[10] = INSTR_BXLR;
    int (*mem_ptr_fun)();
    mem_ptr_fun = (int (*)(void*)) mem_ptr;

    // todo: use separate thread
    // signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_ONSTACK;
    sa.sa_handler = &handle_signal;
    int err_sa = sigaction(SIGILL, &sa, NULL);
    if (-1 == err_sa) {
        perror("sigaction error");
        return errno;
    }
    
    // test instrs
    int* instrs;
    char** instr_names;
    int tests_num = create_test_instrs(&instrs, &instr_names);
    if (-1 == tests_num) {
        puts("create_test_instrs error");
        return -1;
    }

    // run detection
    global_mem_ptr_int = mem_ptr_int + 4;
    full_sync();
    detect(tests_num, instrs, instr_names, mem_ptr_fun);

    // cleanup
    sa.sa_handler = SIG_DFL;
    int err_sacl = sigaction(SIGILL, &sa, NULL);
    if (-1 == err_sacl) {
        perror("sigaction cleanup error");
    }
    free(mem_ptr);
    free(instrs);
    free(instr_names);

    return 0;
}
