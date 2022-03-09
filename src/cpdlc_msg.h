/*
 * Copyright 2022 Saso Kiselkov
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef	_LIBCPDLC_MSG_H_
#define	_LIBCPDLC_MSG_H_

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "cpdlc_core.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define	CPDLC_INVALID_MSG_SEQ_NR	UINT32_MAX

typedef enum {
	CPDLC_UM0_UNABLE,
	CPDLC_UM1_STANDBY,
	CPDLC_UM2_REQ_DEFERRED,
	CPDLC_UM3_ROGER,
	CPDLC_UM4_AFFIRM,
	CPDLC_UM5_NEGATIVE,
	CPDLC_UM6_EXPCT_alt,
	CPDLC_UM7_EXPCT_CLB_AT_time,
	CPDLC_UM8_EXPCT_CLB_AT_pos,
	CPDLC_UM9_EXPCT_DES_AT_time,
	CPDLC_UM10_EXPCT_DES_AT_pos,
	CPDLC_UM11_EXPCT_CRZ_CLB_AT_time,
	CPDLC_UM12_EXPCT_CRZ_CLB_AT_pos,
	CPDLC_UM13_AT_time_EXPCT_CLB_TO_alt,
	CPDLC_UM14_AT_pos_EXPCT_CLB_TO_alt,
	CPDLC_UM15_AT_time_EXPCT_DES_TO_alt,
	CPDLC_UM16_AT_pos_EXPCT_DES_TO_alt,
	CPDLC_UM17_AT_time_EXPCT_CRZ_CLB_TO_alt,
	CPDLC_UM18_AT_pos_EXPCT_CRZ_CLB_TO_alt,
	CPDLC_UM19_MAINT_alt,
	CPDLC_UM20_CLB_TO_alt,
	CPDLC_UM21_AT_time_CLB_TO_alt,
	CPDLC_UM22_AT_pos_CLB_TO_alt,
	CPDLC_UM23_DES_TO_alt,
	CPDLC_UM24_AT_time_DES_TO_alt,
	CPDLC_UM25_AT_pos_DES_TO_alt,
	CPDLC_UM26_CLB_TO_REACH_alt_BY_time,
	CPDLC_UM27_CLB_TO_REACH_alt_BY_pos,
	CPDLC_UM28_DES_TO_REACH_alt_BY_time,
	CPDLC_UM29_DES_TO_REACH_alt_BY_pos,
	CPDLC_UM30_MAINT_BLOCK_alt_TO_alt,
	CPDLC_UM31_CLB_TO_MAINT_BLOCK_alt_TO_alt,
	CPDLC_UM32_DES_TO_MAINT_BLOCK_alt_TO_alt,
	CPDLC_UM33_CRZ_alt,
	CPDLC_UM34_CRZ_CLB_TO_alt,
	CPDLC_UM35_CRZ_CLB_ABV_alt,
	CPDLC_UM36_EXPED_CLB_TO_alt,
	CPDLC_UM37_EXPED_DES_TO_alt,
	CPDLC_UM38_IMM_CLB_TO_alt,
	CPDLC_UM39_IMM_DES_TO_alt,
	CPDLC_UM40_IMM_STOP_CLB_AT_alt,
	CPDLC_UM41_IMM_STOP_DES_AT_alt,
	CPDLC_UM42_EXPCT_CROSS_pos_AT_alt,
	CPDLC_UM43_EXPCT_CROSS_pos_AT_alt_OR_ABV,
	CPDLC_UM44_EXPCT_CROSS_pos_AT_alt_OR_BLW,
	CPDLC_UM45_EXPCT_CROSS_pos_AT_AND_MAINT_alt,
	CPDLC_UM46_CROSS_pos_AT_alt,
	CPDLC_UM47_CROSS_pos_AT_alt_OR_ABV,
	CPDLC_UM48_CROSS_pos_AT_alt_OR_BLW,
	CPDLC_UM49_CROSS_pos_AT_AND_MAINT_alt,
	CPDLC_UM50_CROSS_pos_BTWN_alt_AND_alt,
	CPDLC_UM51_CROSS_pos_AT_time,
	CPDLC_UM52_CROSS_pos_AT_OR_BEFORE_time,
	CPDLC_UM53_CROSS_pos_AT_OR_AFTER_time,
	CPDLC_UM54_CROSS_pos_BTWN_time_AND_time,
	CPDLC_UM55_CROSS_pos_AT_spd,
	CPDLC_UM56_CROSS_pos_AT_OR_LESS_spd,
	CPDLC_UM57_CROSS_pos_AT_OR_GREATER_spd,
	CPDLC_UM58_CROSS_pos_AT_time_AT_alt,
	CPDLC_UM59_CROSS_pos_AT_OR_BEFORE_time_AT_alt,
	CPDLC_UM60_CROSS_pos_AT_OR_AFTER_time_AT_alt,
	CPDLC_UM61_CROSS_pos_AT_AND_MAINT_alt_AT_spd,
	CPDLC_UM62_AT_time_CROSS_pos_AT_AND_MAINT_alt,
	CPDLC_UM63_AT_time_CROSS_pos_AT_AND_MAINT_alt_AT_spd,
	CPDLC_UM64_OFFSET_dist_dir_OF_ROUTE,
	CPDLC_UM65_AT_pos_OFFSET_dist_dir_OF_ROUTE,
	CPDLC_UM66_AT_time_OFFSET_dist_dir_OF_ROUTE,
	CPDLC_UM67_PROCEED_BACK_ON_ROUTE,
	CPDLC_UM68_REJOIN_ROUTE_BY_pos,
	CPDLC_UM69_REJOIN_ROUTE_BY_time,
	CPDLC_UM70_EXPCT_BACK_ON_ROUTE_BY_pos,
	CPDLC_UM71_EXPCT_BACK_ON_ROUTE_BY_time,
	CPDLC_UM72_RESUME_OWN_NAV,
	CPDLC_UM73_PDC_predepclx,
	CPDLC_UM74_DIR_TO_pos,
	CPDLC_UM75_WHEN_ABL_DIR_TO_pos,
	CPDLC_UM76_AT_time_DIR_TO_pos,
	CPDLC_UM77_AT_pos_DIR_TO_pos,
	CPDLC_UM78_AT_alt_DIR_TO_pos,
	CPDLC_UM79_CLR_TO_pos_VIA_route,
	CPDLC_UM80_CLR_route,
	CPDLC_UM81_CLR_proc,
	CPDLC_UM82_CLR_DEVIATE_UP_TO_dist_dir_OF_ROUTE,
	CPDLC_UM83_AT_pos_CLR_route,
	CPDLC_UM84_AT_pos_CLR_proc,
	CPDLC_UM85_EXPCT_route,
	CPDLC_UM86_AT_pos_EXPCT_route,
	CPDLC_UM87_EXPCT_DIR_TO_pos,
	CPDLC_UM88_AT_pos_EXPCT_DIR_TO_pos,
	CPDLC_UM89_AT_time_EXPCT_DIR_TO_pos,
	CPDLC_UM90_AT_alt_EXPCT_DIR_TO_pos,
	CPDLC_UM91_HOLD_AT_pos_MAINT_alt_INBD_deg_TURN_dir_LEG_TIME_time,
	CPDLC_UM92_HOLD_AT_pos_AS_PUBLISHED_MAINT_alt,
	CPDLC_UM93_EXPCT_FURTHER_CLR_AT_time,
	CPDLC_UM94_TURN_dir_HDG_deg,
	CPDLC_UM95_TURN_dir_GND_TRK_deg,
	CPDLC_UM96_FLY_PRESENT_HDG,
	CPDLC_UM97_AT_pos_FLY_HDG_deg,
	CPDLC_UM98_IMM_TURN_dir_HDG_deg,
	CPDLC_UM99_EXPCT_proc,
	CPDLC_UM100_AT_time_EXPCT_spd,
	CPDLC_UM101_AT_pos_EXPCT_spd,
	CPDLC_UM102_AT_alt_EXPCT_spd,
	CPDLC_UM103_AT_time_EXPCT_spd_TO_spd,
	CPDLC_UM104_AT_pos_EXPCT_spd_TO_spd,
	CPDLC_UM105_AT_alt_EXPCT_spd_TO_spd,
	CPDLC_UM106_MAINT_spd,
	CPDLC_UM107_MAINT_PRESENT_SPD,
	CPDLC_UM108_MAINT_spd_OR_GREATER,
	CPDLC_UM109_MAINT_spd_OR_LESS,
	CPDLC_UM110_MAINT_spd_TO_spd,
	CPDLC_UM111_INCR_SPD_TO_spd,
	CPDLC_UM112_INCR_SPD_TO_spd_OR_GREATER,
	CPDLC_UM113_RED_SPD_TO_spd,
	CPDLC_UM114_RED_SPD_TO_spd_OR_LESS,
	CPDLC_UM115_DO_NOT_EXCEED_spd,
	CPDLC_UM116_RESUME_NORMAL_SPD,
	CPDLC_UM117_CTC_icaounitname_freq,
	CPDLC_UM118_AT_pos_CONTACT_icaounitname_freq,
	CPDLC_UM119_AT_time_CONTACT_icaounitname_freq,
	CPDLC_UM120_MONITOR_icaounitname_freq,
	CPDLC_UM121_AT_pos_MONITOR_icaounitname_freq,
	CPDLC_UM122_AT_time_MONITOR_icaounitname_freq,
	CPDLC_UM123_SQUAWK_code,
	CPDLC_UM124_STOP_SQUAWK,
	CPDLC_UM125_SQUAWK_ALT,
	CPDLC_UM126_STOP_ALT_SQUAWK,
	CPDLC_UM127_REPORT_BACK_ON_ROUTE,
	CPDLC_UM128_REPORT_LEAVING_alt,
	CPDLC_UM129_REPORT_LEVEL_alt,
	CPDLC_UM130_REPORT_PASSING_pos,
	CPDLC_UM131_REPORT_RMNG_FUEL_SOULS_ON_BOARD,
	CPDLC_UM132_CONFIRM_POSITION,
	CPDLC_UM133_CONFIRM_ALT,
	CPDLC_UM134_CONFIRM_SPD,
	CPDLC_UM135_CONFIRM_ASSIGNED_ALT,
	CPDLC_UM136_CONFIRM_ASSIGNED_SPD,
	CPDLC_UM137_CONFIRM_ASSIGNED_ROUTE,
	CPDLC_UM138_CONFIRM_TIME_OVER_REPORTED_WPT,
	CPDLC_UM139_CONFIRM_REPORTED_WPT,
	CPDLC_UM140_CONFIRM_NEXT_WPT,
	CPDLC_UM141_CONFIRM_NEXT_WPT_ETA,
	CPDLC_UM142_CONFIRM_ENSUING_WPT,
	CPDLC_UM143_CONFIRM_REQ,
	CPDLC_UM144_CONFIRM_SQUAWK,
	CPDLC_UM145_CONFIRM_HDG,
	CPDLC_UM146_CONFIRM_GND_TRK,
	CPDLC_UM147_REQUEST_POS_REPORT,
	CPDLC_UM148_WHEN_CAN_YOU_ACPT_alt,
	CPDLC_UM149_CAN_YOU_ACPT_alt_AT_pos,
	CPDLC_UM150_CAN_YOU_ACPT_alt_AT_time,
	CPDLC_UM151_WHEN_CAN_YOU_ACPT_spd,
	CPDLC_UM152_WHEN_CAN_YOU_ACPT_dist_dir_OFFSET,
	CPDLC_UM153_ALTIMETER_baro,
	CPDLC_UM154_RDR_SVC_TERM,
	CPDLC_UM155_RDR_CTC_pos,
	CPDLC_UM156_RDR_CTC_LOST,
	CPDLC_UM157_CHECK_STUCK_MIC,
	CPDLC_UM158_ATIS_code,
	CPDLC_UM159_ERROR_description,
	CPDLC_UM160_NEXT_DATA_AUTHORITY_id,
	CPDLC_UM161_END_SVC,
	CPDLC_UM162_SVC_UNAVAIL,
	CPDLC_UM163_FACILITY_designation_tp4table,
	CPDLC_UM164_WHEN_RDY,
	CPDLC_UM165_THEN,
	CPDLC_UM166_DUE_TO_TFC,
	CPDLC_UM167_DUE_TO_AIRSPACE_RESTR,
	CPDLC_UM168_DISREGARD,
	CPDLC_UM169_FREETEXT_NORMAL_text,
	CPDLC_UM170_FREETEXT_DISTRESS_text,
	CPDLC_UM171_CLB_AT_vvi_MIN,
	CPDLC_UM172_CLB_AT_vvi_MAX,
	CPDLC_UM173_DES_AT_vvi_MIN,
	CPDLC_UM174_DES_AT_vvi_MAX,
	CPDLC_UM175_REPORT_REACHING_alt,
	CPDLC_UM176_MAINT_OWN_SEPARATION_AND_VMC,
	CPDLC_UM177_AT_PILOTS_DISCRETION,
	CPDLC_UM178_UNUSED,
	CPDLC_UM179_SQUAWK_IDENT,
	CPDLC_UM180_REPORT_REACHING_BLOCK_alt_TO_alt,
	CPDLC_UM181_REPORT_DISTANCE_tofrom_pos,
	CPDLC_UM182_CONFIRM_ATIS_CODE,
	CPDLC_UM183_FREETEXT_NORM_URG_MED_ALERT_text,
	CPDLC_UM187_FREETEXT_LOW_URG_NORM_ALERT_text,
	CPDLC_UM194_FREETEXT_NORM_URG_LOW_ALERT_text,
	CPDLC_UM195_FREETEXT_LOW_URG_LOW_ALERT_text,
	CPDLC_UM196_FREETEXT_NORM_URG_MED_ALERT_text,
	CPDLC_UM197_FREETEXT_HIGH_URG_MED_ALERT_text,
	CPDLC_UM198_FREETEXT_DISTR_URG_HIGH_ALERT_text,
	CPDLC_UM199_FREETEXT_NORM_URG_LOW_ALERT_text,
	CPDLC_UM201_FREETEXT_LOW_URG_LOW_ALERT_text,
	CPDLC_UM202_FREETEXT_LOW_URG_LOW_ALERT_text,
	CPDLC_UM203_FREETEXT_NORM_URG_MED_ALERT_text,
	CPDLC_UM204_FREETEXT_NORM_URG_MED_ALERT_text,
	CPDLC_UM205_FREETEXT_NORM_URG_MED_ALERT_text,
	CPDLC_UM206_FREETEXT_LOW_URG_NORM_ALERT_text,
	CPDLC_UM207_FREETEXT_LOW_URG_LOW_ALERT_text,
	CPDLC_UM208_FREETEXT_LOW_URG_LOW_ALERT_text
} cpdlc_ul_msg_type_t;

typedef enum {
	CPDLC_DM0_WILCO,
	CPDLC_DM1_UNABLE,
	CPDLC_DM2_STANDBY,
	CPDLC_DM3_ROGER,
	CPDLC_DM4_AFFIRM,
	CPDLC_DM5_NEGATIVE,
	CPDLC_DM6_REQ_alt,
	CPDLC_DM7_REQ_BLOCK_alt_TO_alt,
	CPDLC_DM8_REQ_CRZ_CLB_TO_alt,
	CPDLC_DM9_REQ_CLB_TO_alt,
	CPDLC_DM10_REQ_DES_TO_alt,
	CPDLC_DM11_AT_pos_REQ_CLB_TO_alt,
	CPDLC_DM12_AT_pos_REQ_DES_TO_alt,
	CPDLC_DM13_AT_time_REQ_CLB_TO_alt,
	CPDLC_DM14_AT_time_REQ_DES_TO_alt,
	CPDLC_DM15_REQ_OFFSET_dist_dir_OF_ROUTE,
	CPDLC_DM16_AT_pos_REQ_OFFSET_dist_dir_OF_ROUTE,
	CPDLC_DM17_AT_time_REQ_OFFSET_dist_dir_OF_ROUTE,
	CPDLC_DM18_REQ_spd,
	CPDLC_DM19_REQ_spd_TO_spd,
	CPDLC_DM20_REQ_VOICE_CTC,
	CPDLC_DM21_REQ_VOICE_CTC_ON_freq,
	CPDLC_DM22_REQ_DIR_TO_pos,
	CPDLC_DM23_REQ_proc,
	CPDLC_DM24_REQ_route,
	CPDLC_DM25_REQ_PDC,
	CPDLC_DM26_REQ_WX_DEVIATION_TO_pos_VIA_route,
	CPDLC_DM27_REQ_WX_DEVIATION_UP_TO_dist_dir_OF_ROUTE,
	CPDLC_DM28_LEAVING_alt,
	CPDLC_DM29_CLIMBING_TO_alt,
	CPDLC_DM30_DESCENDING_TO_alt,
	CPDLC_DM31_PASSING_pos,
	CPDLC_DM32_PRESENT_ALT_alt,
	CPDLC_DM33_PRESENT_POS_pos,
	CPDLC_DM34_PRESENT_SPD_spd,
	CPDLC_DM35_PRESENT_HDG_deg,
	CPDLC_DM36_PRESENT_GND_TRK_deg,
	CPDLC_DM37_LEVEL_alt,
	CPDLC_DM38_ASSIGNED_ALT_alt,
	CPDLC_DM39_ASSIGNED_SPD_spd,
	CPDLC_DM40_ASSIGNED_ROUTE_route,
	CPDLC_DM41_BACK_ON_ROUTE,
	CPDLC_DM42_NEXT_WPT_pos,
	CPDLC_DM43_NEXT_WPT_ETA_time,
	CPDLC_DM44_ENSUING_WPT_pos,
	CPDLC_DM45_REPORTED_WPT_pos,
	CPDLC_DM46_REPORTED_WPT_time,
	CPDLC_DM47_SQUAWKING_code,
	CPDLC_DM48_POS_REPORT_posreport,
	CPDLC_DM49_WHEN_CAN_WE_EXPCT_spd,
	CPDLC_DM50_WHEN_CAN_WE_EXPCT_spd_TO_spd,
	CPDLC_DM51_WHEN_CAN_WE_EXPCT_BACK_ON_ROUTE,
	CPDLC_DM52_WHEN_CAN_WE_EXPECT_LOWER_ALT,
	CPDLC_DM53_WHEN_CAN_WE_EXPECT_HIGHER_ALT,
	CPDLC_DM54_WHEN_CAN_WE_EXPECT_CRZ_CLB_TO_alt,
	CPDLC_DM55_PAN_PAN_PAN,
	CPDLC_DM56_MAYDAY_MAYDAY_MAYDAY,
	CPDLC_DM57_RMNG_FUEL_AND_POB,
	CPDLC_DM58_CANCEL_EMERG,
	CPDLC_DM59_DIVERTING_TO_pos_VIA_route,
	CPDLC_DM60_OFFSETTING_dist_dir_OF_ROUTE,
	CPDLC_DM61_DESCENDING_TO_alt,
	CPDLC_DM62_ERROR_errorinfo,
	CPDLC_DM63_NOT_CURRENT_DATA_AUTHORITY,
	CPDLC_DM64_CURRENT_DATA_AUTHORITY_id,
	CPDLC_DM65_DUE_TO_WX,
	CPDLC_DM66_DUE_TO_ACFT_PERF,
	CPDLC_DM67_FREETEXT_NORMAL_text,
	CPDLC_DM68_FREETEXT_DISTRESS_text,
	CPDLC_DM69_REQ_VMC_DES,
	CPDLC_DM70_REQ_HDG_deg,
	CPDLC_DM71_REQ_GND_TRK_deg,
	CPDLC_DM72_REACHING_alt,	/* Not to be used according to GOLD */
	CPDLC_DM73_VERSION_number,
	CPDLC_DM74_MAINT_OWN_SEPARATION_AND_VMC,
	CPDLC_DM75_AT_PILOTS_DISCRETION,
	CPDLC_DM76_REACHING_BLOCK_alt_TO_alt,
	CPDLC_DM77_ASSIGNED_BLOCK_alt_TO_alt,
	CPDLC_DM78_AT_time_dist_tofrom_pos,
	CPDLC_DM79_ATIS_code,
	CPDLC_DM80_DEVIATING_dist_dir_OF_ROUTE
} cpdlc_dl_msg_type_t;

