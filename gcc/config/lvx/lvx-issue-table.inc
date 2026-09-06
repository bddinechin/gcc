/* The processor resources each issue class occupies.  Generated from the LVX
   Machine Description System by MDS/BE/GCC; DO NOT EDIT.  Indexed by the
   "issue" attribute, whose values are declared in scheduling-isa.md.

   ninsns is how many instructions the class covers.  It cannot be read off
   the vector: BCU2 is ONE instruction occupying both BCUs, not two.  A plain
   class is always one instruction; a composite is the sum of its repeats.  */

struct lvx_issue_resources
{
  unsigned char ninsns, tiny, lite, full, lsu, bcu, ext, auxr, auxw, memw, xfer;
};

static const struct lvx_issue_resources lvx_issue_table[] = {
  /* all                          */ { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* tiny                         */ { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* tiny_x                       */ { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* tiny_x2                      */ { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* lite                         */ { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* lite_x                       */ { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* lite_x2                      */ { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* lite_misc                    */ { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* full                         */ { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
  /* full_x                       */ { 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
  /* bcu_brrp                     */ { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 },
  /* bcu_brrp2                    */ { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0 },
  /* bcu2                         */ { 1, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0 },
  /* bcu_xfer                     */ { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
  /* bcu_xfer_brrp                */ { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 },
  /* bcu2_tiny_lsu                */ { 1, 1, 0, 0, 1, 2, 0, 0, 0, 0, 0 },
  /* lsu                          */ { 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
  /* lsu_x                        */ { 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
  /* lsu_x2                       */ { 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
  /* lsu_memw_accr                */ { 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0 },
  /* lsu_memw_accr_x              */ { 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0 },
  /* lsu_memw_accr_x2             */ { 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0 },
  /* lsu2_memw                    */ { 1, 1, 0, 0, 2, 0, 0, 0, 0, 1, 0 },
  /* lsu2_memw_x                  */ { 1, 1, 0, 0, 2, 0, 0, 0, 0, 1, 0 },
  /* lsu2_memw_x2                 */ { 1, 1, 0, 0, 2, 0, 0, 0, 0, 1, 0 },
  /* lsu_memw_auxr                */ { 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0 },
  /* lsu_memw_auxr_x              */ { 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0 },
  /* lsu_memw_auxr_x2             */ { 1, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0 },
  /* lsu_memw_auxw                */ { 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0 },
  /* lsu_memw_auxw_x              */ { 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0 },
  /* lsu_memw_auxw_x2             */ { 1, 1, 0, 0, 1, 0, 0, 0, 1, 1, 0 },
  /* lsu2_memw_auxw               */ { 1, 1, 0, 0, 2, 0, 0, 0, 1, 1, 0 },
  /* lsu2_memw_auxw_x             */ { 1, 1, 0, 0, 2, 0, 0, 0, 1, 1, 0 },
  /* lsu2_memw_auxw_x2            */ { 1, 1, 0, 0, 2, 0, 0, 0, 1, 1, 0 },
  /* lsu_auxw                     */ { 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0 },
  /* lsu_auxw_x                   */ { 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0 },
  /* lsu_auxw_x2                  */ { 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0 },
  /* lsu_auxr_auxw                */ { 1, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0 },
  /* lsu2_memw_auxr_auxw          */ { 1, 1, 0, 0, 2, 0, 0, 1, 1, 1, 0 },
  /* lsu2_memw_auxr_auxw_x        */ { 1, 1, 0, 0, 2, 0, 0, 1, 1, 1, 0 },
  /* lsu2_memw_auxr_auxw_x2       */ { 1, 1, 0, 0, 2, 0, 0, 1, 1, 1, 0 },
  /* ext                          */ { 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0 },
  /* ext_misc_auxw                */ { 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0 },
  /* lite2                        */ { 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* lite2_misc                   */ { 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* lite2_x2                     */ { 2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* tiny2                        */ { 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* tiny2__lite2                 */ { 4, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* tiny4                        */ { 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* tiny4_x4                     */ { 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  /* tiny__lite                   */ { 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
};
