/* rice - A FFI for vim */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <ctype.h>

enum {
    ARG_TYPE_UNK = 0,
    ARG_TYPE_INT,
    ARG_TYPE_STR,
    ARG_TYPE_MAX,
};

typedef struct arg_t {
    int type;
    struct { 
        int32_t i;
        char str[256];
    };
} arg_t;

static
void *
arg_get_ptr (const arg_t *arg) {
    if (arg) {
        switch (arg->type) {
            case ARG_TYPE_STR: return (void *)arg->str;
            case ARG_TYPE_INT: return (void *)arg->i;
        }
    }
    return NULL;
}

static
char *
read_int (char *in, int32_t *val) {
    char *p = in;
    int32_t v;
    int neg;

    if (*p++ != 'i')
        return NULL;

    v = 0;
    neg = 0;

    if (*p == '-') {
        neg = 1;
        p++;
    }

    while (isdigit(*p)) {
        v *= 10;
        v += *p++ - '0';
    }

    if (neg)
        v = -v;

    if (*p++ != 'e')
        return NULL;

    *val = v;

    return p;
}

static
char *
read_str (char *in, char *val, const int N) {
    char *p = in;
    int len;

    len = 0;

    while (isdigit(*p)) {
        len *= 10;
        len += *p++ - '0';
    }

    if (*p++ != ':')
        return NULL;

    if (len > N)
        len = N;

    if (len)
        memcpy(val, p, len);
    val[len] = '\0';

    return p + len;
}

static
char *
fmt_resp (const char fmt, void *r) {
    static char buf[256];

    switch (fmt) {
        case 'V':
            snprintf(buf, sizeof(buf), "v");
            break;
        case 'I':
            snprintf(buf, sizeof(buf), "i%lde", (int32_t)r);
            break;
        case 'S':
            snprintf(buf, sizeof(buf), "%ld:%s", strlen(r), r);
            break;
        case 'Q':
            snprintf(buf, sizeof(buf), "x%lxz", (int32_t)r);
            break;
    }

    return buf;
}

typedef void *(* ptr_0)();
typedef void *(* ptr_1)(void *);
typedef void *(* ptr_2)(void *, void *);
typedef void *(* ptr_3)(void *, void *, void *);

char *
rptr (char *in) {
    static char buf[256];
    int i;
    int32_t val;
    int32_t len;
    char *p = in;

    p = read_int(p, &len);
    if (!p) {
        printf("rptr: Parse error\n");
        return NULL;
    } 

    p = read_int(p, &val);
    if (!p) {
        printf("rptr: Parse error\n");
        return NULL;
    } 

    printf("Val : %x\n", val);

    buf[0] = '\0';

    // NULL pointer check
    if (val && len) {
        unsigned char *p = (unsigned char *)val;

        if (len > 0)
            // Hexdump 'len' bytes
            for (i = 0; i < len; i++)
                snprintf(buf + (2 * i), sizeof(buf) - (2 * i), "%02X", p[i]);
        else {
            // Read until NULL (or we run out of space)
            for (i = 0; p[i] && i < sizeof(buf); i++)
                buf[i] = p[i];
            buf[i] = '\0';
        }
    } 

    return buf;
}

char *
call (char *in) {
    char *c, *p, ret_fmt;
    char *lib_path, *fun_name;
    void *h, *sym, *ret;
    int params, n;

    c = in;

    // Library path
    p = strchr(c, ';');
    if (!p)
        return NULL;
    *p = '\0';
    lib_path = c;
    c = p + 1;

    // Function name
    p = strchr(c, ';');
    if (!p)
        return NULL;
    *p = '\0';
    fun_name = c;
    c = p + 1;

    // Return type 
    ret_fmt = *c++;

    if (*c++ != ';')
        return NULL;

    // Number of parameters passed
    params = *c++ - '0';

    if (*c++ != ';')
        return NULL;

    // Start parsing the arguments
    arg_t a_list[params];
    n = 0;

    if (params) {
        while (*c) {
            if (n >= params) {
                printf("Too many params specified\n");
                return NULL;
            }

            arg_t *a = &a_list[n++];

            switch (*c) {
                case 'i':
                    a->type = ARG_TYPE_INT;
                    c = read_int(c, &a->i);
                    break;
                default:
                    a->type = ARG_TYPE_STR;
                    c = read_str(c, a->str, 255);
                    break;
            }

            if (!c) {
                printf("Parse error!\n");
                return NULL;
            }
        }
    }

    if (n != params) {
        printf("Incomplete parameter list\n");
        return NULL;
    }

    // Do the heavy lifting
    h = dlopen(lib_path, RTLD_LAZY);
    if (!h) {
        printf("dlopen() failed\n");
        return NULL;
    }
    sym = dlsym(h, fun_name);

    ret = NULL;

    // Staircase
    ptr_0 p0 = (ptr_0)sym;
    ptr_1 p1 = (ptr_1)sym;
    ptr_2 p2 = (ptr_2)sym;
    ptr_3 p3 = (ptr_3)sym;

    printf("Calling %s with %i params from lib %s\n", fun_name, params, lib_path);

    switch (params) {
        case 0: ret = p0(); break;
        case 1: ret = p1(arg_get_ptr(&a_list[0])); break;
        case 2: ret = p2(arg_get_ptr(&a_list[0]), arg_get_ptr(&a_list[1])); break;
        case 3: ret = p3(arg_get_ptr(&a_list[0]), arg_get_ptr(&a_list[1]), arg_get_ptr(&a_list[2])); break;
        default: printf("Too many arguments!\n"); break;
    }

    dlclose(h);

    return fmt_resp(ret_fmt, ret);
}
