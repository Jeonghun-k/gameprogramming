#include <conio.h>
#include <windows.h>

int calc_frequency(int octave, int inx);

void practice_piano(void)
{
    int index[] = {0, 2, 4, 5, 7, 9, 11, 12};
    int freq[8], code, i;
    for (i = 0; i < 8; i++)
        freq[i] = calc_frequency(4, index[i]);
    do
    {
        code = getch();
        if ('1' <= code && code <= '8')
        {
            code -= 49;
            Beep(freq[code], 300);
        }
    } while (code != 27);
}
