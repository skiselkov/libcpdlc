/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#include "UM61PositionAltitudeSpeed.h"

int
UM61PositionAltitudeSpeed_constraint(asn_TYPE_descriptor_t *td, const void *sptr,
			asn_app_constraint_failed_f *ctfailcb, void *app_key) {
	/* Replace with underlying type checker */
	td->check_constraints = asn_DEF_PositionAltitudeSpeed.check_constraints;
	return td->check_constraints(td, sptr, ctfailcb, app_key);
}

/*
 * This type is implemented using PositionAltitudeSpeed,
 * so here we adjust the DEF accordingly.
 */
static void
UM61PositionAltitudeSpeed_1_inherit_TYPE_descriptor(asn_TYPE_descriptor_t *td) {
	td->free_struct    = asn_DEF_PositionAltitudeSpeed.free_struct;
	td->print_struct   = asn_DEF_PositionAltitudeSpeed.print_struct;
	td->check_constraints = asn_DEF_PositionAltitudeSpeed.check_constraints;
	td->ber_decoder    = asn_DEF_PositionAltitudeSpeed.ber_decoder;
	td->der_encoder    = asn_DEF_PositionAltitudeSpeed.der_encoder;
	td->xer_decoder    = asn_DEF_PositionAltitudeSpeed.xer_decoder;
	td->xer_encoder    = asn_DEF_PositionAltitudeSpeed.xer_encoder;
	td->uper_decoder   = asn_DEF_PositionAltitudeSpeed.uper_decoder;
	td->uper_encoder   = asn_DEF_PositionAltitudeSpeed.uper_encoder;
	if(!td->per_constraints)
		td->per_constraints = asn_DEF_PositionAltitudeSpeed.per_constraints;
	td->elements       = asn_DEF_PositionAltitudeSpeed.elements;
	td->elements_count = asn_DEF_PositionAltitudeSpeed.elements_count;
	td->specifics      = asn_DEF_PositionAltitudeSpeed.specifics;
}

void
UM61PositionAltitudeSpeed_free(asn_TYPE_descriptor_t *td,
		void *struct_ptr, int contents_only) {
	UM61PositionAltitudeSpeed_1_inherit_TYPE_descriptor(td);
	td->free_struct(td, struct_ptr, contents_only);
}

int
UM61PositionAltitudeSpeed_print(asn_TYPE_descriptor_t *td, const void *struct_ptr,
		int ilevel, asn_app_consume_bytes_f *cb, void *app_key) {
	UM61PositionAltitudeSpeed_1_inherit_TYPE_descriptor(td);
	return td->print_struct(td, struct_ptr, ilevel, cb, app_key);
}

asn_dec_rval_t
UM61PositionAltitudeSpeed_decode_ber(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		void **structure, const void *bufptr, size_t size, int tag_mode) {
	UM61PositionAltitudeSpeed_1_inherit_TYPE_descriptor(td);
	return td->ber_decoder(opt_codec_ctx, td, structure, bufptr, size, tag_mode);
}

asn_enc_rval_t
UM61PositionAltitudeSpeed_encode_der(asn_TYPE_descriptor_t *td,
		void *structure, int tag_mode, ber_tlv_tag_t tag,
		asn_app_consume_bytes_f *cb, void *app_key) {
	UM61PositionAltitudeSpeed_1_inherit_TYPE_descriptor(td);
	return td->der_encoder(td, structure, tag_mode, tag, cb, app_key);
}

asn_dec_rval_t
UM61PositionAltitudeSpeed_decode_xer(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		void **structure, const char *opt_mname, const void *bufptr, size_t size) {
	UM61PositionAltitudeSpeed_1_inherit_TYPE_descriptor(td);
	return td->xer_decoder(opt_codec_ctx, td, structure, opt_mname, bufptr, size);
}

asn_enc_rval_t
UM61PositionAltitudeSpeed_encode_xer(asn_TYPE_descriptor_t *td, void *structure,
		int ilevel, enum xer_encoder_flags_e flags,
		asn_app_consume_bytes_f *cb, void *app_key) {
	UM61PositionAltitudeSpeed_1_inherit_TYPE_descriptor(td);
	return td->xer_encoder(td, structure, ilevel, flags, cb, app_key);
}

asn_dec_rval_t
UM61PositionAltitudeSpeed_decode_uper(asn_codec_ctx_t *opt_codec_ctx, asn_TYPE_descriptor_t *td,
		asn_per_constraints_t *constraints, void **structure, asn_per_data_t *per_data) {
	UM61PositionAltitudeSpeed_1_inherit_TYPE_descriptor(td);
	return td->uper_decoder(opt_codec_ctx, td, constraints, structure, per_data);
}

asn_enc_rval_t
UM61PositionAltitudeSpeed_encode_uper(asn_TYPE_descriptor_t *td,
		asn_per_constraints_t *constraints,
		void *structure, asn_per_outp_t *per_out) {
	UM61PositionAltitudeSpeed_1_inherit_TYPE_descriptor(td);
	return td->uper_encoder(td, constraints, structure, per_out);
}

static const ber_tlv_tag_t asn_DEF_UM61PositionAltitudeSpeed_tags_1[] = {
	(ASN_TAG_CLASS_UNIVERSAL | (16 << 2))
};
asn_TYPE_descriptor_t asn_DEF_UM61PositionAltitudeSpeed = {
	"UM61PositionAltitudeSpeed",
	"UM61PositionAltitudeSpeed",
	UM61PositionAltitudeSpeed_free,
	UM61PositionAltitudeSpeed_print,
	UM61PositionAltitudeSpeed_constraint,
	UM61PositionAltitudeSpeed_decode_ber,
	UM61PositionAltitudeSpeed_encode_der,
	UM61PositionAltitudeSpeed_decode_xer,
	UM61PositionAltitudeSpeed_encode_xer,
	UM61PositionAltitudeSpeed_decode_uper,
	UM61PositionAltitudeSpeed_encode_uper,
	0,	/* Use generic outmost tag fetcher */
	asn_DEF_UM61PositionAltitudeSpeed_tags_1,
	sizeof(asn_DEF_UM61PositionAltitudeSpeed_tags_1)
		/sizeof(asn_DEF_UM61PositionAltitudeSpeed_tags_1[0]), /* 1 */
	asn_DEF_UM61PositionAltitudeSpeed_tags_1,	/* Same as above */
	sizeof(asn_DEF_UM61PositionAltitudeSpeed_tags_1)
		/sizeof(asn_DEF_UM61PositionAltitudeSpeed_tags_1[0]), /* 1 */
	0,	/* No PER visible constraints */
	0, 0,	/* Defined elsewhere */
	0	/* No specifics */
};

