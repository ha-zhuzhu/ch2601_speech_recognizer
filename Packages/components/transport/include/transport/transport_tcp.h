// Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _TRANSPORT_TCP_H_
#define _TRANSPORT_TCP_H_

#include "transport/transport.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief      Create TCP transport, the transport handle must be release transport_destroy callback
 *
 * @return  the allocated transport_handle_t, or NULL if the handle can not be allocated
 */
transport_handle_t transport_tcp_init();


#ifdef __cplusplus
}
#endif

#endif /* _TRANSPORT_TCP_H_ */
