/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#include "Predepartureclearance.h"

static asn_TYPE_member_t asn_MBR_Predepartureclearance_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct Predepartureclearance, aircraftflightidentification),
		(ASN_TAG_CLASS_CONTEXT | (0 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Aircraftflightidentification,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"aircraftflightidentification"
		},
	{ ATF_POINTER, 2, offsetof(struct Predepartureclearance, aircrafttype),
		(ASN_TAG_CLASS_CONTEXT | (1 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Aircrafttype,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"aircrafttype"
		},
	{ ATF_POINTER, 1, offsetof(struct Predepartureclearance, aircraftequipmentcode),
		(ASN_TAG_CLASS_CONTEXT | (2 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Aircraftequipmentcode,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"aircraftequipmentcode"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Predepartureclearance, timedepartureedct),
		(ASN_TAG_CLASS_CONTEXT | (3 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Timedepartureedct,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"timedepartureedct"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Predepartureclearance, routeclearance),
		(ASN_TAG_CLASS_CONTEXT | (4 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Routeclearance,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"routeclearance"
		},
	{ ATF_POINTER, 1, offsetof(struct Predepartureclearance, altituderestriction),
		(ASN_TAG_CLASS_CONTEXT | (5 << 2)),
		+1,	/* EXPLICIT tag at current level */
		&asn_DEF_Altituderestriction,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"altituderestriction"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Predepartureclearance, frequencydeparture),
		(ASN_TAG_CLASS_CONTEXT | (6 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Frequencydeparture,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"frequencydeparture"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Predepartureclearance, beaconcode),
		(ASN_TAG_CLASS_CONTEXT | (7 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_Beaconcode,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"beaconcode"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Predepartureclearance, pDCrevision),
		(ASN_TAG_CLASS_CONTEXT | (8 << 2)),
		-1,	/* IMPLICIT tag at current level */
		&asn_DEF_PDCrevision,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"pDCrevision"
		},
};
static const int asn_MAP_Predepartureclearance_oms_1[] = { 1, 2, 5 };
static const ber_tlv_tag_t asn_DEF_Predepartureclearance_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_Predepartureclearance_tag2el_1[] = {
    { (ASN_TAG_CLASS_CONTEXT | (0 << 2)), 0, 0, 0 }, /* aircraftflightidentification */
    { (ASN_TAG_CLASS_CONTEXT | (1 << 2)), 1, 0, 0 }, /* aircrafttype */
    { (ASN_TAG_CLASS_CONTEXT | (2 << 2)), 2, 0, 0 }, /* aircraftequipmentcode */
    { (ASN_TAG_CLASS_CONTEXT | (3 << 2)), 3, 0, 0 }, /* timedepartureedct */
    { (ASN_TAG_CLASS_CONTEXT | (4 << 2)), 4, 0, 0 }, /* routeclearance */
    { (ASN_TAG_CLASS_CONTEXT | (5 << 2)), 5, 0, 0 }, /* altituderestriction */
    { (ASN_TAG_CLASS_CONTEXT | (6 << 2)), 6, 0, 0 }, /* frequencydeparture */
    { (ASN_TAG_CLASS_CONTEXT | (7 << 2)), 7, 0, 0 }, /* beaconcode */
    { (ASN_TAG_CLASS_CONTEXT | (8 << 2)), 8, 0, 0 } /* pDCrevision */
};
static asn_SEQUENCE_specifics_t asn_SPC_Predepartureclearance_specs_1 = {
	sizeof(struct Predepartureclearance),
	offsetof(struct Predepartureclearance, _asn_ctx),
	asn_MAP_Predepartureclearance_tag2el_1,
	9,	/* Count of tags in the map */
	asn_MAP_Predepartureclearance_oms_1,	/* Optional members */
	3, 0,	/* Root/Additions */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_Predepartureclearance = {
	"Predepartureclearance",
	"Predepartureclearance",
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
	asn_DEF_Predepartureclearance_tags_1,
	sizeof(asn_DEF_Predepartureclearance_tags_1)
		/sizeof(asn_DEF_Predepartureclearance_tags_1[0]), /* 1 */
	asn_DEF_Predepartureclearance_tags_1,	/* Same as above */
	sizeof(asn_DEF_Predepartureclearance_tags_1)
		/sizeof(asn_DEF_Predepartureclearance_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_Predepartureclearance_1,
	9,	/* Elements count */
	&asn_SPC_Predepartureclearance_specs_1	/* Additional specs */
};