typedef enum {
	CPDLC_DM67b_WE_CAN_ACPT_alt_AT_time = 'b',
	CPDLC_DM67c_WE_CAN_ACPT_spd_AT_time = 'c',
	CPDLC_DM67d_WE_CAN_ACPT_dist_dir_AT_time = 'd',
	CPDLC_DM67e_WE_CANNOT_ACPT_alt = 'e',
	CPDLC_DM67f_WE_CANNOT_ACPT_spd = 'f',
	CPDLC_DM67g_WE_CANNOT_ACPT_dist_dir = 'g',
	CPDLC_DM67h_WHEN_CAN_WE_EXPCT_CLB_TO_alt = 'h',
	CPDLC_DM67i_WHEN_CAN_WE_EXPCT_DES_TO_alt = 'i',
} cpdlc_dl_msg_subtype_t;

typedef enum {
	CPDLC_RESP_WU,	/* Wilco / Unable */
	CPDLC_RESP_AN,	/* Affirm / Negative */
	CPDLC_RESP_R,	/* Roger */
	CPDLC_RESP_NE,	/* Operational response */
	CPDLC_RESP_Y,	/* Response required */
	CPDLC_RESP_N	/* Response not required */
} cpdlc_resp_type_t;

typedef enum {
	CPDLC_ARG_ALTITUDE,
	CPDLC_ARG_SPEED,
	CPDLC_ARG_TIME,
	CPDLC_ARG_TIME_DUR,
	CPDLC_ARG_POSITION,
	CPDLC_ARG_DIRECTION,
	CPDLC_ARG_DISTANCE,
	CPDLC_ARG_DISTANCE_OFFSET,
	CPDLC_ARG_VVI,
	CPDLC_ARG_TOFROM,
	CPDLC_ARG_ROUTE,
	CPDLC_ARG_PROCEDURE,
	CPDLC_ARG_SQUAWK,
	CPDLC_ARG_ICAO_ID,
	CPDLC_ARG_ICAO_NAME,
	CPDLC_ARG_FREQUENCY,
	CPDLC_ARG_DEGREES,
	CPDLC_ARG_BARO,
	CPDLC_ARG_FREETEXT,
	CPDLC_ARG_PERSONS,
	CPDLC_ARG_POSREPORT,
	CPDLC_ARG_PDC,
	CPDLC_ARG_TP4TABLE
} cpdlc_arg_type_t;

