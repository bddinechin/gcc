/* args are passed on registers from r0 up to r11 => 12*8 bytes */
#define REG_ARGS_SIZE (12*8)
#define LVX_REGISTER_SIZE (8)
#define LVX_ABI_SLOT_SIZE (LVX_REGISTER_SIZE)
#define LVX_ABI_MAX_AGGREGATE_IN_REG_SIZE (4*LVX_ABI_SLOT_SIZE)
