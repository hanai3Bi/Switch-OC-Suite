#!python3

import sys
import os

def file_replace_str(file_path, search_replace_list):
    assert file_path
    assert search_replace_list
    with open(file_path, "r") as f:
        content = f.read()

    for entry in search_replace_list:
        (search, replace) = entry
        if search in content:
            content = content.replace(search, replace)
        else:
            assert replace in content, f"Pattern \"{search}\" not found"

    with open(file_path, "w") as f:
        f.write(content)


dir_path = os.path.dirname(__file__)

ldr_main = os.path.join(dir_path, "ldr_main.cpp")
file_replace_str(ldr_main, [("constinit u8 g_heap_memory[16_KB];", "constinit u8 g_heap_memory[16_KB + 5_KB];")])

ldr_process_creation = os.path.join(dir_path, "ldr_process_creation.cpp")
file_replace_str(ldr_process_creation,
    [("""#include "ldr_ro_manager.hpp"

namespace ams::ldr {""",
"""#include "ldr_ro_manager.hpp"
#include "oc/oc_suite.hpp"

namespace ams::ldr {"""),
    ("""        NsoHeader g_nso_headers[Nso_Count];

        Result ValidateProgramVersion(ncm::ProgramId program_id, u32 version) {""",
    """        NsoHeader g_nso_headers[Nso_Count];

        /* Pcv/Ptm check cache. */
        bool g_is_pcv;
        bool g_is_ptm;

        Result ValidateProgramVersion(ncm::ProgramId program_id, u32 version) {"""),
    ("""            R_UNLESS(meta->aci->program_id <= meta->acid->program_id_max, ldr::ResultInvalidProgramId());

            /* Validate the kernel capabilities. */""",
    """            R_UNLESS(meta->aci->program_id <= meta->acid->program_id_max, ldr::ResultInvalidProgramId());

            /* Check if nca is pcv or ptm */
            g_is_pcv = meta->aci->program_id == ncm::SystemProgramId::Pcv;
            g_is_ptm = meta->aci->program_id == ncm::SystemProgramId::Ptm;

            /* Validate the kernel capabilities. */"""),
    ("""                LocateAndApplyIpsPatchesToModule(nso_header->module_id, map_address, nso_size);
            }""",
    """                LocateAndApplyIpsPatchesToModule(nso_header->module_id, map_address, nso_size);

                /* Apply pcv and ptm patches. */
                if (g_is_pcv)
                    oc::pcv::Patch(map_address, nso_size);
                if (g_is_ptm)
                    oc::ptm::Patch(map_address, nso_size);
            }""")])

ldr_meta = os.path.join(dir_path, "ldr_meta.cpp")
file_replace_str(ldr_meta,
    [("""        Result ValidateAcidSignature(Meta *meta) {
            /* Loader did not check signatures prior to 10.0.0. */""",
    """        Result ValidateAcidSignature(Meta *meta) {
            R_SUCCEED();
            /* Loader did not check signatures prior to 10.0.0. */""")])
