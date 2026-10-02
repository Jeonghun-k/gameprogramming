#include <math.h>

int calc_frequency(int octave, int inx)
{
    double do_scale = 32.7032;
    double ratio = pow(2., 1 / 12.), temp;
    int i;
    temp = do_scale * pow(2, octave - 1);
    for (i = 0; i < inx; i++)
    {
        temp = (int)(temp + 0.5);
        temp *= ratio;
    }
    return (int)temp;
}
