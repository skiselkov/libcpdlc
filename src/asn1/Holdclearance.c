/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#include "Holdclearance.h"

static asn_TYPE_member_t asn_MBR_Holdclearance_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct Holdclearance, position),
		-1 /* Ambiguous tag (CHOICE?) */,
		0,
		&asn_DEF_Position,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"position"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Holdclearance, altitude),
		-1 /* Ambiguous tag (CHOICE?) */,
		0,
		&asn_DEF_Altitude,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"altitude"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Holdclearance, degrees),
		-1 /* Ambiguous tag (CHOICE?) */,
		0,
		&asn_DEF_Degrees,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"degrees"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Holdclearance, direction),
		(ASN_TAG_CLASS_UNIVERSAL | (10 << 2)),
		0,
		&asn_DEF_Direction,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"direction"
		},
	{ ATF_POINTER, 1, offsetof(struct Holdclearance, legtype),
		-1 /* Ambiguous tag (CHOICE?) */,
		0,
		&asn_DEF_Legtype,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"legtype"
		},
};
static const int asn_MAP_Holdclearance_oms_1[] = { 4 };
static const ber_tlv_tag_t asn_DEF_Holdclearance_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_Holdclearance_tag2el_1[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (10 << 2)), 3, 0, 0 }, /* direction */
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 3 }, /* fixname */
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 1, -1, 2 }, /* altitudeqnh */
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 2, -2, 1 }, /* degreesmagnetic */
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 4, -3, 0 }, /* legdistance */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 0, 0, 3 }, /* navaid */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, -1, 2 }, /* altitudeqnhmeters */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 2, -2, 1 }, /* degreestrue */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 4, -3, 0 }, /* legtime */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 0, 0, 1 }, /* airport */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 1, -1, 0 }, /* altitudeqfe */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 0, 0, 1 }, /* latitudeLongitude */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 1, -1, 0 }, /* altitudeqfemeters */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 0, 0, 1 }, /* placebearingdistance */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 1, -1, 0 }, /* altitudegnssfeet */
    { (ASN_TAG_CLASS_CONTEXT | (5 << 2)), 1, 0, 0 }, /* altitudegnssmeters */
    { (ASN_TAG_CLASS_CONTEXT | (6 << 2)), 1, 0, 0 }, /* altitudeflightlevel */
    { (ASN_TAG_CLASS_CONTEXT | (7 << 2)), 1, 0, 0 } /* altitudeflightlevelmetric */
};
static asn_SEQUENCE_specifics_t asn_SPC_Holdclearance_specs_1 = {
	sizeof(struct Holdclearance),
	offsetof(struct Holdclearance, _asn_ctx),
	asn_MAP_Holdclearance_tag2el_1,
	18,	/* Count of tags in the map */
	asn_MAP_Holdclearance_oms_1,	/* Optional members */
	1, 0,	/* Root/Additions */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_Holdclearance = {
	"Holdclearance",
	"Holdclearance",
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
	asn_DEF_Holdclearance_tags_1,
	sizeof(asn_DEF_Holdclearance_tags_1)
		/sizeof(asn_DEF_Holdclearance_tags_1[0]), /* 1 */
	asn_DEF_Holdclearance_tags_1,	/* Same as above */
	sizeof(asn_DEF_Holdclearance_tags_1)
		/sizeof(asn_DEF_Holdclearance_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_Holdclearance_1,
	5,	/* Elements count */
	&asn_SPC_Holdclearance_specs_1	/* Additional specs */
};

