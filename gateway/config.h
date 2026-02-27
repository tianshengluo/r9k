/*
-* SPDX-License-Identifier: MIT
 * Copyright (c) 2025 Varketh Nockrath
 */
#ifndef CONFIG_H_
#define CONFIG_H_

#include <r9k/size.h>

#define GW_SERVER_PORT        26405     /* server port */

#define MAX_EVT         SIZE_KB( 4)     /* max events */
#define MAX_CNT         SIZE_KB( 4)     /* max message content */
#define MAX_RB          SIZE_KB( 8)     /* max read buffer */
#define MAX_WB          SIZE_KB(32)     /* max write buffer */
#define MAX_TLV         MAX_WB          /* max TLV body */

#define MAX_IDL                 15      /* max idle timeout sec */

#endif /* CONFIG_H_ */