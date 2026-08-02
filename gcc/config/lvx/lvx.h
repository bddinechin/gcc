/* Helpers */

#ifndef IN_LIBGCC2
#include "coretypes.h"
#include "statistics.h"
#include "stringpool.h"
#include "vec.h"
#endif

/* 18.2 Controlling the Compilation Driver {{{  */

/* The macro DRIVER_SELF_SPECS_COMMON is used in the definition of
   DRIVER_SELF_SPECS in each target specific OS files (e.g., lvx-cos.h,
   lvx-mbr.h and lvx-linux.h.)  */
#define DRIVER_SELF_SPECS_COMMON                                               \
  "%{fpic: %{!fPIC:-fPIC}} %<fpic",                                            \
  "%{fPIC: %<fpic}",                                                           \
  "%{!march=*:-march=lvx-1}"

/* Support for a compile-time default arch.  The rules are:
   --with-arch is ignored if -march is specified.  */
#define OPTION_DEFAULT_SPECS {"arch", "%{!march=*:-march=%(VALUE)}"},

/* TODO: Investigate.  */
#define CC1_SPEC " %{G*}"

#define ASM_SPEC "%{march*} --check-resources "

#define LINK_SPEC_COMMON "%{shared} -melf64lvx "
/* 18.2 Controlling the Compilation Driver }}}  */


/* 18.3 Run-time Target Specification {{{  */

#define TARGET_CPU_CPP_BUILTINS()                                              \
  do                                                                           \
    {                                                                          \
      builtin_assert ("cpu=lvx");                                              \
      builtin_define ("__LVX__");                                              \
      builtin_define ("__lvx__");                                              \
      if (lvx_arch_name == LVX_ARCH_LVX_1)                                     \
	{                                                                      \
	  builtin_assert ("machine=lvx-1");                                    \
	  builtin_define ("__lvxarch_lvx_1");                                  \
	  builtin_define ("__lvx_1__");                                        \
	  builtin_define ("__LVX_64__");                                       \
	}                                                                      \
      if (lvx_arch_name == LVX_ARCH_LVX_2)                                     \
	{                                                                      \
	  builtin_assert ("machine=lvx-2");                                    \
	  builtin_define ("__lvxarch_lvx_2");                                  \
	  builtin_define ("__lvx_2__");                                        \
	  builtin_define ("__LVX_64__");                                       \
	}                                                                      \
      if (TARGET_STRICT_ALIGN)                                                 \
	builtin_define ("__STRICT_ALIGN__");                                   \
      if (TARGET_STACK_CHECK_USE_TLS)                                          \
	builtin_define ("__LVX_STACK_LIMIT_TLS");                              \
      cpp_undef (pfile, "__FLT_EVAL_METHOD__");                                \
      builtin_define_with_int_value ("__FLT_EVAL_METHOD__", 0);                \
    }                                                                          \
  while (0)

/* 18.3 Run-time Target Specification }}} */


/* 18.4 Defining data structures for per-function information {{{  */

#define INIT_EXPANDERS lvx_init_expanders ()

/* 18.4 Defining data structures for per-function information }}}  */


/* 18.5 Storage Layout {{{  */

#define BITS_BIG_ENDIAN 0

#define BYTES_BIG_ENDIAN 0

#define WORDS_BIG_ENDIAN 0

#define UNITS_PER_WORD 8

#define POINTER_SIZE 64

#define POINTERS_EXTEND_UNSIGNED 1

#define PROMOTE_MODE(MODE, UNSIGNEDP, TYPE)                                    \
  if (GET_MODE_CLASS (MODE) == MODE_INT                                        \
      && GET_MODE_SIZE (MODE) < UNITS_PER_WORD)                                \
    {                                                                          \
      (MODE) = word_mode;                                                      \
      (UNSIGNEDP) = 1;                                                         \
    }

/* Normal alignment required for function parameters on the stack, in bits.  */
#define PARM_BOUNDARY 64

/* The minimum alignment for the stack pointer on this machine.  */
#define STACK_BOUNDARY 256

/* Alignment required for a function entry point, in bits.  */
#define FUNCTION_BOUNDARY 64

/* Biggest alignment that any data type can require on this machine, in bits. */
#define BIGGEST_ALIGNMENT 256

/* Align definitions of arrays, unions and structures so that
   initializations and copies can be made more efficient.  This is not
   ABI-changing, so it only affects places where we can see the
   definition.  */
#define DATA_ALIGNMENT(EXP, ALIGN)                                             \
  ((ALIGN) < BITS_PER_WORD ? BITS_PER_WORD : (ALIGN))

