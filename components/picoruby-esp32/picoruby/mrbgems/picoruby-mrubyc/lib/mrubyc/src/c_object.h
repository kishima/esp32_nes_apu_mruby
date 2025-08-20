/*! @file
  @brief
  Object, Nil, True and False class.

  <pre>
  Copyright (C) 2015- Kyushu Institute of Technology.
  Copyright (C) 2015- Shimane IT Open-Innovation Center.

  This file is distributed under BSD 3-Clause License.

  </pre>
*/

#ifndef MRBC_SRC_OBJECT_H_
#define MRBC_SRC_OBJECT_H_

/***** Feature test switches ************************************************/
/***** System headers *******************************************************/
/***** Local headers ********************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/***** Constant values ******************************************************/
/***** Macros ***************************************************************/
/***** Typedefs *************************************************************/
/***** Global variables *****************************************************/
/***** Function prototypes **************************************************/
//@cond
struct VM;
struct RObject;

void mrbc_instance_call_initialize(struct VM *vm, struct RObject v[], int argc);
//@endcond


/***** Inline functions *****************************************************/


#ifdef __cplusplus
}
#endif
#endif
