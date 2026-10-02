#include <stdio.h>
#include <math.h>
#include <windows.h>

#define DO 0
#define RE 2
#define MI 4
#define FA 5
#define SOL 7
#define LA 9
#define SI 11
#define DO2 12

#define UNIT 300   /* 1박 = 300ms */
#define GAP  50    /* 음 사이 간격 */

int calc_frequency(int octave, int inx);
void play_school_bell(void);

int main(void)
{
    SetConsoleOutputCP(65001);   /* UTF-8 소스에서 한글 깨짐 방지 */
    printf("학교종이 땡땡땡\n");
    play_school_bell();
    return 0;
}

void play_school_bell(void)
{
    int song[] = {
        SOL, SOL, LA, LA, SOL, SOL, MI,
        SOL, SOL, MI, MI, RE,
        SOL, SOL, LA, LA, SOL, SOL, MI,
        SOL, MI, RE, MI, DO};
    int beat[] = {
        1, 1, 1, 1, 1, 1, 2,
        1, 1, 1, 1, 3,
        1, 1, 1, 1, 1, 1, 2,
        1, 1, 1, 1, 3};
    int note_count = sizeof(song) / sizeof(song[0]);
    int i;

    for (i = 0; i < note_count; i++)
    {
        Beep(calc_frequency(4, song[i]), UNIT * beat[i]);
        Sleep(GAP);
    }
}

int calc_frequency(int octave, int inx)
{
    double base = 32.7032 * pow(2, octave - 1);    /* 해당 옥타브의 도 */
    return (int)(base * pow(2, inx / 12.0) + 0.5); /* 반음 inx개 위, 반올림 */
}