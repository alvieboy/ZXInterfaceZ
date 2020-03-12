#ifndef __NETCOMMS_H__
#define __NETCOMMS_H__


void netcomms__send_ok(int sock);
void netcomms__send_error(int sock, const char *error);
void netcomms__send_progress(int sock, int total, int current);


#endif

