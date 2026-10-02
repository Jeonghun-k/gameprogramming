#include <stdio.h>
#include <math.h>

void print_frequency(int octave)
{
    double do_scale = 32.7032;
    double ratio = pow(2., 1 / 12.), temp;
    int i;
    temp = do_scale * pow(2, octave - 1);
    printf("%d옥타브 : ", octave);
    for (i = 0; i < 12; i++)
    {
        printf("%4ld ", (unsigned long)(temp + 0.5));
        temp *= ratio;
    }
    printf("\n");
}
