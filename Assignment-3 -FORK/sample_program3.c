#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
int main() {

int a = 3;
int b = 5;
pid_t p;

p = fork();

if (p == 0) {
int c = a + b;
printf("Value of c (by child process) : %d\n", c);
} else {
int d = a*b;
printf("Value of d (by parent process) : %d\n", d);

}
int e = b - a;
printf("Value of e (by parent and child process) : %d\n", e);
return 0;
}
