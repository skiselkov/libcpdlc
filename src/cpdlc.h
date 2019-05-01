/*
 * Copyright 2019 Saso Kiselkov
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

#ifndef	_LIBCPDLC_CPDLC_H_
#define	_LIBCPDLC_CPDLC_H_

#include <stdbool.h>

#include "cpdlc_core.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
	CPDLC_UL0_UNABLE,
	CPDLC_UL1_STANDBY,
	CPDLC_UL2_REQ_DEFERRED,
	CPDLC_UL3_ROGER,
	CPDLC_UL4_AFFIRM,
	CPDLC_UL5_NEGATIVE,
	CPDLC_UL6_EXPCT_alt,
	CPDLC_UL7_EXPCT_CLB_AT_time,
	CPDLC_UL8_EXPCT_CLB_AT_pos,
	CPDLC_UL9_EXPCT_DES_AT_time,
	CPDLC_UL10_EXPCT_DES_AT_pos,
	CPDLC_UL11_EXPCT_CRZ_CLB_AT_time,
	CPDLC_UL12_EXPCT_CRZ_CLB_AT_pos,
	CPDLC_UL13_AT_time_EXPCT_CLB_TO_alt,
	CPDLC_UL14_AT_pos_EXPCT_CLB_TO_alt,
	CPDLC_UL15_AT_time_EXPCT_DES_TO_alt,
	CPDLC_UL16_AT_pos_EXPCT_DES_TO_alt,
	CPDLC_UL17_AT_time_EXPCT_CRZ_CLB_TO_alt,
	CPDLC_UL18_AT_pos_EXPCT_CRZ_CLB_TO_alt,
	CPDLC_UL19_MAINT_alt,
	CPDLC_UL20_CLB_TO_alt,
	CPDLC_UL21_AT_time_CLB_TO_alt,
	CPDLC_UL22_AT_pos_CLB_TO_alt,
	CPDLC_UL23_DES_TO_alt,
	CPDLC_UL24_AT_time_DES_TO_alt,
	CPDLC_UL25_AT_pos_DES_TO_alt,
	CPDLC_UL26_CLB_TO_REACH_alt_BY_time,
	CPDLC_UL27_CLB_TO_REACH_alt_BY_pos,
	CPDLC_UL28_DES_TO_REACH_alt_BY_time,
	CPDLC_UL29_DES_TO_REACH_alt_BY_pos,
	CPDLC_UL30_MAINT_BLOCK_alt_TO_alt,
	CPDLC_UL31_CLB_TO_MAINT_BLOCK_alt_TO_alt,
	CPDLC_UL32_DES_TO_MAINT_BLOCK_alt_TO_alt,
	CPDLC_UL33_CRZ_alt,
	CPDLC_UL34_CRZ_CLB_TO_alt,
	CPDLC_UL35_CRZ_CLB_ABV_alt,
	CPDLC_UL36_EXPED_CLB_TO_alt,
	CPDLC_UL37_EXPED_DES_TO_alt,
	CPDLC_UL38_IMM_CLB_TO_alt,
	CPDLC_UL39_IMM_DES_TO_alt,
	CPDLC_UL40_IMM_STOP_CLB_AT_alt,
	CPDLC_UL41_IMM_STOP_DES_AT_alt,
	CPDLC_UL42_EXPCT_CROSS_pos_AT_alt,
	CPDLC_UL43_EXPCT_CROSS_pos_AT_alt_OR_ABV,
	CPDLC_UL44_EXPCT_CROSS_pos_AT_alt_OR_BLW,
	CPDLC_UL45_EXPCT_CROSS_pos_AT_AND_MAINT_alt,
	CPDLC_UL46_CROSS_pos_AT_alt,
	CPDLC_UL47_CROSS_pos_AT_alt_OR_ABV,
	CPDLC_UL48_CROSS_pos_AT_alt_OR_BLW,
	CPDLC_UL49_CROSS_pos_AT_AND_MAINT_alt,
	CPDLC_UL50_CROSS_pos_BTWN_alt_AND_alt,
	CPDLC_UL51_CROSS_pos_AT_time,
	CPDLC_UL52_CROSS_pos_AT_OR_BEFORE_time,
	CPDLC_UL53_CROSS_pos_AT_OR_AFTER_time,
	CPDLC_UL54_CROSS_pos_BTWN_time_AND_time,
	CPDLC_UL55_CROSS_pos_AT_spd,
	CPDLC_UL56_CROSS_pos_AT_OR_LESS_spd,
	CPDLC_UL57_CROSS_pos_AT_OR_GREATER_spd,
	CPDLC_UL58_CROSS_pos_AT_time_AT_alt,
	CPDLC_UL59_CROSS_pos_AT_OR_BEFORE_time_AT_alt,
	CPDLC_UL60_CROSS_pos_AT_OR_AFTER_time_AT_alt,
	CPDLC_UL61_CROSS_pos_AT_AND_MAINT_alt_AT_spd,
	CPDLC_UL62_AT_time_CROSS_pos_AT_AND_MAINT_alt,
	CPDLC_UL63_AT_time_CROSS_pos_AT_AND_MAINT_alt_AT_spd,
	CPDLC_UL64_OFFSET_dir_dist_OF_ROUTE,
	CPDLC_UL65_AT_pos_OFFSET_dir_dist_OF_ROUTE,
	CPDLC_UL66_AT_time_OFFSET_dir_dist_OF_ROUTE,
	CPDLC_UL67_PROCEED_BACK_ON_ROUTE,
	CPDLC_UL68_REJOIN_ROUTE_BY_pos,
	CPDLC_UL69_REJOIN_ROUTE_BY_time,
	CPDLC_UL70_EXPCT_BACK_ON_ROUTE_BY_pos,
	CPDLC_UL71_EXPCT_BACK_ON_ROUTE_BY_time,
	CPDLC_UL72_RESUME_OWN_NAV,
	CPDLC_UL73_PDC_route,
	CPDLC_UL74_DIR_TO_pos,
	CPDLC_UL75_WHEN_ABL_DIR_TO_pos,
	CPDLC_UL76_AT_time_DIR_TO_pos,
	CPDLC_UL77_AT_pos_DIR_TO_pos,
	CPDLC_UL78_AT_alt_DIR_TO_pos,
	CPDLC_UL79_CLR_TO_pos_VIA_route,
	CPDLC_UL80_CLR_route,
	CPDLC_UL81_CLR_proc,
	CPDLC_UL82_CLR_DEVIATE_UP_TO_dir_dist_OF_ROUTE,
	CPDLC_UL83_AT_pos_CLR_route,
	CPDLC_UL84_AT_pos_CLR_proc,
	CPDLC_UL85_EXPCT_route,
	CPDLC_UL86_AT_pos_EXPCT_route,
	CPDLC_UL87_EXPCT_DIR_TO_pos,
	CPDLC_UL88_AT_pos_EXPCT_DIR_TO_pos,
	CPDLC_UL89_AT_time_EXPCT_DIR_TO_pos,
	CPDLC_UL90_AT_alt_EXPCT_DIR_TO_pos,
	CPDLC_UL91_HOLD_AT_pos_MAINT_alt_INBOUND_deg_TURN_dir_LEG_TIME_time,
	CPDLC_UL92_HOLD_AT_pos_AS_PUBLISHED_MAINT_alt,
	CPDLC_UL93_EXPCT_FURTHER_CLR_AT_time,
	CPDLC_UL94_TURN_dir_HDG_deg,
	CPDLC_UL95_TURN_dir_GND_TRK_deg,
	CPDLC_UL96_FLY_PRESENT_HDG,
	CPDLC_UL97_AT_pos_FLY_HDG_deg,
	CPDLC_UL98_IMM_TURN_dir_HDG_deg,
	CPDLC_UL99_EXPCT_proc,
	CPDLC_UL100_AT_time_EXPCT_spd,
	CPDLC_UL101_AT_pos_EXPCT_spd,
	CPDLC_UL102_AT_alt_EXPCT_spd,
	CPDLC_UL103_AT_time_EXPCT_spd_TO_spd,
	CPDLC_UL104_AT_pos_EXPCT_spd_TO_spd,
	CPDLC_UL105_AT_alt_EXPCT_spd_TO_spd,
	CPDLC_UL106_MAINT_spd,
	CPDLC_UL107_MAINT_PRESENT_SPD,
	CPDLC_UL108_MAINT_spd_OR_GREATER,
	CPDLC_UL109_MAINT_spd_OR_LESS,
	CPDLC_UL110_MAINT_spd_TO_spd,
	CPDLC_UL111_INCR_SPD_TO_spd,
	CPDLC_UL112_INCR_SPD_TO_spd_OR_GREATER,
	CPDLC_UL113_RED_SPD_TO_spd,
	CPDLC_UL114_RED_SPD_TO_spd_OR_LESS,
	CPDLC_UL115_DO_NOT_EXCEED_spd,
	CPDLC_UL116_RESUME_NORMAL_SPD,
	CPDLC_UL117_CTC_icaounitname_freq,
	CPDLC_UL118_AT_pos_CONTACT_icaounitname_freq,
	CPDLC_UL119_AT_time_CONTACT_icaounitname_freq,
	CPDLC_UL120_MONITOR_icaounitname_freq,
	CPDLC_UL121_AT_pos_MONITOR_icaounitname_freq,
	CPDLC_UL122_AT_time_MONITOR_icaounitname_freq,
	CPDLC_UL123_SQUAWK_code,
	CPDLC_UL124_STOP_SQUAWK,
	CPDLC_UL125_SQUAWK_ALT,
	CPDLC_UL126_STOP_ALT_SQUAWK,
	CPDLC_UL127_REPORT_BACK_ON_ROUTE,
	CPDLC_UL128_REPORT_LEAVING_alt,
	CPDLC_UL129_REPORT_LEVEL_alt,
	CPDLC_UL130_REPORT_PASSING_pos,
	CPDLC_UL131_REPORT_RMNG_FUEL_SOULS_ON_BOARD,
	CPDLC_UL132_CONFIRM_POSITION,
	CPDLC_UL133_CONFIRM_ALT,
	CPDLC_UL134_CONFIRM_SPD,
	CPDLC_UL135_CONFIRM_ASSIGNED_ALT,
	CPDLC_UL136_CONFIRM_ASSIGNED_SPD,
	CPDLC_UL137_CONFIRM_ASSIGNED_ROUTE,
	CPDLC_UL138_CONFIRM_TIME_OVER_REPORTED_WPT,
	CPDLC_UL139_CONFIRM_REPORTED_WPT,
	CPDLC_UL140_CONFIRM_NEXT_WPT,
	CPDLC_UL141_CONFIRM_NEXT_WPT_ETA,
	CPDLC_UL142_CONFIRM_ENSUING_WPT,
	CPDLC_UL143_CONFIRM_REQ,
	CPDLC_UL144_CONFIRM_SQUAWK,
	CPDLC_UL145_CONFIRM_HDG,
	CPDLC_UL146_CONFIRM_GND_TRK,
	CPDLC_UL147_REQUEST_POS_REPORT,
	CPDLC_UL148_WHEN_CAN_YOU_ACPT_alt,
	CPDLC_UL149_CAN_YOU_ACPT_alt_AT_pos,
	CPDLC_UL150_CAN_YOU_ACPT_alt_AT_time,
	CPDLC_UL151_WHEN_CAN_YOU_ACPT_spd,
	CPDLC_UL152_WHEN_CAN_YOU_ACPT_dir_dist_OFFSET,
	CPDLC_UL153_ALTIMETER_baro,
	CPDLC_UL154_RDR_SVC_TERM,
	CPDLC_UL155_RDR_CTC_pos,
	CPDLC_UL156_RDR_CTC_LOST,
	CPDLC_UL157_CHECK_STUCK_MIC,
	CPDLC_UL158_ATIS_code,
	CPDLC_UL159_ERROR_description,
	CPDLC_UL160_NEXT_DATA_AUTHORITY_id,
	CPDLC_UL161_END_SVC,
	CPDLC_UL162_SVC_UNAVAIL,
	CPDLC_UL163_FACILITY_designation_tp4table,
	CPDLC_UL164_WHEN_RDY,
	CPDLC_UL165_THEN,
	CPDLC_UL166_DUE_TO_TFC,
	CPDLC_UL167_DUE_TO_AIRSPACE_RESTR,
	CPDLC_UL168_DISREGARD,
	CPDLC_UL169_FREETEXT_NORMAL_text,
	CPDLC_UL170_FREETEXT_DISTRESS_text,
	CPDLC_UL171_CLB_AT_vvi_MIN,
	CPDLC_UL172_CLB_AT_vvi_MAX,
	CPDLC_UL173_DES_AT_vvi_MIN,
	CPDLC_UL174_DES_AT_vvi_MAX,
	CPDLC_UL175_REPORT_REACHING_alt,
	CPDLC_UL176_MAINT_OWN_SEPARATION_AND_VMC,
	CPDLC_UL177_AT_PILOTS_DISCRETION,
	CPDLC_UL178_UNUSED,
	CPDLC_UL179_SQUAWK_IDENT,
	CPDLC_UL180_REPORT_REACHING_BLOCK_alt_TO_alt,
	CPDLC_UL181_REPORT_DISTANCE_tofrom_pos,
	CPDLC_UL182_CONFIRM_ATIS_CODE
} cpdlc_ul_msg_type_t;

typedef enum {
	CPDLC_DL0_WILCO,
	CPDLC_DL1_UNABLE,
	CPDLC_DL2_STANDBY,
	CPDLC_DL3_ROGER,
	CPDLC_DL4_AFFIRM,
	CPDLC_DL5_NEGATIVE,
	CPDLC_DL6_REQ_alt,
	CPDLC_DL7_REQ_BLOCK_alt_TO_alt,
	CPDLC_DL8_REQ_CRZ_CLB_TO_alt,
	CPDLC_DL9_REQ_CLB_TO_alt,
	CPDLC_DL10_REQ_DES_TO_alt,
	CPDLC_DL11_AT_pos_REQ_CLB_TO_alt,
	CPDLC_DL12_AT_pos_REQ_DES_TO_alt,
	CPDLC_DL13_AT_time_REQ_CLB_TO_alt,
	CPDLC_DL14_AT_time_REQ_DES_TO_alt,
	CPDLC_DL15_REQ_OFFSET_dir_dist_OF_ROUTE,
	CPDLC_DL16_AT_pos_REQ_OFFSET_dir_dist_OF_ROUTE,
	CPDLC_DL17_AT_time_REQ_OFFSET_dir_dist_OF_ROUTE,
	CPDLC_DL18_REQ_spd,
	CPDLC_DL19_REQ_spd_TO_spd,
	CPDLC_DL20_REQ_VOICE_CTC,
	CPDLC_DL21_REQ_VOICE_CTC_ON_freq,
	CPDLC_DL22_REQ_DIR_TO_pos,
	CPDLC_DL23_REQ_proc,
	CPDLC_DL24_REQ_route,
	CPDLC_DL25_REQ_PDC,
	CPDLC_DL26_REQ_WX_DEVIATION_TO_pos_VIA_route,
	CPDLC_DL27_REQ_WX_DEVIATION_UP_TO_dir_dist_OF_ROUTE,
	CPDLC_DL28_LEAVING_alt,
	CPDLC_DL29_CLIMBING_TO_alt,
	CPDLC_DL30_DESCENDING_TO_alt,
	CPDLC_DL31_PASSING_pos,
	CPDLC_DL32_PRESENT_ALT_alt,
	CPDLC_DL33_PRESENT_POS_pos,
	CPDLC_DL34_PRESENT_SPD_spd,
	CPDLC_DL35_PRESENT_HDG_deg,
	CPDLC_DL36_PRESENT_GND_TRK_deg,
	CPDLC_DL37_LEVEL_alt,
	CPDLC_DL38_ASSIGNED_ALT_alt,
	CPDLC_DL39_ASSIGNED_SPD_spd,
	CPDLC_DL40_ASSIGNED_ROUTE_route,
	CPDLC_DL41_BACK_ON_ROUTE,
	CPDLC_DL42_NEXT_WPT_pos,
	CPDLC_DL43_NEXT_WPT_ETA_time,
	CPDLC_DL44_ENSUING_WPT_pos,
	CPDLC_DL45_REPORTED_WPT_pos,
	CPDLC_DL46_REPORTED_WPT_time,
	CPDLC_DL47_SQUAWKING_code,
	CPDLC_DL48_POS_REPORT_posreport,
	CPDLC_DL49_WHEN_CAN_WE_EXPCT_spd,
	CPDLC_DL50_WHEN_CAN_WE_EXPCT_spd_TO_spd,
	CPDLC_DL51_WHEN_CAN_WE_EXPCT_BACK_ON_ROUTE,
	CPDLC_DL52_WHEN_CAN_WE_EXPECT_LOWER_ALT,
	CPDLC_DL53_WHEN_CAN_WE_EXPECT_HIGHER_ALT,
	CPDLC_DL54_WHEN_CAN_WE_EXPECT_CRZ_CLB_TO_alt,
	CPDLC_DL55_UNUSED,
	CPDLC_DL56_UNUSED,
	CPDLC_DL57_UNUSED,
	CPDLC_DL58_UNUSED,
	CPDLC_DL59_UNUSED,
	CPDLC_DL60_UNUSED,
	CPDLC_DL61_UNUSED,
	CPDLC_DL62_ERROR_errorinfo,
	CPDLC_DL63_NOT_CURRENT_DATA_AUTHORITY,
	CPDLC_DL64_CURRENT_DATA_AUTHORITY_id,
	CPDLC_DL65_DUE_TO_WX,
	CPDLC_DL66_DUE_TO_ACFT_PERF,
	CPDLC_DL67_FREETEXT_NORMAL_text,
	CPDLC_DL68_FREETEXT_DISTRESS_text,
	CPDLC_DL69_UNUSED,
	CPDLC_DL70_REQ_HDG_deg,
	CPDLC_DL71_REQ_GND_TRK_deg,
	CPDLC_DL72_REACHING_alt,
	CPDLC_DL73_VERSION_number,
	CPDLC_DL74_MAINT_OWN_SEPARATION_AND_VMC,
	CPDLC_DL75_AT_PILOTS_DISCRETION,
	CPDLC_DL76_REACHING_BLOCK_alt_TO_alt,
	CPDLC_DL77_ASSIGNED_BLOCK_alt_TO_alt,
	CPDLC_DL78_AT_time_dist_tofrom_pos,
	CPDLC_DL79_ATIS_code,
	CPDLC_DL80_DEVIATING_dir_dist_OF_ROUTE
} cpdlc_dl_msg_type_t;

typedef enum {
	CPDLC_DL67b_WE_CAN_ACPT_alt_AT_time = 'b',
	CPDLC_DL67c_WE_CAN_ACPT_spd_AT_time = 'c',
	CPDLC_DL67d_WE_CAN_ACPT_dir_dist_AT_time = 'd',
	CPDLC_DL67e_WE_CANNOT_ACPT_alt = 'e',
	CPDLC_DL67f_WE_CANNOT_ACPT_spd = 'f',
	CPDLC_DL67g_WE_CANNOT_ACPT_dir_dist = 'g',
	CPDLC_DL67h_WHEN_CAN_WE_EXPCT_CLB_TO_alt = 'h',
	CPDLC_DL67i_WHEN_CAN_WE_EXPCT_DES_TO_alt = 'i',
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
	CPDLC_ARG_POSITION,
	CPDLC_ARG_DIRECTION,
	CPDLC_ARG_DISTANCE,
	CPDLC_ARG_VVI,
	CPDLC_ARG_TOFROM,
	CPDLC_ARG_ROUTE,
	CPDLC_ARG_PROCEDURE,
	CPDLC_ARG_SQUAWK,
	CPDLC_ARG_ICAONAME,
	CPDLC_ARG_FREQUENCY,
	CPDLC_ARG_DEGREES,
	CPDLC_ARG_BARO,
	CPDLC_ARG_FREETEXT
} cpdlc_arg_type_t;

typedef enum {
	CPDLC_DIR_ANY,
	CPDLC_DIR_LEFT,
	CPDLC_DIR_RIGHT
} cpdlc_dir_t;

typedef union {
	struct {
		bool	fl;
		int	alt;	/* feet */
	} alt;
	struct {
		bool	mach;
		int	spd;	/* knots or 1/1000th of Mach */
	} spd;
	struct {
		int	hrs;
		int	mins;
	} time;
	char		pos[32];
	cpdlc_dir_t	dir;
	double		dist;	/* nautical miles */
	int		vvi;	/* feet per minute */
	bool		tofrom;	/* true = to, false = from */
	char		route[512];
	char		proc[16];
	unsigned	squawk;
	char		icaoname[8];
	double		freq;
	unsigned	deg;
	unsigned	baro;
	char		freetext[512];
} cpdlc_arg_t;