/* Similarly, make sure that objects on the stack are sensibly aligned.  */
#define LOCAL_ALIGNMENT(EXP, ALIGN) DATA_ALIGNMENT (EXP, ALIGN)

#define STRICT_ALIGNMENT TARGET_STRICT_ALIGN

#define PCC_BITFIELD_TYPE_MATTERS 1

/* An integer expression for the size in bits of the largest integer machine
   mode that should actually be used.  */
#define MAX_FIXED_MODE_SIZE 256

#undef TARGET_FIXED_POINT_SUPPORTED_P
#define TARGET_FIXED_POINT_SUPPORTED_P hook_bool_void_false


/* 18.5 Storage Layout }}}  */


/* 18.6 Layout of Source Language Data Types {{{  */

#define INT_TYPE_SIZE 32

#define SHORT_TYPE_SIZE 16

#define LONG_TYPE_SIZE 64

#define LONG_LONG_TYPE_SIZE 64




#define DEFAULT_SIGNED_CHAR 1

#define SIZE_TYPE "long unsigned int"

#define PTRDIFF_TYPE "long int"

/* 18.6 Layout of Source Language Data Types }}}  */


/* 18.7 Register Usage {{{  */

/* 18.7.1 Register Basics {{{  */

#define FIXED_REGISTERS \
  { LVX_ABI_REGULAR_FIXED_REGISTERS }

#define CALL_REALLY_USED_REGISTERS \
  { LVX_ABI_REGULAR_CALL_REALLY_USED_REGISTERS }

#define PC_REGNUM LVX_PROGRAM_POINTER_REGNO

/* 18.7.1 Register Basics }}}  */

/* 18.7 Register Usage }}}  */


/* 18.8 Register Classes {{{  */

/* Utils {{{ */

/* Provide the GENERAL_REGS definitions */
#define GENERAL_REGS GPR_REGS

#define TEST_REGNO(R, TEST, VALUE, STRICT)                                     \
  ((!(STRICT) && R >= FIRST_PSEUDO_REGISTER) || (R TEST VALUE)                 \
   || (reg_renumber && ((unsigned) reg_renumber[R] TEST VALUE)))

/* LVX_FRAME_POINTER_VIRT_REGNO is used as part of
   lvx_legitimize_reload_address () because reload wants real addresses
   when it reloads MEMs. This register shouldn't be generated anywhere
   else, thus it causes no correctness issue. */
#define IS_GENERAL_REGNO(num, strict)                                          \
  (TEST_REGNO (num, <, 64, strict)                                             \
   || TEST_REGNO (num, ==, (LVX_FRAME_POINTER_VIRT_REGNO), strict))

/* Utils }}} */

/* The number of distinct register classes, defined as follows: */
#define N_REG_CLASSES (int) LIM_REG_CLASSES

/* A macro whose definition is the name of the class to which a valid
   base register must belong. A base register is one used in an
   address which is the register value plus a displacement. */
#define BASE_REG_CLASS GENERAL_REGS

/* A macro whose definition is the name of the class to which a valid
   index register must belong. An index register is one used in an
   address where its value is either multiplied by a scale factor or
   added to another register (as well as added to a displacement). */
#define INDEX_REG_CLASS GENERAL_REGS

/* A C expression which is nonzero if register number num is suitable
   for use as a base register in operand addresses.  */

#ifdef REG_OK_STRICT
#define REGNO_OK_FOR_BASE_P(num) (IS_GENERAL_REGNO (num, 1))
#else
#define REGNO_OK_FOR_BASE_P(num) (IS_GENERAL_REGNO (num, 0))
#endif

/* A C expression which is nonzero if register number num is suitable
   for use as an index register in operand addresses. It may be either
   a suitable hard register or a pseudo register that has been
   allocated such a hard register.

   The difference between an index register and a base register is
   that the index register may be scaled. If an address involves the
   sum of two registers, neither one of them scaled, then either one
   may be labeled the “base” and the other the “index”; but whichever
   labeling is used must fit the machine's constraints of which
   registers may serve in each capacity. The compiler will try both
   labelings, looking for one that is valid, and will reload one or
   both registers only if neither labeling works.

   This macro also has strict and non-strict variants. */
#ifdef REG_OK_STRICT
#define REGNO_OK_FOR_INDEX_P(num) IS_GENERAL_REGNO (num, 1)
#else
#define REGNO_OK_FOR_INDEX_P(num) IS_GENERAL_REGNO (num, 0)
#endif

