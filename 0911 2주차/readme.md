#2주차

# 입출력 버퍼와 ASCII 코드 정리

## 1. 입출력 버퍼

- 입력이나 출력을 할 때는 **버퍼(buffer)**라는 공간을 사용한다.
- 이유: CPU가 입출력을 기다리지 않고 다른 작업을 처리할 수 있게 하기 위해서다.

### 1.1 scanf의 버퍼 문제

`scanf`를 사용할 때, 입력 버퍼에 남은 개행 문자(`\n`) 등으로 인해 예상치 못한 동작이 발생하는 경우가 있다. 이를 해결하려면 버퍼를 비우는 방법이 필요하다.

### 1.2 버퍼를 비우는 해결 방법들

| # | 방법 | 설명 |
|---|------|------|
| 1 | `getchar();` 추가 | 가장 간단한 방법. 남은 개행 문자 하나를 소비한다. |
| 2 | `fflush(stdin);` 추가 | **gcc에서는 동작하지 않음.** 표준 `fflush`는 출력 버퍼를 비우는 함수이며, 입력 버퍼에 쓰는 것은 표준이 아니다. VC(Visual C++)는 이를 확장 기능으로 지원하지만 gcc는 지원하지 않는다. |
| 3 | `tcflush(0, TCIFLUSH);` 추가 | 0번 디스크립터(표준입력)를 비운다. 유닉스 계열 터미널 관련 함수로 보인다. |
| 4 | `rewind(stdin);` 추가 | 매개변수로 들어온 스트림을 초기화하는 데 사용한다. |
| 5 | `__fpurge(stdin);` | 리눅스에서만 동작하며 표준 함수가 아니다. `stdio_ext.h` 헤더 추가가 필요하다. |
| 6 | `fgets`로 대체 입력 | 문자열 입력을 `scanf` 대신 `fgets(string, sizeof(string), stdin);`로 받는다. 단, 입력 크기가 버퍼 크기를 넘으면 동일한 문제가 재발하며, 넘지 않더라도 문자열 끝에 개행 문자가 남으므로 `string[strlen(string)-1] = '\0';`로 강제로 널 문자를 넣어줘야 한다. (개인적으로 가장 많이 사용하는 방식) |
| 7 | `scanf("%*c", c);` | `%*c`는 입력은 받지만 저장하지 않는다. 즉, 비어있는 `\n`을 소비해서 버린다. |
| 8 | `scanf(" %c", &c);` | `%c` 앞에 공백을 추가하면 공백 문자(whitespace)를 구분자로 인식해 건너뛴다. |

> 출처: [plustag.tistory.com](https://plustag.tistory.com/1)

---

## 2. ASCII 코드

- 전 세계 표준 문자 코드 체계다.

### 2.1 확장키 vs 아스키 코드 구분 예제

```c
#include <stdio.h>
#include <conio.h>

int main(void)
{
    int chr;
    do
    {
        chr = getch();
        if (chr == 0 || chr == 0xe0)  // 0xe0는 2진수로 1110 (e = 1110)
        {
            chr = getch();
            printf("확장키 code=%d\n", chr);
        }
        else
        {
            printf("아스키 code=%d\n", chr);
        }
    } while (1);

    return 0;
}
```

- ⚠️ 이 코드는 종료 조건이 없어 무한루프로 동작한다.
- **설명**: 스캔 코드와 아스키 코드를 구분 없이 저장하려면 2바이트가 필요하다. 확장키 코드가 입력된 경우 첫 번째 `getch()`는 0 또는 224(`0xe0`)를 반환하므로, `getch()`를 한 번 더 호출해 실제 확장 키 코드를 얻는다.

### 2.2 방향키(화살표) 스캔 코드 처리

```c
void move_arrow_key(char key, int *x1, int *y1, int x_b, int y_b)
{
    switch (key)
    {
        case 72: // 위쪽(상) 방향 화살표 키
            *y1 = *y1 - 1;
            if (*y1 < 1) *y1 = 1; // y좌표 최소값
            break;
        case 75: // 왼쪽(좌) 방향 화살표 키
            *x1 = *x1 - 1;
            if (*x1 < 1) *x1 = 1; // x좌표 최소값
            break;
        case 77: // 오른쪽(우) 방향 화살표 키
            *x1 = *x1 + 1;
            if (*x1 > x_b) *x1 = x_b; // x좌표 최대값
            break;
        case 80: // 아래쪽(하) 방향 화살표 키
            *y1 = *y1 + 1;
            if (*y1 > y_b) *y1 = y_b; // y좌표 최대값
            break;
        default:
            return;
    }
}
// case의 각 숫자는 키보드 방향키의 스캔 코드를 의미한다.
```

### 2.3 방향키로 문자 이동시키는 전체 예제

```c
#include <stdio.h>
#include <conio.h>
#include <windows.h>

#define X_MAX 79 // 가로(열) 방향 최대값
#define Y_MAX 24 // 세로(행) 방향 최대값

void move_arrow_key(char chr, int *x, int *y, int x_b, int y_b);
void gotoxy(int x, int y);

int main(void)
{
    char key;
    int x = 10, y = 5;

    do
    {
        gotoxy(x, y);
        printf("A");
        key = getch();
        move_arrow_key(key, &x, &y, X_MAX, Y_MAX);
    } while (key != 27); // ESC(27) 입력 시 무한루프 종료

    return 0;
}
```

### 2.4 (다음 정리 예정)
- 길이가 n개인 사각형 — 내용 추가 필요
