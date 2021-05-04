
add_library(${TARGET} STATIC)

message( STATUS "========== Configuring ${TARGET} ==========" )

set(INCLUDES PUBLIC ${TARGET_ROOT})
set(DEFINES "")

if(APPLE)
    set(SOURCES PRIVATE
        ${TARGET_ROOT}/client/minidump_file_writer.cc
        ${TARGET_ROOT}/client/mac/crash_generation/crash_generation_client.cc
        ${TARGET_ROOT}/client/mac/handler/breakpad_nlist_64.cc
        ${TARGET_ROOT}/client/mac/handler/dynamic_images.cc
        ${TARGET_ROOT}/client/mac/handler/exception_handler.cc
        ${TARGET_ROOT}/client/mac/handler/minidump_generator.cc
        ${TARGET_ROOT}/client/mac/handler/protected_memory_allocator.cc

        ${TARGET_ROOT}/common/convert_UTF.cc
        ${TARGET_ROOT}/common/md5.cc
        ${TARGET_ROOT}/common/string_conversion.cc
        ${TARGET_ROOT}/common/mac/bootstrap_compat.cc
        ${TARGET_ROOT}/common/mac/file_id.cc
        ${TARGET_ROOT}/common/mac/macho_id.cc
        ${TARGET_ROOT}/common/mac/macho_utilities.cc
        ${TARGET_ROOT}/common/mac/macho_walker.cc
        ${TARGET_ROOT}/common/mac/string_utilities.cc
        ${TARGET_ROOT}/common/mac/MachIPc.mm
    )
    list(APPEND DEFINES PRIVATE -DHAVE_CONFIG_H)
elseif(UNIX)
    set(SOURCES PRIVATE
        ${TARGET_ROOT}/client/minidump_file_writer.cc
        ${TARGET_ROOT}/client/linux/crash_generation/crash_generation_client.cc
        ${TARGET_ROOT}/client/linux/dump_writer_common/thread_info.cc
        ${TARGET_ROOT}/client/linux/dump_writer_common/ucontext_reader.cc
        ${TARGET_ROOT}/client/linux/handler/exception_handler.cc
        ${TARGET_ROOT}/client/linux/handler/minidump_descriptor.cc
        ${TARGET_ROOT}/client/linux/log/log.cc
        ${TARGET_ROOT}/client/linux/microdump_writer/microdump_writer.cc
        ${TARGET_ROOT}/client/linux/minidump_writer/linux_dumper.cc
        ${TARGET_ROOT}/client/linux/minidump_writer/linux_ptrace_dumper.cc
        ${TARGET_ROOT}/client/linux/minidump_writer/minidump_writer.cc

        ${TARGET_ROOT}/common/convert_UTF.cc
        ${TARGET_ROOT}/common/string_conversion.cc
        ${TARGET_ROOT}/common/linux/elfutils.cc
        ${TARGET_ROOT}/common/linux/file_id.cc
        ${TARGET_ROOT}/common/linux/guid_creator.cc
        ${TARGET_ROOT}/common/linux/linux_libc_support.cc
        ${TARGET_ROOT}/common/linux/memory_mapped_file.cc
        ${TARGET_ROOT}/common/linux/safe_readlink.cc
    )

    list(APPEND DEFINES PRIVATE -DHAVE_CONFIG_H)
elseif(WIN32)
    list(APPEND DEFINES PRIVATE -DUNICODE )
    set(SOURCES PRIVATE
        ${TARGET_ROOT}/client/windows/handler/exception_handler.cc
        ${TARGET_ROOT}/client/windows/crash_generation/crash_generation_client.cc
        ${TARGET_ROOT}/common/windows/guid_string.cc)
else()
    message(FATAL_ERROR "Target platform not supported")
endif()

target_include_directories(${TARGET} ${INCLUDES})
target_link_libraries(${TARGET} ${LIBRARIES})
target_sources(${TARGET} ${SOURCES})
target_compile_definitions(${TARGET} ${DEFINES})

set_target_properties( ${TARGET} PROPERTIES FOLDER "lib-src" )

organize_source( "${TARGET_ROOT}" "" "${SOURCES}" )

set(SENDER_TARGET breakpad_sender)
message( STATUS "========== Configuring ${SENDER_TARGET} ==========" )
add_library(${SENDER_TARGET} STATIC)
target_include_directories(${SENDER_TARGET} ${INCLUDES})
target_compile_definitions(${SENDER_TARGET} ${DEFINES})

