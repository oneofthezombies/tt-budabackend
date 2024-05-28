// An example of a simple C++ program that can be compiled with the RISC-V GCC toolchain.
#include <stdint.h>

// Registers for debug register access

#define RISCV_DEBUG_REGS_START_ADDR             0xFFB12000
#define RISCV_DEBUG_REG_WALL_CLOCK_L            (RISCV_DEBUG_REGS_START_ADDR | 0x1F0)
#define RISCV_DEBUG_REG_WALL_CLOCK_H            (RISCV_DEBUG_REGS_START_ADDR | 0x1F8)

extern void (* __init_array_start[])();
extern void (* __init_array_end[])();

#ifdef USE_GLOBAL_VARS
volatile uint32_t *RISC_DBG_CNTL0 = reinterpret_cast<volatile uint32_t *> (0xFFB12080);
volatile uint32_t *RISC_DBG_CNTL1 = reinterpret_cast<volatile uint32_t *> (0xFFB12084);
volatile uint32_t *RISC_DBG_STATUS0 = reinterpret_cast<volatile uint32_t *> (0xFFB12088);
volatile uint32_t *RISC_DBG_STATUS1 = reinterpret_cast<volatile uint32_t *> (0xFFB1208C);
#endif

volatile uint32_t g_MAILBOX;

inline void wait(uint32_t cycles) {
    volatile uint32_t * clock_lo = reinterpret_cast<volatile uint32_t * >(RISCV_DEBUG_REG_WALL_CLOCK_L);
    volatile uint32_t * clock_hi = reinterpret_cast<volatile uint32_t * >(RISCV_DEBUG_REG_WALL_CLOCK_H);
    uint64_t wall_clock_timestamp = clock_lo[0] | ((uint64_t)clock_hi[0]<<32);
    uint64_t wall_clock = 0;
    do {
       wall_clock = clock_lo[0] | ((uint64_t)clock_hi[0]<<32);
    }
    while (wall_clock < (wall_clock_timestamp+cycles));
}

void halt(int DBG_RISC_ID) {
#ifndef USE_GLOBAL_VARS
    volatile uint32_t *RISC_DBG_CNTL0 = reinterpret_cast<volatile uint32_t *> (0xFFB12080);
    volatile uint32_t *RISC_DBG_CNTL1 = reinterpret_cast<volatile uint32_t *> (0xFFB12084);
    volatile uint32_t *RISC_DBG_STATUS0 = reinterpret_cast<volatile uint32_t *> (0xFFB12088);
    volatile uint32_t *RISC_DBG_STATUS1 = reinterpret_cast<volatile uint32_t *> (0xFFB1208C);
#endif
    *RISC_DBG_CNTL1 = 0x80000000;
    *RISC_DBG_CNTL0 = 0x80010001 | (DBG_RISC_ID << 17);
    *RISC_DBG_CNTL0 = 0x00000000;
    *RISC_DBG_CNTL1 = 0x80000001;
    *RISC_DBG_CNTL0 = 0x80010001  | (DBG_RISC_ID << 17);
    *RISC_DBG_CNTL0 = 0x00000000;
}

extern "C" void wzerorange(uint32_t *start, uint32_t *end)
{
    for (; start != end; start++)
    {
        *start = 0;
    }
}

void decrement_mailbox() {
  g_MAILBOX--;
}

extern "C" void infloop() {
  for (;;);
}

int main() {
#ifndef USE_GLOBAL_VARS
    volatile uint32_t *RISC_DBG_CNTL0 = reinterpret_cast<volatile uint32_t *> (0xFFB12080);
    volatile uint32_t *RISC_DBG_CNTL1 = reinterpret_cast<volatile uint32_t *> (0xFFB12084);
    volatile uint32_t *RISC_DBG_STATUS0 = reinterpret_cast<volatile uint32_t *> (0xFFB12088);
    volatile uint32_t *RISC_DBG_STATUS1 = reinterpret_cast<volatile uint32_t *> (0xFFB1208C);
#endif

  for (void (** fptr)() = __init_array_start; fptr < __init_array_end; fptr++) {
      (**fptr)();
  }

  // STEP 1: Set the mailbox to RISC_DBG_STATUS1
  g_MAILBOX = (uint32_t) RISC_DBG_STATUS1;

  // STEP 2: Wait for mailbox to become 0x1234
  while (g_MAILBOX != 0x1234) {
    // wait for mailbox to be set to 0x1234
  }

  // STEP 3: Set the mailbox to RISC_DBG_CNTL0
  g_MAILBOX = (uint32_t) RISC_DBG_CNTL0;

  // STEP 4: Put the core in halted state
  halt(0);

  // STEP 5: Set the mailbox to 10
  g_MAILBOX = 10;

  // STEP 6: call decrement_mailbox until it gets to 0. The debugger will set breakpoints and watchpoints and
  // test that they have been observed by the core.
  while (g_MAILBOX > 0) {
    decrement_mailbox();
  }


  // STEP END: Set the mailbox to RISC_DBG_STATUS0
  g_MAILBOX = (uint32_t) RISC_DBG_STATUS0;
  infloop();
  return 0;
}
