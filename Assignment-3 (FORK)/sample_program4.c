#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
pid_t p;

int i;

for (i=0; i < 5; i++) {

p = fork();

if (p == 0) {
printf("Child process %d !\n", i+1);

exit(0);

} else {
printf("Parent process!");
}
}
return 0;
}
