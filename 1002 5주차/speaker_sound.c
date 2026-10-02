#include <stdio.h>
#include <math.h>
#include <windows.h>

int calc_frequency(int octave, int inx);

int main(void)
{
    int index[] = {0, 2, 4, 5, 7, 9, 11, 12};
    int freq[8];
    int i;
    for (i = 0; i < 8; i++)
        freq[i] = calc_frequency(4, index[i]);
    for (i = 0; i <= 7; i++)
        Beep(freq[i], 500);
    Sleep(1000); // 음간의 지연시간 1초
    for (i = 7; i >= 0; i--)
        Beep(freq[i], 500);
    return 0;
}

int calc_frequency(int octave, int inx)
{

}