set(SENDER_SOURCES
    PRIVATE
        ${TARGET_ROOT}/processor/basic_code_modules.cc
        ${TARGET_ROOT}/processor/basic_source_line_resolver.cc
        ${TARGET_ROOT}/processor/call_stack.cc
        ${TARGET_ROOT}/processor/cfi_frame_info.cc
        ${TARGET_ROOT}/processor/convert_old_arm64_context.cc
        ${TARGET_ROOT}/processor/disassembler_x86.cc
        ${TARGET_ROOT}/processor/dump_context.cc
        ${TARGET_ROOT}/processor/dump_object.cc
        ${TARGET_ROOT}/processor/exploitability.cc
        ${TARGET_ROOT}/processor/exploitability_linux.cc
        ${TARGET_ROOT}/processor/exploitability_win.cc
        ${TARGET_ROOT}/processor/fast_source_line_resolver.cc
        ${TARGET_ROOT}/processor/logging.cc
        ${TARGET_ROOT}/processor/microdump_processor.cc
        ${TARGET_ROOT}/processor/minidump.cc
        ${TARGET_ROOT}/processor/minidump_processor.cc
        ${TARGET_ROOT}/processor/module_comparer.cc
        ${TARGET_ROOT}/processor/module_serializer.cc
        ${TARGET_ROOT}/processor/pathname_stripper.cc
        ${TARGET_ROOT}/processor/proc_maps_linux.cc
        ${TARGET_ROOT}/processor/process_state.cc
        ${TARGET_ROOT}/processor/simple_symbol_supplier.cc
        ${TARGET_ROOT}/processor/source_line_resolver_base.cc
        ${TARGET_ROOT}/processor/stack_frame_cpu.cc
        ${TARGET_ROOT}/processor/stack_frame_symbolizer.cc
        ${TARGET_ROOT}/processor/stackwalk_common.cc
        ${TARGET_ROOT}/processor/stackwalker.cc
        ${TARGET_ROOT}/processor/stackwalker_address_list.cc
        ${TARGET_ROOT}/processor/stackwalker_amd64.cc
        ${TARGET_ROOT}/processor/stackwalker_arm.cc
        ${TARGET_ROOT}/processor/stackwalker_arm64.cc
        ${TARGET_ROOT}/processor/stackwalker_mips.cc
        ${TARGET_ROOT}/processor/stackwalker_ppc.cc
        ${TARGET_ROOT}/processor/stackwalker_ppc64.cc
        ${TARGET_ROOT}/processor/stackwalker_selftest.cc
        ${TARGET_ROOT}/processor/stackwalker_sparc.cc
        ${TARGET_ROOT}/processor/stackwalker_x86.cc
        ${TARGET_ROOT}/processor/symbolic_constants_win.cc
        ${TARGET_ROOT}/processor/synth_minidump.cc
        ${TARGET_ROOT}/processor/tokenize.cc

        ${TARGET_ROOT}/third_party/libdisasm/ia32_implicit.c
        ${TARGET_ROOT}/third_party/libdisasm/ia32_insn.c
        ${TARGET_ROOT}/third_party/libdisasm/ia32_invariant.c
        ${TARGET_ROOT}/third_party/libdisasm/ia32_modrm.c
        ${TARGET_ROOT}/third_party/libdisasm/ia32_opcode_tables.c
        ${TARGET_ROOT}/third_party/libdisasm/ia32_operand.c
        ${TARGET_ROOT}/third_party/libdisasm/ia32_reg.c
        ${TARGET_ROOT}/third_party/libdisasm/ia32_settings.c
        ${TARGET_ROOT}/third_party/libdisasm/x86_disasm.c
        ${TARGET_ROOT}/third_party/libdisasm/x86_format.c
        ${TARGET_ROOT}/third_party/libdisasm/x86_imm.c
        ${TARGET_ROOT}/third_party/libdisasm/x86_insn.c
        ${TARGET_ROOT}/third_party/libdisasm/x86_misc.c
        ${TARGET_ROOT}/third_party/libdisasm/x86_operand_list.c)

if(UNIX)
    find_package(CURL REQUIRED)
    target_link_libraries(${SENDER_TARGET} dl ${CURL_LIBRARIES})
    set(SENDER_SOURCES ${SENDER_SOURCES} 
        PRIVATE
            ${TARGET_ROOT}/common/linux/http_upload.cc
    )
elseif(WIN32)
    target_link_libraries(${SENDER_TARGET} wininet.lib)
    set(SENDER_SOURCES ${SENDER_SOURCES}
        PRIVATE
            ${TARGET_ROOT}/client/windows/sender/crash_report_sender.cc
            ${TARGET_ROOT}/common/windows/http_upload.cc
    )
endif()
target_sources(${SENDER_TARGET} ${SENDER_SOURCES})
set_target_properties(${SENDER_TARGET} PROPERTIES FOLDER "lib-src")
