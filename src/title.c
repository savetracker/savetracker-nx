#include "title.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <switch.h>

bool title_init(void) {
    if (R_FAILED(pmdmntInitialize())) return false;

    if (R_FAILED(pminfoInitialize())) {
        pmdmntExit();
        return false;
    }

    if (R_FAILED(nsInitialize())) {
        pminfoExit();
        pmdmntExit();
        return false;
    }

    return true;
}

void title_exit(void) {
    nsExit();
    pminfoExit();
    pmdmntExit();
}

bool title_get_current(uint64_t *out_title_id) {
    if (!out_title_id) return false;

    u64 pid = 0;
    if (R_FAILED(pmdmntGetApplicationProcessId(&pid))) return false;

    u64 program_id = 0;
    if (R_FAILED(pminfoGetProgramId(&program_id, pid))) return false;

    *out_title_id = (uint64_t)program_id;
    return true;
}

static const NacpLanguageEntry *pick_language_entry(const NacpStruct *nacp) {
    for (int i = 0; i < 16; i++) {
        if (nacp->lang[i].name[0] != '\0') return &nacp->lang[i];
    }
    return NULL;
}

static void sanitize_name(const char *in, size_t in_len, char *out, size_t out_cap) {
    if (out_cap == 0) return;

    size_t       written     = 0;
    const size_t max_written = (out_cap - 1 < TITLE_NAME_MAX) ? out_cap - 1 : TITLE_NAME_MAX;

    for (size_t i = 0; i < in_len && written < max_written; i++) {
        const unsigned char c = (unsigned char)in[i];

        if (c == '\0') break;

        if (c < 0x20 || c == 0x7F) {
            out[written++] = '_';
            continue;
        }

        switch (c) {
        case '/':
        case '\\':
        case ':':
        case '*':
        case '?':
        case '"':
        case '<':
        case '>':
        case '|':
            out[written++] = '_';
            continue;
        default:
            out[written++] = (char)c;
        }
    }

    while (written > 0 && (out[written - 1] == ' ' || out[written - 1] == '.')) written--;

    out[written] = '\0';
}

bool title_get_name(uint64_t title_id, char *out, size_t cap) {
    if (!out || cap == 0) return false;

    out[0] = '\0';

    // ~40 KB — reuse across calls instead of malloc/free per title transition
    static NsApplicationControlData s_ctrl;
    memset(&s_ctrl, 0, sizeof(s_ctrl));

    u64          actual_size = 0;
    const Result rc          = nsGetApplicationControlData(
                 NsApplicationControlSource_Storage, (u64)title_id, &s_ctrl, sizeof(s_ctrl), &actual_size);
    if (R_FAILED(rc) || actual_size < sizeof(s_ctrl.nacp)) return true;

    const NacpLanguageEntry *entry = pick_language_entry(&s_ctrl.nacp);
    if (entry) sanitize_name(entry->name, sizeof(entry->name), out, cap);

    return true;
}

bool title_build_dir_name(uint64_t title_id, const char *name, char *out, size_t cap) {
    if (!out || cap == 0) return false;

    int n;
    if (name && name[0] != '\0') {
        n = snprintf(out, cap, "%016lX_%s", (unsigned long)title_id, name);
    } else {
        n = snprintf(out, cap, "%016lX", (unsigned long)title_id);
    }
    return n > 0 && (size_t)n < cap;
}
