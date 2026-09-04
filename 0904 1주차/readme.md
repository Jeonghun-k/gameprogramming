# 1주차 정리

**주의사항**
```
수업자료는 윈도우 사용자를 위한 자료임
현재 사용자는 맥OS를 사용하기에 자료와 다른점이 존재.
맥OS와 맞지 않는 코드들은 ai를 이용하여 최대한 해당 OS에서 원활하게 작동이 될 수있도록 수정함.
또한, 기존 윈도우에 해당하는 코드들은 주석처리를하여 어떠한점이 다른지 한눈에 볼 수있도록 작성해둠.

추가로 <windows.h>에서 사용할 수있는 코드들은 맥OS에서 사용할 수 없어서
그것과 같은 기능을 하는 코드들을 mylib.h파일 안에 제작을 해 두어서
해당 파일을 include시켜서 사용 할 수 있도록 제작함.
```
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