/* A C expression that places additional restrictions on the register
   class to use when it is necessary to copy value x into a register
   in class class. The value is a register class; perhaps class, or
   perhaps another, smaller class. On many machines, the following
   definition is safe:

   #define PREFERRED_RELOAD_CLASS(X,CLASS) CLASS

   Sometimes returning a more restrictive class makes better code. For
   example, on the 68000, when x is an integer  constant that is in
   range for a `moveq' instruction, the value of this macro is always
   DATA_REGS as long as class includes the data registers. Requiring a
   data register guarantees that a `moveq' will be used.

   One case where PREFERRED_RELOAD_CLASS must not return class is if x
   is a legitimate constant which cannot be loaded into some register
   class. By returning NO_REGS you can force x into a memory
   location. For example, rs6000 can load immediate values into
   general-purpose registers, but does not have an instruction for
   loading an immediate value into a floating-point register, so
   PREFERRED_RELOAD_CLASS returns NO_REGS when x is a floating-point
   constant. If the constant can't be loaded into any kind of
   register, code generation will be better if LEGITIMATE_CONSTANT_P
   makes the constant illegitimate instead of using
   PREFERRED_RELOAD_CLASS.

   If an insn has pseudos in it after register allocation, reload will
   go through the alternatives and call repeatedly
   PREFERRED_RELOAD_CLASS to find the best one. Returning NO_REGS, in
   this case, makes reload add a ! in front of the constraint: the x86
   back-end uses this feature to discourage usage of 387 registers
   when math is done in the SSE registers (and vice versa). */
#define PREFERRED_RELOAD_CLASS(X, CLASS) CLASS

/* 18.7.8 Register Classes }}}  */


/* 18.9 Stack Layout and Calling Conventions {{{ */

/* 18.9.1 Basic Stack Layout {{{ */

#define STACK_GROWS_DOWNWARD 1

#define FRAME_GROWS_DOWNWARD 1

#undef ARGS_GROWS_DOWNWARD

/* We handle the alignment of automatic variables in lvx_compute_frame_info.  */
#define STACK_ALIGNMENT_NEEDED 0

#define FIRST_PARM_OFFSET(funcdecl) lvx_first_parm_offset (funcdecl)

#define RETURN_ADDR_RTX(COUNT, FRAMEADDR) lvx_return_addr_rtx (COUNT, FRAMEADDR)

#define INCOMING_RETURN_ADDR_RTX \
  gen_rtx_REG (Pmode, LVX_RETURN_POINTER_REGNO)

/* If INCOMING_RETURN_ADDR_RTX is a 'REG', DWARF_FRAME_RETURN_COLUMN should
   also defined to 'DWARF_FRAME_REGNUM'.  */
#define DWARF_FRAME_RETURN_COLUMN \
  DBX_REGISTER_NUMBER (LVX_RETURN_POINTER_REGNO)

/* 18.9.1 Basic Stack Layout }}} */

/* 18.9.4 Registers That Address the Stack Frame {{{ */

#define STACK_POINTER_REGNUM LVX_STACK_POINTER_REGNO

#define FRAME_POINTER_REGNUM LVX_FRAME_POINTER_VIRT_REGNO

#define HARD_FRAME_POINTER_REGNUM LVX_FRAME_POINTER_REGNO

#define ARG_POINTER_REGNUM FRAME_POINTER_REGNUM

/* 18.9.4 Registers That Address the Stack Frame }}} */

/* 18.9.5 Eliminating Frame Pointer and Arg Pointer {{{ */

#define ELIMINABLE_REGS                                                        \
  {                                                                            \
    {FRAME_POINTER_REGNUM, STACK_POINTER_REGNUM},                              \
    {FRAME_POINTER_REGNUM, HARD_FRAME_POINTER_REGNUM},                         \
  }

#define INITIAL_ELIMINATION_OFFSET(FROM, TO, OFFSET)                           \
  (OFFSET) = lvx_initial_elimination_offset (FROM, TO)

/* 18.9.5 Eliminating Frame Pointer and Arg Pointer }}} */

/* 18.9.6 Passing Function Arguments on the Stack {{{ */

/* A C expression. If nonzero, the maximum amount of space required
   for outgoing arguments will be computed and placed into the
   variable current_function_outgoing_args_size. No space will be
   pushed onto the stack for each call; instead, the function prologue
   should increase the stack frame size by this amount.

   Setting both PUSH_ARGS and ACCUMULATE_OUTGOING_ARGS is not proper. */
#define ACCUMULATE_OUTGOING_ARGS 1

/* 18.9.6 Passing Function Arguments on the Stack }}} */

/* 18.9.7 Passing Arguments in Registers {{{ */

