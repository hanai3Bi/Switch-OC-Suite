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
#include "oc_suite_test.hpp"
#include "oc_suite.hpp"

void* loadExec(const char* file_loc, size_t* out_size) {
    FILE* fp = fopen(file_loc, "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open file: \"%s\"\n", file_loc);
        exit(-1);
    }

    if (fseek(fp, 0, SEEK_END) < 0) {
        fprintf(stderr, "fseek error\n");
        exit(-1);
    }

    long size = ftell(fp);
    if (size == -1L) {
        fprintf(stderr, "\"%s\" is a directory", file_loc);
        exit(-1);
    }

    fseek(fp, 0, SEEK_SET);
    void* buf = malloc(size);

    fread(buf, size, 1, fp);
    fclose(fp);

    if (size < 8192) {
        fprintf(stderr, "File is too small to process: \"%s\" (%ld B)\n", file_loc, size);
        exit(-1);
    }

    *out_size = size;
    return buf;
}

void saveExec(const char* file_loc, const void* buf, size_t size) {
    FILE* fp = fopen(file_loc, "wb");
    if (!fp) {
        fprintf(stderr, "Cannot write to \"%s\"\n", file_loc);
        exit(-1);
    }
    printf("Saving to \"%s\"...\n", file_loc);
    fwrite(buf, size, 1, fp);
    fclose(fp);
}

int main(int argc, char** argv) {
    const char* pcv_opt    = "pcv";
    const char* ptm_opt    = "ptm";
    const char* save_opt   = "-s";
    const char* mariko_ext = ".mariko";
    const char* erista_ext = ".erista";
    enum EXE_OPTION {
        EXE_PCV,
        EXE_PTM,
        UNKNOWN
    };
    EXE_OPTION exe_opt = UNKNOWN;
    if (argc > 2) {
        if (!strcmp(argv[1], pcv_opt))
            exe_opt = EXE_PCV;

        if (!strcmp(argv[1], ptm_opt))
            exe_opt = EXE_PTM;
    }
    if ((argc != 3 && argc != 4) || exe_opt == UNKNOWN) {
        fprintf(stderr, "Usage:\n"\
                        "    %s  %s | %s  [%s]  <exec_path>\n\n"\
                        "    %s : Save patched executable with extension \"%s\" / \"%s\"\n"
                        , argv[0], pcv_opt, ptm_opt, save_opt
                        , save_opt, mariko_ext, erista_ext);
        return -1;
    }

    bool save_patched = false;
    char* exec_path = argv[2];
    if (argc == 4 && !strcmp(argv[2], save_opt)) {
        save_patched = true;
        exec_path = argv[3];
    }

    size_t file_size;
    void* file_buffer = loadExec(exec_path, &file_size);

    size_t exec_path_len = strlen(reinterpret_cast<const char *>(exec_path));
    size_t exec_path_patched_len = exec_path_len + std::max(strlen(mariko_ext), strlen(erista_ext)) + 1;

    if (exe_opt == EXE_PCV) {
        ams::ldr::oc::pcv::SafetyCheck();

        {
            void* erista_buf = malloc(file_size);
            std::memcpy(erista_buf, file_buffer, file_size);

            printf("Patching %s for Erista...\n", pcv_opt);
            ams::ldr::oc::pcv::erista::Patch(reinterpret_cast<uintptr_t>(erista_buf), file_size);
            if (save_patched) {
                char* exec_path_erista = reinterpret_cast<char *>(malloc(exec_path_patched_len));
                strlcpy(exec_path_erista, exec_path, exec_path_patched_len);
                strlcat(exec_path_erista, erista_ext, exec_path_patched_len);
                saveExec(exec_path_erista, erista_buf, file_size);
                free(exec_path_erista);
            }
            free(erista_buf);
        }

        {
            void* mariko_buf = malloc(file_size);
            std::memcpy(mariko_buf, file_buffer, file_size);

            printf("Patching %s for Mariko...\n", pcv_opt);
            ams::ldr::oc::pcv::mariko::Patch(reinterpret_cast<uintptr_t>(mariko_buf), file_size);
            if (save_patched) {
                char* exec_path_mariko = reinterpret_cast<char *>(malloc(exec_path_patched_len));
                strlcpy(exec_path_mariko, exec_path, exec_path_patched_len);
                strlcat(exec_path_mariko, mariko_ext, exec_path_patched_len);
                saveExec(exec_path_mariko, mariko_buf, file_size);
                free(exec_path_mariko);
            }
            free(mariko_buf);
        }
    }

    if (exe_opt == EXE_PTM) {
        void* mariko_buf = malloc(file_size);
        std::memcpy(mariko_buf, file_buffer, file_size);

        printf("Patching %s (Mariko Only)...\n", ptm_opt);
        ams::ldr::oc::ptm::Patch(reinterpret_cast<uintptr_t>(mariko_buf), file_size);
        if (save_patched) {
            char* exec_path_mariko = reinterpret_cast<char *>(malloc(exec_path_patched_len));
            strlcpy(exec_path_mariko, exec_path, exec_path_patched_len);
            strlcat(exec_path_mariko, mariko_ext, exec_path_patched_len);
            saveExec(exec_path_mariko, mariko_buf, file_size);
            free(exec_path_mariko);
        }
        free(mariko_buf);
    }

    free(file_buffer);
    printf("Passed!\n\n");
    return 0;
}
#endif
