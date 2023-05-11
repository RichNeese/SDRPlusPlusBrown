#pragma once
#include "base4_fft.h"
#include "head.h"
#include <iostream>
using namespace std;
class G_calculate
{
public:
	G_calculate();
 	short G_calculate_process(Complex_num* m_winData, int blockInd);
	short Initialize(int wlen);
	~G_calculate();
	double abc[frame_max]; double bcd[frame_max]; double cde[frame_max]; double aa[frame_max];
private:
	int m_int_value1[1500] = { 123380,87676,71942,62610,56275,51622,48025,45140,42763,40762,39050,37565,36261,35106,34074,33145,32304,31538,30837,30193,29599,29049,28538,28062,27617,27200,26809,26442,26095,25768,25458,25165,24887,24622,24371,24131,23903,23685,23477,23277,23086,22903,22728,22559,22397,22242,22092,21948,21809,21675,21546,21421,21301,21185,21072,20964,20858,20757,20658,20563,20470,20380,20293,20209,20127,20047,19970,19894,19821,19750,19681,19614,19548,19485,19423,19362,19304,19246,19191,19136,19083,19031,18981,18932,18883,18837,18791,18746,18702,18660,18618,18577,18538,18499,18461,18424,18387,18352,18317,18283,18250,18217,18185,18154,18124,18094,18064,18036,18008,17980,17953,17927,17901,17876,17851,17827,17803,17779,17756,17734,17712,17690,17669,17648,17628,17608,17588,17569,17550,17532,17513,17495,17478,17461,17444,17427,17411,17395,17379,17364,17349,17334,17319,17305,17291,17277,17263,17250,17237,17224,17211,17199,17186,17174,17162,17151,17139,17128,17117,17106,17095,17085,17074,17064,17054,17044,17035,17025,17016,17007,16998,16989,16980,16971,16963,16954,16946,16938,16930,16922,16915,16907,16900,16892,16885,16878,16871,16864,16857,16850,16844,16837,16831,16825,16819,16812,16806,16801,16795,16789,16783,16778,16772,16767,16762,16756,16751,16746,16741,16736,16731,16727,16722,16717,16713,16708,16704,16700,16695,16691,16687,16683,16679,16675,16671,16667,16663,16659,16656,16652,16648,16645,16641,16638,16634,16631,16628,16625,16621,16618,16615,16612,16609,16606,16603,16600,16597,16594,16592,16589,16586,16584,16581,16578,16576,16573,16571,16568,16566,16563,16561,16559,16557,16554,16552,16550,16548,16546,16543,16541,16539,16537,16535,16533,16531,16530,16528,16526,16524,16522,16520,16519,16517,16515,16513,16512,16510,16509,16507,16505,16504,16502,16501,16499,16498,16496,16495,16494,16492,16491,16489,16488,16487,16485,16484,16483,16482,16480,16479,16478,16477,16476,16474,16473,16472,16471,16470,16469,16468,16467,16466,16465,16464,16463,16462,16461,16460,16459,16458,16457,16456,16455,16454,16453,16452,16452,16451,16450,16449,16448,16447,16447,16446,16445,16444,16444,16443,16442,16441,16441,16440,16439,16439,16438,16437,16437,16436,16435,16435,16434,16433,16433,16432,16432,16431,16430,16430,16429,16429,16428,16428,16427,16427,16426,16426,16425,16425,16424,16424,16423,16423,16422,16422,16421,16421,16420,16420,16419,16419,16418,16418,16418,16417,16417,16416,16416,16416,16415,16415,16414,16414,16414,16413,16413,16413,16412,16412,16412,16411,16411,16411,16410,16410,16410,16409,16409,16409,16408,16408,16408,16408,16407,16407,16407,16406,16406,16406,16406,16405,16405,16405,16405,16404,16404,16404,16404,16403,16403,16403,16403,16402,16402,16402,16402,16402,16401,16401,16401,16401,16400,16400,16400,16400,16400,16400,16399,16399,16399,16399,16399,16398,16398,16398,16398,16398,16398,16397,16397,16397,16397,16397,16397,16396,16396,16396,16396,16396,16396,16396,16395,16395,16395,16395,16395,16395,16395,16394,16394,16394,16394,16394,16394,16394,16394,16393,16393,16393,16393,16393,16393,16393,16393,16393,16392,16392,16392,16392,16392,16392,16392,16392,16392,16392,16391,16391,16391,16391,16391,16391,16391,16391,16391,16391,16391,16391,16390,16390,16390,16390,16390,16390,16390,16390,16390,16390,16390,16390,16390,16389,16389,16389,16389,16389,16389,16389,16389,16389,16389,16389,16389,16389,16389,16389,16389,16388,16388,16388,16388,16388,16388,16388,16388,16388,16388,16388,16388,16388,16388,16388,16388,16388,16388,16388,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16387,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16386,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16385,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384,16384 };
	short m_expsub_value1[1500] = { 16220,16059,15899,15741,15584,15429,15276,15124,14973,14824,14677,14531,14386,14243,14101,13961,13822,13685,13548,13414,13280,13148,13017,12888,12759,12632,12507,12382,12259,12137,12016,11897,11778,11661,11545,11430,11316,11204,11092,10982,10873,10765,10657,10551,10446,10342,10240,10138,10037,9937,9838,9740,9643,9547,9452,9358,9265,9173,9082,8991,8902,8813,8725,8639,8553,8468,8383,8300,8217,8136,8055,7974,7895,7817,7739,7662,7586,7510,7435,7361,7288,7216,7144,7073,7002,6933,6864,6795,6728,6661,6594,6529,6464,6400,6336,6273,6210,6149,6087,6027,5967,5907,5849,5791,5733,5676,5619,5563,5508,5453,5399,5345,5292,5239,5187,5136,5085,5034,4984,4934,4885,4837,4788,4741,4694,4647,4601,4555,4510,4465,4420,4376,4333,4290,4247,4205,4163,4121,4080,4040,4000,3960,3920,3881,3843,3804,3767,3729,3692,3655,3619,3583,3547,3512,3477,3442,3408,3374,3341,3307,3274,3242,3210,3178,3146,3115,3084,3053,3023,2993,2963,2933,2904,2875,2847,2818,2790,2762,2735,2708,2681,2654,2628,2602,2576,2550,2525,2500,2475,2450,2426,2402,2378,2354,2331,2307,2284,2262,2239,2217,2195,2173,2151,2130,2109,2088,2067,2046,2026,2006,1986,1966,1947,1927,1908,1889,1870,1852,1833,1815,1797,1779,1761,1744,1726,1709,1692,1675,1659,1642,1626,1610,1594,1578,1562,1546,1531,1516,1501,1486,1471,1456,1442,1428,1413,1399,1385,1372,1358,1344,1331,1318,1305,1292,1279,1266,1253,1241,1229,1216,1204,1192,1180,1169,1157,1146,1134,1123,1112,1101,1090,1079,1068,1057,1047,1036,1026,1016,1006,996,986,976,966,957,947,938,928,919,910,901,892,883,874,866,857,849,840,832,823,815,807,799,791,783,775,768,760,752,745,738,730,723,716,709,702,695,688,681,674,667,661,654,648,641,635,628,622,616,610,604,598,592,586,580,574,569,563,557,552,546,541,535,530,525,520,514,509,504,499,494,489,484,480,475,470,465,461,456,452,447,443,438,434,430,425,421,417,413,409,405,401,397,393,389,385,381,377,373,370,366,362,359,355,352,348,345,341,338,334,331,328,325,321,318,315,312,309,306,303,300,297,294,291,288,285,282,279,277,274,271,268,266,263,260,258,255,253,250,248,245,243,240,238,236,233,231,229,226,224,222,220,217,215,213,211,209,207,205,203,201,199,197,195,193,191,189,187,185,183,182,180,178,176,174,173,171,169,168,166,164,163,161,159,158,156,155,153,152,150,149,147,146,144,143,141,140,138,137,136,134,133,132,130,129,128,126,125,124,123,122,120,119,118,117,116,114,113,112,111,110,109,108,107,106,105,103,102,101,100,99,98,97,96,95,95,94,93,92,91,90,89,88,87,86,85,85,84,83,82,81,80,80,79,78,77,77,76,75,74,73,73,72,71,71,70,69,68,68,67,66,66,65,64,64,63,63,62,61,61,60,59,59,58,58,57,57,56,55,55,54,54,53,53,52,52,51,51,50,50,49,49,48,48,47,47,46,46,45,45,44,44,43,43,43,42,42,41,41,41,40,40,39,39,39,38,38,37,37,37,36,36,36,35,35,34,34,34,33,33,33,32,32,32,31,31,31,31,30,30,30,29,29,29,28,28,28,28,27,27,27,26,26,26,26,25,25,25,25,24,24,24,24,23,23,23,23,22,22,22,22,22,21,21,21,21,20,20,20,20,20,19,19,19,19,19,18,18,18,18,18,18,17,17,17,17,17,17,16,16,16,16,16,16,15,15,15,15,15,15,14,14,14,14,14,14,14,13,13,13,13,13,13,13,12,12,12,12,12,12,12,12,11,11,11,11,11,11,11,11,11,10,10,10,10,10,10,10,10,10,10,9,9,9,9,9,9,9,9,9,9,8,8,8,8,8,8,8,8,8,8,8,8,7,7,7,7,7,7,7,7,7,7,7,7,7,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	
	short m_num_mag, m_num_mag_pow;  //2^7
	int m_num_mag_pow2; //2^28
	short m_amp_para, m_amp_para_double, m_amp_para_three;
	short m_snpramin, beta;
	short m_cosen_max, m_cosen_min;  // %:0.1 : 10
	int m_cosen_pmin, m_cosen_max_min;
	int m_linc, m_linc_move;
	int m_lwlen, N_wlen1;
	int m_cosen_pmax, m_w_global, m_wt_global;
	short* m_dataBuf;
	int*m_init_S_min, *m_init_S, *m_init_S_tmp;
	int* m_arr_temp;
	int *m_init_p1, *m_S_f;
	short *m_EN_cos, *m_EN_sin;
	int *m_post_SNR, *m_E_pr_SNR, *m_pr_SNR;
	int *m_v, *m_integra;
	int *m_Gh1, *m_G;
	int *m_q, *m_pp;
	short* m_h_global;
	int *m_cosen, *m_old_cosen;
	int* m_cosen_local, *m_cosen_global;
	int* m_plocal, *m_pglobal;

	short* m_ns_storage;
	unsigned int* m_abs_Y, *m_M;
	int* m_int_value, *m_expsub_value, *m_G_value; //��������ݴ�ռ�
	int* m_lamda_d;

	template<class T>
	T* file_read(const char* Filename);  //��ȡ�ļ�����ģ��

	void NoiseEstimation(int blockInd);
	void SpeechAbsenceEstm();
	int expintpow_solution(int v_subscript);
	int subexp_solution(int v_subscript);
	int Gvalue_solution(int Gh1_subscript, int pp_subscript);
};