enum { CPDLC_MAX_ARGS = 5, CPDLC_MAX_RESP_MSGS = 4, CPDLC_MAX_MSG_SEGS = 5 };

typedef struct {
	bool			is_dl;
	int			msg_type;
	char			msg_subtype;	/* for the weird 67x messages */
	const char		*text;
	unsigned		num_args;
	cpdlc_arg_type_t	args[CPDLC_MAX_ARGS];
	cpdlc_resp_type_t	resp;
	unsigned		num_resp_msgs;
	int			resp_msg_types[CPDLC_MAX_RESP_MSGS];
	int			resp_msg_subtypes[CPDLC_MAX_RESP_MSGS];
} cpdlc_msg_info_t;

typedef struct {
	const cpdlc_msg_info_t	*info;
	cpdlc_arg_t		args[CPDLC_MAX_ARGS];
} cpdlc_msg_seg_t;

typedef struct {
	char		from[16];
	char		to[16];
	unsigned	min;
	unsigned	mrn;
	unsigned	num_segs;
	cpdlc_msg_seg_t	segs[CPDLC_MAX_MSG_SEGS];
} cpdlc_msg_t;

const cpdlc_msg_info_t *cpdlc_ul_infos;
const cpdlc_msg_info_t *cpdlc_dl_infos;

