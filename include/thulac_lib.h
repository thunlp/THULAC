#ifndef _THULAC_LIB_H
#define _THULAC_LIB_H

#ifdef __cplusplus
extern "C" {
#endif
int init(const char* model, const char* dict, int ret_size, int t2s, int just_seg);
void deinit();
const char *getResult();
int seg(const char *in);

#ifdef __cplusplus
}
#endif
#endif