typedef enum {
	CPDLC_DIR_LEFT =	0,
	CPDLC_DIR_RIGHT =	1,
	CPDLC_DIR_EITHER =	2,
	CPDLC_DIR_NORTH =	3,
	CPDLC_DIR_SOUTH =	4,
	CPDLC_DIR_EAST =	5,
	CPDLC_DIR_WEST =	6,
	CPDLC_DIR_NE =		7,
	CPDLC_DIR_NW =		8,
	CPDLC_DIR_SE =		9,
	CPDLC_DIR_SW =		10
} cpdlc_dir_t;

#define	CPDLC_NULL_SPD		((cpdlc_spd_t){0, 0})
#define	CPDLC_IS_NULL_SPD(_sp)	((_sp).spd == 0)

typedef struct {
	bool			mach;
	bool			tru;
	bool			gnd;
	unsigned		spd;	/* knots or 1/1000th of Mach */
} cpdlc_spd_t;

#define	CPDLC_NULL_ALT		((cpdlc_alt_t){false, false, -9999})
#define	CPDLC_IS_NULL_ALT(a)	((a).alt <= -2000)

typedef struct {
	bool			fl;	/* flight level? */
	bool			met;	/* metric? */
	int			alt;	/* feet */
} cpdlc_alt_t;

#define	CPDLC_NULL_TIME		((cpdlc_time_t){-1, -1})
#define	CPDLC_IS_NULL_TIME(tim)	((tim).hrs < 0 || (tim).mins < 0)

