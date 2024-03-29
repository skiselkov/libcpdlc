/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_LatitudeLongitude_H_
#define	_LatitudeLongitude_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Latitude.h"
#include "Longitude.h"
#include <constr_SEQUENCE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* LatitudeLongitude */
typedef struct LatitudeLongitude {
	Latitude_t	 latitude;
	Longitude_t	 longitude;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} LatitudeLongitude_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_LatitudeLongitude;

#ifdef __cplusplus
}
#endif

#endif	/* _LatitudeLongitude_H_ */
#include <asn_internal.h>
