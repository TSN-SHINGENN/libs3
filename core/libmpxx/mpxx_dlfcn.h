#ifndef INC_MPXX_DLFCN_H
#define INC_MPXX_DLFCN_H

typedef struct {
    void *ext;
} mpxx_dlfcn_t;

typedef union {
    unsigned int flags;
} mpxx_dlfcn_attr_t;

#if defined(__cplusplus)
extern "C" {
#endif

int mpxx_dlfcn_open(mpxx_dlfcn_t *self_p, const char *filename, mpxx_dlfcn_attr_t *attr_p);
void *mpxx_dlfcn_sym(mpxx_dlfcn_t *self_p, const char *symbol);
int mpxx_dlfcn_close(mpxx_dlfcn_t *self_p);

#if defined(__cplusplus)
}
#endif

#endif /* end of INC_MPXX_DLFCN_H */
