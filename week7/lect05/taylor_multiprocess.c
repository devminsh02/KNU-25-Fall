#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>

#define N 4
#define MAXLINE 100

static void die(const char* msg) {
    perror(msg);
    _exit(1);
}

void sinx_taylor(int num_elements, int terms, const double* x, double* result) {
    int fd[2 * N];
    pid_t pids[N];

    for (int i = 0; i < num_elements; i++) {
        if (pipe(fd + 2*i) == -1) die("pipe");
    }

    for (int i = 0; i < num_elements; i++) {
        pid_t pid = fork();
        if (pid < 0) die("fork");
        if (pid == 0) {
            for (int k = 0; k < num_elements; k++) {
                if (k == i) {
                    close(fd[2*k]);
                } else {
                    close(fd[2*k]);
                    close(fd[2*k + 1]);
                }
            }

            double xi = x[i];
            double value = xi;
            double numer = xi * xi * xi;
            double denom = 6.0;
            int sign = -1;

            for (int j = 1; j <= terms; j++) {
                value += (sign) * (numer / denom);
                numer *= (xi * xi);
                double a = 2.0*j + 2.0;
                double b = 2.0*j + 3.0;
                denom *= (a * b);
                sign = -sign;
            }

            char message[MAXLINE];
            int len = snprintf(message, sizeof(message), "%.17g", value);
            if (len < 0 || len >= (int)sizeof(message)) _exit(2);

            write(fd[2*i + 1], message, (size_t)len + 1);
            close(fd[2*i + 1]);

            _exit(i & 0xFF);
        } else {
            pids[i] = pid;
            close(fd[2*i + 1]);
        }
    }

    for (int c = 0; c < num_elements; c++) {
        int status = 0;
        pid_t wpid = wait(&status);
        if (wpid < 0) die("wait");

        if (!WIFEXITED(status)) {
            fprintf(stderr, "Child %d did not exit normally\n", (int)wpid);
            continue;
        }
        int child_id = WEXITSTATUS(status);

        char line[MAXLINE] = {0};
        ssize_t r = read(fd[2*child_id], line, sizeof(line)-1);
        if (r < 0) die("read");

        result[child_id] = atof(line);

        close(fd[2*child_id]);
    }
}

int main(void) {
    double x[N] = {0.0, M_PI/6.0, M_PI/3.0, 0.314};
    double res[N];

    sinx_taylor(N, 3, x, res);

    for (int i = 0; i < N; i++) {
        printf("sin(%.2f) by Taylor series = %.10f\n", x[i], res[i]);
        printf("sin(%.2f) (libm)          = %.10f\n", x[i], sin(x[i]));
    }
    return 0;
}