typedef struct {
	int			hrs;
	int			mins;
} cpdlc_time_t;

typedef enum {
	CPDLC_AT,
	CPDLC_AT_OR_ABV,
	CPDLC_AT_OR_BLW
} cpdlc_alt_cstr_type_t;

typedef struct {
	cpdlc_alt_cstr_type_t	toler;
	cpdlc_alt_t		alt;
} cpdlc_alt_cstr_t;

#define	CPDLC_NULL_LAT_LON		((cpdlc_lat_lon_t){NAN, NAN})
#define	CPDLC_IS_NULL_LAT_LON(ll)	(isnan((ll).lat) || isnan((ll).lon))

typedef struct {
	double		lat;		/* NAN if CPDLC_NULL_LAT_LON */
	double		lon;		/* NAN if CPDLC_NULL_LAT_LON */
} cpdlc_lat_lon_t;

typedef struct {
	char		fixname[8];	/* required */
	cpdlc_lat_lon_t	lat_lon;	/* optional, CPDLC_NULL_LAT_LON */
} cpdlc_pub_ident_t;

typedef struct {
	char		fixname[8];	/* required */
	cpdlc_lat_lon_t	lat_lon;	/* optional, CPDLC_NULL_LAT_LON */
	unsigned	degrees;	/* required */
} cpdlc_pb_t;

