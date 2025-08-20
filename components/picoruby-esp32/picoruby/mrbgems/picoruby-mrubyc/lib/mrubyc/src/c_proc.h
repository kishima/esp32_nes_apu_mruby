/*! @file
  @brief
  mruby/c Proc class

  <pre>
  Copyright (C) 2015- Kyushu Institute of Technology.
  Copyright (C) 2015- Shimane IT Open-Innovation Center.

  This file is distributed under BSD 3-Clause License.

  </pre>
*/

#ifndef MRBC_SRC_C_PROC_H_
#define MRBC_SRC_C_PROC_H_

/***** Feature test switches ************************************************/
/***** System headers *******************************************************/
//@cond
#include "vm_config.h"
#include <stdint.h>
//@endcond

/***** Local headers ********************************************************/
#include "value.h"

#ifdef __cplusplus
extern "C" {
#endif

/***** Constat values *******************************************************/
/***** Macros ***************************************************************/
/***** Typedefs *************************************************************/
//================================================================
/*!@brief
  Proc object.

  @extends RBasic
*/
typedef struct RProc {
  MRBC_OBJECT_HEADER;

  uint8_t block_or_method;
  struct CALLINFO *callinfo;
  struct CALLINFO *callinfo_self;
  struct IREP *irep;
  mrbc_value self;
  mrbc_value ret_val;

} mrbc_proc;
typedef struct RProc mrb_proc;


/***** Global variables *****************************************************/
/***** Function prototypes **************************************************/
//@cond
mrbc_value mrbc_proc_new(struct VM *vm, void *irep, uint8_t b_or_m);
void mrbc_proc_delete(mrbc_value *val);
void mrbc_proc_clear_vm_id(mrbc_value *v);
//@endcond


/***** Inline functions *****************************************************/


#ifdef __cplusplus
}
#endif
#endif
