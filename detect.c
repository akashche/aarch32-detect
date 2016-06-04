
#include <errno.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**
 *   0: no sigill
 *   1: sigill
 *  -1: read error
 *  -2: different signal
 *  -3: more than one signal
 */
int check_sigill(int sfd, struct signalfd_siginfo* sfdsi_ptr) {
    size_t len = sizeof(struct signalfd_siginfo);
    ssize_t retlen = read(sfd, sfdsi_ptr, len);
    if (-1 == retlen && EAGAIN == errno) {
        // no sigill
        return 0;
    } else if (len == retlen) {
        if (SIGILL == sfdsi_ptr->ssi_signo) {
            // check for next signal
            int next_signal = check_sigill(sfd, sfdsi_ptr);
            if (0 == next_signal) {
                // sigill
                return 1;
            } else if (-1 == next_signal) {
                // read error
                return -1;
            } else {
                // more than one signal
                return -3;
            }
        } else {
            // diferent signal
            return -2;
        }
    } else {
        // read error
        return -1; 
    }
}

void detect(int sfd, struct signalfd_siginfo* sfdsi_ptr) {
    // do checks here
    printf("check_sigill result: [%d]\n", check_sigill(sfd, sfdsi_ptr));
    raise(SIGILL);
    printf("check_sigill result: [%d]\n", check_sigill(sfd, sfdsi_ptr));
}

int main() {
    // init
    sigset_t mask;
    sigset_t oldmask;
    sigemptyset(&mask);
    sigemptyset(&oldmask);
    sigaddset(&mask, SIGILL);
    int err_block = pthread_sigmask(SIG_BLOCK, &mask, &oldmask);
    if (err_block) {
        puts("pthread_sigmask block error");
        return err_block;
    }
    int sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (-1 == sfd) {
        puts("signalfd error");
        return errno;
    }
    struct signalfd_siginfo sfdsi;
    memset(&sfdsi, 0, sizeof(struct signalfd_siginfo));

    detect(sfd, &sfdsi);

    // cleanup
    int err_close = close(sfd);
    if (err_close) {
        puts("sfd close error");
        return errno;
    }
    int err_unblock = pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
    if (err_unblock) {
        puts("pthread_sigmask unblock error");
        return errno;
    }
    int err_reblock = pthread_sigmask(SIG_BLOCK, &oldmask, NULL);
    if (err_unblock) {
        puts("pthread_sigmask reblock error");
        return errno;
    }

    return 0;
}
