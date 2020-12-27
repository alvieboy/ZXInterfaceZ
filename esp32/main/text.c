#include "text.h"
#include <string.h>

void chomp(char *line)
{
    int l = strlen(line);

    if (l==0)
        return;

    char *endl = &line[l-1];

    do {
        if ((*endl)=='\r' || (*endl)=='\n') {
            (*endl)='\0';
            endl--;
        } else {
            break;
        }
        if (endl==line)
            break;
    } while (1);
}