typedef cpdlc_pb_t cpdlc_pbpb_t[2];

typedef struct {
	char		fixname[8];	/* required */
	cpdlc_lat_lon_t	lat_lon;	/* optional, CPDLC_NULL_LAT_LON */
	unsigned	degrees;	/* required */
	float		dist_nm;	/* required */
} cpdlc_pbd_t;

typedef enum {
	CPDLC_POS_FIXNAME,
	CPDLC_POS_NAVAID,
	CPDLC_POS_AIRPORT,
	CPDLC_POS_LAT_LON,
	CPDLC_POS_PBD,
	CPDLC_POS_UNKNOWN
} cpdlc_pos_type_t;

#define	CPDLC_NULL_POS		((cpdlc_pos_t){false})
#define	CPDLC_IS_NULL_POS(pos)	(!(pos)->set)

typedef struct {
	bool			set;
	cpdlc_pos_type_t	type;
	union {
		char		fixname[8];	/* CPDLC_POS_FIXNAME */
		char		navaid[8];	/* CPDLC_POS_NAVAID */
		char		airport[8];	/* CPDLC_POS_AIRPORT */
		cpdlc_lat_lon_t	lat_lon;	/* CPDLC_POS_LAT_LON */
		cpdlc_pbd_t	pbd;		/* CPDLC_POS_PBD */
		char		str[16];	/* CPDLC_POS_UNKNOWN */
	};
} cpdlc_pos_t;

typedef struct {
	cpdlc_pos_t		pos;
	float			dist_nm;
	bool			spd_cstr_present;
	cpdlc_spd_t		spd_cstr;
	unsigned		num_alt_cstr;
	cpdlc_alt_cstr_t	alt_cstr[2];
} cpdlc_atk_wpt_t;

typedef enum {
	CPDLC_INTC_FROM_PUB_IDENT,
	CPDLC_INTC_FROM_LAT_LON,
	CPDLC_INTC_FROM_PBPB,
	CPDLC_INTC_FROM_PBD
} cpdlc_intc_from_type_t;

typedef struct {
	cpdlc_intc_from_type_t		type;
	union {
		cpdlc_pub_ident_t	pub_ident;
		cpdlc_lat_lon_t		lat_lon;
		cpdlc_pbpb_t		pbpb;
		cpdlc_pbd_t		pbd;
	};
	unsigned			degrees;
} cpdlc_intc_from_t;

typedef enum {
	CPDLC_HOLD_LEG_NONE,
	CPDLC_HOLD_LEG_DIST,
	CPDLC_HOLD_LEG_TIME
} cpdlc_hold_leg_type_t;

typedef struct {
	cpdlc_hold_leg_type_t	type;
	union {
		float		dist_nm;	/* 0.1 - 99.9 NM */
		float		time_min;	/* 0.1 - 9.9 minutes */
	};
} cpdlc_hold_leg_t;

typedef struct {
	cpdlc_pos_t		pos;	/* required */
	cpdlc_spd_t		spd_low;/* optional, CPDLC_NULL_SPD */
	bool			alt_present;
	cpdlc_alt_cstr_t	alt;	/* optional */
	cpdlc_spd_t		spd_high;/* optional, CPDLC_NULL_SPD */
	cpdlc_dir_t		dir;	/* optional, CPDLC_DIR_EITHER */
	unsigned		degrees;/* optional, 0 */
	cpdlc_time_t		efc;	/* optional, CPDLC_NULL_TIME */
	cpdlc_hold_leg_t	leg;	/* optional, CPDLC_HOLD_LEG_NONE */
} cpdlc_hold_at_t;

typedef struct {
	cpdlc_pos_t		pos;
	cpdlc_spd_t		spd;	/* optional, CPDLC_NULL_SPD */
	cpdlc_alt_cstr_t	alt[2];	/* optional, CPDLC_NULL_ALT */
} cpdlc_wpt_spd_alt_t;

typedef enum {
	CPDLC_TIME_TOLER_AT =		0,
	CPDLC_TIME_TOLER_AT_OR_AFTER =	1,
	CPDLC_TIME_TOLER_AT_OR_BEFORE =	2,
} cpdlc_time_toler_t;

typedef struct {
	cpdlc_pos_t		pos;
	cpdlc_time_t		time;
	cpdlc_time_toler_t	toler;
} cpdlc_rta_t;

typedef struct {
	bool			rpt_lat;
	double			degrees;	/* lat/lon, degrees */
	unsigned		deg_incr;	/* optional, 0=unset */
} cpdlc_rpt_pts_t;

