/*
 *
 * Copyright 2019 AT&T Intellectual Property
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "X2AP-IEs"
 * 	found in "../../asnFiles/X2AP-IEs.asn"
 * 	`asn1c -fcompound-names -fincludes-quoted -fno-include-deps -findirect-choice -gen-PER -no-gen-OER -D.`
 */

#ifndef	_RelativeNarrowbandTxPower_H_
#define	_RelativeNarrowbandTxPower_H_


#include "asn_application.h"

/* Including external dependencies */
#include "BIT_STRING.h"
#include "RNTP-Threshold.h"
#include "NativeEnumerated.h"
#include "NativeInteger.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum RelativeNarrowbandTxPower__numberOfCellSpecificAntennaPorts {
	RelativeNarrowbandTxPower__numberOfCellSpecificAntennaPorts_one	= 0,
	RelativeNarrowbandTxPower__numberOfCellSpecificAntennaPorts_two	= 1,
	RelativeNarrowbandTxPower__numberOfCellSpecificAntennaPorts_four	= 2
	/*
	 * Enumeration is extensible
	 */
} e_RelativeNarrowbandTxPower__numberOfCellSpecificAntennaPorts;

/* Forward declarations */
struct ProtocolExtensionContainer;

/* RelativeNarrowbandTxPower */
typedef struct RelativeNarrowbandTxPower {
	BIT_STRING_t	 rNTP_PerPRB;
	RNTP_Threshold_t	 rNTP_Threshold;
	long	 numberOfCellSpecificAntennaPorts;
	long	 p_B;
	long	 pDCCH_InterferenceImpact;
	struct ProtocolExtensionContainer	*iE_Extensions;	/* OPTIONAL */
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} RelativeNarrowbandTxPower_t;

/* Implementation */
/* extern asn_TYPE_descriptor_t asn_DEF_numberOfCellSpecificAntennaPorts_4;	// (Use -fall-defs-global to expose) */
extern asn_TYPE_descriptor_t asn_DEF_RelativeNarrowbandTxPower;
extern asn_SEQUENCE_specifics_t asn_SPC_RelativeNarrowbandTxPower_specs_1;
extern asn_TYPE_member_t asn_MBR_RelativeNarrowbandTxPower_1[6];

#ifdef __cplusplus
}
#endif

#endif	/* _RelativeNarrowbandTxPower_H_ */
#include "asn_internal.h"