/* Struct used for CUMULATIVE_ARGS. */
#ifdef HOST_WIDE_INT
struct lvx_args
{
  /* Next argument register to be used.
     If >= LVX_ARG_REG_SLOTS, then all slots are used.  */
  int next_arg_reg;

  /* Offset from ARG_POINTER_REGNUM to anonymous argument area, if any */
  int anonymous_arg_offset;

  bool anonymous_arg_offset_valid;
};
#endif

#define CUMULATIVE_ARGS struct lvx_args

#define INIT_CUMULATIVE_ARGS(CUM, FNTYPE, LIBNAME, FNDECL, N_NAMED_ARGS)       \
  lvx_init_cumulative_args (&CUM, FNTYPE, LIBNAME, FNDECL, N_NAMED_ARGS)

/* A C expression that is nonzero if regno is the number of a hard
   register in which function arguments are sometimes passed. This
   does not include implicit arguments such as the static chain and
   the structure-value address. On many machines, no registers can be
   used for this purpose since all function arguments are pushed on‰
   the stack. */
#define FUNCTION_ARG_REGNO_P(regno)                                            \
  ((int) (regno) >= LVX_ARGUMENT_POINTER_REGNO                                 \
   && (regno) < LVX_ARGUMENT_POINTER_REGNO + LVX_ARG_REG_SLOTS)

/* 18.9.7 Passing Arguments in Registers }}} */

/* 18.9.8 How Scalar Function Values Are Returned {{{ */

/* A C expression to create an RTX representing the place where a
   library function returns a value of mode mode.

   Note that “library function” in this context means a compiler
   support routine, used to perform arithmetic, whose name is known
   specially by the compiler and was not mentioned in the C code being
   compiled. */
#define LIBCALL_VALUE(MODE) \
  gen_rtx_REG (MODE, LVX_ARGUMENT_POINTER_REGNO)

/* A C expression that is nonzero if REGNO is the number of a hard register in
   which the values of called function may come back.  */
#define FUNCTION_VALUE_REGNO_P(REGNO) \
  FUNCTION_ARG_REGNO_P (REGNO)

/* 18.9.8 How Scalar Function Values Are Returned }}} */

/* 18.9.9 How Large Values Are Returned {{{ */

/* Define this macro to be 1 if all structure and union return values
   must be in memory. Since this results in slower code, this should
   be defined only if needed for compatibility with other compilers or
   with an ABI. If you define this macro to be 0, then the conventions
   used for structure and union return values are decided by the
   TARGET_RETURN_IN_MEMORY target hook.

   If not defined, this defaults to the value 1. */
#define DEFAULT_PCC_STRUCT_RETURN 0

/* 18.9.9 How Large Values Are Returned }}} */

/* 18.9.11 Function Entry and Exit {{{ */

/* Define this macro as a C expression that is nonzero if the return
   instruction or the function epilogue ignores the value of the stack
   pointer; in other words, if it is safe to delete an instruction to
   adjust the stack pointer before a return from the function. The
   default is 0.

   Note that this macro's value is relevant only for functions for
   which frame pointers are maintained. It is never safe to delete a
   final stack adjustment in a function that has no frame pointer, and
   the compiler knows this regardless of EXIT_IGNORE_STACK. */
#define EXIT_IGNORE_STACK 1

/* Define this macro as a C expression that is nonzero for registers
   that are used by the epilogue or the `return' pattern. The stack
   and frame pointer registers are already assumed to be used as
   needed. */
#define EPILOGUE_USES(regno) (regno == LVX_RETURN_POINTER_REGNO)

/* 18.9.11 Function Entry and Exit }}} */

/* 18.9.12 Generating Code for Profiling {{{ */

/* A C statement or compound statement to output to file some
   assembler code to call the profiling subroutine mcount.

   The details of how mcount expects to be called are determined by
   your operating system environment, not by GCC. To figure them out,
   compile a small program for profiling using the system's installed
   C compiler and look at the assembler code that results.

   Older implementations of mcount expect the address of a counter
   variable to be loaded into some register. The name of this variable
   is `LP' followed by the number labelno, so you would generate the
   name using `LP%d' in a fprintf.  */
#define FUNCTION_PROFILER(STREAM, LABELNO) \
  do { /* nothing */ } while (0)

/* Emit rtl for profiling to call "_mcount" for profiling a function entry. */
void lvx_profile_hook (void);
#define PROFILE_HOOK(LABEL) \
  lvx_profile_hook ()

/* 18.9.12 Generating Code for Profiling }}} */

/* 18.9 Stack Layout and Calling Conventions }}} */


/* 18.11 Support for Nested Functions {{{ */

