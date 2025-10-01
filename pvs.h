#include <stdint.h>

struct pvs_instruction {
  uint32_t op_dst_operand;
  uint32_t src_operand[0];
};
