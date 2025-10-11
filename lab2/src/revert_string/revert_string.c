#include "revert_string.h"
#include <string.h>

void RevertString(char *str) {
    int left = 0;
    int right = strlen(str) - 1;
    char temp;
    while (left < right) {
        temp = str[left];
        str[left] = str[right];
        str[right] = temp;
        left++;
        right--;
    }
}