CPDLC_API cpdlc_msg_t *cpdlc_msg_alloc(const char *from, const char *to,
    unsigned min, unsigned mrn);
CPDLC_API void cpdlc_msg_free(cpdlc_msg_t *msg);

CPDLC_API unsigned cpdlc_msg_get_num_segs(const cpdlc_msg_t *msg);
CPDLC_API int cpdlc_msg_add_seg(cpdlc_msg_t *msg, bool is_dl, int msg_type,
    int msg_subtype);

CPDLC_API unsigned cpdlc_msg_seg_get_num_args(const cpdlc_msg_t *msg,
    unsigned seg_nr);
CPDLC_API cpdlc_arg_type_t cpdlc_msg_seg_get_arg_type(const cpdlc_msg_t *msg,
    unsigned seg_nr, unsigned arg_nr);
CPDLC_API void cpdlc_msg_seg_set_arg(cpdlc_msg_t *msg, unsigned seg_nr,
    unsigned arg_nr, void *arg_val1, void *arg_val2);
CPDLC_API unsigned cpdlc_msg_seg_get_arg(const cpdlc_msg_t *msg,
    unsigned seg_nr, unsigned arg_nr, void *arg_val1, unsigned str_cap,
    void *arg_val2);

CPDLC_API unsigned cpdlc_msg_encode(const cpdlc_msg_t *msg, char *buf,
    unsigned cap);
CPDLC_API int cpdlc_msg_decode(const char *in_buf, cpdlc_msg_t *msg);

#ifdef	__cplusplus
}
#endif

#endif	/* _LIBCPDLC_CPDLC_H_ */
