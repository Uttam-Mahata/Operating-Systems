#include<stdio.h>
#include<unistd.h>

int main() {
printf("Hello World - by Parent Process\n");
fork();
printf("Hello Uttam - by Parent and Child Process\n");
return 0;
}