/* Length in units of the trampoline for entering a nested function.  */
#define TRAMPOLINE_SIZE (12 /* make */ + 12 /* make */ + 4 /* igoto */)

/* Alignment required for a trampoline in bits .  */
#define TRAMPOLINE_ALIGNMENT 32

/* 18.11 Support for Nested Functions }}} */


/* 18.13 Addressing Modes {{{ */

/* A C expression that is 1 if the RTX x is a constant which is a
   valid address. */
#define CONSTANT_ADDRESS_P(x) (CONSTANT_P (x) && GET_CODE (x) != CONST_DOUBLE)

/* A number, the maximum number of registers that can appear in a
   valid memory address. Note that it is up to you to specify a value
   equal to the maximum number that TARGET_LEGITIMATE_ADDRESS_P would
   ever accept. */
#define MAX_REGS_PER_ADDRESS 2

/* 18.13 Addressing Modes }}} */


/* 18.16 Describing Relative Costs of Operations {{{ */

#define BRANCH_COST(speed, predictable) 6

/* Define this macro as a C expression which is nonzero if accessing
   less than a word of memory (i.e. a char or a short) is no faster
   than accessing a word of memory, i.e., if such access require more
   than one instruction or if there is no difference in cost between
   byte and (aligned) word loads.

   When this macro is not defined, the compiler will access a field by
   finding the smallest containing object; when it is defined, a
   fullword load will be used if alignment permits. Unless bytes
   accesses are faster than word accesses, using word accesses is
   preferable since it may eliminate subsequent memory access if
   subsequent accesses occur to other fields in the same word of the
   structure, but to different bytes. */
#define SLOW_BYTE_ACCESS 1

/* Define this macro to be the value 1 if memory accesses described by
   the mode and alignment parameters have a cost many times greater
   than aligned accesses, for example if they are emulated in a trap
   handler.

   When this macro is nonzero, the compiler will act as if
   STRICT_ALIGNMENT were nonzero when generating code for block
   moves. This can cause significantly more instructions to be
   produced. Therefore, do not set this macro nonzero if unaligned
   accesses only add a cycle or two to the time for a memory access.

   If the value of this macro is always zero, it need not be
   defined. If this macro is defined, it should produce a nonzero
   value when STRICT_ALIGNMENT is nonzero. */

#define NO_FUNCTION_CSE 1

/* 18.16 Describing Relative Costs of Operations }}} */


/* 18.18 Dividing the Output into Sections {{{ */

/* FIXME: Missing in the manual */
#define GLOBAL_ASM_OP "\t.global "

#define TEXT_SECTION_ASM_OP "\t.text"

#define DATA_SECTION_ASM_OP "\t.data"

#define BSS_SECTION_ASM_OP "\t.section .bss"

