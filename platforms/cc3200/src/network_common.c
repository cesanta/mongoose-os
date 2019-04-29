// clang-format off
//*****************************************************************************
// network_common.c
//
// Networking common callback for CC3200 device
//
// Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/ 
// 
// 
//  Redistribution and use in source and binary forms, with or without 
//  modification, are permitted provided that the following conditions 
//  are met:
//
//    Redistributions of source code must retain the above copyright 
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the   
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

#include <stdlib.h>

// Simplelink includes 
#include "simplelink.h"


//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- Start
//*****************************************************************************

//*****************************************************************************
//
//! \brief This function serves as first level handler for HTTP GET/POST tokens
//!        It runs under driver context and performs only operation that can run
//!        from this context. For operations that can't is sets an indication of
//!        received token and preempts the provisioning context.
//!
//! \param pSlHttpServerEvent Pointer indicating http server event
//! \param pSlHttpServerResponse Pointer indicating http server response
//!
//! \return None
//!
//*****************************************************************************
_SlEventPropogationStatus_e sl_Provisioning_HttpServerEventHdl(
                            SlHttpServerEvent_t    *apSlHttpServerEvent,
                            SlHttpServerResponse_t *apSlHttpServerResponse)
{
	// Unused in this application
	return EVENT_PROPAGATION_CONTINUE;
}

//*****************************************************************************
//
//! \brief This function serves as first level network application events handler.
//!        It runs under driver context and performs only operation that can run
//!        from this context. For operations that can't is sets an indication of
//!        received token and preempts the provisioning context.
//!
//! \param apEventInfo Pointer to the net app event information
//!
//! \return None
//!
//*****************************************************************************
_SlEventPropogationStatus_e sl_Provisioning_NetAppEventHdl(SlNetAppEvent_t *apNetAppEvent)
{
	// Unused in this application
	return EVENT_PROPAGATION_CONTINUE;
}

//*****************************************************************************
//
//! \brief This function serves as first level WLAN events handler.
//!        It runs under driver context and performs only operation that can run
//!        from this context. For operations that can't is sets an indication of
//!        received token and preempts the provisioning context.
//!
//! \param apEventInfo Pointer to the WLAN event information
//!
//! \return None
//!
//*****************************************************************************
_SlEventPropogationStatus_e sl_Provisioning_WlanEventHdl(SlWlanEvent_t *apEventInfo)
{
	// Unused in this application
	return EVENT_PROPAGATION_CONTINUE;
}

//*****************************************************************************
// SimpleLink Asynchronous Event Handlers -- End
//*****************************************************************************
