/*
 * Copyright (C) Switch-OC-Suite
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef ATMOSPHERE_IS_STRATOSPHERE
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace ams::ldr::oc {
    namespace pcv {
        void Patch(uintptr_t mapped_nso, size_t nso_size);
    }

    namespace ptm {
        void Patch(uintptr_t mapped_nso, size_t nso_size);
    }
}

static void* ReadFile(const char* file_loc, long* out_size) {
    FILE* fp;
    void* buf;
    long size;

    fp = fopen(file_loc, "r");
    if (!fp) {
        fprintf(stderr, "Cannot open file: %s\n", file_loc);
        exit(-1);
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf = malloc((size + 1) * sizeof(char));
    fread(buf, sizeof(char), size, fp);
    fclose(fp);
    if (size < 8192) {
        fprintf(stderr, "File is too small to process: %u Bytes\n", size);
        exit(-1);
    }

    *out_size = size;
    return buf;
}

int main(int argc, char** argv) {
    const char* pcv_opt = "pcv";
    const char* ptm_opt = "ptm";
    enum {
        EXE_PCV,
        EXE_PTM,
        UNKNOWN
    };
    int option = UNKNOWN;
    if (argc == 3) {
        if (!strcmp(argv[1], pcv_opt))
            option = EXE_PCV;

        if (!strcmp(argv[1], ptm_opt))
            option = EXE_PTM;
    }
    if (option == UNKNOWN) {
        fprintf(stderr, "Usage: %s %s/%s <executable>\n", argv[0], pcv_opt, ptm_opt);
        return -1;
    }

    long file_size;
    void* file_buffer = ReadFile(argv[2], &file_size);
    uintptr_t mapped_exe = reinterpret_cast<uintptr_t>(file_buffer);
    size_t exe_size = reinterpret_cast<size_t>(file_size * sizeof(char));
    switch (option) {
        case EXE_PCV:
            printf("Patching %s...\n", pcv_opt);
            ams::ldr::oc::pcv::Patch(mapped_exe, exe_size);
            break;
        case EXE_PTM:
            printf("Patching %s...\n", ptm_opt);
            ams::ldr::oc::ptm::Patch(mapped_exe, exe_size);
            break;
    }
    free(file_buffer);
    printf("Passed!\n\n");
    return 0;
}
#endif
