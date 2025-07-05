#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <errno.h>
#include <string.h>

#ifndef STACK_SIZE
#define STACK_SIZE (1024 * 64)    // We allocate 64 KB per child stack
#endif

#define ITER 2000                 
#define PAUSE_SEC 1        // seconds 

// to children process don't become zombies
static void sigusr1_handler(int sig) {
    (void)sig;
}

// child function for clone()
static int child_fn(void *arg) {
    (void)arg;
    // syscall(SYS_exit_group, 0);
    return 0;
}

// allocate one stack for a clone() child
static void *alloc_stack(void) {
    void *s = malloc(STACK_SIZE);
    if (!s) {
        perror("malloc(stack)");
        exit(1);
    }
    return s;
}

static void reap_children(void) {
    while (waitpid(-1, NULL, WNOHANG) > 0) { /* nothing */ }
}

int main(void) {
    pid_t pids[ITER];
    int i, ret;

    // install no-op SIGUSR1 handler
    struct sigaction sa = { .sa_handler = sigusr1_handler };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGUSR1, &sa, NULL);

    // PHASE 1: clone()
    printf("\n=== PHASE 1: clone() x %d ===\n", ITER);
    for (i = 0; i < ITER; i++) {
        void *stack = (char*)alloc_stack() + STACK_SIZE;
        ret = clone(child_fn, stack,
                    CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD,
                    NULL);
        if (ret < 0) {
            fprintf(stderr, "clone #%d failed: %s\n", i, strerror(errno));
        } else {
            pids[i] = ret;
        }
    }
    reap_children();
    sleep(PAUSE_SEC);

    // PHASE 2: execve()
    printf("\n=== PHASE 2: execve(\"/bin/true\") x %d ===\n", ITER);
    for (i = 0; i < ITER; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
        } else if (pid == 0) {
            char *argv[] = { "/bin/true", NULL };
            char *envp[] = { NULL };
            syscall(SYS_execve, argv[0], argv, envp);
            _exit(1);
        }
        pids[i] = pid;
    }
    reap_children();
    sleep(PAUSE_SEC);

    // PHASE 3: kill()
    // PHASE 3.1: clone()
    printf("\n=== PHASE 3.1: clone() x %d ===\n", ITER);
    
    // We have create new process to kill them after 
    for (i = 0; i < ITER; i++) {
        void *stack = (char*)alloc_stack() + STACK_SIZE;
        ret = clone(child_fn, stack,
                    CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | SIGCHLD,
                    NULL);
        if (ret < 0) {
            fprintf(stderr, "clone #%d failed: %s\n", i, strerror(errno));
        } else {
            pids[i] = ret;
        }
    }

    printf("\n=== PHASE 3: kill(SIGUSR1) x %d ===\n", ITER);
    // reuse the clone-phase pids array
    for (i = 0; i < ITER; i++) {
        if (pids[i] > 0) {
            if (syscall(SYS_kill, pids[i], SIGUSR1) < 0) {
                fprintf(stderr, "kill(%d) failed: %s\n",
                        pids[i], strerror(errno));
            }
        }
    }
    reap_children();
    sleep(PAUSE_SEC);

    // PHASE 4: exit_group()
    printf("\n=== PHASE 4: exit_group() x %d ===\n", ITER);
    for (i = 0; i < ITER; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
        } else if (pid == 0) {
            // In child: call exit_group (kills this child only)
            syscall(SYS_exit_group, 0);
            // never returns
        } else {
            // Parent: record and continue
            pids[i] = pid;
        }
    }
    // Parent reaps them all
    reap_children();

    return 0;
}
// strace -o out_data.txt -c -e trace=process,signal ./stress