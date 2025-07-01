#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifndef STACK_SIZE
#define STACK_SIZE (1024 * 1024)    // 1 MB for child stacks
#endif

// Number of iterations for each stress loop
#define ITER 1000

static char *child_stack;

// Simple child function for clone()
// It immediately returns to parent, so child just exits
static int child_fn(void *arg) {
    (void)arg;
    syscall(SYS_exit_group, 0); // Terminates all threads in the calling process's thread group
    return 0;
}

int main(int argc, char *argv[], char *envp[]) {
    pid_t pids[ITER];
    int i;

    // Allocate one big buffer and carve up stacks for child threads
    child_stack = malloc(ITER * STACK_SIZE);
    if (!child_stack) {
        perror("malloc");
        return 1;
    }

    // 1) Stress clone()
    for (i = 0; i < ITER; i++) {
        // Clone flags: CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|SIGCHLD
        // Creates a thread-like child that shares most state
        pid_t pid = syscall(SYS_clone,
                            CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD,
                            child_stack + (i+1)*STACK_SIZE,
                            NULL, NULL, NULL);
        if (pid < 0) {
            fprintf(stderr, "clone #%d failed: %s\n", i, strerror(errno));
        } else {
            pids[i] = pid;
        }
    }

    // 2) Stress execve()
    // We’ll repeatedly exec /bin/true (which immediately returns exit code 0).
    // We fork() here so our stress driver isn’t replaced.
    for (i = 0; i < ITER; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
        } else if (pid == 0) {
            // Child: execve /bin/true
            char *args[] = { "/bin/true", NULL };
            char *env[]  = { NULL };
            syscall(SYS_execve, args[0], args, env);
            // If execve fails:
            _exit(1);
        }
        // Parent continues without waiting
    }

    // 3) Stress kill()
    // Send SIGUSR1 to each of the clone()-ed children
    for (i = 0; i < ITER; i++) {
        if (pids[i] > 0) {
            if (syscall(SYS_kill, pids[i], SIGUSR1) < 0) {
                fprintf(stderr, "kill(%d) failed: %s\n", pids[i], strerror(errno));
            }
        }
    }

    // 4) Cleanup: reap any forked execve() children
    for (;;) {
        // printf("CLEAN HERE!");
        pid_t w = waitpid(-1, NULL, 0);
        if (w < 0) break;
    }

    // Finally, exit via exit_group syscall
    syscall(SYS_exit_group, 0);

    return 0;  // never reached
}
