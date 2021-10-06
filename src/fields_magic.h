#ifndef FIELDS_MAGIC_H
#define FIELDS_MAGIC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct formdata {
	char name[16];
	char data[64];
};

int buildForm(struct formdata* _formdata, uint8_t _numfields);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif /* #ifndef FIELDS_MAGIC_H */
