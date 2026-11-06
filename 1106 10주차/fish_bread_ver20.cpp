// =============================================================
//  붕어빵 타이쿤  (기본 틀: 응용 8-2-1 야구 스코어보드)
//  빌드: g++ -g bungeo.cpp -o bungeo.exe
//        (-fexec-charset=CP949, -lwinmm 없어도 됨: 한글 변환과 음악 DLL을 실행 중에 처리)
//  다른 PC에서도 돌아가게 처리한 것
//   - 한글: 소스가 UTF-8로 컴파일돼도 출력 직전에 CP949로 변환
//   - 키 입력: 한/영 상태와 상관없이 물리 키 위치(스캔 코드)로 판단
//   - 빠른 편집 모드 끄기: 창을 클릭해도 게임이 멈추지 않음
//   - 콘솔이 없으면(디버그 콘솔 등) 새 콘솔 창을 직접 염
//   - 창이 작으면 키워 달라는 안내 표시
//   - Visual Studio(MSVC)로 빌드해도 sprintf 경고/에러가 나지 않게 처리
// -------------------------------------------------------------
//  원본에서 유지한 것
//   - draw_check02 / display_scoreboard / control_scoreboard 구조
//   - 정규 영업 + 동점이면 연장, 마지막 날까지 동점이면 무승부 (원본 9+3회 -> 3+2일)
//   - 정규 마지막 날부터 승부가 나면 끝나는 종료 규칙 (초 = 라이벌, 말 = 나)
//  원본에서 바꾼 것
//   - 점수 직접 입력(scanf) -> 미니게임 결과가 전광판에 자동 기록
//   - R H E B -> 매출 / 손님 / 실수 / 단골
//   - 미사용 배열 baseball[12][2] 제거, 실책이 상대 줄에 찍히던 버그 수정
//   - 경기 종료 시 승리 쪽 표시
// =============================================================
#define _CRT_SECURE_NO_WARNINGS   // Visual Studio에서 sprintf, fopen 에러 방지
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <conio.h>
#include <windows.h>
#include <mmsystem.h>   // PlaySound (오프닝 음악)

// ---------------- 한글 출력 (컴파일 옵션과 무관하게) ----------------
// 소스의 한글 문자열이 UTF-8로 컴파일됐는지 CP949로 컴파일됐는지 실행 중에 확인.
// "가"가 3바이트면 UTF-8 -> 출력하기 전에 CP949로 바꿔 줌 (콘솔은 항상 CP949로 사용)
int src_is_utf8 = (sizeof("가") - 1 == 3);

// in(UTF-8 또는 CP949)을 CP949로 바꿔 out에 저장
void to_cp949(const char* in, char* out, int outSize)
{
    int ascii = 1;
    for (const unsigned char* p = (const unsigned char*)in; *p; p++) if (*p >= 0x80) { ascii = 0; break; }
    if (!src_is_utf8 || ascii) {                 // 바꿀 필요 없음
        strncpy(out, in, outSize - 1);
        out[outSize - 1] = 0;
        return;
    }
    wchar_t wbuf[1024];
    int n = MultiByteToWideChar(CP_UTF8, 0, in, -1, wbuf, 1024);
    if (n <= 0) { strncpy(out, in, outSize - 1); out[outSize - 1] = 0; return; }
    if (WideCharToMultiByte(949, 0, wbuf, -1, out, outSize, NULL, NULL) <= 0) {
        strncpy(out, in, outSize - 1); out[outSize - 1] = 0;
    }
}

// printf 대신 쓰는 함수: 한글을 CP949로 바꾼 뒤 출력
int kprintf(const char* fmt, ...)
{
    char buf[1024], out[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    to_cp949(buf, out, sizeof(out));
    return fputs(out, stdout);
}
#define printf kprintf     // 아래 코드의 printf는 전부 kprintf로 바뀜

// ---------------- 영업 일수 ----------------
// 원본은 1~9회 + 연장 10~12회. 붕어빵 타이쿤은 정규 3일 + 연장 2일 = 최대 5일 (시연용)
#define REG_DAYS 3     // 정규 영업 일수
#define MAX_DAYS 5     // 연장 포함 최대 일수
#define SAVE_FILE "bungeo_save.txt"   // 세이브 파일 (상점에서 나올 때 자동 저장)

// ---------------- 화면 기본 ----------------
// ---------------- 화면 중앙 정렬 ----------------
// 화면마다 내용 크기를 정해 두고, 창 크기에 맞춰 왼쪽/위 여백을 계산
#define GAME_W 116   // 게임 화면: 전광판 + 가게 + 진열대
#define GAME_H 30
#define MENU_W 88    // 오프닝 / 메뉴 / 설명 화면
#define MENU_H 28
int off_x = 0, off_y = 0;
int content_w = MENU_W, content_h = MENU_H;
int last_w = 0, last_h = 0;   // 마지막으로 여백을 계산할 때의 창 크기

void get_window_size(int* w, int* h)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    *w = 80; *h = 25;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        *w = info.srWindow.Right - info.srWindow.Left + 1;
        *h = info.srWindow.Bottom - info.srWindow.Top + 1;
    }
}

// 현재 창 크기로 여백 계산
void center_screen(void)
{
    int w, h;
    get_window_size(&w, &h);
    last_w = w; last_h = h;
    off_x = (w > content_w) ? (w - content_w) / 2 : 0;
    off_y = (h > content_h) ? (h - content_h) / 2 : 0;
}

// 창 크기가 바뀌었는지 확인 (바뀌었으면 화면을 다시 그려야 함)
int window_resized(void)
{
    int w, h;
    get_window_size(&w, &h);
    return w != last_w || h != last_h;
}

void set_layout(int w, int h)
{
    content_w = w;
    content_h = h;
}

void gotoxy(int x, int y)
{
    COORD pos = { (SHORT)(x - 1 + off_x), (SHORT)(y - 1 + off_y) };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

int ignore_small = 0;   // 창이 작다는 안내에서 ENTER를 누르면 그냥 진행
int key_ready(void);
int read_key(void);

// 창이 내용보다 작으면 키워 달라고 안내하고, 커질 때까지 기다림
void check_window_size(void)
{
    int w, h, shown = 0, lw = 0, lh = 0;
    while (!ignore_small) {
        get_window_size(&w, &h);
        if (w >= content_w && h >= content_h) break;
        if (!shown || w != lw || h != lh) {
            system("cls");
            off_x = off_y = 0;
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
            gotoxy(2, 2); printf("창이 너무 작아요. 창을 키우거나 최대화해 주세요.");
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
            gotoxy(2, 4); printf("필요한 크기 : 가로 %d칸 x 세로 %d줄", content_w, content_h);
            gotoxy(2, 5); printf("지금 창 크기: 가로 %d칸 x 세로 %d줄", w, h);
            gotoxy(2, 7); printf("(ENTER: 이대로 계속하기)");
            shown = 1; lw = w; lh = h;
        }
        if (key_ready() && read_key() == 13) ignore_small = 1;
        Sleep(100);
    }
}

void clear_screen(void)
{
    check_window_size();
    system("cls");
    center_screen();
}

void on_resize(void);   // 게임 중 창 크기가 바뀌면 전광판 + 화면 다시 그리기 (아래에서 정의)

void set_color(int c)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), (WORD)c);
}

