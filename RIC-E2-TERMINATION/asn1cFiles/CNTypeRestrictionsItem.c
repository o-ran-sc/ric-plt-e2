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

#include "CNTypeRestrictionsItem.h"

#include "ProtocolExtensionContainer.h"
/*
 * This type is implemented using NativeEnumerated,
 * so here we adjust the DEF accordingly.
 */
static asn_per_constraints_t asn_PER_type_cn_type_constr_3 CC_NOTUSED = {
	{ APC_CONSTRAINED | APC_EXTENSIBLE,  0,  0,  0,  0 }	/* (0..0,...) */,
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	0, 0	/* No PER value map */
};
static const asn_INTEGER_enum_map_t asn_MAP_cn_type_value2enum_3[] = {
	{ 0,	16,	"fiveGC-forbidden" }
	/* This list is extensible */
};
static const unsigned int asn_MAP_cn_type_enum2value_3[] = {
	0	/* fiveGC-forbidden(0) */
	/* This list is extensible */
};
static const asn_INTEGER_specifics_t asn_SPC_cn_type_specs_3 = {
	asn_MAP_cn_type_value2enum_3,	/* "tag" => N; sorted by tag */
	asn_MAP_cn_type_enum2value_3,	/* N => "tag"; sorted by N */
	1,	/* Number of elements in the maps */
	2,	/* Extensions before this member */
	1,	/* Strict enumeration */
	0,	/* Native long size */
	0
};
static const ber_tlv_tag_t asn_DEF_cn_type_tags_3[] = {
	(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
	(ASN_TAG_CLASS_UNIVERSAL | (10 << 2))
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_cn_type_3 = {
	"cn-type",
	"cn-type",
	&asn_OP_NativeEnumerated,
	asn_DEF_cn_type_tags_3,
	sizeof(asn_DEF_cn_type_tags_3)
		/sizeof(asn_DEF_cn_type_tags_3[0]) - 1, /* 1 */
	asn_DEF_cn_type_tags_3,	/* Same as above */
	sizeof(asn_DEF_cn_type_tags_3)
		/sizeof(asn_DEF_cn_type_tags_3[0]), /* 2 */
	{ 0, &asn_PER_type_cn_type_constr_3, NativeEnumerated_constraint },
	0, 0,	/* Defined elsewhere */
	&asn_SPC_cn_type_specs_3	/* Additional specs */
};

asn_TYPE_member_t asn_MBR_CNTypeRestrictionsItem_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct CNTypeRestrictionsItem, plmn_Id),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_PLMN_Identity,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"plmn-Id"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct CNTypeRestrictionsItem, cn_type),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_cn_type_3,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"cn-type"
		},
	{ ATF_POINTER, 1, offsetof(struct CNTypeRestrictionsItem, iE_Extensions),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_ProtocolExtensionContainer_170P116,
		0,
		{ 0, 0, 0 },
		0, 0, /* No default value */
		"iE-Extensions"
		},
};
static const int asn_MAP_CNTypeRestrictionsItem_oms_1[] = { 2 };
static const ber_tlv_tag_t asn_DEF_CNTypeRestrictionsItem_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_CNTypeRestrictionsItem_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* plmn-Id */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* cn-type */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 } /* iE-Extensions */
};
asn_SEQUENCE_specifics_t asn_SPC_CNTypeRestrictionsItem_specs_1 = {
	sizeof(struct CNTypeRestrictionsItem),
	offsetof(struct CNTypeRestrictionsItem, _asn_ctx),
	asn_MAP_CNTypeRestrictionsItem_tag2el_1,
	3,	/* Count of tags in the map */
	asn_MAP_CNTypeRestrictionsItem_oms_1,	/* Optional members */
	1, 0,	/* Root/Additions */
	3,	/* First extension addition */
};
asn_TYPE_descriptor_t asn_DEF_CNTypeRestrictionsItem = {
	"CNTypeRestrictionsItem",
	"CNTypeRestrictionsItem",
	&asn_OP_SEQUENCE,
	asn_DEF_CNTypeRestrictionsItem_tags_1,
	sizeof(asn_DEF_CNTypeRestrictionsItem_tags_1)
		/sizeof(asn_DEF_CNTypeRestrictionsItem_tags_1[0]), /* 1 */
	asn_DEF_CNTypeRestrictionsItem_tags_1,	/* Same as above */
	sizeof(asn_DEF_CNTypeRestrictionsItem_tags_1)
		/sizeof(asn_DEF_CNTypeRestrictionsItem_tags_1[0]), /* 1 */
	{ 0, 0, SEQUENCE_constraint },
	asn_MBR_CNTypeRestrictionsItem_1,
	3,	/* Elements count */
	&asn_SPC_CNTypeRestrictionsItem_specs_1	/* Additional specs */
};

