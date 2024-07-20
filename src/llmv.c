#include <string.h>

#include "llmv.h"

int llmv_write_raw(llmv_writer *w, const void* pv, size_t size) {
    if (w->err) {
        return 1;
    }

    size_t cur = w->len;
    w->len += size; // we are going to ignore the possibility of overflow...
    if (w->len > w->cap) {
        w->err = 1;
        return 1;
    }

    memcpy(&w->buf[cur], pv, size);
    return 0;
}

#define llmv_write(w, v) \
    llmv_write_raw(w, &v, sizeof(v))

int llmv_write_ptr(llmv_writer *w, const void* p) {
    uint64_t v = (uintptr_t)p;
    return llmv_write(w, v);
}

int llmv_write_size(llmv_writer *w, size_t sz) {
    uint64_t v = sz;
    return llmv_write(w, v);
}

int llmv_write_flag(llmv_writer *w, llmv_flag flag) {
    uint8_t f = flag;
    return llmv_write(w, f);
}

int llmv_write_string(llmv_writer* w, const char* str) {
    return llmv_write_raw(w, str, strlen(str) + 1);
}

llmv_writer llmv_new_writer(unsigned char* buf, size_t size) {
    return (llmv_writer){
        .buf = buf,
        .cap = size,
    };
}

int llmv_start(llmv_writer* w, const char* kind, const void* addr, size_t size) {
    llmv_write_flag(w, LLMV_START);
    llmv_write_string(w, kind);
    llmv_write_ptr(w, addr);
    llmv_write_size(w, size);
    return w->err;
}

int llmv_end(llmv_writer* w) {
    llmv_write_flag(w, LLMV_END);
    return w->err;
}

int llmv_field(llmv_writer* w, const char* name, const void* addr, size_t size) {
    llmv_write_flag(w, LLMV_FIELD);
    llmv_write_ptr(w, addr);
    llmv_write_size(w, size);
    llmv_write_string(w, name);
    return w->err;
}