void hide_cursor(void)
{
    CONSOLE_CURSOR_INFO ci = { 1, FALSE };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

// 색 번호
#define C_GRAY   8
#define C_BLUE   9
#define C_GREEN 10
#define C_CYAN  11
#define C_RED   12
#define C_PINK  13
#define C_YEL   14
#define C_WHITE 15
#define C_BROWN  6
#define C_NORM   7

// ---------------- 효과음 ----------------
enum { S_BATTER, S_FLIP, S_SELL, S_BAD, S_ERR, S_ENTER, S_BELL, S_WIN, S_HURRY, S_LOSE, S_DRAW };

// 소리 길이는 60ms 이상: 요즘 Windows는 Beep가 사운드 카드로 나가면서 시작이 늦어져
// 30~40ms짜리 짧은 소리는 잘려서 안 들릴 수 있음
void play_sfx(int t)
{
    switch (t) {
    case S_BATTER: Beep(1200, 60); break;                   // 반죽 붓기
    case S_FLIP:   Beep(650, 70);  break;                   // 뒤집기
    case S_SELL:   Beep(1500, 70); Beep(1900, 90); break;   // 계산대 "띵동"
    case S_BAD:    Beep(220, 200); break;                   // 탐 / 손님 화남
    case S_ERR:    Beep(400, 70);  break;                   // 잘못된 조작
    case S_ENTER:  Beep(1000, 60); Beep(1300, 60); break;   // 손님 입장
    case S_BELL:   Beep(1568, 130); Beep(2093, 230); break; // 다 익음 알림 벨 "띵-동~" (솔 -> 높은 도)
    case S_WIN:    Beep(1046, 120); Beep(1318, 120); Beep(1568, 250); break;
    case S_LOSE:   Beep(523, 250); Beep(440, 250); Beep(349, 500); break;   // 패배: 도 -> 라 -> 파 (내려가는 소리)
    case S_DRAW:   Beep(784, 180); Sleep(60); Beep(784, 180); break;        // 무승부: 솔 솔
    case S_HURRY:                                            // 곧 타요! "삐- 삐- 삐-" (세 번이 또렷하게 끊기도록)
        for (int n = 0; n < 3; n++) { Beep(1760, 160); Sleep(90); }
        break;
    }
}

// 효과음은 별도 스레드에서 재생 -> 소리가 나는 동안에도 게임이 멈추지 않음
// 여러 소리가 동시에 나면 서로 잘려서 안 들리므로, 한 번에 하나씩 순서대로 재생
CRITICAL_SECTION sfx_lock;
int sfx_lock_ready = 0;

DWORD WINAPI sfx_thread(LPVOID p)
{
    if (sfx_lock_ready) EnterCriticalSection(&sfx_lock);
    play_sfx((int)(INT_PTR)p);
    if (sfx_lock_ready) LeaveCriticalSection(&sfx_lock);
    return 0;
}

void sfx(int t)
{
    HANDLE h = CreateThread(NULL, 0, sfx_thread, (LPVOID)(INT_PTR)t, 0, NULL);
    if (h) CloseHandle(h);
    else   play_sfx(t);            // 스레드를 못 만들면 그냥 바로 재생
}

// ---------------- 키 입력 ----------------
#define K_LEFT  1000
#define K_RIGHT 1001
#define K_UP    1002
#define K_DOWN  1003

HANDLE hIn;   // 콘솔 입력 핸들 (main에서 설정)

// 키 이벤트 하나를 게임에서 쓰는 키 값으로 바꿈 (쓰지 않는 키는 -1)
// 터미널 종류(콘솔 창 / Windows Terminal)와 한/영 상태에 따라 키 정보가 다르게 들어와서
// 아래 순서로 여러 번 확인함
//   1) 가상 키 코드 (VK_Q 등)   2) 입력된 글자 (q, Q)
//   3) 한글 자모 (한글 상태에서 Q W E R = ㅂ ㅈ ㄷ ㄱ)   4) 스캔 코드 (키보드 물리 위치)
int map_key(const KEY_EVENT_RECORD* k)
{
    if (!k->bKeyDown) return -1;
    WORD vk = k->wVirtualKeyCode;
    WCHAR ch = k->uChar.UnicodeChar;
    switch (vk) {
    case VK_LEFT:   return K_LEFT;
    case VK_RIGHT:  return K_RIGHT;
    case VK_UP:     return K_UP;
    case VK_DOWN:   return K_DOWN;
    case VK_RETURN: return 13;
    case VK_ESCAPE: return 27;
    case VK_SPACE:  return ' ';
    case VK_NUMPAD1: return '1';
    case VK_NUMPAD2: return '2';
    }
    // 1) 가상 키 코드
    if (vk >= 'A' && vk <= 'Z') {
        int c = vk + 32;
        if (strchr("qwerbm", c)) return c;
    }
    if (vk == '1' || vk == '2') return vk;
    // 2) 입력된 글자
    if (ch >= 'A' && ch <= 'Z') ch += 32;
    if (ch && ch < 128 && strchr("qwerbm12", (int)ch)) return ch;
    if (ch == ' ') return ' ';
    if (ch == '\r') return 13;
    // 3) 한글 자모 (쌍자음 = Shift를 누른 경우)
    switch (ch) {
    case 0x3142: case 0x3143: return 'q';   // ㅂ ㅃ
    case 0x3148: case 0x3149: return 'w';   // ㅈ ㅉ
    case 0x3137: case 0x3138: return 'e';   // ㄷ ㄸ
    case 0x3131: case 0x3132: return 'r';   // ㄱ ㄲ
    case 0x3160:              return 'b';   // ㅠ
    case 0x3161:              return 'm';   // ㅡ
    }
    // 4) 스캔 코드
    switch (k->wVirtualScanCode) {
    case 0x10: return 'q';  case 0x11: return 'w';  case 0x12: return 'e';  case 0x13: return 'r';
    case 0x30: return 'b';  case 0x32: return 'm';
    case 0x02: return '1';  case 0x03: return '2';
    case 0x4F: return '1';  case 0x50: return '2';   // 숫자 키패드 1, 2
    }
    return -1;
}

// 한글 입력기는 키 하나에 "처리 중 키(VK_PROCESSKEY)"와 "조합된 자모 글자" 두 이벤트를 보낼 수 있음
// -> 같은 키가 두 번 눌린 것처럼 되지 않도록 바로 뒤에 오는 자모 이벤트는 무시
int   last_key = -1;
DWORD last_key_tick = 0;
int   last_was_process = 0;

int is_duplicate(const KEY_EVENT_RECORD* k, int key)
{
    WCHAR ch = k->uChar.UnicodeChar;
    int jamo = (ch >= 0x3131 && ch <= 0x3163);
    return jamo && last_was_process && key == last_key && GetTickCount() - last_key_tick < 500;
}

void remember_key(const KEY_EVENT_RECORD* k, int key)
{
    last_key = key;
    last_key_tick = GetTickCount();
    last_was_process = (k->wVirtualKeyCode == VK_PROCESSKEY);
}

// 게임에서 쓰지 않는 키가 눌렸을 때 정보 (영업 화면에서 보여 줘서 원인 확인용)
int unk_new = 0, unk_vk = 0, unk_sc = 0, unk_ch = 0;
void remember_unknown(const KEY_EVENT_RECORD* k)
{
    if (!k->bKeyDown) return;
    WORD vk = k->wVirtualKeyCode;
    if (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU || vk == VK_HANGUL || vk == VK_CAPITAL) return;
    unk_new = 1; unk_vk = vk; unk_sc = k->wVirtualScanCode; unk_ch = k->uChar.UnicodeChar;
}

// 기다리지 않고 확인: 쓸 수 있는 키가 눌려 있으면 1 (쓸모없는 이벤트는 버림)
int key_ready(void)
{
    INPUT_RECORD r;
    DWORD n = 0;
    while (GetNumberOfConsoleInputEvents(hIn, &n) && n > 0) {
        if (!PeekConsoleInputW(hIn, &r, 1, &n) || n == 0) return 0;
        if (r.EventType == KEY_EVENT) {
            int key = map_key(&r.Event.KeyEvent);
            if (key != -1 && !is_duplicate(&r.Event.KeyEvent, key)) return 1;
            if (key == -1) remember_unknown(&r.Event.KeyEvent);
        }
        ReadConsoleInputW(hIn, &r, 1, &n);    // 키를 뗀 이벤트, 중복, 마우스 등은 버림
    }
    return 0;
}

// 키 하나를 읽음 (누를 때까지 기다림)
int read_key(void)
{
    INPUT_RECORD r;
    DWORD n;
    while (1) {
        if (!ReadConsoleInputW(hIn, &r, 1, &n)) { Sleep(30); continue; }
        if (r.EventType != KEY_EVENT) continue;
        int key = map_key(&r.Event.KeyEvent);
        if (key == -1) { remember_unknown(&r.Event.KeyEvent); continue; }
        if (is_duplicate(&r.Event.KeyEvent, key)) continue;
        remember_key(&r.Event.KeyEvent, key);
        return key;
    }
}

void flush_keys(void)
{
    FlushConsoleInputBuffer(hIn);
}

// "아무 키나 누르세요"용: 게임에서 안 쓰는 키(N, A 등)도 인정
// 반환: 눌린 키 (게임에서 안 쓰는 키는 0), 아무것도 안 눌렸으면 -1
int any_key(void)
{
    INPUT_RECORD r;
    DWORD n = 0;
    while (GetNumberOfConsoleInputEvents(hIn, &n) && n > 0) {
        if (!ReadConsoleInputW(hIn, &r, 1, &n) || n == 0) return -1;
        if (r.EventType != KEY_EVENT || !r.Event.KeyEvent.bKeyDown) continue;
        int key = map_key(&r.Event.KeyEvent);
        WORD vk = r.Event.KeyEvent.wVirtualKeyCode;
        if (key != -1 && is_duplicate(&r.Event.KeyEvent, key)) continue;
        if (key == -1 && (vk == VK_SHIFT || vk == VK_CONTROL || vk == VK_MENU ||
                          vk == VK_HANGUL || vk == VK_CAPITAL || vk == VK_PROCESSKEY)) continue;
        if (key != -1) { remember_key(&r.Event.KeyEvent, key); return key; }
        return 0;
    }
    return -1;
}

// 키를 기다리다가 창 크기가 바뀌면 -2를 돌려줌 (그 자리에서 화면을 다시 그리게)
int wait_key_live(void)
{
    while (!key_ready()) {
        if (window_resized()) return -2;
        Sleep(30);
    }
    return read_key();
}

// =============================================================
//  전광판 (원본 틀)
// =============================================================
#define BOARD_Y 2
// 선 모양: 0 = ASCII(+ - |, 어떤 터미널에서도 항상 1칸이라 정확히 나눠짐)
//          1 = 박스문자 2칸 (예전 콘솔 창)   2 = 박스문자 1칸 (Windows Terminal)
int box_style = 0;
int box_w = 1;   // 가로선 문자 1개가 차지하는 칸 수
// 전광판을 두 개의 표로 분리
//   왼쪽 표: 일차별 매출  [ 이름 | 1 | 2 | ... | 12 ]
//   오른쪽 표: 누적 합계  [ 이름 | 매출 | 손님 | 실수 | 단골 ]
int dayw[MAX_DAYS + 1] = { 6, 2,2,2,2,2 };        // 왼쪽 표 칸 너비 (이름 + 1~5일)
int totalw[5]  = { 6, 4,4,4,4 };                  // 오른쪽 표 칸 너비
int colx[17];    // 0 = 왼쪽 이름칸, 1~12 = 일차, 13~16 = 매출 손님 실수 단골 (내용 시작 x)
int name2x;      // 오른쪽 표 이름칸 x

enum { B_H, B_V, B_TL, B_TM, B_TR, B_ML, B_MM, B_MR, B_BL, B_BM, B_BR };
const char* BX[3][11] = {
    { "-", "|", "+", "+", "+", "+", "+", "+", "+", "+", "+" },
    { "─", "│", "┌", "┬", "┐", "├", "┼", "┤", "└", "┴", "┘" },
    { "─", "│", "┌", "┬", "┐", "├", "┼", "┤", "└", "┴", "┘" },
};
const char* STYLE_NAME[3] = { "ASCII (기본)", "박스문자 2칸", "박스문자 1칸" };

const char* bx(int i) { return BX[box_style][i]; }

void set_box_style(int s)
{
    box_style = s;
    box_w = (s == 1) ? 2 : 1;
}

// 가로 테두리 한 줄: 칸마다 (칸 너비 / 가로선 폭)개의 가로선 + 교차점
void hline(int x0, int y, const int* w, int c, int left, int mid, int right)
{
    gotoxy(x0, y);
    printf("%s", bx(left));
    for (int k = 0; k < c; k++) {
        for (int n = 0; n < w[k] / box_w; n++) printf("%s", bx(B_H));
        printf("%s", bx(k == c - 1 ? right : mid));
    }
}

// 세로선과 빈칸으로 된 내용 줄 한 줄
void vline(int x0, int y, const int* w, int c)
{
    gotoxy(x0, y);
    printf("%s", bx(B_V));
    for (int k = 0; k < c; k++) printf("%*s%s", w[k], "", bx(B_V));
}

// x0 위치에 c칸 r줄 표를 그리고, 각 칸 내용 시작 x를 cx[]에 저장. 표 오른쪽 끝 x를 반환
int draw_table(int x0, const int* w, int c, int r, int* cx)
{
    int x = x0 + box_w;              // 세로선도 가로선과 같은 폭으로 그려짐
    for (int k = 0; k < c; k++) {
        cx[k] = x;
        x += w[k] + box_w;
    }
    hline(x0, BOARD_Y, w, c, B_TL, B_TM, B_TR);
    for (int i = 0; i < r; i++) {
        vline(x0, BOARD_Y + 1 + 2 * i, w, c);
        if (i < r - 1) hline(x0, BOARD_Y + 2 + 2 * i, w, c, B_ML, B_MM, B_MR);
        else           hline(x0, BOARD_Y + 2 + 2 * i, w, c, B_BL, B_BM, B_BR);
    }
    return x - 1;
}

// 원본 draw_check02: 이제 일차 표와 합계 표 두 개를 나란히 그림
void draw_check02(int c, int r)
{
    int right = draw_table(1, dayw, c, r, colx);            // 왼쪽: 일차 표 (c = 13칸)
    int tx[5];
    draw_table(right + 3, totalw, 5, r, tx);                // 오른쪽: 합계 표 (2칸 띄움)
    name2x = tx[0];
    for (int k = 0; k < 4; k++) colx[13 + k] = tx[1 + k];
}

int row_y(int j) { return BOARD_Y + 3 + 2 * j; }   // j=0 라이벌, j=1 나

void display_totals(int j, int score[], int hit[], int error[], int ball[])
{
    int v[4] = { score[j], hit[j], error[j], ball[j] };
    int col[4] = { C_YEL, C_WHITE, C_RED, C_GREEN };
    for (int k = 0; k < 4; k++) {
        gotoxy(colx[13 + k], row_y(j));
        set_color(col[k]);
        printf("%4d", v[k]);
    }
    set_color(C_NORM);
}

// 창 크기가 바뀌었을 때 전광판을 다시 그리기 위해 기록을 보관
int day_sales[2][13];          // -1 = 아직 기록 없음
int *g_score, *g_hit, *g_error, *g_ball;
int board_active = 0;

void display_scoreboard(int score[], int hit[], int error[], int ball[])
{
    g_score = score; g_hit = hit; g_error = error; g_ball = ball;
    board_active = 1;
    set_color(C_BROWN);
    draw_check02(MAX_DAYS + 1, 3);

    gotoxy(1, 1);
    set_color(C_YEL);  printf("일차별 매출");      // 7일 표는 폭이 좁아서 제목을 짧게 (오른쪽 "누적 합계"와 안 겹치게)
    set_color(C_GRAY); printf(" (단위: 천원)");
    gotoxy(name2x - box_w, 1);
    set_color(C_YEL);  printf("누적 합계");

    // 왼쪽 표: 일차
    set_color(C_NORM);
    gotoxy(colx[0], BOARD_Y + 1); printf(" 일차 ");
    for (int i = 1; i <= MAX_DAYS; i++) {
        gotoxy(colx[i], BOARD_Y + 1);
        set_color(i > REG_DAYS ? C_GRAY : C_NORM);   // 4~5일은 연장 영업
        printf("%2d", i);
    }
    gotoxy(colx[0], row_y(0)); set_color(C_RED);  printf("라이벌");
    gotoxy(colx[0], row_y(1)); set_color(C_CYAN); printf("  나  ");

    // 오른쪽 표: 매출 / 손님 / 실수 / 단골
    const char* head[4] = { "매출", "손님", "실수", "단골" };
    set_color(C_NORM);
    gotoxy(name2x, BOARD_Y + 1); printf(" 가게 ");
    for (int k = 0; k < 4; k++) {
        gotoxy(colx[13 + k], BOARD_Y + 1);
        printf("%s", head[k]);
    }
    gotoxy(name2x, row_y(0)); set_color(C_RED);  printf("라이벌");
    gotoxy(name2x, row_y(1)); set_color(C_CYAN); printf("  나  ");
    for (int j = 0; j < 2; j++) display_totals(j, score, hit, error, ball);

    // 이미 기록된 일차 칸 다시 출력
    for (int j = 0; j < 2; j++)
        for (int i = 1; i <= MAX_DAYS; i++)
            if (day_sales[j][i] >= 0) {
                gotoxy(colx[i], row_y(j));
                set_color(j == 0 ? C_RED : C_CYAN);
                printf("%2d", day_sales[j][i]);
            }
    set_color(C_NORM);
}

// 그날 칸에 매출 기록 (깜빡이며 들어감)
void record_cell(int day, int j, int sales)
{
    int shown = sales > 99 ? 99 : sales;
    day_sales[j][day] = shown;
    for (int n = 0; n < 6; n++) {
        gotoxy(colx[day], row_y(j));
        if (n % 2 == 0) { set_color(C_YEL | BACKGROUND_RED); printf("%2d", shown); }
        else            { set_color(C_NORM); printf("  "); }
        Sleep(90);
    }
    gotoxy(colx[day], row_y(j));
    set_color(j == 0 ? C_RED : C_CYAN);
    printf("%2d", shown);
    set_color(C_NORM);
}

// =============================================================
//  게임 화면 캔버스 (전광판 아래 영역)
//  한 프레임을 버퍼에 그린 뒤 바뀐 줄만 출력 -> cls 없이 깜빡임 제거
//  CP949에서 한글 1글자 = 2바이트 = 2칸이라 바이트 위치 = 화면 칸 위치
// =============================================================
#define CV_W 116   // 왼쪽 = 가게 화면(손님, 틀), 오른쪽 = 붕어빵 진열대
#define CV_H 21    // 21줄 (전광판 아래 10~30번째 줄)
#define CV_Y 10

char cv[CV_H][CV_W + 1];
unsigned char ca[CV_H][CV_W];
char pcv[CV_H][CV_W + 1];
unsigned char pca[CV_H][CV_W];

void cv_clear(void)
{
    for (int y = 0; y < CV_H; y++) {
        memset(cv[y], ' ', CV_W);
        cv[y][CV_W] = 0;
        memset(ca[y], C_NORM, CV_W);
    }
}

void cv_put(int x, int y, const char* src, int attr)
{
    if (y < 0 || y >= CV_H) return;
    char s[512];
    to_cp949(src, s, sizeof(s));        // 한글 1글자 = 2바이트 = 2칸이 되도록 CP949로
    for (int k = 0; s[k]; k++) {
        int xx = x + k;
        if (xx < 0) continue;
        if (xx >= CV_W) break;
        cv[y][xx] = s[k];
        ca[y][xx] = (unsigned char)attr;
    }
}

void cv_printf(int x, int y, int attr, const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    cv_put(x, y, buf, attr);
}

void cv_invalidate(void)
{
    memset(pcv, 0, sizeof(pcv));
}

void on_resize(void)
{
    clear_screen();
    display_scoreboard(g_score, g_hit, g_error, g_ball);
    cv_invalidate();   // 캔버스 전체를 다시 출력하게
}

void cv_flush(void)
{
    if (board_active && window_resized()) on_resize();
    for (int y = 0; y < CV_H; y++) {
        if (memcmp(cv[y], pcv[y], CV_W) == 0 && memcmp(ca[y], pca[y], CV_W) == 0) continue;
        gotoxy(1, CV_Y + y);
        int x = 0;
        while (x < CV_W) {
            int a = ca[y][x], e = x;
            while (e < CV_W && ca[y][e] == a) e++;
            set_color(a);
            fwrite(&cv[y][x], 1, e - x, stdout);
            x = e;
        }
        memcpy(pcv[y], cv[y], CV_W + 1);
        memcpy(pca[y], ca[y], CV_W);
    }
    set_color(C_NORM);
    fflush(stdout);
}

void comma(int v, char* out)
{
    char t[32];
    sprintf(t, "%d", v < 0 ? -v : v);
    int len = (int)strlen(t), o = 0;
    if (v < 0) out[o++] = '-';
    for (int i = 0; i < len; i++) {
        out[o++] = t[i];
        if ((len - i - 1) % 3 == 0 && i != len - 1) out[o++] = ',';
    }
    out[o] = 0;
}

void wait_key_msg(int y, const char* msg)
{
    cv_put(2, y, msg, C_GRAY);
    cv_flush();
    flush_keys();
    while (wait_key_live() == -2) cv_flush();   // 창 크기가 바뀌면 다시 그리고 계속 대기
}

// =============================================================
//  게임 데이터
// =============================================================
struct Weather { const char* name; double mul; int color; const char* desc; };
Weather WEATHER[5] = {
    { "맑음", 1.0, C_YEL,   "평범한 하루" },
    { "흐림", 0.9, C_GRAY,  "손님이 조금 적어요" },
    { "비",   0.7, C_BLUE,  "손님이 많이 줄어요" },
    { "한파", 1.3, C_CYAN,  "붕어빵이 잘 팔리는 날!" },
    { "축제", 1.6, C_PINK,  "손님 폭주! 대신 성격이 급해요" },
};
int weather = 0;

int roll_weather(void)
{
    int r = rand() % 100;
    if (r < 35) return 0;
    if (r < 55) return 1;
    if (r < 70) return 2;
    if (r < 90) return 3;
    return 4;
}

// 내 가게 상태 (상점 업그레이드)
int money = 20000;   // 보유금 (원) - 게임 시작 때 난이도에 맞게 다시 정함

// 난이도: 메뉴에서 ← → 로 고름
struct Difficulty {
    const char* name;
    double rival_rate;     // 라이벌 매출 배율
    double patience_rate;  // 손님 인내심이 줄어드는 속도 배율 (클수록 빨리 화냄)
    int    start_money;    // 시작 보유금 (원)
    int    color;          // 표시 색
};
Difficulty diff_table[3] = {
    { "쉬움",   0.85, 0.75, 30000, 10 },
    { "보통",   1.00, 1.00, 20000, 14 },
    { "어려움", 1.20, 1.35, 15000, 12 },
};
int diff = 1;          // 0 쉬움 / 1 보통 / 2 어려움
int price = 1000;    // 붕어빵 1마리 가격
int molds = 2;       // 붕어빵 틀 개수 (최대 4)
int up_sign = 0;     // 간판 교체: 손님 +25%
int up_bell = 0;     // 알림 벨: 다 익으면 소리
int up_chair = 0;    // 대기 의자: 인내심 +30%

struct DayResult { int sales, cust, miss, bonus; };

#define TICK_MS     50
#define DAY_MS      45000
#define COOK        50      // 한 면 익는 시간 (틱) = 2.5초
#define BURN        60      // 다 익은 뒤 타기까지 (틱) = 3초
#define BATTER_COST 400     // 틀 1번(2마리) 재료비
#define TIP         500
#define RIVAL_BASE  10.0    // 라이벌 1일차 기본 매출 (천원)  <- 라이벌이 세면 이 숫자를 낮추기
#define RIVAL_GROW  1.0     // 하루 지날 때마다 늘어나는 매출 (천원)
#define PATIENCE_BASE 480   // 손님 인내심 기본값 (틱) = 24초  <- 이 숫자를 키우면 더 천천히 닳음
#define PATIENCE_DAY  12    // 하루 지날 때마다 줄어드는 양 (틱) = 0.6초
#define PATIENCE_MIN  300   // 아무리 늦은 날이어도 최소 15초

// =============================================================
//  라이벌 가게 (초 공격)
// =============================================================
void draw_shop_art(int x, int y, int attr, const char* name)
{
    cv_put(x, y + 0, "   ____________________   ", attr);
    cv_put(x, y + 1, "  /                    \\  ", attr);
    cv_printf(x, y + 2, attr, " |  %-18s |  ", name);
    cv_put(x, y + 3, " |____________________|  ", attr);
    cv_put(x, y + 4, " |  [==]  ~ ~ ~  [==]  |  ", attr);
    cv_put(x, y + 5, " |____________________|  ", attr);
}

DayResult rival_day(int day)
{
    DayResult d;
    double base = (RIVAL_BASE + day * RIVAL_GROW) * WEATHER[weather].mul;
    base *= diff_table[diff].rival_rate;                 // 난이도 배율
    d.sales = (int)(base * (80 + rand() % 41) / 100.0);   // 99천원이 넘어도 점수는 그대로 (칸 표시만 99)
    d.cust  = d.sales / 2 + rand() % 3;
    d.miss  = rand() % 4;
    d.bonus = rand() % (d.cust / 4 + 1);

    for (int f = 0; f <= 40; f++) {
        cv_clear();
        cv_printf(2, 0, C_WHITE, "%d일차 아침", day);
        cv_printf(14, 0, WEATHER[weather].color, "날씨: %s", WEATHER[weather].name);
        cv_printf(28, 0, C_GRAY, "(%s)", WEATHER[weather].desc);
        if (day > REG_DAYS) cv_put(62, 0, "** 연장 영업 **", C_PINK);
        cv_put(2, 2, "라이벌 '황금붕어' 가게가 먼저 장사를 합니다...", C_RED);
        draw_shop_art(4, 4, C_RED, "황금붕어");
        // 손님이 라이벌 가게로 들어가는 애니메이션
        int px = 60 - (f * 3) % 30;
        cv_put(px, 8, (f % 2) ? "(o_o)" : "(^_^)", C_NORM);
        cv_put(px, 9, (f % 2) ? " /|\\ " : " \\|/ ", C_NORM);
        cv_put(px, 10, (f % 2) ? " / \\ " : "  |  ", C_NORM);
        int now = d.sales * f / 40;
        char bar[31];
        int fill = 30 * f / 40;
        for (int k = 0; k < 30; k++) bar[k] = k < fill ? '#' : '.';
        bar[30] = 0;
        cv_printf(4, 12, C_RED, "영업 진행 [%s]", bar);
        cv_printf(4, 13, C_YEL, "라이벌 매출  %2d천원", now);
        cv_flush();
        Sleep(50);
    }
    cv_printf(4, 15, C_WHITE, "라이벌 오늘 매출: %d천원  (손님 %d명)", d.sales, d.cust);
    wait_key_msg(18, "아무 키나 누르면 내 가게 영업을 준비합니다...");
    return d;
}

// =============================================================
//  내 가게 (말 공격)
// =============================================================
struct Mold { int state, t, fill; };     // state 0 빈틀 1 반죽 2 앞면 3 뒷면 4 탐 / fill 1 팥 2 슈크림
struct Cust { int active, arrived, off, pat, patMax, a, s, step; };

const char* fill_name(int f) { return f == 1 ? "팥" : "슈크림"; }

// 아침 준비: 가격 정하기
void prep_screen(int day, int rival_today, int score[])
{
    flush_keys();
    char mbuf[32];
    while (1) {
        cv_clear();
        cv_printf(2, 0, C_WHITE, "%d일차 내 가게 영업 준비", day);
        cv_printf(80, 0, diff_table[diff].color, "난이도: %s", diff_table[diff].name);
        cv_printf(28, 0, WEATHER[weather].color, "날씨: %s - %s", WEATHER[weather].name, WEATHER[weather].desc);
        cv_printf(2, 2, C_RED,  "라이벌 오늘 매출 : %d천원", rival_today);
        int diff = score[1] - score[0];
        cv_printf(2, 3, diff >= 0 ? C_GREEN : C_RED, "누적 매출 차이  : %+d천원 (나 - 라이벌)", diff);
        comma(money, mbuf);
        cv_printf(2, 4, C_YEL,  "보유금          : %s원", mbuf);
        cv_printf(2, 5, C_NORM, "붕어빵 틀       : %d개 / 간판 %s / 벨 %s / 의자 %s", molds,
                  up_sign ? "O" : "X", up_bell ? "O" : "X", up_chair ? "O" : "X");

        cv_put(2, 7, "오늘 붕어빵 가격을 정하세요   ← →", C_WHITE);
        cv_printf(6, 9, C_YEL, "<<  %4d원  >>", price);
        double demand = WEATHER[weather].mul * pow(1000.0 / price, 1.6) * (up_sign ? 1.25 : 1.0);
        const char* dtxt; int dc;
        if (demand > 1.4)      { dtxt = "손님 예상: 아주 많음"; dc = C_GREEN; }
        else if (demand > 1.0) { dtxt = "손님 예상: 많음";      dc = C_GREEN; }
        else if (demand > 0.75){ dtxt = "손님 예상: 보통";      dc = C_YEL; }
        else                   { dtxt = "손님 예상: 적음";      dc = C_RED; }
        cv_put(26, 9, dtxt, dc);
        cv_put(2, 11, "비싸게 받으면 손님이 줄고, 싸게 받으면 많이 와도 남는 게 적어요.", C_GRAY);
        cv_printf(2, 12, C_GRAY, "재료비: 틀 1번(2마리)에 %d원 / 단골(빨리 받은 손님)은 팁 %d원", BATTER_COST, TIP);

        cv_put(2, 14, "[조작]  ← → 틀 선택   SPACE 반죽 / 뒤집기 / 꺼내기", C_CYAN);
        cv_put(2, 15, "        1 팥   2 슈크림   Q W E R 해당 손님에게 판매", C_CYAN);
        cv_put(2, 16, "        (한/영 키가 영문 상태인지 확인하세요)", C_GRAY);
        cv_put(2, 18, "ENTER: 영업 시작!", C_WHITE);
        cv_flush();

        int k = wait_key_live();
        if (k == K_LEFT  && price > 500)  price -= 100;
        if (k == K_RIGHT && price < 1500) price += 100;
        if (k == 13) break;
    }
}

// 붕어빵 진열대 (게임 화면 오른쪽): 왼쪽 칸 팥, 오른쪽 칸 슈크림
#define SLOT_W     23    // 손님 자리 / 붕어빵 틀 한 칸 너비 (간격 포함)
#define SLOT_X0    2     // 첫 번째 칸 시작 x
#define SHELF_X    93
#define SHELF_ROWS 7     // 물고기 줄 수 (한 줄에 2마리 -> 칸마다 최대 14마리 그림)

void shelf_cell(int x, int y, int w, int count, int row, int color)
{
    char buf[16];
    int first = row * 2;                         // 이 줄에 그릴 첫 번째 붕어빵 번호
    int maxShow = SHELF_ROWS * 2;
    if (count == 0) {
        if (row == SHELF_ROWS / 2) cv_put(x + 2, y, "(없음)", C_GRAY);
        return;
    }
    if (count > maxShow && row == SHELF_ROWS - 1) {   // 넘치면 마지막 줄에 +N
        sprintf(buf, " +%d", count - (maxShow - 1));   // 그림으로 13마리 + 나머지 숫자
        cv_put(x + 1, y, "<><", color);
        cv_put(x + 4, y, buf, C_WHITE);
        return;
    }
    if (first < count)     cv_put(x + 1, y, "<><", color);
    if (first + 1 < count) cv_put(x + 5, y, "<><", color);
    (void)w;
}

void draw_shelf(int stockA, int stockS)
{
    int x = SHELF_X, bc = C_BROWN;
    cv_put(x, 3, "+--------------------+", bc);
    cv_put(x + 5, 3, "[ 진열대 ]", C_YEL);
    cv_put(x, 4, "|", bc); cv_put(x + 1, 4, "   팥    ", C_RED); cv_put(x + 10, 4, "|", bc);
    cv_put(x + 11, 4, "  슈크림  ", C_YEL); cv_put(x + 21, 4, "|", bc);
    cv_put(x, 5, "+---------+----------+", bc);
    for (int r = 0; r < SHELF_ROWS; r++) {
        int y = 6 + r;
        cv_put(x, y, "|", bc); cv_put(x + 10, y, "|", bc); cv_put(x + 21, y, "|", bc);
        shelf_cell(x + 1,  y, 9,  stockA, SHELF_ROWS - 1 - r, C_RED);   // 아래부터 쌓임
        shelf_cell(x + 11, y, 10, stockS, SHELF_ROWS - 1 - r, C_YEL);
    }
    cv_put(x, 13, "+---------+----------+", bc);
    cv_put(x, 14, "|", bc); cv_put(x + 10, 14, "|", bc); cv_put(x + 21, 14, "|", bc);
    cv_printf(x + 1, 14, stockA ? C_WHITE : C_GRAY, "%4d마리 ", stockA);
    cv_printf(x + 11, 14, stockS ? C_WHITE : C_GRAY, "%5d마리 ", stockS);
    cv_put(x, 15, "+---------+----------+", bc);
}

DayResult my_day(int day, int rival_today, int score[])
{
    prep_screen(day, rival_today, score);
    int moneyStart = money;   // 영업 시작 전 보유금 (정산에서 계산 확인용)

    Mold mold[4];
    Cust cust[4];
    memset(mold, 0, sizeof(mold));
    memset(cust, 0, sizeof(cust));
    int sel = 0, stockA = 0, stockS = 0;
    int revenue = 0, served = 0, miss = 0, bonus = 0, burnt = 0, lost = 0, cost = 0;
    int tipTotal = 0;   // 팁은 영업 중에는 모아 두고, 정산할 때 보유금에 넣음
    int salesTotal = 0; // 붕어빵 판매 금액 합계 (팁 제외)
    int sold = 0;       // 판매한 붕어빵 수
    char msg[128] = "영업 시작! 반죽을 부어 보세요 (SPACE)";
    int msgColor = C_WHITE, msgTimer = 60, frame = 0;
    const char* keys = "qwer";

    double spawnP = (1.0 / 50.0) * WEATHER[weather].mul * pow(1000.0 / price, 1.6) * (up_sign ? 1.25 : 1.0);
    // 손님 인내심 (틱 단위, 20틱 = 1초): 1일차 약 24초 -> 5일차 약 21초, 최소 15초
    int patDay = PATIENCE_BASE - day * PATIENCE_DAY;
    if (patDay < PATIENCE_MIN) patDay = PATIENCE_MIN;
    int patBase = (int)(patDay * (up_chair ? 1.3 : 1.0) * (weather == 4 ? 0.85 : 1.0)
                        / diff_table[diff].patience_rate);   // 어려울수록 빨리 화냄

    DWORD start = GetTickCount();
    flush_keys();
    unk_new = 0;                                 // 이전 화면에서 눌린 키 정보는 버림
    cv_invalidate();

    while (1) {
        int remain = DAY_MS - (int)(GetTickCount() - start);
        if (remain <= 0) break;
        frame++;

        // ---------- 입력 ----------
        while (key_ready()) {
            int k = read_key();
            Mold* m = &mold[sel];
            if (k == K_LEFT)  sel = (sel + molds - 1) % molds;
            else if (k == K_RIGHT) sel = (sel + 1) % molds;
            else if (k == ' ') {
                if (m->state == 0) {
                    if (money >= BATTER_COST) {
                        money -= BATTER_COST; cost += BATTER_COST;
                        m->state = 1;
                        sprintf(msg, "%d번 틀에 반죽을 부었어요. 1:팥 2:슈크림", sel + 1);
                        msgColor = C_WHITE; sfx(S_BATTER);
                    } else {
                        strcpy(msg, "재료 살 돈이 부족해요!"); msgColor = C_RED; sfx(S_ERR);
                    }
                } else if (m->state == 1) {
                    strcpy(msg, "먼저 1(팥) 또는 2(슈크림)로 속을 넣으세요"); msgColor = C_YEL; sfx(S_ERR);
                } else if (m->state == 2) {
                    if (m->t < COOK) { strcpy(msg, "아직 앞면이 덜 익었어요"); msgColor = C_YEL; sfx(S_ERR); }
                    else { m->state = 3; m->t = 0; sprintf(msg, "%d번 틀 뒤집기!", sel + 1); msgColor = C_WHITE; sfx(S_FLIP); }
                } else if (m->state == 3) {
                    if (m->t < COOK) { strcpy(msg, "아직 뒷면이 덜 익었어요"); msgColor = C_YEL; sfx(S_ERR); }
                    else {
                        if (m->fill == 1) stockA += 2; else stockS += 2;
                        sprintf(msg, "%s 붕어빵 2마리 완성! 진열대로", fill_name(m->fill));
                        msgColor = C_GREEN; m->state = 0; m->t = 0; sfx(S_FLIP);
                    }
                } else if (m->state == 4) {
                    m->state = 0; m->t = 0;
                    strcpy(msg, "탄 붕어빵을 버렸어요"); msgColor = C_GRAY;
                }
                msgTimer = 60;
            }
            else if (k == '1' || k == '2') {
                if (m->state == 1) {
                    m->fill = k - '0'; m->state = 2; m->t = 0;
                    sprintf(msg, "%s을(를) 넣고 굽기 시작", fill_name(m->fill));
                    msgColor = C_WHITE; sfx(S_BATTER);
                } else { strcpy(msg, "반죽을 부은 틀에만 속을 넣을 수 있어요"); msgColor = C_YEL; sfx(S_ERR); }
                msgTimer = 60;
            }
            else if (k == 'q' || k == 'w' || k == 'e' || k == 'r') {
                int ci = (int)(strchr(keys, k) - keys);
                Cust* c = &cust[ci];
                if (!c->active || !c->arrived) {
                    strcpy(msg, "그 자리에는 손님이 없어요"); msgColor = C_GRAY; sfx(S_ERR);
                } else if (stockA < c->a || stockS < c->s) {
                    sprintf(msg, "재고 부족! 팥 %d / 슈크림 %d 필요해요", c->a, c->s); msgColor = C_RED; sfx(S_ERR);
                } else {
                    stockA -= c->a; stockS -= c->s;
                    int pay = (c->a + c->s) * price;    // 1마리 가격 x 주문한 마리 수 (예: 팥2+슈1, 1000원 -> 3,000원)
                    revenue += pay; served++;
                    salesTotal += pay; sold += c->a + c->s;
                    money += pay;                       // 판매 금액은 바로 보유금으로 (그날 재료비로 쓸 수 있음)
                    char pbuf[32]; comma(pay, pbuf);
                    if (c->pat * 10 > c->patMax * 6) {
                        bonus++; revenue += TIP; tipTotal += TIP;   // 팁은 정산 때 지급
                        sprintf(msg, "%c 손님 판매 +%s원  빨리 줘서 단골 등록! 팁 +%d원", k - 32, pbuf, TIP);
                        msgColor = C_GREEN;
                    } else {
                        sprintf(msg, "%c 손님 판매 +%s원", k - 32, pbuf);
                        msgColor = C_YEL;
                    }
                    c->active = 0;
                    sfx(S_SELL);
                }
                msgTimer = 60;
            }
        }

        if (unk_new) {                               // 인식 못한 키 -> 정보 표시 (문제 확인용)
            sprintf(msg, "인식 못한 키 (VK %d / 스캔 %d / 문자 %d) - 한/영 키나 키보드 상태를 확인하세요", unk_vk, unk_sc, unk_ch);
            msgColor = C_GRAY; msgTimer = 60; unk_new = 0;
        }

        // ---------- 굽기 진행 ----------
        for (int i = 0; i < molds; i++) {
            Mold* m = &mold[i];
            if (m->state == 2 || m->state == 3) {
                m->t++;
                if (m->t == COOK && up_bell) {           // 알림 벨: 소리 + 메시지
                    sfx(S_BELL);
                    sprintf(msg, "[벨] %d번 틀 %s이 다 익었어요! SPACE로 %s", i + 1,
                            m->state == 2 ? "앞면" : "뒷면", m->state == 2 ? "뒤집기" : "꺼내기");
                    msgColor = C_YEL; msgTimer = 40;
                }
                if (m->t == COOK + BURN * 6 / 10 && up_bell) {   // 알림 벨 2차: 곧 타기 직전 (약 1.2초 전)
                    sfx(S_HURRY);
                    sprintf(msg, "[벨] %d번 틀이 곧 타요!! 빨리 SPACE!", i + 1);
                    msgColor = C_RED; msgTimer = 40;
                }
                if (m->t >= COOK + BURN) {
                    m->state = 4; miss++; burnt++;
                    sprintf(msg, "앗! %d번 틀 붕어빵이 탔어요 (실수 +1)", i + 1);
                    msgColor = C_RED; msgTimer = 60; sfx(S_BAD);
                }
            }
        }

        // ---------- 손님 ----------
        for (int i = 0; i < 4; i++) {
            Cust* c = &cust[i];
            if (!c->active) continue;
            if (!c->arrived) {
                c->off -= 3; c->step++;
                if (c->off <= 0) { c->off = 0; c->arrived = 1; sfx(S_ENTER); }
            } else {
                c->pat--;
                if (c->pat <= 0) {
                    c->active = 0; miss++; lost++;
                    sprintf(msg, "%c 손님이 화가 나서 돌아갔어요 (실수 +1)", keys[i] - 32);
                    msgColor = C_RED; msgTimer = 60; sfx(S_BAD);
                }
            }
        }
        if (remain > 3000 && rand() / (RAND_MAX + 1.0) < spawnP) {   // MinGW는 RAND_MAX가 32767이라 비율로 비교
            int slot = -1, freeCnt = 0;
            for (int i = 0; i < 4; i++) if (!cust[i].active) { freeCnt++; if (rand() % freeCnt == 0) slot = i; }
            if (slot >= 0) {
                Cust* c = &cust[slot];
                int n = 1 + rand() % 3;
                c->a = rand() % (n + 1); c->s = n - c->a;
                c->active = 1; c->arrived = 0; c->step = 0;
                c->off = (SHELF_X - 7) - (SLOT_X0 + slot * SLOT_W + 6);   // 진열대 앞에서 걸어 들어옴
                c->patMax = c->pat = patBase + rand() % 100;   // 손님마다 0~5초 차이
            }
        }

        // ---------- 그리기 ----------
        //  0 상단 정보         1 매출 / 보유금 / 팁    2 (여백)
        //  3 주문  5~7 손님  8 인내심   9 카운터       10 (여백)
        // 11~16 붕어빵 틀                              17 (여백)
        // 18 조작키  19 메시지  20 오늘 기록
        cv_clear();
        char b1[32], b2[32], b3[32];
        int sec = remain / 1000 + 1;
        cv_printf(2, 0, C_WHITE, "%d일차", day);
        cv_printf(10, 0, WEATHER[weather].color, "[%s]", WEATHER[weather].name);
        {
            char bar[21]; int fill = 20 * remain / DAY_MS;
            for (int k = 0; k < 20; k++) bar[k] = k < fill ? '#' : '.';
            bar[20] = 0;
            cv_printf(24, 0, sec <= 10 ? C_RED : C_GREEN, "남은시간 [%s] %2d초", bar, sec);
        }
        cv_printf(66, 0, C_YEL, "가격 %d원", price);

        comma(revenue, b1); comma(money, b2); comma(tipTotal, b3);
        cv_printf(2, 1, C_YEL, "오늘 매출 %s원", b1);
        cv_printf(30, 1, C_NORM, "보유금 %s원", b2);
        cv_printf(56, 1, C_GREEN, "받은 팁 %s원", b3);
        cv_put(74, 1, "(정산 때 지급)", C_GRAY);

        // 손님 자리 (빈자리 표시는 걷는 손님과 겹치지 않는 줄에)
        for (int i = 0; i < 4; i++) {
            int x0 = SLOT_X0 + i * SLOT_W;
            Cust* c = &cust[i];
            if (!c->active) { cv_put(x0 + 3, 8, "(빈 자리)", C_GRAY); continue; }
            if (c->arrived) {
                int ratio = c->pat * 100 / c->patMax;
                int col = ratio > 60 ? C_GREEN : (ratio > 30 ? C_YEL : C_RED);
                cv_printf(x0, 3, C_CYAN, "[%c]", keys[i] - 32);
                if (c->a && c->s) cv_printf(x0 + 4, 3, C_YEL, "팥%d 슈%d", c->a, c->s);
                else if (c->a)    cv_printf(x0 + 4, 3, C_YEL, "팥%d", c->a);
                else              cv_printf(x0 + 4, 3, C_YEL, "슈%d", c->s);
                char pb[13]; int f = 12 * c->pat / c->patMax;
                for (int k = 0; k < 12; k++) pb[k] = k < f ? '=' : ' ';
                pb[12] = 0;
                cv_printf(x0, 8, col, "[%s]", pb);
            }
        }
        // 손님 몸 (걸어오는 손님은 다른 자리 위를 지나가므로 ASCII만 사용)
        for (int i = 0; i < 4; i++) {
            Cust* c = &cust[i];
            if (!c->active) continue;
            int px = SLOT_X0 + i * SLOT_W + 6 + c->off;
            const char* face;
            if (!c->arrived) face = "(o_o)";
            else {
                int ratio = c->pat * 100 / c->patMax;
                face = ratio > 60 ? "(^_^)" : (ratio > 30 ? "(-_-)" : ((frame / 4) % 2 ? "(>_<)" : "(`^')"));
            }
            int fc = c->arrived ? (c->pat * 100 / c->patMax > 30 ? C_WHITE : C_RED) : C_NORM;
            cv_put(px, 5, face, fc);
            cv_put(px, 6, " /|\\ ", C_NORM);
            if (c->arrived) cv_put(px, 7, " / \\ ", C_NORM);
            else            cv_put(px, 7, (c->step / 2) % 2 ? " / \\ " : "  |  ", C_NORM);
        }
        {   // 카운터: 손님 자리 너비만큼 (진열대 앞까지)
            char line[CV_W + 1];
            int len = SHELF_X - 4;
            memset(line, '=', len); line[len] = 0;
            cv_put(1, 9, line, C_BROWN);
            cv_put(len / 2 - 8, 9, "[  카  운  터  ]", C_BROWN);
        }
        draw_shelf(stockA, stockS);

        // 붕어빵 틀 (11~16번 줄)
        for (int i = 0; i < 4; i++) {
            int x0 = SLOT_X0 + i * SLOT_W;
            if (i >= molds) {
                cv_printf(x0, 11, C_GRAY, " [%d번 틀]", i + 1);
                cv_put(x0, 12, "+--------------+", C_GRAY);
                cv_put(x0, 13, "|     잠김     |", C_GRAY);
                cv_put(x0, 14, "|              |", C_GRAY);
                cv_put(x0, 15, "+--------------+", C_GRAY);
                cv_put(x0, 16, "  상점에서 구매", C_GRAY);
                continue;
            }
            Mold* m = &mold[i];
            int boxc = (i == sel) ? C_WHITE : C_BROWN;
            int ready = (m->state == 2 || m->state == 3) && m->t >= COOK;
            if (up_bell && ready) {                     // 알림 벨: 다 익은 틀을 눈에 띄게
                int blink = (frame / 4) % 2;            // 0.2초마다 번갈아
                int danger = m->t > COOK + BURN * 6 / 10;
                const char* tag = danger ? " !! 곧 타요 !! " : "  ((띵동!))    ";
                int tagc;
                if (danger) tagc = blink ? (C_WHITE | BACKGROUND_RED) : C_RED;            // 빨간 바탕 / 빨간 글씨
                else        tagc = blink ? (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_INTENSITY) : C_YEL; // 노란 바탕 / 노란 글씨
                cv_put(x0, 10, tag, tagc);
                boxc = blink ? (danger ? C_RED : C_YEL) : C_WHITE; // 테두리 번쩍
            }
            cv_printf(x0, 11, i == sel ? (C_YEL | BACKGROUND_BLUE) : C_NORM, i == sel ? ">>%d번 틀<<" : "  %d번 틀  ", i + 1);
            if (m->state == 2 || m->state == 3) cv_put(x0 + 11, 11, (frame / 5) % 2 ? "~ ~" : " ~ ~", C_GRAY);
            cv_put(x0, 12, "+--------------+", boxc);
            cv_put(x0, 15, "+--------------+", boxc);
            cv_put(x0, 13, "|", boxc); cv_put(x0 + 15, 13, "|", boxc);
            cv_put(x0, 14, "|", boxc); cv_put(x0 + 15, 14, "|", boxc);

            char st[32] = "", bar[13]; int barc = C_GREEN; const char* hint = "";
            int hintc = C_GRAY;
            memset(bar, ' ', 12); bar[12] = 0;
            switch (m->state) {
            case 0: strcpy(st, "비어있음"); hint = "SPACE: 반죽"; break;
            case 1: strcpy(st, "반죽 OK"); hint = "1 팥 / 2 슈크림"; hintc = C_YEL;
                    memset(bar, '~', 12); barc = C_WHITE; break;
            case 2: case 3: {
                sprintf(st, "%s %s", fill_name(m->fill), m->state == 2 ? "앞면" : "뒷면");
                if (m->t < COOK) {
                    int f = 12 * m->t / COOK;
                    for (int k = 0; k < 12; k++) bar[k] = k < f ? '#' : '.';
                    barc = C_GREEN; hint = "굽는 중...";
                } else {
                    memset(bar, '#', 12);
                    int danger = m->t > COOK + BURN * 6 / 10;
                    barc = danger ? ((frame / 3) % 2 ? C_RED : C_YEL) : C_YEL;
                    hint = m->state == 2 ? "SPACE: 뒤집기!" : "SPACE: 꺼내기!";
                    hintc = danger ? C_RED : C_YEL;
                }
                break;
            }
            case 4: strcpy(st, "까맣게 탐"); memset(bar, 'X', 12); barc = C_RED;
                    hint = "SPACE: 버리기"; hintc = C_RED; break;
            }
            cv_printf(x0 + 2, 13, C_WHITE, "%s", st);
            cv_put(x0 + 2, 14, bar, barc);
            cv_put(x0 + 1, 16, hint, hintc);
        }

        cv_put(2, 18, "←→ 틀 선택     SPACE 반죽/뒤집기/꺼내기     1 팥   2 슈크림     Q W E R 판매", C_GRAY);
        if (msgTimer > 0) { msgTimer--; cv_put(2, 19, msg, msgColor); }
        cv_printf(2, 20, C_NORM, "손님 %d     실수 %d     단골 %d", served, miss, bonus);
        cv_printf(56, 20, C_RED, "라이벌 오늘 %d천원", rival_today);
        cv_printf(80, 20, (revenue + 500) / 1000 >= rival_today ? C_GREEN : C_YEL, "나 %d천원", (revenue + 500) / 1000);
        cv_flush();

        Sleep(TICK_MS);
    }

    // ---------- 마감 정산 ----------
    int waste = stockA + stockS;
    DayResult d;
    d.sales = (revenue + 500) / 1000;   // 전광판은 천원 단위 (반올림), 99가 넘어도 점수는 그대로
    d.cust = served; d.miss = miss; d.bonus = bonus;
    money += tipTotal;                  // 판매 금액은 이미 들어갔으므로 팁만 정산에서 추가

    char b[32], b2[32];
    cv_clear();
    cv_printf(2, 0, C_WHITE, "%d일차 영업 마감!", day);

    // 왼쪽: 오늘 장사 결과
    comma(salesTotal, b); comma(price, b2);
    cv_printf(4, 2, C_NORM,  "판매 금액  : %s원  (%d마리 x %s원)", b, sold, b2);
    comma(tipTotal, b);
    cv_printf(4, 3, C_GREEN, "팁         : %s원  (단골 %d명 x %d원)", b, bonus, TIP);
    comma(revenue, b);
    cv_printf(4, 4, C_YEL,   "오늘 매출  : %s원  -> 전광판 %d천원 (반올림)", b, d.sales);
    cv_printf(4, 6, C_NORM,  "받은 손님  : %d명", served);
    cv_printf(4, 7, C_RED,   "실수       : %d (탄 붕어빵 %d / 화나서 간 손님 %d)", miss, burnt, lost);
    if (waste) cv_printf(4, 8, C_GRAY, "팔지 못한 붕어빵 %d마리는 식어서 폐기했어요", waste);

    // 오른쪽: 보유금 계산 (아침 보유금 - 재료비 + 판매 금액 + 팁)
    int x2 = 66;
    cv_put(x2, 2, "[ 보유금 계산 ]", C_YEL);
    comma(moneyStart, b); cv_printf(x2, 3, C_NORM,  "  아침 보유금 %12s원", b);
    comma(cost, b);       cv_printf(x2, 4, C_RED,   "- 재료비      %12s원", b);
    comma(salesTotal, b); cv_printf(x2, 5, C_GREEN, "+ 판매 금액   %12s원", b);
    comma(tipTotal, b);   cv_printf(x2, 6, C_GREEN, "+ 팁          %12s원", b);
    cv_put(x2, 7, "------------------------------", C_GRAY);
    comma(money, b);      cv_printf(x2, 8, C_YEL,   "= 지금 보유금 %12s원", b);
    if (moneyStart - cost + salesTotal + tipTotal != money)    // 계산 검증 (맞으면 아무것도 안 나옴)
        cv_put(x2, 9, "계산이 맞지 않아요! 코드를 확인하세요", C_RED);

    if (d.sales > rival_today) cv_put(4, 11, "오늘은 라이벌보다 많이 팔았어요!", C_GREEN);
    else if (d.sales < rival_today) cv_put(4, 11, "오늘은 라이벌에게 졌어요...", C_RED);
    else cv_put(4, 11, "오늘은 라이벌과 똑같이 팔았어요", C_YEL);
    Sleep(400);
    wait_key_msg(14, "아무 키나 누르면 전광판에 기록합니다...");
    return d;
}

// =============================================================
//  상점 (하루가 끝날 때마다)
// =============================================================
void shop(void)
{
    flush_keys();
    int cur = 0;
    char b[32], c[32];
    while (1) {
        int moldCost = 30000;
        const char* name[5] = { "붕어빵 틀 추가", "간판 교체", "알림 벨", "대기 의자", "다음 날로" };
        const char* desc[5] = { "한 번에 더 많이 구울 수 있어요", "손님이 25% 더 와요",
                                "다 익으면 벨이 울려요", "손님 인내심 +30%", "" };
        int cost[5] = { moldCost, 25000, 10000, 20000, 0 };
        int owned[5] = { molds >= 4, up_sign, up_bell, up_chair, 0 };

        cv_clear();
        cv_put(2, 0, "붕어빵 가게 상점", C_YEL);
        comma(money, b);
        cv_printf(30, 0, C_YEL, "보유금 %s원", b);
        for (int i = 0; i < 5; i++) {
            int y = 2 + i * 2;
            int attr = (i == cur) ? (C_WHITE | BACKGROUND_BLUE) : C_NORM;
            cv_put(2, y, i == cur ? ">>" : "  ", C_YEL);
            if (i == 4) { cv_put(5, y, name[i], attr); continue; }
            comma(cost[i], c);
            if (i == 0) cv_printf(5, y, owned[i] ? C_GRAY : attr, "%-14s (%d/4)  %8s원", name[i], molds, c);
            else        cv_printf(5, y, owned[i] ? C_GRAY : attr, "%-14s        %8s원", name[i], c);
            cv_put(42, y, owned[i] ? "보유 중" : desc[i], owned[i] ? C_GRAY : C_CYAN);
        }
        cv_put(2, 13, "↑↓ 선택   ENTER 구매", C_GRAY);
        cv_flush();

        int k = wait_key_live();
        if (k == K_UP)   cur = (cur + 4) % 5;
        if (k == K_DOWN) cur = (cur + 1) % 5;
        if (k == 13) {
            if (cur == 4) break;
            if (owned[cur]) { sfx(S_ERR); continue; }
            if (money < cost[cur]) {
                cv_put(2, 15, "돈이 부족해요!", C_RED); cv_flush(); sfx(S_ERR); Sleep(600); continue;
            }
            money -= cost[cur];
            if (cur == 0) molds++;
            if (cur == 1) up_sign = 1;
            if (cur == 2) up_bell = 1;
            if (cur == 3) up_chair = 1;
            sfx(S_SELL);
        }
    }
}

// =============================================================
//  경기(장사) 종료
// =============================================================
void game_over(int day, int score[], int hit[], int error[], int ball[])
{
    remove(SAVE_FILE);                     // 끝난 게임은 이어할 수 없음

    // 최고 기록은 난이도마다 따로: 파일에 "쉬움 보통 어려움" 3개 숫자
    int bests[3] = { 0, 0, 0 };
    FILE* fp = fopen("bungeo_best.txt", "r");
    if (fp) {
        for (int i = 0; i < 3; i++) if (fscanf(fp, "%d", &bests[i]) != 1) bests[i] = 0;
        fclose(fp);
    }
    int best = bests[diff];
    int newRecord = score[1] > best;
    if (newRecord) {
        bests[diff] = score[1];
        fp = fopen("bungeo_best.txt", "w");
        if (fp) { fprintf(fp, "%d %d %d\n", bests[0], bests[1], bests[2]); fclose(fp); }
    }

    cv_clear();
    cv_printf(2, 0, C_WHITE, "%d일간의 붕어빵 대결이 끝났습니다!", day);
    cv_printf(50, 0, diff_table[diff].color, "[난이도: %s]", diff_table[diff].name);
    cv_printf(4, 2, C_RED,  "라이벌 총매출 : %3d천원  (손님 %d / 실수 %d / 단골 %d)", score[0], hit[0], error[0], ball[0]);
    cv_printf(4, 3, C_CYAN, "내 총매출     : %3d천원  (손님 %d / 실수 %d / 단골 %d)", score[1], hit[1], error[1], ball[1]);

    if (score[1] > score[0]) {
        cv_put(4, 5, "  ____  _   _ ____   ____ _____ ____ ____  _ ", C_YEL);
        cv_put(4, 6, " / ___|| | | / ___| / ___| ____/ ___/ ___|| |", C_YEL);
        cv_put(4, 7, " \\___ \\| | | \\___ \\| |   |  _| \\___ \\___ \\| |", C_YEL);
        cv_put(4, 8, "  ___) | |_| |___) | |___| |___ ___) |__) |_|", C_YEL);
        cv_put(4, 9, " |____/ \\___/|____/ \\____|_____|____/____/(_)", C_YEL);
        const char* title;
        if (score[1] >= score[0] * 13 / 10) title = "칭호: 붕어빵 거리의 전설";
        else if (error[1] <= day)           title = "칭호: 실수 없는 장인";
        else                                title = "칭호: 동네 붕어빵 사장님";
        cv_put(4, 11, "승리! 라이벌 '황금붕어'가 가게를 접었습니다.", C_GREEN);
        cv_printf(4, 12, C_PINK, "[%s] %s", diff_table[diff].name, title);
        sfx(S_WIN);
    } else if (score[1] < score[0]) {
        cv_put(4, 5, "  ____    _    __  __ _____    _____     _______ ____", C_RED);
        cv_put(4, 6, " / ___|  / \\  |  \\/  | ____|  / _ \\ \\   / / ____|  _ \\", C_RED);
        cv_put(4, 7, "| |  _  / _ \\ | |\\/| |  _|   | | | \\ \\ / /|  _| | |_) |", C_RED);
        cv_put(4, 8, "| |_| |/ ___ \\| |  | | |___  | |_| |\\ V / | |___|  _ <", C_RED);
        cv_put(4, 9, " \\____/_/   \\_\\_|  |_|_____|  \\___/  \\_/  |_____|_| \\_\\", C_RED);
        cv_printf(4, 11, C_RED, "패배... 라이벌 '황금붕어'가 %d천원 차이로 거리를 차지했습니다.", score[0] - score[1]);
        cv_put(4, 12, "틀을 늘리고, 가격을 조절해서 다시 도전해 보세요.", C_GRAY);
        sfx(S_LOSE);
    } else {
        cv_put(4, 5, " ____  ____      ___        __", C_YEL);
        cv_put(4, 6, "|  _ \\|  _ \\    / \\ \\      / /", C_YEL);
        cv_put(4, 7, "| | | | |_) |  / _ \\ \\ /\\ / /", C_YEL);
        cv_put(4, 8, "| |_| |  _ <  / ___ \\ V  V /", C_YEL);
        cv_put(4, 9, "|____/|_| \\_\\/_/   \\_\\_/\\_/", C_YEL);
        cv_printf(4, 11, C_YEL, "무승부! %d일 동안 두 가게가 똑같이 %d천원을 팔았어요.", day, score[1]);
        cv_put(4, 12, "다음 판에서 승부를 가려 보세요.", C_GRAY);
        sfx(S_DRAW);
    }
    if (newRecord) cv_printf(4, 14, C_YEL,  "[%s] 최고 기록 갱신! %d천원", diff_table[diff].name, score[1]);
    else           cv_printf(4, 14, C_GRAY, "[%s] 최고 기록: %d천원", diff_table[diff].name, best);
    Sleep(500);
    cv_put(2, 17, "ESC를 누르면 게임을 종료합니다", C_WHITE);
    cv_flush();
    flush_keys();                          // 영업 중에 누르던 키가 남아 있어도 무시
    while (1) {                            // ESC를 누를 때까지 결과 화면 유지
        int k = wait_key_live();
        if (k == -2) { cv_flush(); continue; }   // 창 크기가 바뀌면 다시 그림
        if (k == 27) break;
    }
    set_color(C_NORM);
    gotoxy(1, CV_Y + CV_H);
}

// =============================================================
//  하루 진행 (원본 control_scoreboard 구조 유지)
//  i = 일차(이닝), j = 0 라이벌(초) / 1 나(말)
// =============================================================
// =============================================================
//  세이브와 이어하기
//  - 하루 장사가 끝나고 상점에서 나올 때 자동 저장
//  - 메뉴의 "이어하기"로 다음 날부터 다시 시작
//  - 게임이 끝나면(승/패/무) 저장 파일 삭제
// =============================================================
void save_game(int nextDay, int score[], int hit[], int error[], int ball[])
{
    FILE* fp = fopen(SAVE_FILE, "w");
    if (!fp) return;
    fprintf(fp, "%d %d %d %d %d %d %d\n", nextDay, diff, money, molds, up_sign, up_bell, up_chair);
    for (int j = 0; j < 2; j++)
        fprintf(fp, "%d %d %d %d\n", score[j], hit[j], error[j], ball[j]);
    for (int j = 0; j < 2; j++) {
        for (int d = 1; d <= MAX_DAYS; d++) fprintf(fp, "%d ", day_sales[j][d]);
        fprintf(fp, "\n");
    }
    fclose(fp);

    cv_clear();
    cv_printf(2, 1, C_GREEN, "[저장 완료] 다음에 메뉴의 '이어하기'로 %d일차부터 계속할 수 있어요.", nextDay);
    cv_flush();
    Sleep(900);
}

// 저장 파일의 첫 줄만 읽어서 메뉴에 보여 줄 정보 확인 (없으면 0)
int save_peek(int* d, int* df)
{
    FILE* fp = fopen(SAVE_FILE, "r");
    if (!fp) return 0;
    int ok = fscanf(fp, "%d %d", d, df) == 2 && *d >= 2 && *d <= MAX_DAYS && *df >= 0 && *df <= 2;
    fclose(fp);
    return ok;
}

// 저장 파일 읽기: 성공하면 다음 일차, 실패하면 0 (실패하면 아무 값도 바꾸지 않음)
int load_game(int score[], int hit[], int error[], int ball[])
{
    FILE* fp = fopen(SAVE_FILE, "r");
    if (!fp) return 0;
    int nd, df, mo, ml, sg, bl, ch;
    int sc[2], ht[2], er[2], bb[2], ds[2][MAX_DAYS + 1];
    int ok = fscanf(fp, "%d %d %d %d %d %d %d", &nd, &df, &mo, &ml, &sg, &bl, &ch) == 7;
    for (int j = 0; ok && j < 2; j++)
        ok = fscanf(fp, "%d %d %d %d", &sc[j], &ht[j], &er[j], &bb[j]) == 4;
    for (int j = 0; ok && j < 2; j++)
        for (int d = 1; ok && d <= MAX_DAYS; d++)
            ok = fscanf(fp, "%d", &ds[j][d]) == 1;
    fclose(fp);
    if (!ok || nd < 2 || nd > MAX_DAYS || df < 0 || df > 2 || ml < 2 || ml > 4) return 0;

    diff = df; money = mo; molds = ml;
    up_sign = sg; up_bell = bl; up_chair = ch;
    for (int j = 0; j < 2; j++) {
        score[j] = sc[j]; hit[j] = ht[j]; error[j] = er[j]; ball[j] = bb[j];
        for (int d = 1; d <= MAX_DAYS; d++) day_sales[j][d] = ds[j][d];
    }
    return nd;
}

void resume_notice(int day)
{
    cv_clear();
    cv_printf(2, 1, C_GREEN, "** 이어하기 **");
    cv_printf(2, 3, C_WHITE, "저장된 게임을 불러왔습니다. %d일차부터 이어서 영업합니다.", day);
    cv_printf(2, 4, diff_table[diff].color, "난이도: %s", diff_table[diff].name);
    sfx(S_ENTER);
    wait_key_msg(7, "아무 키나 누르면 시작합니다...");
}

void control_scoreboard(int start, int end, int score[], int hit[], int error[], int ball[])
{
    int rival_today = 0;
    for (int i = start; i <= end; i++) {
        weather = roll_weather();
        for (int j = 0; j < 2; j++) {
            DayResult d;
            if (j == 0) { d = rival_day(i); rival_today = d.sales; }
            else        d = my_day(i, rival_today, score);

            score[j] += d.sales;
            hit[j]   += d.cust;
            error[j] += d.miss;    // 원본은 실책이 상대 줄에 찍혔음 -> 자기 줄로 수정
            ball[j]  += d.bonus;
            record_cell(i, j, d.sales);
            display_totals(j, score, hit, error, ball);

            // 중간에 끝내지 않음: start~end일을 둘 다 끝까지 영업 (승패 판정은 main에서)
            if (j == 1 && i < end) {             // 이 구간의 마지막 날이 아니면 상점
                shop();
                save_game(i + 1, score, hit, error, ball);   // 상점에서 나오면 자동 저장
            }
        }
    }
}

// =============================================================
//  오프닝 / 메뉴 / 게임 설명
// =============================================================
// 큰 붕어빵: 큰 머리, 몸통을 채운 비늘 ))), 아가미 선, 꼬리지느러미 (모든 줄 56칸)
#define FISH_H 9
#define FISH_W 56
const char* FISH[FISH_H] = {
    "                ___.---~~~~~~~~~~~~~~~---.___           ",
    "          _.--~~   )))))))))))))))))))))     ~~--._   /|",
    "       .-~   .-.  ( ))))))))))))))))))))))))       `-/ |",
    "     .'     ( @ ) | )))))))))))))))))))))))))))    <   |",
    "    <   *    `-'  | ))))))))))))))))))))))))))) ---<   |",
    "     `.     v     | )))))))))))))))))))))))))))    <   |",
    "       `-._       ( ))))))))))))))))))))))))     _.-\\ |",
    "           `--.__   )))))))))))))))))))))  __.--'      \\|",
    "                 `~~~---.............---~~~'            ",
};

// 눈 / 볼 / 입에 색을 다시 칠함. blink = 1이면 눈을 감은 모양
void draw_fish_face(int x, int y, int blink)
{
    set_color(C_WHITE); gotoxy(x + 12, y + 3); printf(blink ? "( - )" : "( @ )");
    set_color(C_PINK);  gotoxy(x + 8,  y + 4); printf("*");
    set_color(C_RED);   gotoxy(x + 12, y + 5); printf("v");
    set_color(C_NORM);
}

// 몸은 color, 비늘 ')'는 갈색으로 한 글자씩 출력
void draw_fish(int x, int y, int color)
{
    for (int i = 0; i < FISH_H; i++) {
        gotoxy(x, y + i);
        int cur = -1;
        for (int k = 0; FISH[i][k]; k++) {
            int want = (FISH[i][k] == ')') ? C_BROWN : color;
            if (want != cur) { set_color(want); cur = want; }
            putchar(FISH[i][k]);
        }
        printf("  ");               // 뒤에 공백 2칸: 왼쪽으로 움직일 때 이전 그림 지우기
    }
    draw_fish_face(x, y, 0);
}

void draw_title_logo(int y)
{
    set_color(C_YEL);
    gotoxy(24, y);     printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    set_color(C_WHITE);
    gotoxy(29, y + 1); printf("붕   어   빵      타   이   쿤");
    set_color(C_YEL);
    gotoxy(24, y + 2); printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    set_color(C_NORM);
}

// =============================================================
//  오프닝 음악 (직접 작곡한 곡을 실행할 때 코드로 합성)
//  음악 파일 없이 메모리에서 wav 데이터를 만들어 PlaySound로 반복 재생
//  * winmm.dll은 실행 중에 직접 불러옴 (빌드 옵션 추가 필요 없음)
// =============================================================
#define BGM_RATE   22050          // 초당 샘플 수
#define BGM_BPM    150            // 빠르기
#define BGM_STEPS  64             // 8분음표 64개 = 8마디
#define HOLD       -1             // 앞 음을 이어서 길게
#define REST        0             // 쉼표

// 멜로디 (MIDI 음 번호: 60 = 가운데 도, 72 = 한 옥타브 위 도)
const int BGM_MELODY[BGM_STEPS] = {
    76, 79, 84, 79, 76, 74, 72, REST,        // 1마디 C   미 솔 도 솔 미 레 도
    69, 72, 76, 81, 79, 76, 72, REST,        // 2마디 Am  라 도 미 라 솔 미 도
    77, 81, 84, 81, 79, 77, 76, 74,          // 3마디 F   파 라 도 라 솔 파 미 레
    74, 76, 77, 79, HOLD, HOLD, 71, 74,      // 4마디 G   레 미 파 솔~  시 레
    76, 79, 84, 88, 86, 84, 79, 76,          // 5마디 C   미 솔 도 미 레 도 솔 미
    81, 79, 76, 72, 74, 76, 72, 69,          // 6마디 Am  라 솔 미 도 레 미 도 라
    77, 81, 84, 81, 79, 83, 86, 83,          // 7마디 F-G 파 라 도 라 / 솔 시 레 시
    84, HOLD, 79, 76, 72, HOLD, HOLD, REST,  // 8마디 C   도~ 솔 미 도~~
};
// 베이스: 마디(반 마디)마다 근음 (C=48, A=45, F=41, G=43)
const int BGM_BASS[16] = { 48,48, 45,45, 41,41, 43,43, 48,48, 45,45, 41,43, 48,48 };

unsigned char* bgm_wav = NULL;    // 완성된 wav (헤더 + 데이터)
int music_on = 1;                 // 메뉴에서 M 키로 켜고 끄기

double note_freq(int midi) { return 440.0 * pow(2.0, (midi - 69) / 12.0); }

// 소리 하나를 buf에 더하기  wave: 0 사각파(멜로디) 1 삼각파(베이스) 2 잡음(하이햇) 3 킥드럼
void bgm_add(float* buf, int total, int start, int len, double freq, int wave, double vol)
{
    unsigned int seed = 12345u + (unsigned)start;
    double phase = 0;
    for (int i = 0; i < len && start + i < total; i++) {
        double t = (double)i / BGM_RATE, s = 0;
        if (wave == 3) {                                   // 킥: 낮아지는 사인파
            double f = 50 + 90 * exp(-t * 25);
            phase += f / BGM_RATE;
            s = sin(2 * 3.14159265 * phase) * exp(-t * 18);
        } else if (wave == 2) {                            // 하이햇: 짧은 잡음
            seed = seed * 1103515245u + 12345u;
            s = ((seed >> 16) & 0x7FFF) / 16384.0 - 1.0;
            s *= exp(-t * 60);
        } else {
            phase += freq / BGM_RATE;
            double p = phase - floor(phase);
            if (wave == 0) s = (p < 0.25) ? 1.0 : -1.0;    // 25% 사각파 (레트로 게임 소리)
            else           s = 4.0 * fabs(p - 0.5) - 1.0;  // 삼각파
            double att = (i < BGM_RATE / 200) ? (double)i / (BGM_RATE / 200) : 1.0;          // 5ms 시작
            double rel = (len - i < BGM_RATE / 50) ? (double)(len - i) / (BGM_RATE / 50) : 1.0; // 20ms 끝
            double dec = (wave == 0) ? 0.55 + 0.45 * exp(-t * 6) : 1.0;                     // 멜로디는 살짝 감쇠
            s *= att * rel * dec;
        }
        buf[start + i] += (float)(s * vol);
    }
}

void bgm_build(void)
{
    int step = BGM_RATE * 60 / BGM_BPM / 2;                // 8분음표 1개 샘플 수
    int total = step * BGM_STEPS;
    float* mix = (float*)calloc(total, sizeof(float));
    if (!mix) return;

    // 멜로디 (HOLD는 앞 음 길이를 늘림)
    for (int i = 0; i < BGM_STEPS; i++) {
        int m = BGM_MELODY[i];
        if (m == REST || m == HOLD) continue;
        int len = 1;
        while (i + len < BGM_STEPS && BGM_MELODY[i + len] == HOLD) len++;
        bgm_add(mix, total, i * step, len * step - step / 8, note_freq(m), 0, 0.16);
        bgm_add(mix, total, i * step, len * step - step / 8, note_freq(m - 12), 0, 0.05); // 한 옥타브 아래로 두껍게
    }
    // 베이스: 4분음표마다 근음 / 옥타브 위 번갈아
    for (int q = 0; q < BGM_STEPS / 2; q++) {
        int root = BGM_BASS[q / 2];
        int m = (q % 2 == 0) ? root : root + 12;
        bgm_add(mix, total, q * 2 * step, step * 2 - step / 4, note_freq(m), 1, 0.30);
    }
    // 드럼: 1, 3박 킥 / 8분음표마다 하이햇
    for (int i = 0; i < BGM_STEPS; i++) {
        if (i % 4 == 0) bgm_add(mix, total, i * step, step * 2, 0, 3, 0.45);
        bgm_add(mix, total, i * step, step / 2, 0, 2, (i % 2) ? 0.07 : 0.04);
    }

    // 16비트 모노 wav로 변환
    int dataBytes = total * 2;
    bgm_wav = (unsigned char*)malloc(44 + dataBytes);
    if (!bgm_wav) { free(mix); return; }
    unsigned char* h = bgm_wav;
    int v32; short v16;
    memcpy(h, "RIFF", 4); v32 = 36 + dataBytes; memcpy(h + 4, &v32, 4);
    memcpy(h + 8, "WAVEfmt ", 8);
    v32 = 16;            memcpy(h + 16, &v32, 4);
    v16 = 1;             memcpy(h + 20, &v16, 2);   // PCM
    v16 = 1;             memcpy(h + 22, &v16, 2);   // 모노
    v32 = BGM_RATE;      memcpy(h + 24, &v32, 4);
    v32 = BGM_RATE * 2;  memcpy(h + 28, &v32, 4);   // 초당 바이트
    v16 = 2;             memcpy(h + 32, &v16, 2);
    v16 = 16;            memcpy(h + 34, &v16, 2);
    memcpy(h + 36, "data", 4); memcpy(h + 40, &dataBytes, 4);
    short* out = (short*)(h + 44);
    for (int i = 0; i < total; i++) {
        float s = mix[i];
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        out[i] = (short)(s * 30000);
    }
    free(mix);
}

// PlaySound는 winmm.dll 안에 있어서 보통 빌드할 때 -lwinmm이 필요함.
// 실행 중에 winmm.dll을 직접 불러와 함수 주소를 얻으면 빌드 옵션을 고치지 않아도 됨.
typedef BOOL (WINAPI *PlaySoundFunc)(LPCSTR, HMODULE, DWORD);
PlaySoundFunc pPlaySound = NULL;

int load_playsound(void)
{
    if (!pPlaySound) {
        HMODULE dll = LoadLibraryA("winmm.dll");
        if (dll) pPlaySound = (PlaySoundFunc)(void*)GetProcAddress(dll, "PlaySoundA");
    }
    return pPlaySound != NULL;     // 실패하면 음악 없이 게임은 그대로 진행
}

void bgm_play(void)
{
    if (!music_on || !load_playsound()) return;
    if (!bgm_wav) bgm_build();
    if (bgm_wav) pPlaySound((LPCSTR)bgm_wav, NULL, SND_MEMORY | SND_ASYNC | SND_LOOP | SND_NODEFAULT);
}

void bgm_stop(void)
{
    if (load_playsound()) pPlaySound(NULL, NULL, 0);
}

// 1) 오프닝: 붕어빵이 헤엄쳐 들어오고 제목이 한 글자씩 나타남
const char* OP_TITLE[6] = { "붕", "어", "빵", "타", "이", "쿤" };
int OP_TX[6] = { 30, 34, 38, 48, 52, 56 };

void draw_opening_text(void)
{
    set_color(C_BROWN);
    gotoxy(24, 15); printf("~ 붕어빵 거리 최고의 가게를 가려라! ~");
    set_color(C_GRAY);
    gotoxy(26, 17); printf("라이벌 '황금붕어'와의 %d일 매출 대결", REG_DAYS);
}

void opening_screen(void)
{
    set_layout(MENU_W, MENU_H);
    clear_screen();
    flush_keys();
    bgm_play();                                  // 오프닝 음악 시작 (메뉴, 설명 화면까지 반복)
    int fx = (MENU_W - FISH_W) / 2;              // 붕어빵 최종 x (가운데)
    for (int x = MENU_W - FISH_W - 2; x >= fx; x -= 2) {   // 붕어빵 등장
        draw_fish(x, 3, C_YEL);
        Sleep(35);
        if (key_ready()) break;                  // 키를 누르면 애니메이션 건너뛰기
    }
    draw_fish(fx, 3, C_YEL);

    int skip = key_ready();
    for (int i = 0; i < 6; i++) {                // 제목 한 글자씩
        set_color(i < 3 ? C_YEL : C_WHITE);
        gotoxy(OP_TX[i], 13);
        printf("%s", OP_TITLE[i]);
        if (!skip) Sleep(140);                   // 음악과 겹치지 않게 Beep 대신 잠깐 쉬기
    }
    draw_opening_text();

    flush_keys();
    int frame = 0;
    while (1) {                                  // 깜빡이는 안내 문구
        int k = any_key();
        if (k == 'm') {                          // 오프닝 화면에서도 M으로 음악 켜기 / 끄기
            music_on = !music_on;
            if (music_on) bgm_play(); else bgm_stop();
        }
        else if (k != -1) break;                 // 그 밖의 아무 키 -> 메뉴로
        set_color(C_GRAY);
        gotoxy(30, 24); printf("M 음악 %s", music_on ? "끄기 (현재: 켜짐)" : "켜기 (현재: 꺼짐)");
        if (window_resized()) {                  // 창 크기가 바뀌면 가운데로 다시 그리기
            clear_screen();
            draw_fish(fx, 3, C_YEL);
            for (int i = 0; i < 6; i++) {
                set_color(i < 3 ? C_YEL : C_WHITE);
                gotoxy(OP_TX[i], 13);
                printf("%s", OP_TITLE[i]);
            }
            draw_opening_text();
        }
        draw_fish_face(fx, 3, frame % 40 < 2);   // 약 3초마다 눈 깜빡
        gotoxy(30, 22);
        if ((frame / 6) % 2 == 0) { set_color(C_WHITE); printf("- 아무 키나 누르세요 -"); }
        else                      { printf("                      "); }
        int steam = (frame / 3) % 3;             // 붕어빵 위 김
        set_color(C_GRAY);
        gotoxy(fx + 26, 2); printf("%s", steam == 0 ? " ~  ~ " : (steam == 1 ? "  ~  ~" : "~  ~  "));
        frame++;
        Sleep(80);
    }
    flush_keys();
    set_color(C_NORM);
}

// 2) 메뉴: 0 = 게임 시작, 1 = 게임 설명, 2 = 이어하기 (저장 파일이 있을 때만)
int menu_screen(void)
{
    int sd = 0, sdf = 1;
    int hasSave = save_peek(&sd, &sdf);
    const int n = 3;
    const char* item[3] = { "게임 시작", "이어하기", "게임 설명" };
    int code[3] = { 0, 2, 1 };
    int cur = hasSave ? 1 : 0;                   // 저장이 있으면 '이어하기'에서 시작
    flush_keys();
    while (1) {
        clear_screen();
        draw_fish((MENU_W - FISH_W) / 2, 2, C_YEL);
        draw_title_logo(12);

        for (int i = 0; i < n; i++) {
            int off = (i == 1 && !hasSave);      // 저장이 없으면 '이어하기'는 회색
            gotoxy(36, 16 + i);
            if (i == cur) { set_color((off ? C_GRAY : C_YEL) | BACKGROUND_BLUE); printf(" >> %s << ", item[i]); }
            else          { set_color(off ? C_GRAY : C_NORM);                  printf("    %s    ", item[i]); }
        }
        if (hasSave) {                           // 저장된 게임 정보
            set_color(C_GREEN);
            gotoxy(26, 21); printf("저장된 게임: %d일차부터 / 난이도 %s", sd, diff_table[sdf].name);
        } else {
            set_color(C_GRAY);
            gotoxy(18, 21); printf("저장된 게임 없음 (하루 장사 후 상점에서 나오면 자동 저장)");
        }
        // 난이도 (← → 로 변경)
        gotoxy(32, 20); set_color(C_NORM); printf("난이도 :  ");
        set_color(diff_table[diff].color); printf("< %-6s >", diff_table[diff].name);
        set_color(C_GRAY);
        gotoxy(22, 22); printf("↑↓ 선택   ← → 난이도   ENTER 결정   ESC 종료");
        gotoxy(30, 23); printf("M 음악 %s", music_on ? "끄기 (현재: 켜짐)" : "켜기 (현재: 꺼짐)");

        // 표 확인 (전광판 박스문자 폭)
        set_color(C_NORM);
        gotoxy(8, 24); printf("[표 확인] 아래 상자 모양 그대로 전광판이 그려집니다");
        {
            int tw[2] = { 4, 4 };
            hline(8, 25, tw, 2, B_TL, B_TM, B_TR);
            vline(8, 26, tw, 2);
            hline(8, 27, tw, 2, B_BL, B_BM, B_BR);
        }
        set_color(C_GRAY);
        gotoxy(22, 26); printf("B 키: 선 모양 바꾸기 (현재: %s)", STYLE_NAME[box_style]);
        set_color(C_NORM);

        int k = wait_key_live();
        if (k == K_UP)   { cur = (cur + n - 1) % n; Beep(900, 20); }
        if (k == K_DOWN) { cur = (cur + 1) % n;     Beep(900, 20); }
        if (k == K_LEFT  && diff > 0) { diff--; Beep(700, 30); }
        if (k == K_RIGHT && diff < 2) { diff++; Beep(1100, 30); }
        if (k >= '1' && k < '1' + n) cur = k - '1';
        if (k == 'b') set_box_style((box_style + 1) % 3);
        if (k == 'm') {                          // 음악 켜기 / 끄기
            music_on = !music_on;
            if (music_on) bgm_play(); else bgm_stop();
        }
        if (k == 13) {
            if (code[cur] == 2 && !hasSave) { sfx(S_ERR); continue; }   // 저장이 없으면 못 고름
            Beep(1300, 40); return code[cur];
        }
        if (k == 27) exit(0);
    }
}

// 설명 페이지 출력 도우미
int help_y;
void hp(int color, const char* fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    set_color(color);
    gotoxy(6, help_y++);
    printf("%s", buf);
}

void help_page(int page)
{
    help_y = 5;
    switch (page) {
    case 0:
        hp(C_YEL,   "[ 게임 목표 ]");
        help_y++;
        hp(C_NORM,  "붕어빵 거리에 라이벌 가게 '황금붕어'가 생겼습니다!");
        hp(C_NORM,  "%d일 동안 장사해서 총매출이 더 많은 가게가 승리합니다.", REG_DAYS);
        help_y++;
        hp(C_WHITE, "- 매일 라이벌이 먼저 장사하고, 그다음 내가 장사합니다.");
        hp(C_WHITE, "- %d일 장사를 둘 다 모두 마친 뒤 총매출로 승패를 정합니다.", REG_DAYS);
        hp(C_WHITE, "- 총매출이 같으면 %d~%d일차 연장 영업을 모두 마친 뒤 다시 판정!", REG_DAYS + 1, MAX_DAYS);
        hp(C_WHITE, "- %d일까지도 같으면 무승부입니다.", MAX_DAYS);
        hp(C_WHITE, "- 메뉴에서 ← → 로 난이도 선택 (쉬움 / 보통 / 어려움)");
        hp(C_WHITE, "- 상점에서 나올 때마다 자동 저장 -> 메뉴의 '이어하기'로 계속");
        help_y++;
        hp(C_YEL,   "[ 전광판 보는 법 ]");
        hp(C_NORM,  "  1~%d 칸  : 그날 매출 (단위: 천원)", MAX_DAYS);
        hp(C_NORM,  "  매출     : 총매출        손님 : 받은 손님 수");
        hp(C_NORM,  "  실수     : 탄 붕어빵 + 화나서 돌아간 손님");
        hp(C_NORM,  "  단골     : 빨리 받아서 팁을 준 손님");
        break;
    case 1:
        hp(C_YEL,   "[ 하루 흐름 ]");
        help_y++;
        hp(C_RED,   "1. 라이벌 영업    라이벌 '황금붕어'의 그날 매출이 먼저 공개됩니다.");
        hp(C_WHITE, "2. 아침 준비      ← → 로 붕어빵 가격을 정합니다 (500원 ~ 1,500원).");
        hp(C_GRAY,  "                  비싸면 손님이 줄고, 싸면 많이 와도 남는 게 적어요.");
        hp(C_GREEN, "3. 영업 (45초)    붕어빵을 구워서 손님에게 팝니다.");
        hp(C_YEL,   "4. 마감 정산      그날 매출이 전광판에 기록됩니다.");
        hp(C_GRAY,  "                  팔지 못한 붕어빵은 식어서 버려요.");
        hp(C_CYAN,  "5. 상점           번 돈으로 가게를 업그레이드합니다.");
        help_y++;
        hp(C_NORM,  "재료비: 틀 1번 굽기(2마리)마다 400원이 보유금에서 빠져요.");
        break;
    case 2:
        hp(C_YEL,   "[ 붕어빵 굽는 법 ]");
        help_y++;
        hp(C_WHITE, "  빈 틀 --SPACE--> 반죽 --1 또는 2--> 앞면 굽기");
        hp(C_WHITE, "        --SPACE(뒤집기)--> 뒷면 굽기 --SPACE(꺼내기)--> 진열대에 2마리!");
        help_y++;
        hp(C_GREEN, "  ######......   초록색 : 굽는 중 (아직 뒤집거나 꺼낼 수 없어요)");
        hp(C_YEL,   "  ############   노란색 : 다 익음! 지금 SPACE");
        hp(C_RED,   "  ############   빨간색으로 깜빡임 : 곧 타요! 서두르세요");
        hp(C_RED,   "  XXXXXXXXXXXX   탐 : 실수 +1, SPACE로 버리고 다시 굽기");
        help_y++;
        hp(C_YEL,   "[ 조작키 ]");
        hp(C_CYAN,  "  ← →       붕어빵 틀 선택");
        hp(C_CYAN,  "  SPACE     반죽 붓기 / 뒤집기 / 꺼내기 / 탄 것 버리기");
        hp(C_CYAN,  "  1 / 2     속 넣기 (1 팥, 2 슈크림)");
        hp(C_CYAN,  "  Q W E R   1~4번 자리 손님에게 판매");
        hp(C_GRAY,  "  * 한/영 키가 영문 상태여야 Q W E R가 눌려요");
        break;
    case 3:
        hp(C_YEL,   "[ 손님 ]");
        help_y++;
        hp(C_NORM,  "  [Q] 팥2 슈1      <- 자리 키와 주문 (팥 2마리, 슈크림 1마리)");
        hp(C_NORM,  "     (^_^)");
        hp(C_NORM,  "     /|\\         진열대에 주문한 만큼 있을 때 Q를 누르면 판매!");
        hp(C_NORM,  "     / \\");
        hp(C_GREEN, "  [==========]     인내심 게이지: 초록 -> 노랑 -> 빨강");
        help_y++;
        hp(C_GREEN, "  게이지가 60% 넘게 남았을 때 주면 단골 등록 + 팁 500원!");
        hp(C_RED,   "  게이지가 다 닳으면 화가 나서 돌아가요 (실수 +1)");
        hp(C_GRAY,  "  날이 갈수록 손님 성격이 급해져요.");
        help_y++;
        hp(C_YEL,   "[ 진열대 ]  화면 오른쪽");
        hp(C_NORM,  "  다 구운 붕어빵이 쌓여요. 왼쪽 칸 = 팥, 오른쪽 칸 = 슈크림");
        hp(C_NORM,  "  손님 주문과 진열대 개수를 비교해서 판매하세요.");
        break;
    case 4:
        hp(C_YEL,   "[ 날씨 ]  두 가게 모두 같은 날씨의 영향을 받아요");
        hp(C_YEL,   "  맑음  평범한 하루");
        hp(C_GRAY,  "  흐림  손님이 조금 적어요");
        hp(C_BLUE,  "  비    손님이 많이 줄어요");
        hp(C_CYAN,  "  한파  붕어빵이 잘 팔리는 날!");
        hp(C_PINK,  "  축제  손님 폭주! 대신 성격이 급해요");
        help_y++;
        hp(C_YEL,   "[ 상점 ]  하루 장사가 끝나면 열려요");
        hp(C_NORM,  "  붕어빵 틀 추가  30,000원   한 번에 더 많이 구워요 (최대 4개)");
        hp(C_NORM,  "  간판 교체       25,000원   손님 +25%%");
        hp(C_NORM,  "  알림 벨         10,000원   다 익으면 벨이 울려요");
        hp(C_NORM,  "  대기 의자       20,000원   손님 인내심 +30%%");
        help_y++;
        hp(C_GREEN, "  TIP: 처음엔 틀 2개로 시작해요. 틀을 먼저 늘리는 게 유리해요!");
        break;
    }
}

// 3) 게임 설명: 1 = 다 읽고 게임 시작, 0 = 메뉴로
int help_screen(void)
{
    const int PAGES = 5;
    int page = 0;
    flush_keys();
    while (1) {
        clear_screen();
        set_color(C_BROWN);
        gotoxy(3, 2);  printf("==================================================================================");
        set_color(C_YEL);
        gotoxy(6, 3);  printf("게임 설명");
        set_color(C_GRAY);
        gotoxy(70, 3); printf("( %d / %d )", page + 1, PAGES);
        set_color(C_BROWN);
        gotoxy(3, 4);  printf("==================================================================================");

        help_page(page);

        set_color(C_BROWN);
        gotoxy(3, 25); printf("==================================================================================");
        gotoxy(6, 26);
        if (page < PAGES - 1) {
            set_color(C_WHITE); printf("→ / ENTER  다음 페이지");
            set_color(C_GRAY);  printf("      ← 이전 페이지      ESC 메뉴로");
        } else {
            set_color(C_YEL | BACKGROUND_BLUE); printf(" ENTER  게임 시작! ");
            set_color(C_GRAY); printf("      ← 이전 페이지      ESC 메뉴로");
        }
        set_color(C_NORM);

        int k = wait_key_live();
        if ((k == K_RIGHT || k == 13) && page < PAGES - 1) { page++; Beep(900, 20); }
        else if (k == 13 && page == PAGES - 1) { Beep(1300, 40); return 1; }
        else if (k == K_LEFT && page > 0) { page--; Beep(700, 20); }
        else if (k == 27) return 0;
    }
}

// 오프닝 -> 메뉴 -> (게임 설명) -> 게임 시작
// 반환: 0 = 새 게임, 2 = 이어하기
int title_screen(void)
{
    opening_screen();
    while (1) {
        int choice = menu_screen();
        if (choice == 2) { bgm_stop(); return 2; }        // 이어하기
        if (choice == 0) { bgm_stop(); return 0; }        // 게임 시작 -> 음악 끄고 바로 시작
        if (help_screen() == 1) { bgm_stop(); return 0; } // 설명을 다 읽고 ENTER -> 시작
        // ESC를 누르면 메뉴로 돌아감
    }
}

// 5일차까지 동점일 때 연장 영업 안내
void overtime_notice(int score[])
{
    cv_clear();
    cv_printf(2, 1, C_PINK,  "** 연장 영업! **");
    cv_printf(2, 3, C_WHITE, "%d일 동안의 총매출이 %d천원으로 똑같습니다.", REG_DAYS, score[1]);
    cv_printf(2, 4, C_WHITE, "%d~%d일차 연장 영업을 모두 마친 뒤 다시 승패를 정합니다.", REG_DAYS + 1, MAX_DAYS);
    sfx(S_ENTER);
    wait_key_msg(7, "아무 키나 누르면 상점으로 이동합니다...");
}

// 어느 PC, 어느 실행 방식에서도 같은 콘솔 환경을 만듦
void setup_console(void)
{
    // 콘솔 창이 아닌 곳(VS Code 디버그 콘솔 등)에서 실행되면 새 콘솔 창을 염
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        FreeConsole();
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONIN$", "r", stdin);
    }
    hIn = GetStdHandle(STD_INPUT_HANDLE);
    InitializeCriticalSection(&sfx_lock);   // 효과음 순서대로 재생용
    sfx_lock_ready = 1;

    SetConsoleOutputCP(949);            // 한글 출력 코드 페이지 (문자열은 to_cp949로 맞춤)
    SetConsoleCP(949);

    // 빠른 편집 모드 끄기: 켜져 있으면 창을 클릭하는 순간 게임이 멈춤
    // 줄 단위 입력 / 화면에 입력 글자 표시도 끔 (키를 바로 읽기 위해)
    SetConsoleMode(hIn, ENABLE_EXTENDED_FLAGS | ENABLE_PROCESSED_INPUT);

    char title[64];
    to_cp949("붕어빵 타이쿤", title, sizeof(title));
    SetConsoleTitleA(title);
}

