# 1주차 정리
---
1. 커서 위치 제어  
   a. 커서 위치 이동 함수 : gotoxy()  
   b. 기본적인 코드
   ```c
     void gotoxy(int x, int y)
     {
       COORD Pos = {x - 1, y - 1}; // 구조체로된 변수에 값들을 각각 넣은거임 / 배열 초기화 할 때 쓴다
       SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
     }
    ```
2. 화면 지우기  
   a.기본적인 코드(윈도우)
   ```c
   #include <stdlib.h>

   system("cls")
   ```
   b. 예시
   ```c
    #include <stdio.h>
    #include <stdlib.h>
    int main(void)
    {
	    char ch;
	    printf("문자를 입력하고 Enter>");
	    scanf("%c", &ch);
	    system("clear"); // mac os라서 clear | window는 cls다
	    printf("입력된 문자 %c\n", ch);
	    return 0;
    }

    // 숫자를 입력하면 화면을 클리어 하고 내가 입력한 숫자가 맨위에 표시된다
   ```