typedef struct {
	unsigned		num_atk_wpt;
	cpdlc_atk_wpt_t		atk_wpt[8];
	cpdlc_rpt_pts_t		rpt_pts;
	unsigned		num_intc_from;
	cpdlc_intc_from_t	intc_from[4];
	unsigned		num_hold_at_wpt;
	cpdlc_hold_at_t		hold_at_wpt[4];
	unsigned		num_wpt_spd_alt;
	cpdlc_wpt_spd_alt_t	wpt_spd_alt[32];
	unsigned		num_rta;
	cpdlc_rta_t		rta[32];
} cpdlc_route_add_info_t;

typedef enum {
	CPDLC_PROC_UNKNOWN = 0,
	CPDLC_PROC_ARRIVAL = 1,
	CPDLC_PROC_APPROACH = 2,
	CPDLC_PROC_DEPARTURE = 3
} cpdlc_proc_type_t;

typedef struct {
	cpdlc_proc_type_t	type;
	char			name[8];
	char			trans[8];
} cpdlc_proc_t;

#define	CPDLC_TRK_DETAIL_MAX_LAT_LON	128

typedef struct {
	char			name[8];
	unsigned		num_lat_lon;
	cpdlc_lat_lon_t		lat_lon[CPDLC_TRK_DETAIL_MAX_LAT_LON];
} cpdlc_trk_detail_t;

typedef enum {
	CPDLC_ROUTE_PUB_IDENT,		/* Published database identifier */
	CPDLC_ROUTE_LAT_LON,		/* Latitude+Longitude */
	CPDLC_ROUTE_PBPB,		/* Place-Bearing/Place-Bearing */
	CPDLC_ROUTE_PBD,		/* Place/Bearing/Distance */
	CPDLC_ROUTE_AWY,		/* Airway */
	CPDLC_ROUTE_TRACK_DETAIL,	/* Prescribed track */
	CPDLC_ROUTE_UNKNOWN		/* Unknown - fixname or airway */
} cpdlc_route_info_type_t;

typedef struct {
	cpdlc_route_info_type_t		type;
	union {
		/* CPDLC_ROUTE_PUB_IDENT */
		cpdlc_pub_ident_t	pub_ident;
		/* CPDLC_ROUTE_LAT_LON */
		cpdlc_lat_lon_t		lat_lon;
		/* CPDLC_ROUTE_PBPB */
		cpdlc_pbpb_t		pbpb;
		/* CPDLC_ROUTE_PBD */
		cpdlc_pbd_t		pbd;
		/* CPDLC_ROUTE_AWY */
		char			awy[8];
		/* CPDLC_ROUTE_UNKNOWN */
		char			str[24];
	};
} cpdlc_route_info_t;

#define	CPDLC_ROUTE_MAX_INFO	128

typedef struct {
	char			orig_icao[8];
	char			dest_icao[8];
	char			orig_rwy[8];
	char			dest_rwy[8];
	cpdlc_proc_t		sid;
	cpdlc_proc_t		star;
	cpdlc_proc_t		appch;
	char			awy_intc[8];
	unsigned		num_info;
	cpdlc_route_info_t	info[CPDLC_ROUTE_MAX_INFO];
	cpdlc_route_add_info_t	add_info;
	/* CPDLC_ROUTE_TRACK_DETAIL */
	cpdlc_trk_detail_t	trk_detail;
} cpdlc_route_t;

#define	CPDLC_NULL_WIND		((cpdlc_wind_t){0, 0})
#define	CPDLC_IS_NULL_WIND(wind) ((wind).dir == 0)

typedef struct {
	unsigned		dir;		/* Degrees, 1-360 */
	unsigned		spd;		/* Knots */
} cpdlc_wind_t;

typedef enum {
	CPDLC_TURB_NONE,
	CPDLC_TURB_LIGHT,
	CPDLC_TURB_MOD,
	CPDLC_TURB_SEV
} cpdlc_turb_t;

typedef enum {
	CPDLC_ICING_NONE,
	CPDLC_ICING_TRACE,
	CPDLC_ICING_LIGHT,
	CPDLC_ICING_MOD,
	CPDLC_ICING_SEV
} cpdlc_icing_t;

#define	CPDLC_NULL_TEMP		-100
#define	CPDLC_IS_NULL_TEMP(temp) ((temp) <= -100)

#define	CPDLC_NULL_POS_REP	\
	((cpdlc_pos_rep_t){ \
		CPDLC_NULL_POS,		/* cur_pos */ \
		CPDLC_NULL_TIME,	/* time_cur_pos */ \
		CPDLC_NULL_ALT,		/* alt */ \
		CPDLC_NULL_POS,		/* fix_next */ \
		CPDLC_NULL_TIME,	/* time_fix_next */ \
		CPDLC_NULL_POS,		/* fix_next_p1 */ \
		CPDLC_NULL_TIME,	/* time_dest */ \
		CPDLC_NULL_TIME,	/* rmng_fuel */ \
		CPDLC_NULL_TEMP,	/* temp */ \
		CPDLC_NULL_WIND,	/* wind */ \
		CPDLC_TURB_NONE,	/* turb */ \
		CPDLC_ICING_NONE,	/* icing */ \
		CPDLC_NULL_SPD,		/* spd */ \
		CPDLC_NULL_SPD,		/* spd_gnd */ \
		false,			/* vvi_set */ \
		0,			/* vvi */ \
		0,			/* trk */ \
		0,			/* hdg_true */ \
		false,			/* dist_set */ \
		0,			/* dist_nm */ \
		{},			/* remarks */ \
		CPDLC_NULL_POS,		/* rpt_wpt_pos */ \
		CPDLC_NULL_TIME,	/* rpt_wpt_time */ \
		CPDLC_NULL_ALT		/* rpt_wpt_alt */ \
	})