int main(void)
{
    int score[2] = { 0 }, hit[2] = { 0 }, error[2] = { 0 }, ball[2] = { 0 };

    srand((unsigned)time(NULL));
    setup_console();
    hide_cursor();

    memset(day_sales, -1, sizeof(day_sales));
    int start = 1;                                 // 오늘 시작할 일차
    if (title_screen() == 2) start = load_game(score, hit, error, ball);
    if (start <= 1) {                              // 새 게임 (또는 저장 파일이 깨졌을 때)
        start = 1;
        money = diff_table[diff].start_money;      // 난이도별 시작 보유금
        remove(SAVE_FILE);                         // 예전 저장은 지움
    }
    set_layout(GAME_W, GAME_H);   // 여기부터는 게임 화면 크기 기준으로 가운데 정렬
    clear_screen();
    display_scoreboard(score, hit, error, ball);   // 이어하기면 저장된 기록까지 다시 그림
    if (start > 1) resume_notice(start);

    // 1) 정규 영업 1~3일: 둘 다 3일차까지 모두 끝낸 뒤 총매출로 판정
    if (start <= REG_DAYS) {
        control_scoreboard(start, REG_DAYS, score, hit, error, ball);
        if (score[0] != score[1]) {
            game_over(REG_DAYS, score, hit, error, ball);      // 승 또는 패
            return 0;
        }
        // 동점이면 연장 안내 -> 상점 -> 저장
        overtime_notice(score);
        shop();
        save_game(REG_DAYS + 1, score, hit, error, ball);
        start = REG_DAYS + 1;
    }

    // 2) 연장 영업 4~5일: 둘 다 5일차까지 모두 끝낸 뒤 다시 판정
    control_scoreboard(start, MAX_DAYS, score, hit, error, ball);
    game_over(MAX_DAYS, score, hit, error, ball);          // 승 / 패 / 무승부
    return 0;
}
