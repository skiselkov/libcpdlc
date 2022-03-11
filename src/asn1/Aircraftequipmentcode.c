/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#include "Aircraftequipmentcode.h"

static int
memb_cOMNAVequipmentstatus_seqOf_constraint_1(asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	size_t size;
	
	if(!sptr) {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: value not given (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
	
	/* Determine the number of elements */
	size = _A_CSEQUENCE_FROM_VOID(sptr)->count;
	
	if((size >= 1 && size <= 16)) {
		/* Perform validation of the inner elements */
		return td->check_constraints(td, sptr, ctfailcb, app_key);
	} else {
		ASN__CTFAIL(app_key, td, sptr,
			"%s: constraint failed (%s:%d)",
			td->name, __FILE__, __LINE__);
		return -1;
	}
}

static asn_per_constraints_t asn_PER_type_cOMNAVequipmentstatus_seqOf_constr_3 GCC_NOTUSED = {
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	{ APC_CONSTRAINED,	 4,  4,  1,  16 }	/* (SIZE(1..16)) */,
	0, 0	/* No PER value map */
};
static asn_per_constraints_t asn_PER_memb_cOMNAVequipmentstatus_seqOf_constr_3 GCC_NOTUSED = {
	{ APC_UNCONSTRAINED,	-1, -1,  0,  0 },
	{ APC_CONSTRAINED,	 4,  4,  1,  16 }	/* (SIZE(1..16)) */,
	0, 0	/* No PER value map */
};
static asn_TYPE_member_t asn_MBR_cOMNAVequipmentstatus_seqOf_3[] = {
	{ ATF_POINTER, 0, 0,
		(ASN_TAG_CLASS_UNIVERSAL | (10 << 2)),
		0,
		&asn_DEF_COMNAVequipmentstatus,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		""
		},
};
static const ber_tlv_tag_t asn_DEF_cOMNAVequipmentstatus_seqOf_tags_3[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static asn_SET_OF_specifics_t asn_SPC_cOMNAVequipmentstatus_seqOf_specs_3 = {
	sizeof(struct Aircraftequipmentcode__cOMNAVequipmentstatus_seqOf),
	offsetof(struct Aircraftequipmentcode__cOMNAVequipmentstatus_seqOf, _asn_ctx),
	1,	/* XER encoding is XMLValueList */
};
static /* Use -fall-defs-global to expose */
asn_TYPE_descriptor_t asn_DEF_cOMNAVequipmentstatus_seqOf_3 = {
	"cOMNAVequipmentstatus-seqOf",
	"cOMNAVequipmentstatus-seqOf",
	SEQUENCE_OF_free,
	SEQUENCE_OF_print,
	SEQUENCE_OF_constraint,
	SEQUENCE_OF_decode_ber,
	SEQUENCE_OF_encode_der,
	SEQUENCE_OF_decode_xer,
	SEQUENCE_OF_encode_xer,
	SEQUENCE_OF_decode_uper,
	SEQUENCE_OF_encode_uper,
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_cOMNAVequipmentstatus_seqOf_tags_3,
	sizeof(asn_DEF_cOMNAVequipmentstatus_seqOf_tags_3)
		/sizeof(asn_DEF_cOMNAVequipmentstatus_seqOf_tags_3[0]), /* 1 */
	asn_DEF_cOMNAVequipmentstatus_seqOf_tags_3,	/* Same as above */
	sizeof(asn_DEF_cOMNAVequipmentstatus_seqOf_tags_3)
		/sizeof(asn_DEF_cOMNAVequipmentstatus_seqOf_tags_3[0]), /* 1 */
	&asn_PER_type_cOMNAVequipmentstatus_seqOf_constr_3,
	asn_MBR_cOMNAVequipmentstatus_seqOf_3,
	1,	/* Single element */
	&asn_SPC_cOMNAVequipmentstatus_seqOf_specs_3	/* Additional specs */
};

static asn_TYPE_member_t asn_MBR_Aircraftequipmentcode_1[] = {
	{ ATF_NOFLAGS, 0, offsetof(struct Aircraftequipmentcode, cOMNAVapproachequipmentavailable),
		(ASN_TAG_CLASS_UNIVERSAL | (1 << 2)),
		0,
		&asn_DEF_COMNAVapproachequipmentavailable,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"cOMNAVapproachequipmentavailable"
		},
	{ ATF_POINTER, 1, offsetof(struct Aircraftequipmentcode, cOMNAVequipmentstatus_seqOf),
		(ASN_TAG_CLASS_UNIVERSAL | (16 << 2)),
		0,
		&asn_DEF_cOMNAVequipmentstatus_seqOf_3,
		memb_cOMNAVequipmentstatus_seqOf_constraint_1,
		&asn_PER_memb_cOMNAVequipmentstatus_seqOf_constr_3,
		0,
		"cOMNAVequipmentstatus-seqOf"
		},
	{ ATF_NOFLAGS, 0, offsetof(struct Aircraftequipmentcode, sSRequipmentavailable),
		(ASN_TAG_CLASS_UNIVERSAL | (10 << 2)),
		0,
		&asn_DEF_SSRequipmentavailable,
		0,	/* Defer constraints checking to the member type */
		0,	/* No PER visible constraints */
		0,
		"sSRequipmentavailable"
		},
};
static const int asn_MAP_Aircraftequipmentcode_oms_1[] = { 1 };
static const ber_tlv_tag_t asn_DEF_Aircraftequipmentcode_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
static const asn_TYPE_tag2member_t asn_MAP_Aircraftequipmentcode_tag2el_1[] = {
    { (ASN_TAG_CLASS_UNIVERSAL | (1 << 2)), 0, 0, 0 }, /* cOMNAVapproachequipmentavailable */
    { (ASN_TAG_CLASS_UNIVERSAL | (10 << 2)), 2, 0, 0 }, /* sSRequipmentavailable */
    { (ASN_TAG_CLASS_UNIVERSAL | (16 << 2)), 1, 0, 0 } /* cOMNAVequipmentstatus-seqOf */
};
static asn_SEQUENCE_specifics_t asn_SPC_Aircraftequipmentcode_specs_1 = {
	sizeof(struct Aircraftequipmentcode),
	offsetof(struct Aircraftequipmentcode, _asn_ctx),
	asn_MAP_Aircraftequipmentcode_tag2el_1,
	3,	/* Count of tags in the map */
	asn_MAP_Aircraftequipmentcode_oms_1,	/* Optional members */
	1, 0,	/* Root/Additions */
	-1,	/* Start extensions */
	-1	/* Stop extensions */
};
asn_TYPE_descriptor_t asn_DEF_Aircraftequipmentcode = {
	"Aircraftequipmentcode",
	"Aircraftequipmentcode",
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
	asn_DEF_Aircraftequipmentcode_tags_1,
	sizeof(asn_DEF_Aircraftequipmentcode_tags_1)
		/sizeof(asn_DEF_Aircraftequipmentcode_tags_1[0]), /* 1 */
	asn_DEF_Aircraftequipmentcode_tags_1,	/* Same as above */
	sizeof(asn_DEF_Aircraftequipmentcode_tags_1)
		/sizeof(asn_DEF_Aircraftequipmentcode_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	asn_MBR_Aircraftequipmentcode_1,
	3,	/* Elements count */
	&asn_SPC_Aircraftequipmentcode_specs_1	/* Additional specs */
};
