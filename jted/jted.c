#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(){
	char c;
	while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
	printf("test\n");
	return 0;
}
