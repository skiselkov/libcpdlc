/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#include "Procedurename.h"

static asn_TYPE_member_t asn_MBR_Procedurename_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct Procedurename, proceduretype),
		(ASN_TAG_CLASS_UNIVERSAL | (10 << 2)),
		0,
		&asn_DEF_Proceduretype,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"proceduretype"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Procedurename, procedure),
		(ASN_TAG_CLASS_UNIVERSAL | (22 << 2)),
		0,
		&asn_DEF_Procedure,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"procedure"
		},
	{ ATF_POINTER, 1, offsetof(struct Procedurename, proceduretransition),
		(ASN_TAG_CLASS_UNIVERSAL | (22 << 2)),
		0,
		&asn_DEF_Proceduretransition,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"proceduretransition"
		},
};
static const int asn_MAP_Procedurename_oms_1[] = { 2 };
static const ber_tlv_tag_t asn_DEF_Procedurename_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_Procedurename_tag2el_1[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (10 << 2)), 0, 0, 0 }, /* proceduretype */
    { (ASN_TAG_CLASS_UNIVERSAL | (22 << 2)), 1, 0, 1 }, /* procedure */
    { (ASN_TAG_CLASS_UNIVERSAL | (22 << 2)), 2, -1, 0 } /* proceduretransition */
};
static asn_SEQUENCE_specifics_t asn_SPC_Procedurename_specs_1 = {
	sizeof(struct Procedurename),
	offsetof(struct Procedurename, _asn_ctx),
	asn_MAP_Procedurename_tag2el_1,
	3,	/* Count of tags in the map */
	asn_MAP_Procedurename_oms_1,	/* Optional members */
	1, 0,	/* Root/Additions */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_Procedurename = {
	"Procedurename",
	"Procedurename",
	SEQUENCE_free,
	SEQUENCE_print,
	SEQUENCE_constraint,
	SEQUENCE_decode_ber,
	SEQUENCE_encode_der,
	SEQUENCE_decode_xer,
	SEQUENCE_encode_xer,
	SEQUENCE_decode_uper,
	SEQUENCE_encode_uper,
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_Procedurename_tags_1,
	sizeof(asn_DEF_Procedurename_tags_1)
		/sizeof(asn_DEF_Procedurename_tags_1[0]), /* 1 */
	asn_DEF_Procedurename_tags_1,	/* Same as above */
	sizeof(asn_DEF_Procedurename_tags_1)
		/sizeof(asn_DEF_Procedurename_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_Procedurename_1,
	3,	/* Elements count */
	&asn_SPC_Procedurename_specs_1	/* Additional specs */
};