typedef struct {
	cpdlc_pos_t		cur_pos;	/* Required */
	cpdlc_time_t		time_cur_pos;	/* Required */
	cpdlc_alt_t		cur_alt;	/* Required */
	cpdlc_pos_t		fix_next;	/* Optional, CPDLC_NULL_POS */
	cpdlc_time_t		time_fix_next;	/* Optional, CPDLC_NULL_TIME */
	cpdlc_pos_t		fix_next_p1;	/* Optional, CPDLC_NULL_POS */
	cpdlc_time_t		time_dest;	/* Optional, CPDLC_NULL_TIME */
	cpdlc_time_t		rmng_fuel;	/* Optional, CPDLC_NULL_TIME */
	int			temp;		/* Optional, CPDLC_NULL_TEMP */
	cpdlc_wind_t		wind;		/* Optional, CPDLC_NULL_WIND */
	cpdlc_turb_t		turb;		/* Optional, CPDLC_TURB_NONE */
	cpdlc_icing_t		icing;		/* Optional, CPDLC_ICING_NONE */
	cpdlc_spd_t		spd;		/* Optional, CPDLC_NULL_SPD */
	cpdlc_spd_t		spd_gnd;	/* Optional, CPDLC_NULL_SPD */
	bool			vvi_set;
	int			vvi;		/* Optional, vvi_set=false */
	unsigned		trk;		/* Optional, trk=0 */
	unsigned		hdg_true;	/* Optional, hdg_true=0 */
	bool			dist_set;
	float			dist_nm;	/* Optional, dist_set=false */
	char			remarks[256];	/* Optional, empty */
	cpdlc_pos_t		rpt_wpt_pos;	/* Optional, CPDLC_NULL_POS */
	cpdlc_time_t		rpt_wpt_time;	/* Optional, CPDLC_NULL_TIME */
	cpdlc_alt_t		rpt_wpt_alt;	/* Optional, CPDLC_NULL_ALT */
} cpdlc_pos_rep_t;

typedef enum {
	CPDLC_COM_NAV_LORAN_A,
	CPDLC_COM_NAV_LORAN_C,
	CPDLC_COM_NAV_DME,
	CPDLC_COM_NAV_DECCA,
	CPDLC_COM_NAV_ADF,
	CPDLC_COM_NAV_GNSS,
	CPDLC_COM_NAV_HF_RTF,
	CPDLC_COM_NAV_INS,
	CPDLC_COM_NAV_ILS,
	CPDLC_COM_NAV_OMEGA,
	CPDLC_COM_NAV_VOR,
	CPDLC_COM_NAV_DOPPLER,
	CPDLC_COM_NAV_RNAV,
	CPDLC_COM_NAV_TACAN,
	CPDLC_COM_NAV_UHF_RTF,
	CPDLC_COM_NAV_VHF_RTF
} cpdlc_com_nav_eqpt_st_t;

typedef enum {
	CPDLC_SSR_EQPT_NIL,
	CPDLC_SSR_EQPT_XPDR_MODE_A,
	CPDLC_SSR_EQPT_XPDR_MODE_AC,
	CPDLC_SSR_EQPT_XPDR_MODE_S,
	CPDLC_SSR_EQPT_XPDR_MODE_SPA,
	CPDLC_SSR_EQPT_XPDR_MODE_SID,
	CPDLC_SSR_EQPT_XPDR_MODE_SPAID
} cpdlc_ssr_eqpt_t;

typedef struct {
	bool			com_nav_app_eqpt_avail;
	unsigned		num_com_nav_eqpt_st;
	cpdlc_com_nav_eqpt_st_t	com_nav_eqpt_st[16];
	cpdlc_ssr_eqpt_t	ssr_eqpt;
} cpdlc_acf_eqpt_code_t;

typedef struct {
	char			acf_id[8];	/* required */
	char			acf_type[8];	/* optional */
	cpdlc_acf_eqpt_code_t	acf_eqpt_code;	/* optional */
	cpdlc_time_t		time_dep;	/* required */
	cpdlc_route_t		route;		/* required */
	cpdlc_alt_t		alt_restr;	/* optional */
	double			freq;		/* required */
	unsigned		squawk;		/* required */
	unsigned		revision;	/* required */
} cpdlc_pdc_t;

typedef enum {
	CPDLC_TP4_LABEL_A,
	CPDLC_TP4_LABEL_B
} cpdlc_tp4table_t;

typedef enum {
	CPDLC_FAC_FUNC_CENTER,
	CPDLC_FAC_FUNC_APPROACH,
	CPDLC_FAC_FUNC_TOWER,
	CPDLC_FAC_FUNC_FINAL,
	CPDLC_FAC_FUNC_GROUND,
	CPDLC_FAC_FUNC_CLX,
	CPDLC_FAC_FUNC_DEPARTURE,
	CPDLC_FAC_FUNC_CONTROL
} cpdlc_fac_func_t;

typedef struct {
	bool			is_name;
	union {
		char		icao_id[8];
		char		name[24];
	};
	cpdlc_fac_func_t        func;
} cpdlc_icao_name_t;

typedef struct {
	bool			hpa;
	double			val;
} cpdlc_altimeter_t;

typedef union {
	cpdlc_alt_t		alt;
	cpdlc_spd_t		spd;
	cpdlc_time_t		time;
	cpdlc_pos_t		pos;
	cpdlc_dir_t		dir;
	double			dist;	/* nautical miles */
	int			vvi;	/* feet per minute */
	bool			tofrom;	/* true = to, false = from */
	cpdlc_route_t		*route;
	cpdlc_proc_t		proc;
	unsigned		squawk;
	char			icao_id[8];
	cpdlc_icao_name_t	icao_name;
	double			freq;
	struct {
		unsigned	deg;
		bool		tru;
	} deg;
	cpdlc_altimeter_t	baro;
	char			*freetext;
	unsigned		pob;
	cpdlc_pos_rep_t		pos_rep;
	cpdlc_pdc_t		*pdc;
	cpdlc_tp4table_t	tp4;
} cpdlc_arg_t;

enum {
    CPDLC_MAX_ARGS = 5,
    CPDLC_MAX_RESP_MSGS = 4,
    CPDLC_MAX_MSG_SEGS = 5,
    CPDLC_MAX_VERSION_NR = 1,
    CPDLC_CALLSIGN_LEN = 16
};