#define CRT_CALL_STATIC_FUNCTION(SECTION_OP, FUNC) \
  asm(SECTION_OP "\ncall " #FUNC "\n;;\n.previous\n");

/* Place jump tables next to code for PIC so that PCREL relocations
 are resolved during assembly */
#define JUMP_TABLES_IN_TEXT_SECTION (flag_pic || CASE_VECTOR_PC_RELATIVE)

/* 18.18 Dividing the Output into Sections }}} */


/* 18.19 Position Independent Code {{{ */

#define LEGITIMATE_PIC_OPERAND_P(X) \
  lvx_legitimate_pic_operand_p (X)

/* 18.19 Position Independent Code }}} */


/* 18.20 Defining the Output Assembler Language {{{ */

/* 18.20.1 The Overall Framework of an Assembler File {{{ */

#define ASM_COMMENT_START "#"

#define ASM_APP_ON "\t;;\n#APP\n"

#define ASM_APP_OFF "\n\t;;\n#NO_APP\n"

/* 18.20.1 The Overall Framework of an Assembler File }}} */

/* 18.20.7 Output of Assembler Instructions {{{ */

/* A C initializer containing the assembler's names for the machine
   registers, each one as a C string constant. This is what translates
   register numbers in the compiler into assembler language. */
#define REGISTER_NAMES \
  { LVX_REGISTER_NAMES }

/* If defined, a C initializer for an array of structures containing a
   name and a register number. This macro defines additional names for
   hard registers, thus allowing the asm option in declarations to
   refer to registers using alternate names. */
#define ADDITIONAL_REGISTER_NAMES                                              \
  {                                                                            \
    {"r0r1", 0}, {"r2r3", 2}, {"r4r5", 4}, {"r6r7", 6}, {"r8r9", 8},           \
    {"r9r11", 10}, {"r12r13", 12}, {"r14r15", 14}, {"r16r17", 16},             \
    {"r18r19", 18}, {"r20r21", 20}, {"r22r23", 22}, {"r24r25", 24},            \
    {"r26r27", 26}, {"r28r29", 28}, {"r30r31", 30}, {"r32r33", 32},            \
    {"r34r35", 34}, {"r36r37", 36}, {"r38r39", 38}, {"r40r41", 40},            \
    {"r42r43", 42}, {"r44r45", 44}, {"r46r47", 46}, {"r48r49", 48},            \
    {"r50r51", 50}, {"r52r53", 52}, {"r54r55", 54}, {"r56r57", 56},            \
    {"r58r59", 58}, {"r60r61", 60}, {"r62r63", 62}, {"r0r1r2r3", 0},           \
    {"r4r5r6r7", 4}, {"r8r9r10r11", 8}, {"r12r13r14r15", 12},                  \
    {"r16r17r18r19", 16}, {"r20r21r22r23", 20}, {"r24r25r26r27", 24},          \
    {"r28r29r30r31", 28}, {"r32r33r34r35", 32}, {"r36r37r38r39", 36},          \
    {"r40r41r42r43", 40}, {"r44r45r46r47", 44}, {"r48r49r50r51", 48},          \
    {"r52r53r54r55", 52}, {"r56r57r58r59", 56}, {"r60r61r62r63", 60},          \
    {"s0", 64}, {"s1", 65}, {"s2", 66}, {"s3", 67}, {"s4", 68}, {"s5", 69},    \
    {"s6", 70}, {"s7", 71}, {"s8", 72}, {"s9", 73}, {"s10", 74},               \
    {"s11", 75}, {"s12", 76}, {"s13", 77}, {"s14", 78}, {"s15", 79},           \
    {"s16", 80}, {"s17", 81}, {"s18", 82}, {"s19", 83}, {"s20", 84},           \
    {"s21", 85}, {"s22", 86}, {"s23", 87}, {"s24", 88}, {"s25", 89},           \
    {"s26", 90}, {"s27", 91}, {"s28", 92}, {"s29", 93}, {"s30", 94},           \
    {"s31", 95}, {"s32", 96}, {"s33", 97}, {"s34", 98}, {"s35", 99},           \
    {"s36", 100}, {"s37", 101}, {"s38", 102}, {"s39", 103}, {"s40", 104},      \
    {"s41", 105}, {"s42", 106}, {"s43", 107}, {"s44", 108}, {"s45", 109},      \
    {"s46", 110}, {"s47", 111}, {"s48", 112}, {"s49", 113}, {"s50", 114},      \
    {"s51", 115}, {"s52", 116}, {"s53", 117}, {"s54", 118}, {"s55", 119},      \
    {"s56", 120}, {"s57", 121}, {"s58", 122}, {"s59", 123}, {"s60", 124},      \
    {"s61", 125}, {"s62", 126}, {"s63", 127}, {"sp", 12},                      \
  }

#define ASM_OUTPUT_OPCODE(STREAM, PTR) \
  (PTR = lvx_asm_output_opcode (STREAM, PTR))

#define FINAL_PRESCAN_INSN(INSN, OPVEC, NOPERANDS) \
  lvx_final_prescan_insn (INSN)

#define REGISTER_PREFIX "$"

/* Local compiler-generated symbols must have a prefix that the assembler
   understands.   By default, this is $, although some targets (e.g.,
   NetBSD-ELF) need to override this.  */

#define LOCAL_LABEL_PREFIX "."

/* 18.20.7 Output of Assembler Instructions }}} */

/* 18.20.8 Output of Dispatch Tables {{{ */

/* Output a relative address table.  */
#define ASM_OUTPUT_ADDR_DIFF_ELT(STREAM, BODY, VALUE, REL)                     \
  do                                                                           \
    {                                                                          \
      char buf[256];                                                           \
      const char *asm_type = (GET_MODE (BODY) == SImode) ? ".word" : ".long";  \
      gcc_assert (flag_pic);                                                   \
      gcc_assert ((GET_MODE (BODY) == DImode) || (GET_MODE (BODY) == SImode)); \
      asm_fprintf ((STREAM), "\t%s ", asm_type);                               \
      ASM_GENERATE_INTERNAL_LABEL (buf, "L", VALUE);                           \
      assemble_name ((STREAM), buf);                                           \
      asm_fprintf ((STREAM), "-");                                             \
      ASM_GENERATE_INTERNAL_LABEL (buf, "L", REL);                             \
      assemble_name ((STREAM), buf);                                           \
      asm_fprintf ((STREAM), "\n");                                            \
    }                                                                          \
  while (0)

#define ASM_OUTPUT_ADDR_VEC_ELT(FILE, VALUE)                                   \
  do                                                                           \
    {                                                                          \
      char label[256];                                                         \
      ASM_GENERATE_INTERNAL_LABEL (label, "L", (VALUE));                       \
      fprintf (FILE, "%s", integer_asm_op (GET_MODE_SIZE (Pmode), TRUE));      \
      assemble_name (FILE, label);                                             \
      fprintf (FILE, "\n");                                                    \
    }                                                                          \
  while (0)

/* 18.20.8 Output of Dispatch Tables }}} */

/* 18.20.9 Assembler Commands for Exception Regions {{{ */

#define DWARF2_UNWIND_INFO 1

/* 18.20.9 Assembler Commands for Exception Regions }}} */

/* 18.20.10 Assembler Commands for Alignment {{{ */

#define ASM_OUTPUT_ALIGN(FILE, LOG)                                            \
  fprintf (FILE, "\n\t.align %d\n", 1 << (LOG))

/* 18.20.10 Assembler Commands for Alignment }}} */

/* 18.20 Defining the Output Assembler Language }}} */


/* 18.21 Controlling Debugging Information Format {{{ */

/* 18.21.1 Macros Affecting All Debugging Formats {{{ */

#define DBX_REGISTER_NUMBER(REGNO) (REGNO)

/* 18.21.1 Macros Affecting All Debugging Formats }}} */


/* 18.21.5 Macros for DWARF Output {{{ */

#define DWARF2_ASM_LINE_DEBUG_INFO 1

/* 18.21.5 Macros for DWARF Output }}} */

/* 18.21 Controlling Debugging Information Format }}} */


/* 18.31 Miscellaneous Parameters {{{ */

/* Set MAX_CONDITIONAL_EXECUTE to 2*BRANCH_COST */

/* When doing PIC jumptable, stores relative address as 32bits.  */
#define CASE_VECTOR_MODE \
  ((CASE_VECTOR_PC_RELATIVE || flag_pic) ? SImode : Pmode)

/* Define this macro to 1 if operations between registers with integral mode
   smaller than a word are always performed on the entire register.  */
#define WORD_REGISTER_OPERATIONS 1

/* Define this macro to be a C expression indicating when insns that read memory
   in mem mode, an integral mode narrower than a word, set the bits outside of
   mem mode to be either the sign-extension or the zero-extension of the data
   read.  */
#define LOAD_EXTEND_OP(MODE) ZERO_EXTEND

/* Define this macro to 1 if loading short immediate values into registers sign
   extends.  */
#define SHORT_IMMEDIATES_SIGN_EXTEND 1

/* The maximum number of bytes that a single instruction can move quickly from
   memory to memory.  */
#define MOVE_MAX 16

/* A C expression that is nonzero if on this machine the number of bits actually
   used for the count of a shift operation is equal to the number of bits needed
   to represent the size of the object being shifted.  */
#define SHIFT_COUNT_TRUNCATED 1

#define STORE_FLAG_VALUE 1

/* A C expression that indicates whether the architecture defines a
   value for clz or ctz with a zero operand.  */
#define CLZ_DEFINED_VALUE_AT_ZERO(mode, value)                                 \
  (mode == SImode ? (value = 32, 1) : (mode == DImode ? (value = 64, 1) : 0))

#define CTZ_DEFINED_VALUE_AT_ZERO(mode, value)                                 \
  CLZ_DEFINED_VALUE_AT_ZERO (mode, value)

/* An alias for the machine mode for pointers.  */
#define Pmode DImode

/* An alias for the machine mode used for memory references to
   functions being called, in call RTL expressions. On most CISC
   machines, where an instruction can begin at any byte address, this
   should be QImode. On most RISC machines, where all instructions
   have fixed size and alignment, this should be a mode with the same
   size and alignment as the machine instruction words - typically
   SImode or HImode. */
#define FUNCTION_MODE Pmode

/* The __bypass, __preload and __speculate address spaces refers to the same
 * memory but direct load instructions to use a variant modifier.  */
#define LVX_ADDR_SPACE_BYPASS 1
#define LVX_ADDR_SPACE_PRELOAD 2
#define LVX_ADDR_SPACE_SPECULATE 3
#define LVX_ADDR_SPACE_CONVERT 4
#define LVX_ADDR_SPACE_SYSCALL 5

#define REGISTER_TARGET_PRAGMAS()                                              \
  do                                                                           \
    {                                                                          \
      c_register_addr_space ("__bypass", LVX_ADDR_SPACE_BYPASS);               \
      c_register_addr_space ("__preload", LVX_ADDR_SPACE_PRELOAD);             \
      c_register_addr_space ("__speculate", LVX_ADDR_SPACE_SPECULATE);         \
      c_register_addr_space ("__convert", LVX_ADDR_SPACE_CONVERT);             \
      /* The __syscall address space is for internal use only.  */             \
    }                                                                          \
  while (0)

#define MAX_CONDITIONAL_EXECUTE 12

#define IFCVT_MODIFY_TESTS(CE_INFO, TRUE_EXPR, FALSE_EXPR)                     \
  lvx_ifcvt_modify_tests (CE_INFO, TRUE_EXPR, FALSE_EXPR,                      \
			  then_start, then_end, else_start, else_end)

#define IFCVT_MODIFY_INSN(CE_INFO, PATTERN, INSN)                              \
  ((PATTERN) = lvx_ifcvt_modify_insn (CE_INFO, PATTERN, INSN))

#define IFCVT_MACHDEP_INIT(ce_info)                                            \
  (lvx_ifcvt_machdep_init (ce_info, ifcvt_after_combine))

#define TARGET_SUPPORTS_WIDE_INT 1

/* 18.31 Miscellaneous Parameters }}} */

/* FIXME: Not in the manual.  */
#define TARGET_SUPPORTS_COMPLEX_TO_VECTOR 1

/* FIXME: Is this really used?  */
/* Which ABI to use?  */
enum lvx_abi_type
{
  LVX_ABI_LP64 = 0
};

#ifndef LVX_ABI_DEFAULT
#define LVX_ABI_DEFAULT LVX_ABI_LP64
#endif

/* Utils {{{ */

#define SYMBOLIC_CONST(X)                                                      \
  (GET_CODE (X) == SYMBOL_REF || GET_CODE (X) == LABEL_REF                     \
   || (GET_CODE (X) == CONST && symbolic_reference_mentioned_p (X)))


/* Constraints {{{ */

#define SIGNED_INT_FITS_N_BITS(imm, N)                                         \
  __extension__({                                                              \
    HOST_WIDE_INT __i = (imm);                                                 \
    ((HOST_WIDE_INT) (__i) < ((HOST_WIDE_INT) 1 << ((N) -1))                   \
   && (HOST_WIDE_INT) (__i) >= -((HOST_WIDE_INT) 1 << ((N) -1)));              \
 })

#define UNSIGNED_INT_FITS_N_BITS(imm, N)                                       \
  __extension__({                                                              \
    HOST_WIDE_INT __i = (imm);                                                 \
    ((unsigned HOST_WIDE_INT) (__i) < ((unsigned HOST_WIDE_INT) 1 << (N)));    \
 })

/* Constraints }}} */

#define REG_SUBREG_P(exp)                                                      \
  __extension__({                                                              \
    rtx __x = (exp);                                                           \
    (REG_P (__x) || (GET_CODE (__x) == SUBREG && REG_P (SUBREG_REG (__x))));   \
 })


#define lvx_strict_to_nonstrict_comparison_operator(code)                      \
  __extension__({                                                              \
    enum rtx_code comp;                                                        \
                                                                               \
    switch (code)                                                              \
      {                                                                        \
      case GT:                                                                 \
	comp = GE;                                                             \
	break;                                                                 \
      case GE:                                                                 \
	comp = GT;                                                             \
	break;                                                                 \
      case GTU:                                                                \
	comp = GEU;                                                            \
	break;                                                                 \
      case GEU:                                                                \
	comp = GTU;                                                            \
	break;                                                                 \
      case LT:                                                                 \
	comp = LE;                                                             \
	break;                                                                 \
      case LE:                                                                 \
	comp = LT;                                                             \
	break;                                                                 \
      case LTU:                                                                \
	comp = LEU;                                                            \
	break;                                                                 \
      case LEU:                                                                \
	comp = LTU;                                                            \
	break;                                                                 \
      default:                                                                 \
	gcc_unreachable ();                                                    \
      }                                                                        \
                                                                               \
    comp;                                                                      \
  })


/* Utils }}} */


#ifndef IN_LIBGCC2

extern GTY (()) rtx lvx_link_reg_rtx;

extern GTY (()) rtx lvx_divmod_zero;

enum lvx_ifcvt_ce
{
  LVX_IFCVT_,
  LVX_IFCVT_CE1,
  LVX_IFCVT_CE2,
  LVX_IFCVT_CE3
};

extern GTY (()) enum lvx_ifcvt_ce lvx_ifcvt_ce_level;

#endif
