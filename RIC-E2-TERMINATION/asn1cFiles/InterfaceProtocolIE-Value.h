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
 * From ASN.1 module "E2SM-gNB-X2-IEs"
 * 	found in "../../asnFiles/e2sm-gNB-X2-release-1-v041.asn"
 * 	`asn1c -fcompound-names -fincludes-quoted -fno-include-deps -findirect-choice -gen-PER -no-gen-OER -D.`
 */

#ifndef	_InterfaceProtocolIE_Value_H_
#define	_InterfaceProtocolIE_Value_H_


#include "asn_application.h"

/* Including external dependencies */
#include "NativeInteger.h"
#include "BOOLEAN.h"
#include "BIT_STRING.h"
#include "OCTET_STRING.h"
#include "constr_CHOICE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum InterfaceProtocolIE_Value_PR {
	InterfaceProtocolIE_Value_PR_NOTHING,	/* No components present */
	InterfaceProtocolIE_Value_PR_valueInt,
	InterfaceProtocolIE_Value_PR_valueEnum,
	InterfaceProtocolIE_Value_PR_valueBool,
	InterfaceProtocolIE_Value_PR_valueBitS,
	InterfaceProtocolIE_Value_PR_valueOctS
	/* Extensions may appear below */
	
} InterfaceProtocolIE_Value_PR;

/* InterfaceProtocolIE-Value */
typedef struct InterfaceProtocolIE_Value {
	InterfaceProtocolIE_Value_PR present;
	union InterfaceProtocolIE_Value_u {
		long	 valueInt;
		long	 valueEnum;
		BOOLEAN_t	 valueBool;
		BIT_STRING_t	 valueBitS;
		OCTET_STRING_t	 valueOctS;
		/*
		 * This type is extensible,
		 * possible extensions are below.
		 */
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} InterfaceProtocolIE_Value_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_InterfaceProtocolIE_Value;
extern asn_CHOICE_specifics_t asn_SPC_InterfaceProtocolIE_Value_specs_1;
extern asn_TYPE_member_t asn_MBR_InterfaceProtocolIE_Value_1[5];
extern asn_per_constraints_t asn_PER_type_InterfaceProtocolIE_Value_constr_1;

#ifdef __cplusplus
}
#endif

#endif	/* _InterfaceProtocolIE_Value_H_ */
#include "asn_internal.h"