typedef struct {
	uintptr_t		offset;
	bool			is_seq;
	uintptr_t		seq_idx;
} cpdlc_asn_arg_info_t;

typedef struct {
	bool			is_dl;
	int			msg_type;
	char			msg_subtype;	/* for the weird 67x messages */
	const char		*text;
	unsigned		num_args;
	cpdlc_arg_type_t	args[CPDLC_MAX_ARGS];
	unsigned		asn_elem_id;
	cpdlc_asn_arg_info_t	asn_arg_info[CPDLC_MAX_ARGS];
	cpdlc_resp_type_t	resp;
	unsigned		timeout;	/* seconds */
	unsigned		num_resp_msgs;
	int			resp_msg_types[CPDLC_MAX_RESP_MSGS];
	int			resp_msg_subtypes[CPDLC_MAX_RESP_MSGS];
} cpdlc_msg_info_t;

typedef struct {
	const cpdlc_msg_info_t	*info;
	cpdlc_arg_t		args[CPDLC_MAX_ARGS];
} cpdlc_msg_seg_t;

typedef enum {
	CPDLC_PKT_CPDLC,
	CPDLC_PKT_PING,
	CPDLC_PKT_PONG
} cpdlc_pkt_t;

typedef struct {
	bool			set;
	unsigned		hrs;
	unsigned		mins;
	unsigned		secs;
} cpdlc_timestamp_t;

typedef struct {
	cpdlc_pkt_t		pkt_type;
	unsigned		min;
	unsigned		mrn;
	cpdlc_timestamp_t	ts;
	char			from[CPDLC_CALLSIGN_LEN];
	char			to[CPDLC_CALLSIGN_LEN];
	bool			is_logon;
	bool			is_logoff;
	char			*logon_data;
	unsigned		num_segs;
	cpdlc_msg_seg_t		segs[CPDLC_MAX_MSG_SEGS];
} cpdlc_msg_t;

extern const cpdlc_msg_info_t *cpdlc_ul_infos;
extern const cpdlc_msg_info_t *cpdlc_dl_infos;

CPDLC_API cpdlc_msg_t *cpdlc_msg_alloc(cpdlc_pkt_t pkt_type);
CPDLC_API cpdlc_msg_t *cpdlc_msg_copy(const cpdlc_msg_t *oldmsg);
CPDLC_API void cpdlc_msg_free(cpdlc_msg_t *msg);

CPDLC_API unsigned cpdlc_msg_encode(const cpdlc_msg_t *msg, char *buf,
    unsigned cap);
CPDLC_API unsigned cpdlc_msg_encode_asn1(const cpdlc_msg_t *msg, char *buf,
    unsigned cap);
CPDLC_API unsigned cpdlc_msg_readable(const cpdlc_msg_t *msg, char *buf,
    unsigned cap);
CPDLC_API bool cpdlc_msg_decode(const char *in_buf, bool is_dl,
    cpdlc_msg_t **msg, int *consumed, char *reason, unsigned reason_cap);

void cpdlc_encode_msg_arg(const cpdlc_arg_type_t arg_type,
    const cpdlc_arg_t *arg, bool readable, unsigned *n_bytes_p,
    char **buf_p, unsigned *cap_p);

CPDLC_API void cpdlc_msg_set_to(cpdlc_msg_t *msg, const char *to);
CPDLC_API const char *cpdlc_msg_get_to(const cpdlc_msg_t *msg);
CPDLC_API void cpdlc_msg_set_from(cpdlc_msg_t *msg, const char *from);
CPDLC_API const char *cpdlc_msg_get_from(const cpdlc_msg_t *msg);
CPDLC_API bool cpdlc_msg_get_dl(const cpdlc_msg_t *msg);

CPDLC_API void cpdlc_msg_set_min(cpdlc_msg_t *msg, unsigned min);
CPDLC_API unsigned cpdlc_msg_get_min(const cpdlc_msg_t *msg);
CPDLC_API void cpdlc_msg_set_mrn(cpdlc_msg_t *msg, unsigned mrn);
CPDLC_API unsigned cpdlc_msg_get_mrn(const cpdlc_msg_t *msg);

CPDLC_API const char *cpdlc_msg_get_logon_data(const cpdlc_msg_t *msg);
CPDLC_API void cpdlc_msg_set_logon_data(cpdlc_msg_t *msg,
    const char *logon_data);

CPDLC_API void cpdlc_msg_set_logoff(cpdlc_msg_t *msg, bool is_logoff);
CPDLC_API bool cpdlc_msg_get_logoff(const cpdlc_msg_t *msg);

CPDLC_API unsigned cpdlc_msg_get_num_segs(const cpdlc_msg_t *msg);
CPDLC_API int cpdlc_msg_add_seg(cpdlc_msg_t *msg, bool is_dl,
    unsigned msg_type, unsigned char msg_subtype);
CPDLC_API void cpdlc_msg_del_seg(cpdlc_msg_t *msg, unsigned seg_nr);

CPDLC_API unsigned cpdlc_msg_seg_get_num_args(const cpdlc_msg_t *msg,
    unsigned seg_nr);
CPDLC_API cpdlc_arg_type_t cpdlc_msg_seg_get_arg_type(const cpdlc_msg_t *msg,
    unsigned seg_nr, unsigned arg_nr);
CPDLC_API void cpdlc_msg_seg_set_arg(cpdlc_msg_t *msg, unsigned seg_nr,
    unsigned arg_nr, const void *arg_val1, const void *arg_val2);
CPDLC_API unsigned cpdlc_msg_seg_get_arg(const cpdlc_msg_t *msg,
    unsigned seg_nr, unsigned arg_nr, void *arg_val1, unsigned str_cap,
    void *arg_val2);

unsigned CPDLC_API cpdlc_escape_percent(const char *in_buf, char *out_buf,
    unsigned cap);
CPDLC_API int cpdlc_unescape_percent(const char *in_buf, char *out_buf,
    unsigned cap);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_MSG_H_ */
