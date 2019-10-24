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
 * From ASN.1 module "X2AP-PDU-Contents"
 * 	found in "../../asnFiles/X2AP-PDU-Contents.asn"
 * 	`asn1c -fcompound-names -fincludes-quoted -fno-include-deps -findirect-choice -gen-PER -no-gen-OER -D.`
 */

#ifndef	_E_RABs_ToBeAdded_Item_H_
#define	_E_RABs_ToBeAdded_Item_H_


#include "asn_application.h"

/* Including external dependencies */
#include "constr_CHOICE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum E_RABs_ToBeAdded_Item_PR {
	E_RABs_ToBeAdded_Item_PR_NOTHING,	/* No components present */
	E_RABs_ToBeAdded_Item_PR_sCG_Bearer,
	E_RABs_ToBeAdded_Item_PR_split_Bearer
	/* Extensions may appear below */
	
} E_RABs_ToBeAdded_Item_PR;

/* Forward declarations */
struct E_RABs_ToBeAdded_Item_SCG_Bearer;
struct E_RABs_ToBeAdded_Item_Split_Bearer;

/* E-RABs-ToBeAdded-Item */
typedef struct E_RABs_ToBeAdded_Item {
	E_RABs_ToBeAdded_Item_PR present;
	union E_RABs_ToBeAdded_Item_u {
		struct E_RABs_ToBeAdded_Item_SCG_Bearer	*sCG_Bearer;
		struct E_RABs_ToBeAdded_Item_Split_Bearer	*split_Bearer;
		/*
		 * This type is extensible,
		 * possible extensions are below.
		 */
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} E_RABs_ToBeAdded_Item_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_E_RABs_ToBeAdded_Item;

#ifdef __cplusplus
}
#endif

#endif	/* _E_RABs_ToBeAdded_Item_H_ */
#include "asn_internal.h"
