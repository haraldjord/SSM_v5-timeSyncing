/*
 * iof_app.h
 *
 *  Created on: 10. feb. 2022
 *      Author: Jon Andreas Kornberg
 */

#ifndef IOF_IOF_APP_H_
#define IOF_IOF_APP_H_

#include "../devices_header/tbr.h"
#include "../devices_header/ublox.h"
#include "../resource_managers_header/lpwan_manager.h"
#include "../devices_header/analog.h"
#include "iof_protocol.h"
#include "../devices_header/display.h"

#include "em_burtc.h"

void iof_app_init( osjob_t *j );

#endif /* IOF_IOF_APP_H_ */
