#ifndef __ROM_H__
#define __ROM_H__

#define ROM_SIZE 8192

int rom__load_from_flash(void);
int rom__load_custom_from_file(const char *);

#endif