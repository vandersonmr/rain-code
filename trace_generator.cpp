#include <unistd.h>
#include <inttypes.h>
#include <unicorn/unicorn.h>

#include <trace_io.h>
#include <fstream>

#include <string.h>
#include <stdio.h>

using namespace std;

// memory address where emulation starts
#define ADDRESS 0x1000

#define LINUX_SYS_THRESHOLD   0xB2D05E00         // 3000000000
#define WINDOWS_SYS_THRESHOLD 0xF9CCD8A1C5080000 // 18000000000000000000 

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

long read_binary(char *filename, char **buffer) {
    long length;
    FILE * f = fopen (filename, "rb");

    if (f) {
      fseek (f, 0, SEEK_END);
      length = ftell (f);
      fseek (f, 0, SEEK_SET);
      *buffer = (char*) malloc (length);
      if (*buffer)
      {
          fread (*buffer, 1, length, f);
      }
      fclose (f);
    }
    return length;
}

static void test_x86_64(char *filename_code, char *filename_data)
{
    uc_engine *uc;
    uc_err err;
    uc_hook trace1, trace2, trace3, trace4;

    int64_t rsp = ADDRESS + 1024;

    printf("Emulate x86_64 code\n");

    // Read the binary from a file
    char * buffer_code = 0;
    char * buffer_data = 0;
    long len_code = 0;
    long len_data = 0;

    len_code = read_binary(filename_code, &buffer_code);
    len_data = read_binary(filename_data, &buffer_data);

    // Initialize emulator in X86-64bit mode
    err = uc_open(UC_ARCH_X86, UC_MODE_64, &uc);
    if (err) {
        printf("Failed on uc_open() with error returned: %u\n", err);
        return;
    }

    // map 10MB memory for this emulation
    uc_mem_map(uc, ADDRESS, 1024 * 1024 * 1024, UC_PROT_ALL);

    // write machine code to be emulated to memory
    if (uc_mem_write(uc, ADDRESS, buffer_code, len_code-1) | 
        uc_mem_write(uc, ADDRESS+0x1000, buffer_data, len_data)) {
        printf("Failed to write emulation code to memory, quit!\n");
        return;
    }

    // initialize machine registers
    uc_reg_write(uc, UC_X86_REG_RSP, &rsp);

    // tracing all instructions in the range [ADDRESS, ADDRESS+20]
    uc_hook_add(uc, &trace2, UC_HOOK_CODE, (void*) hook_code64, NULL, ADDRESS, ADDRESS+1024);

    // emulate machine code in infinite time (last param = 0), or when
    // finishing all the code.
    err = uc_emu_start(uc, ADDRESS, ADDRESS + len_code-1, 0, 0);
    if (err) {
        printf("Failed on uc_emu_start() with error returned %u: %s\n",
                err, uc_strerror(err));
    }

    // now print out some registers
    printf(">>> Emulation done. Below is the CPU context\n");

    uc_close(uc);
}

int main(int argc, char **argv, char **envp)
{
  out = new trace_io::raw_output_pipe_t(string("test"));

  for (int i = 0; i < 100; i++) {
  }

  if (argc < 3) {
    printf("You should pass the data filepath and the text filepath as argument!\n");
    return 1;
  }

  test_x86_64(argv[1], argv[2]);

  delete out;
  return 0;
}
