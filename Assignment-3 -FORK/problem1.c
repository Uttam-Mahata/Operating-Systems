#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<sys/types.h>

//Strings will be reversed using this function.
void reverse_string(char* str) {
int len=strlen(str);
for(int i = len -1 ; i >=0; i--) {
printf("%c", str[i]);
}
printf("\n");
}

int main(int argc, char *argv[]) {
int i;
pid_t p;
int n = argc -1; // No. of Command line arguments
for(i = 0; i <n; i++) {
p = fork();

if(p == -1) {
printf("Fork Failed....");
exit(0);
}

if(p == 0) {
printf("Child Process %d\n", i+1);
reverse_string(argv[i+1]);
exit(9);
} else {
printf("Parent Process\n");
}
}

return 0;
}
