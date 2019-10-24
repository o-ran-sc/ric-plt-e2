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

#include "CellInformation-Item.h"

#include "UL-InterferenceOverloadIndication.h"
#include "UL-HighInterferenceIndicationInfo.h"
#include "RelativeNarrowbandTxPower.h"
#include "ProtocolExtensionContainer.h"
static asn_TYPE_member_t asn_MBR_CellInformation_Item_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct CellInformation_Item, cell_ID),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ECGI,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"cell-ID"
		},
	{ ATF_POINTER, 4, offsetof(struct CellInformation_Item, ul_InterferenceOverloadIndication),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_UL_InterferenceOverloadIndication,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"ul-InterferenceOverloadIndication"
		},
	{ ATF_POINTER, 3, offsetof(struct CellInformation_Item, ul_HighInterferenceIndicationInfo),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_UL_HighInterferenceIndicationInfo,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"ul-HighInterferenceIndicationInfo"
		},
	{ ATF_POINTER, 2, offsetof(struct CellInformation_Item, relativeNarrowbandTxPower),
		(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_RelativeNarrowbandTxPower,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"relativeNarrowbandTxPower"
		},
	{ ATF_POINTER, 1, offsetof(struct CellInformation_Item, iE_Extensions),
		(ASN_TAG_CLASS_CONTEXT | (4 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ProtocolExtensionContainer_170P7,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"iE-Extensions"
		},
};
static const int asn_MAP_CellInformation_Item_oms_1[] = { 1, 2, 3, 4 };
static const ber_tlv_tag_t asn_DEF_CellInformation_Item_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_CellInformation_Item_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* cell-ID */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* ul-InterferenceOverloadIndication */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 }, /* ul-HighInterferenceIndicationInfo */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 3, 0, 0 }, /* relativeNarrowbandTxPower */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 4, 0, 0 } /* iE-Extensions */
};
static asn_SEQUENCE_specifics_t asn_SPC_CellInformation_Item_specs_1 = {
	sizeof(struct CellInformation_Item),
	offsetof(struct CellInformation_Item, _asn_ctx),
	asn_MAP_CellInformation_Item_tag2el_1,
	5,	/* Count of tags in the map */
	asn_MAP_CellInformation_Item_oms_1,	/* Optional members */
	4, 0,	/* Root/Additions */
	5,	/* First extension addition */
};
asn_TYPE_descriptor_t asn_DEF_CellInformation_Item = {
	"CellInformation-Item",
	"CellInformation-Item",
	&asn_OP_SEQUENCE,
	asn_DEF_CellInformation_Item_tags_1,
	sizeof(asn_DEF_CellInformation_Item_tags_1)
		/sizeof(asn_DEF_CellInformation_Item_tags_1[0]), /* 1 */
	asn_DEF_CellInformation_Item_tags_1,	/* Same as above */
	sizeof(asn_DEF_CellInformation_Item_tags_1)
		/sizeof(asn_DEF_CellInformation_Item_tags_1[0]), /* 1 */
	{ 0, 0, SEQUENCE_constraint },
	asn_MBR_CellInformation_Item_1,
	5,	/* Elements count */
	&asn_SPC_CellInformation_Item_specs_1	/* Additional specs */
};

