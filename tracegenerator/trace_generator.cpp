#include <unistd.h>
#include <inttypes.h>
#include <unicorn/unicorn.h>
#include <elfio/elfio.hpp>

#include <trace_io.h>
#include <fstream>

#include <string.h>
#include <stdio.h>

using namespace std;
using namespace ELFIO;

trace_io::raw_output_pipe_t *out;

// callback for tracing instruction
static void hook_code64(uc_engine *uc, uint64_t address, uint32_t size, void *user_data)
{
    uint64_t rip;

    uc_reg_read(uc, UC_X86_REG_RIP, &rip);

    trace_io::trace_item_t t1;
    t1.type = 2;
    t1.opcode[0] = 0x90;
    t1.length = size;
    t1.addr = address;
    out->write_trace_item(t1);
}

static void test_x86_64(char *filename)
{
    elfio reader;

    uc_engine *uc;
    uc_err err;
    uc_hook trace1, trace2, trace3, trace4;

    printf("Emulating x86_64 code\n");

    // Read the binary from a file
    char *buffer_code;
    long len_code;
    int64_t base_address;

    reader.load(filename);

    // Iterate over all elf sections
    Elf_Half sec_num = reader.sections.size();
    for (int i = 0; i < sec_num; i++) {
      section* psec = reader.sections[i];
      if (psec->get_name() == ".text") {
        buffer_code = (char*) psec->get_data();
        len_code = psec->get_size();
        base_address = psec->get_address();
      }
    }

    int64_t rsp = base_address + 1024;

    // Initialize emulator in X86-64bit mode
    err = uc_open(UC_ARCH_X86, UC_MODE_64, &uc);
    if (err) {
        printf("Failed on uc_open() with error returned: %u\n", err);
        return;
    }

    // map 10MB memory for this emulation
    uc_mem_map(uc, base_address, 1024 * 1024 * 2, UC_PROT_ALL);

    // write machine code to be emulated to memory
    if (uc_mem_write(uc, base_address, buffer_code, len_code-1)) {
        printf("Failed to write emulation code to memory, quit!\n");
        return;
    }

    // initialize machine registers
    uc_reg_write(uc, UC_X86_REG_RSP, &rsp);

    // tracing all instructions in the range [ADDRESS, ADDRESS+20]
    uc_hook_add(uc, &trace2, UC_HOOK_CODE, (void*) hook_code64, NULL,
                base_address, base_address+1024);

    // emulate machine code in infinite time (last param = 0), or when
    // finishing all the code.
    err = uc_emu_start(uc, base_address, base_address + len_code-1, 0, 0);
    if (err) {
        printf("Failed on uc_emu_start() with error returned %u: %s\n",
                err, uc_strerror(err));
    }

    // now print out some registers
    printf(">>> Emulation done.\n");

    uc_close(uc);
}

int main(int argc, char **argv, char **envp)
{
  out = new trace_io::raw_output_pipe_t(string("test"));

  if (argc < 2) {
    printf("You must pass the binary filepath as argument!\n");
    return 1;
  }

  test_x86_64(argv[1]);

  delete out;
  return 0;
}
