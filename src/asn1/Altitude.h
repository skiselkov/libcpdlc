/*
 * Generated by asn1c-0.9.28 (http://lionet.info/asn1c)
 * From ASN.1 module "ATCTwoWayDataLinkCommunications"
 * 	found in "struct.asn"
 * 	`asn1c -gen-PER -fcompound-names`
 */

#ifndef	_Altitude_H_
#define	_Altitude_H_


#include <asn_application.h>

/* Including external dependencies */
#include "Altitudeqnh.h"
#include "Altitudeqnhmeters.h"
#include "Altitudeqfe.h"
#include "Altitudeqfemeters.h"
#include "Altitudegnssfeet.h"
#include "Altitudegnssmeters.h"
#include "Altitudeflightlevel.h"
#include "Altitudeflightlevelmetric.h"
#include <constr_CHOICE.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum Altitude_PR {
	Altitude_PR_NOTHING,	/* No components present */
	Altitude_PR_altitudeqnh,
	Altitude_PR_altitudeqnhmeters,
	Altitude_PR_altitudeqfe,
	Altitude_PR_altitudeqfemeters,
	Altitude_PR_altitudegnssfeet,
	Altitude_PR_altitudegnssmeters,
	Altitude_PR_altitudeflightlevel,
	Altitude_PR_altitudeflightlevelmetric
} Altitude_PR;

/* Altitude */
typedef struct Altitude {
	Altitude_PR present;
	union Altitude_u {
		Altitudeqnh_t	 altitudeqnh;
		Altitudeqnhmeters_t	 altitudeqnhmeters;
		Altitudeqfe_t	 altitudeqfe;
		Altitudeqfemeters_t	 altitudeqfemeters;
		Altitudegnssfeet_t	 altitudegnssfeet;
		Altitudegnssmeters_t	 altitudegnssmeters;
		Altitudeflightlevel_t	 altitudeflightlevel;
		Altitudeflightlevelmetric_t	 altitudeflightlevelmetric;
	} choice;
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} Altitude_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_Altitude;

#ifdef __cplusplus
}
#endif

#endif	/* _Altitude_H_ */
#include <asn_internal.h>
