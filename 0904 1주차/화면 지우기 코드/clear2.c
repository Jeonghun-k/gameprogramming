#include <stdio.h>
#include <stdlib.h>
// #include <conio.h> // 윈도우 전용 헤더
#include "mylib.h"

int main(void){
    int i, j;
    for(j=1;j<=9;j++){
        // system("cls"); 윈도우용
        clear_screen(); // mylib에서 가져온 함수
        for(i=1;i<=9;i++)
            printf("%d*%d=%d\n", j, i, j*i);
        printf("아무키나 누르시오.\n");
        // getch();
        // getchar(); // Enter를 눌렀을 때 버퍼에 남는 '\n'을 다음 getchar()가 즉시 읽어버려서
                       // 짝수 단이 뜨자마자 바로 지나가버리는 버그가 있었음 (홀수 단만 보이는 것처럼 보임)
        int c;
        while ((c = getchar()) != '\n' && c != EOF); // 엔터로 입력된 문자와 '\n'까지 버퍼를 모두 비워서 다음 단이 제대로 보이게 함
    }
    return 0;
}