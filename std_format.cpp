#include "std_format.h"
#include <math.h>

void std_itoa(int n, int precision, char *buf) {
    if (n < 0) {
        *buf++ = '-';
        n = -n;
    }
  
    int i = 0, j = 0;
    do {
        buf[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);
    while (i < precision) buf[i++] = '0';
    buf[i] = '\0';
  
    // reverse
    char tmp;
    for (j = i-1, i = 0; i < j; i++, j--) {
        tmp = buf[i];
        buf[i] = buf[j];
        buf[j] = tmp;
    }
}

void std_ftoa(float f, int precision, char *buf) {
    if (f < 0) {
        *buf++ = '-';
        f = -f;
    }

    std_itoa((int) f, 0, buf);  // convert integer part to string
  
    while (*buf != '\0') buf++;
    *buf++ = '.';

    float d = fmod(f, 1);  // extract floating part
    if (precision < 1) {
        d = 0;
    } else {
        d *= pow(10, precision);  // convert floating part to string
    
        // round floating part
        float dx = fmod(d, 1);
        if (dx < 0.5) {
          d = floor(d);
        } else {
          d = ceilf(d);
        }
    }

    std_itoa((int) d, precision, buf);  // convert floating part to string
}