/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Timetolerance_H_
#define	_Timetolerance_H_


#include <asn_application.h>

/* Including external dependencies */
#include <NativeEnumerated.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum Timetolerance {
	Timetolerance_at	= 0,
	Timetolerance_atorafter	= 1,
	Timetolerance_atorbefore	= 2
} e_Timetolerance;

/* Timetolerance */
typedef long	 Timetolerance_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Timetolerance;
asn_struct_free_f Timetolerance_free;
asn_struct_print_f Timetolerance_print;
asn_constr_check_f Timetolerance_constraint;
ber_type_decoder_f Timetolerance_decode_ber;
der_type_encoder_f Timetolerance_encode_der;
xer_type_decoder_f Timetolerance_decode_xer;
xer_type_encoder_f Timetolerance_encode_xer;
per_type_decoder_f Timetolerance_decode_uper;
per_type_encoder_f Timetolerance_encode_uper;

#ifdef __cplusplus
}
#endif

#endif	/* _Timetolerance_H_ */
#include <asn_internal.h>
