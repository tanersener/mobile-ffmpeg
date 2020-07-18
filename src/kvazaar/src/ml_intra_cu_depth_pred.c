/*****************************************************************************
 * This file is part of Kvazaar HEVC encoder.
 *
 * Copyright (C) 2013-2015 Tampere University of Technology and others (see
 * COPYING file).
 *
 * Kvazaar is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * Kvazaar is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Kvazaar.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "ml_intra_cu_depth_pred.h"


static int tree_predict_merge_depth_1(features_s* p_features, double* p_nb_iter, double* p_nb_bad)
{
		if (p_features->merge_variance <= 140.3129)
		{
				if (p_features->var_of_sub_var <= 569.6553)
				{
						if (p_features->merge_variance <= 20.8854)
						{
								*p_nb_iter = 19428.0;
								*p_nb_bad = 1740.0;
								return -1.0000;
						}
						else if (p_features->sub_variance_0 <= 9.1015)
						{
								if (p_features->merge_variance <= 39.132)
								{
										*p_nb_iter = 1166.0;
										*p_nb_bad = 358.0;
										return -1.0000;
								}
								else {
										*p_nb_iter = 1049.0;
										*p_nb_bad = 392.0;
										return 1.0000;
								}
						}
						else {
								*p_nb_iter = 9371.0;
								*p_nb_bad = 1805.0;
								return -1.0000;
						}
				}
				else if (p_features->sub_variance_2 <= 23.3193)
				{
						*p_nb_iter = 1059.0;
						*p_nb_bad = 329.0;
						return 1.0000;
				}
				else if (p_features->sub_variance_1 <= 30.7348)
				{
						*p_nb_iter = 1042.0;
						*p_nb_bad = 395.0;
						return 1.0000;
				}
				else {
						*p_nb_iter = 1756.0;
						*p_nb_bad = 588.0;
						return -1.0000;
				}
		}
		else if (p_features->merge_variance <= 857.8047)
		{
				if (p_features->var_of_sub_var <= 66593.5553)
				{
						if (p_features->sub_variance_0 <= 12.1697)
						{
								*p_nb_iter = 2006.0;
								*p_nb_bad = 374.0;
								return 1.0000;
						}
						else if (p_features->neigh_variance_C <= 646.8204)
						{
								if (p_features->neigh_variance_A <= 664.7609)
								{
										if (p_features->neigh_variance_B <= 571.2004)
										{
												if (p_features->var_of_sub_mean <= 4.1069)
												{
														*p_nb_iter = 1208.0;
														*p_nb_bad = 399.0;
														return 1.0000;
												}
												else if (p_features->var_of_sub_var <= 11832.6635)
												{
														*p_nb_iter = 8701.0;
														*p_nb_bad = 3037.0;
														return -1.0000;
												}
												else if (p_features->neigh_variance_A <= 142.298)
												{
														*p_nb_iter = 1025.0;
														*p_nb_bad = 290.0;
														return 1.0000;
												}
												else if (p_features->variance <= 394.4839)
												{
														*p_nb_iter = 1156.0;
														*p_nb_bad = 489.0;
														return 1.0000;
												}
												else {
														*p_nb_iter = 1150.0;
														*p_nb_bad = 503.0;
														return -1.0000;
												}
										}
										else {
												*p_nb_iter = 1777.0;
												*p_nb_bad = 558.0;
												return 1.0000;
										}
								}
								else {
										*p_nb_iter = 1587.0;
										*p_nb_bad = 411.0;
										return 1.0000;
								}
						}
						else {
								*p_nb_iter = 1980.0;
								*p_nb_bad = 474.0;
								return 1.0000;
						}
				}
				else {
						*p_nb_iter = 3613.0;
						*p_nb_bad = 475.0;
						return 1.0000;
				}
		}
		else {
				*p_nb_iter = 20926.0;
				*p_nb_bad = 1873.0;
				return 1.0000;
		}
}



static int tree_predict_merge_depth_2(features_s* p_features, double* p_nb_iter, double* p_nb_bad)
{
		if (p_features->merge_variance <= 119.4611)
		{
				if (p_features->var_of_sub_var <= 1078.0638)
				{
						if (p_features->neigh_variance_B <= 70.2189)
						{
								*p_nb_iter = 29253.0;
								*p_nb_bad = 3837.0;
								return -1.0000;
						}
						else if (p_features->variance <= 20.8711)
						{
								*p_nb_iter = 1292.0;
								*p_nb_bad = 458.0;
								return 2.0000;
						}
						else {
								*p_nb_iter = 1707.0;
								*p_nb_bad = 399.0;
								return -1.0000;
						}
				}
				else if (p_features->var_of_sub_var <= 3300.4034)
				{
						*p_nb_iter = 1554.0;
						*p_nb_bad = 675.0;
						return -1.0000;
				}
				else {
						*p_nb_iter = 1540.0;
						*p_nb_bad = 429.0;
						return 2.0000;
				}
		}
		else if (p_features->merge_variance <= 696.1989)
		{
				if (p_features->var_of_sub_var <= 31803.3242)
				{
						if (p_features->sub_variance_2 <= 10.3845)
						{
								*p_nb_iter = 3473.0;
								*p_nb_bad = 768.0;
								return 2.0000;
						}
						else if (p_features->neigh_variance_C <= 571.5329)
						{
								if (p_features->neigh_variance_B <= 492.8159)
								{
										if (p_features->neigh_variance_B <= 38.9672)
										{
												*p_nb_iter = 1887.0;
												*p_nb_bad = 588.0;
												return 2.0000;
										}
										else if (p_features->neigh_variance_A <= 380.5927)
										{
												if (p_features->sub_variance_1 <= 19.9678)
												{
														*p_nb_iter = 1686.0;
														*p_nb_bad = 721.0;
														return 2.0000;
												}
												else if (p_features->neigh_variance_A <= 66.6749)
												{
														*p_nb_iter = 1440.0;
														*p_nb_bad = 631.0;
														return 2.0000;
												}
												else {
														*p_nb_iter = 5772.0;
														*p_nb_bad = 2031.0;
														return -1.0000;
												}
										}
										else {
												*p_nb_iter = 1791.0;
												*p_nb_bad = 619.0;
												return 2.0000;
										}
								}
								else {
										*p_nb_iter = 1624.0;
										*p_nb_bad = 494.0;
										return 2.0000;
								}
						}
						else {
								*p_nb_iter = 1298.0;
								*p_nb_bad = 312.0;
								return 2.0000;
						}
				}
				else {
						*p_nb_iter = 4577.0;
						*p_nb_bad = 892.0;
						return 2.0000;
				}
		}
		else {
				*p_nb_iter = 21106.0;
				*p_nb_bad = 2744.0;
				return 2.0000;
		}
}



static int tree_predict_merge_depth_3(features_s* p_features, double* p_nb_iter, double* p_nb_bad)
{
		if (p_features->merge_variance <= 80.1487)
		{
				if (p_features->neigh_variance_C <= 83.7148)
				{
						*p_nb_iter = 29806.0;
						*p_nb_bad = 3603.0;
						return -1.0000;
				}
				else {
						*p_nb_iter = 1003.0;
						*p_nb_bad = 421.0;
						return 3.0000;
				}
		}
		else if (p_features->merge_variance <= 351.8138)
		{
				if (p_features->neigh_variance_C <= 255.4236)
				{
						if (p_features->neigh_variance_B <= 260.5349)
						{
								if (p_features->var_of_sub_var <= 6381.513)
								{
										if (p_features->neigh_variance_A <= 244.2556)
										{
												if (p_features->sub_variance_0 <= 4.75)
												{
														*p_nb_iter = 1290.0;
														*p_nb_bad = 525.0;
														return 3.0000;
												}
												else if (p_features->neigh_variance_B <= 16.9287)
												{
														*p_nb_iter = 1045.0;
														*p_nb_bad = 499.0;
														return 3.0000;
												}
												else {
														*p_nb_iter = 6901.0;
														*p_nb_bad = 2494.0;
														return -1.0000;
												}
										}
										else {
												*p_nb_iter = 1332.0;
												*p_nb_bad = 408.0;
												return 3.0000;
										}
								}
								else {
										*p_nb_iter = 2929.0;
										*p_nb_bad = 842.0;
										return 3.0000;
								}
						}
						else {
								*p_nb_iter = 2239.0;
								*p_nb_bad = 572.0;
								return 3.0000;
						}
				}
				else {
						*p_nb_iter = 2777.0;
						*p_nb_bad = 714.0;
						return 3.0000;
				}
		}
		else {
				*p_nb_iter = 30678.0;
				*p_nb_bad = 5409.0;
				return 3.0000;
		}
}



static int tree_predict_merge_depth_4(features_s* p_features, double* p_nb_iter, double* p_nb_bad)
{
		if (p_features->neigh_variance_C <= 240.2773)
		{
				if (p_features->neigh_variance_B <= 227.5898)
				{
						if (p_features->neigh_variance_A <= 195.4844)
						{
								if (p_features->variance <= 203.3086)
								{
										if (p_features->qp <= 32)
										{
												if (p_features->neigh_variance_C <= 102.2344)
												{
														if (p_features->neigh_variance_B <= 116.4961)
														{
																if (p_features->variance <= 89.4023)
																{
																		*p_nb_iter = 27398.0;
																		*p_nb_bad = 4665.0;
																		return -1.0000;
																}
																else {
																		*p_nb_iter = 1676.0;
																		*p_nb_bad = 795.0;
																		return 4.0000;
																}
														}
														else {
																*p_nb_iter = 1405.0;
																*p_nb_bad = 566.0;
																return 4.0000;
														}
												}
												else {
														*p_nb_iter = 2827.0;
														*p_nb_bad = 1173.0;
														return 4.0000;
												}
										}
										else {
												*p_nb_iter = 8871.0;
												*p_nb_bad = 822.0;
												return -1.0000;
										}
								}
								else {
										*p_nb_iter = 3162.0;
										*p_nb_bad = 718.0;
										return 4.0000;
								}
						}
						else {
								*p_nb_iter = 6154.0;
								*p_nb_bad = 1397.0;
								return 4.0000;
						}
				}
				else {
						*p_nb_iter = 9385.0;
						*p_nb_bad = 1609.0;
						return 4.0000;
				}
		}
		else {
				*p_nb_iter = 19122.0;
				*p_nb_bad = 2960.0;
				return 4.0000;
		}
}



static int tree_predict_split_depth_0(features_s* p_features, double* p_nb_iter, double* p_nb_bad)
{
		if (p_features->var_of_sub_var <= 12754.7856)
		{
				if (p_features->var_of_sub_var <= 137.9034)
				{
						*p_nb_iter = 25155.0;
						*p_nb_bad = 2959.0;
						return 0.0000;
				}
				else if (p_features->sub_variance_2 <= 13.2892)
				{
						*p_nb_iter = 1080.0;
						*p_nb_bad = 383.0;
						return -1.0000;
				}
				else if (p_features->variance <= 564.1738)
				{
						if (p_features->var_of_sub_var <= 1185.4728)
						{
								*p_nb_iter = 6067.0;
								*p_nb_bad = 1699.0;
								return 0.0000;
						}
						else if (p_features->var_of_sub_mean <= 46.2388)
						{
								if (p_features->sub_variance_0 <= 46.8708)
								{
										*p_nb_iter = 1088.0;
										*p_nb_bad = 377.0;
										return -1.0000;
								}
								else if (p_features->sub_variance_3 <= 61.4213)
								{
										*p_nb_iter = 1183.0;
										*p_nb_bad = 498.0;
										return -1.0000;
								}
								else {
										*p_nb_iter = 3416.0;
										*p_nb_bad = 1373.0;
										return 0.0000;
								}
						}
						else {
								*p_nb_iter = 3769.0;
								*p_nb_bad = 1093.0;
								return 0.0000;
						}
				}
				else {
						*p_nb_iter = 1036.0;
						*p_nb_bad = 434.0;
						return -1.0000;
				}
		}
		else if (p_features->var_of_sub_var <= 98333.8279)
		{
				if (p_features->variance <= 987.2333)
				{
						if (p_features->var_of_sub_var <= 37261.2896)
						{
								if (p_features->variance <= 238.2248)
								{
										*p_nb_iter = 1323.0;
										*p_nb_bad = 301.0;
										return -1.0000;
								}
								else if (p_features->var_of_sub_var <= 17347.3971)
								{
										*p_nb_iter = 1215.0;
										*p_nb_bad = 550.0;
										return 0.0000;
								}
								else if (p_features->qp <= 22)
								{
										*p_nb_iter = 1000.0;
										*p_nb_bad = 493.0;
										return 0.0000;
								}
								else {
										*p_nb_iter = 2640.0;
										*p_nb_bad = 1121.0;
										return -1.0000;
								}
						}
						else {
								*p_nb_iter = 5188.0;
								*p_nb_bad = 1248.0;
								return -1.0000;
						}
				}
				else {
						*p_nb_iter = 2323.0;
						*p_nb_bad = 274.0;
						return -1.0000;
				}
		}
		else {
				*p_nb_iter = 21357.0;
				*p_nb_bad = 1829.0;
				return -1.0000;
		}
}


static int tree_predict_split_depth_1(features_s* p_features, double* p_nb_iter, double* p_nb_bad)
{
		if (p_features->var_of_sub_var <= 1138.9473)
		{
				*p_nb_iter = 32445.0;
				*p_nb_bad = 4580.0;
				return 1.0000;
		}
		else if (p_features->var_of_sub_var <= 27289.2117)
		{
				if (p_features->sub_variance_1 <= 12.0603)
				{
						*p_nb_iter = 1900.0;
						*p_nb_bad = 401.0;
						return -1.0000;
				}
				else if (p_features->var_of_sub_var <= 5841.4773)
				{
						if (p_features->variance <= 72.4175)
						{
								*p_nb_iter = 1000.0;
								*p_nb_bad = 356.0;
								return -1.0000;
						}
						else if (p_features->neigh_variance_A <= 633.8163)
						{
								*p_nb_iter = 5279.0;
								*p_nb_bad = 1961.0;
								return 1.0000;
						}
						else {
								*p_nb_iter = 1176.0;
								*p_nb_bad = 527.0;
								return -1.0000;
						}
				}
				else if (p_features->sub_variance_0 <= 38.3035)
				{
						*p_nb_iter = 1251.0;
						*p_nb_bad = 293.0;
						return -1.0000;
				}
				else if (p_features->neigh_variance_B <= 664.9494)
				{
						if (p_features->sub_variance_3 <= 45.8181)
						{
								*p_nb_iter = 1276.0;
								*p_nb_bad = 471.0;
								return -1.0000;
						}
						else if (p_features->sub_variance_3 <= 404.3086)
						{
								if (p_features->sub_variance_1 <= 99.8715)
								{
										*p_nb_iter = 1005.0;
										*p_nb_bad = 435.0;
										return -1.0000;
								}
								else if (p_features->sub_variance_0 <= 282.3064)
								{
										*p_nb_iter = 1370.0;
										*p_nb_bad = 539.0;
										return 1.0000;
								}
								else {
										*p_nb_iter = 1013.0;
										*p_nb_bad = 495.0;
										return -1.0000;
								}
						}
						else {
								*p_nb_iter = 1000.0;
								*p_nb_bad = 379.0;
								return -1.0000;
						}
				}
				else {
						*p_nb_iter = 2270.0;
						*p_nb_bad = 679.0;
						return -1.0000;
				}
		}
		else {
				*p_nb_iter = 29015.0;
				*p_nb_bad = 3950.0;
				return -1.0000;
		}
}


static int tree_predict_split_depth_2(features_s* p_features, double* p_nb_iter, double* p_nb_bad)
{
		if (p_features->var_of_sub_var <= 2597.4529)
		{
				if (p_features->var_of_sub_var <= 146.7734)
				{
						*p_nb_iter = 23216.0;
						*p_nb_bad = 1560.0;
						return 2.0000;
				}
				else if (p_features->merge_variance <= 259.6952)
				{
						*p_nb_iter = 7470.0;
						*p_nb_bad = 1902.0;
						return 2.0000;
				}
				else if (p_features->qp <= 27)
				{
						if (p_features->variance <= 73.9929)
						{
								*p_nb_iter = 1138.0;
								*p_nb_bad = 486.0;
								return -1.0000;
						}
						else {
								*p_nb_iter = 1619.0;
								*p_nb_bad = 716.0;
								return 2.0000;
						}
				}
				else {
						*p_nb_iter = 2425.0;
						*p_nb_bad = 861.0;
						return 2.0000;
				}
		}
		else if (p_features->var_of_sub_var <= 60850.5208)
		{
				if (p_features->var_of_sub_var <= 10144.602)
				{
						if (p_features->neigh_variance_C <= 926.8972)
						{
								if (p_features->sub_variance_0 <= 26.6006)
								{
										*p_nb_iter = 1796.0;
										*p_nb_bad = 586.0;
										return -1.0000;
								}
								else if (p_features->neigh_variance_A <= 493.5849)
								{
										if (p_features->neigh_variance_A <= 72.9516)
										{
												*p_nb_iter = 1326.0;
												*p_nb_bad = 557.0;
												return -1.0000;
										}
										else if (p_features->variance <= 156.4014)
										{
												*p_nb_iter = 1210.0;
												*p_nb_bad = 563.0;
												return -1.0000;
										}
										else {
												*p_nb_iter = 1920.0;
												*p_nb_bad = 817.0;
												return 2.0000;
										}
								}
								else {
										*p_nb_iter = 1106.0;
										*p_nb_bad = 437.0;
										return -1.0000;
								}
						}
						else {
								*p_nb_iter = 1001.0;
								*p_nb_bad = 278.0;
								return -1.0000;
						}
				}
				else {
						*p_nb_iter = 13068.0;
						*p_nb_bad = 3612.0;
						return -1.0000;
				}
		}
		else {
				*p_nb_iter = 22705.0;
				*p_nb_bad = 2687.0;
				return -1.0000;
		}
}



static int tree_predict_split_depth_3(features_s* p_features, double* p_nb_iter, double* p_nb_bad)
{
		if (p_features->var_of_sub_var <= 818.5173)
		{
				if (p_features->merge_variance <= 62.7641)
				{
						*p_nb_iter = 20568.0;
						*p_nb_bad = 767.0;
						return 3.0000;
				}
				else if (p_features->qp <= 27)
				{
						if (p_features->variance <= 9.4219)
						{
								*p_nb_iter = 1255.0;
								*p_nb_bad = 206.0;
								return 3.0000;
						}
						else if (p_features->merge_variance <= 375.2185)
						{
								*p_nb_iter = 3999.0;
								*p_nb_bad = 1321.0;
								return 3.0000;
						}
						else {
								*p_nb_iter = 1786.0;
								*p_nb_bad = 817.0;
								return -1.0000;
						}
				}
				else {
						*p_nb_iter = 5286.0;
						*p_nb_bad = 737.0;
						return 3.0000;
				}
		}
		else if (p_features->var_of_sub_var <= 37332.3018)
		{
				if (p_features->var_of_sub_var <= 7585.0282)
				{
						if (p_features->qp <= 32)
						{
								if (p_features->neigh_variance_C <= 330.2178)
								{
										if (p_features->sub_variance_0 <= 8.5273)
										{
												*p_nb_iter = 1114.0;
												*p_nb_bad = 346.0;
												return -1.0000;
										}
										else if (p_features->neigh_variance_B <= 221.5469)
										{
												if (p_features->var_of_sub_var <= 1989.7928)
												{
														*p_nb_iter = 1539.0;
														*p_nb_bad = 606.0;
														return 3.0000;
												}
												else if (p_features->variance <= 155.5974)
												{
														*p_nb_iter = 1298.0;
														*p_nb_bad = 634.0;
														return 3.0000;
												}
												else {
														*p_nb_iter = 1076.0;
														*p_nb_bad = 456.0;
														return -1.0000;
												}
										}
										else {
												*p_nb_iter = 1644.0;
												*p_nb_bad = 639.0;
												return -1.0000;
										}
								}
								else {
										*p_nb_iter = 2401.0;
										*p_nb_bad = 713.0;
										return -1.0000;
								}
						}
						else if (p_features->merge_variance <= 281.9509)
						{
								*p_nb_iter = 1020.0;
								*p_nb_bad = 262.0;
								return 3.0000;
						}
						else {
								*p_nb_iter = 1278.0;
								*p_nb_bad = 594.0;
								return -1.0000;
						}
				}
				else {
						*p_nb_iter = 10507.0;
						*p_nb_bad = 2943.0;
						return -1.0000;
				}
		}
		else {
				*p_nb_iter = 25229.0;
				*p_nb_bad = 3060.0;
				return -1.0000;
		}
}



 /**
 *	Allocate the structure and buffer
 */
ml_intra_ctu_pred_t* kvz_init_ml_intra_depth_const() {
	ml_intra_ctu_pred_t* ml_intra_depth_ctu = NULL;
	// Allocate the ml_intra_ctu_pred_t strucutre
	ml_intra_depth_ctu = MALLOC(ml_intra_ctu_pred_t, 1);
	if (!ml_intra_depth_ctu) {
		fprintf(stderr, "Memory allocation failed!\n");
		assert(0);
	}
	// Set the number of number of deth add to 1 by default
	ml_intra_depth_ctu->i_nb_addDepth = 1;
	// Set the extra Upper Expansion in the upper_depth enabled by default 
	ml_intra_depth_ctu->b_extra_up_exp = true;

	// Allocate the depth matrices 
	ml_intra_depth_ctu->_mat_lower_depth = MALLOC(uint8_t, LCU_DEPTH_MAT_SIZE);
	if (!ml_intra_depth_ctu->_mat_lower_depth) {
		fprintf(stderr, "Memory allocation failed!\n");
		assert(0);
	}
	ml_intra_depth_ctu->_mat_upper_depth = MALLOC(uint8_t, LCU_DEPTH_MAT_SIZE);
	if (!ml_intra_depth_ctu->_mat_upper_depth) {
		fprintf(stderr, "Memory allocation failed!\n");
		assert(0);
	}

	return ml_intra_depth_ctu;
};

/**
*	Fee the bufer and structure
*/
void kvz_end_ml_intra_depth_const(ml_intra_ctu_pred_t* ml_intra_depth_ctu) {
	FREE_POINTER(ml_intra_depth_ctu->_mat_lower_depth);
	FREE_POINTER(ml_intra_depth_ctu->_mat_upper_depth);

	FREE_POINTER(ml_intra_depth_ctu);
}

// Initialize to 0 all the features
static void features_init_array(features_s* arr_features, int16_t _size, int _qp)//, int _NB_pixels)
{
	int16_t i = 0;
	for (i = 0; i < _size; ++i)
	{
		arr_features[i].variance = 0.0;
		arr_features[i].sub_variance_0 = 0.0;
		arr_features[i].sub_variance_1 = 0.0;
		arr_features[i].sub_variance_2 = 0.0;
		arr_features[i].sub_variance_3 = 0.0;
		arr_features[i].merge_variance = 0.0;
		arr_features[i].neigh_variance_A = 0.0;
		arr_features[i].neigh_variance_B = 0.0;
		arr_features[i].neigh_variance_C = 0.0;
		arr_features[i].var_of_sub_mean = 0.0;
		arr_features[i].qp = _qp;
		//arr_features[i].NB_pixels = _NB_pixels;
	}
}

/*!
* \brief Compute the average of a block inside an 8 bits 2D vector.
*
* \param _mat_src 	First depth map.
* \param _x 		X coordinate of the start of the block inside the matrix.
* \param _x_end 	X coordinate of the end of the block inside the matrix.
* \param _y 		Y coordinate of the start of the block inside the matrix.
* \param _y_end     Y coordinate of the end of the block inside the matrix.
* \param _width     Width of the matrix.
* \return average value of the block, -1 if error.
*/
static INLINE double vect_average_blck_int8(const uint8_t* _mat_src, size_t _x, size_t _x_end, size_t _y, size_t _y_end, size_t _width)
{
	if (_mat_src == NULL)
	{
		fprintf(stderr, "null pointer as parameter.");
		assert(0);
		return -1.0;
	}
	double   block_size = (double)(_x_end - _x) * (double)(_y_end - _y);
	double   avg_vect = 0.0;
	//STD_print_matrix(_mat_src,64, 64);
	for (size_t i_y = _y; i_y < _y_end; ++i_y)
	{
		size_t i_y_line = i_y * _width;
		for (size_t i_x = _x; i_x < _x_end; ++i_x)
		{
			avg_vect = avg_vect + (double)_mat_src[i_x + i_y_line];
		}
	}
	return avg_vect / (double)(block_size);
}

/*!
* \brief Compute the variance of a block inside an 8 bits 2D vector.
*
* \param _mat_src 	First depth map.
* \param _x 		X coordinate of the start of the block inside the matrix.
* \param _x_end 	X coordinate of the end of the block inside the matrix.
* \param _y 		Y coordinate of the start of the block inside the matrix.
* \param _y_end     Y coordinate of the end of the block inside the matrix.
* \param _avg_blck  Average value of the block.
* \param _width     Width of the matrix.
* \return average value of the block, -1 if error.
*/
static INLINE double vect_variance_blck_int8(const uint8_t* _mat_src, size_t _x, size_t _x_end, size_t _y, size_t _y_end, double _avg_blck, size_t _width)
{
	if (_mat_src == NULL)
	{
		fprintf(stderr, "null pointer as parameter.");
		assert(0);
		return -1.0;
	}
	double   block_size = (double)(_x_end - _x) * (double)(_y_end - _y);
	double   variance = 0.0;
	for (size_t i_y = _y; i_y < _y_end; ++i_y)
	{
		size_t i_y_line = i_y * _width;
		for (size_t i_x = _x; i_x < _x_end; ++i_x)
		{
			variance = variance + pow2((double)(_mat_src[i_x + i_y_line]) - _avg_blck);
		}
	}
	return variance / (double)(block_size);
}

/*!
* \brief Function to compute the average and the variance of a pixel block inside of a LCU.
*
* \param arr_luma_px Array of the pixels of the block.
* \param i_xLcu      X coordinate of the lcu.
* \param i_yLcu      Y coordinate of the lcu.
* \param i_xBlck     X coordinate of the pixel block inside the LCU.
* \param i_yBlck     Y coordinate of the pixel block inside the LCU.
* \param i_blockSize Size of the block in pixels (4, 8, 16, 32 or 64).
* \param i_width     Width  of the frame in pixels.
* \param i_height    Height of the frame in pixels.
* \param p_average   Pointer to be filled with the average.
* \param p_variance  Pointer to be filled with the variance;
* \return None.
*/
static INLINE void features_var_avg_blck(uint8_t* arr_luma_px, uint32_t i_xLcu, uint32_t i_yLcu,
	uint32_t i_xBlck, uint32_t i_yBlck, uint8_t i_blockSize,
	int32_t i_width, int32_t i_height,
	double* p_average, double* p_variance)
{
	uint32_t iXMax = CR_XMAX(i_xLcu, i_blockSize + i_xBlck, i_width);
	uint32_t iYMax = CR_YMAX(i_yLcu, i_blockSize + i_yBlck, i_height);
	*p_average = vect_average_blck_int8(arr_luma_px, i_xBlck, iXMax, i_yBlck, iYMax, 64);
	*p_variance = vect_variance_blck_int8(arr_luma_px, i_xBlck, iXMax, i_yBlck, iYMax, (*p_average), 64);
}

/*!
* \brief Function to combine the variance and mean values of four blocks.
*
* \param arr_var     Array of 4*4 variances of the LCU.
* \param arr_avgLuma Array of 4*4 mean values of the LCU.
* \param i_x         X coordinate of the top left block.
* \param i_y         Y coordinate of the top left block.
* \param i_depth     Depth of the blocks (0,1,2,3 or 4).
* \param p_varianceC Pointer to be filled with the combined variance.
* \param p_avgLumaC  Pointer to be filled with the combined average.
* \return None.
*/
static INLINE void features_combine_var(double* arr_var, double* arr_avgLuma, uint32_t i_x, uint32_t i_y, uint32_t i_depth,
	double* p_varianceC, double* p_avgLumaC)
{
	double d_var_temp_1 = 0.0;
	double d_var_temp_2 = 0.0;
	double d_avg_temp_1 = 0.0;
	double d_avg_temp_2 = 0.0;

	int16_t i_subCU = (i_x + (i_y << 4)) << (4 - i_depth);
	int16_t i_rows = (16 << (3 - i_depth));

	int16_t i_sb0 = i_subCU; 									/*!< Top left sub block index */
	int16_t i_sb1 = i_subCU + (1 << (3 - i_depth));			/*!< Top right sub block index */
	int16_t i_sb2 = i_subCU + i_rows;							/*!< Bottom left sub block index */
	int16_t i_sb3 = i_subCU + i_rows + (1 << (3 - i_depth)); 	/*!< Bottom right sub block index */

	d_avg_temp_1 = (arr_avgLuma[i_sb0] + arr_avgLuma[i_sb1]) / 2.0;
	d_avg_temp_2 = (arr_avgLuma[i_sb2] + arr_avgLuma[i_sb3]) / 2.0;

	d_var_temp_1 = (2.0 * (arr_var[i_sb0] + arr_var[i_sb1]) + pow2((arr_avgLuma[i_sb0] - arr_avgLuma[i_sb1]))) / 4.0;
	d_var_temp_2 = (2.0 * (arr_var[i_sb2] + arr_var[i_sb3]) + pow2((arr_avgLuma[i_sb2] - arr_avgLuma[i_sb3]))) / 4.0;

	if (p_avgLumaC)
	{
		*p_avgLumaC = (d_avg_temp_1 + d_avg_temp_2) / 2.0;
	}
	*p_varianceC = (2.0 * (d_var_temp_1 + d_var_temp_2) + pow2(d_avg_temp_1 - d_avg_temp_2)) / 4.0;
}


/*!
* \brief Function to combine the variance of the mean values of the sub block.
*
* \param arr_avgLuma   	Array of 4*4 mean values of the LCU.
* \param i_sb0         	Index of the sub_blocks 0 in the array of avg values .
* \param i_sb1     		Index of the sub_blocks 1 in the array of avg values.
* \param i_sb2 			Index of the sub_blocks 2 in the array of avg values
* \param i_sb3  		Index of the sub_blocks 3 in the array of avg values
* \return variance of the average of the sub blocks.
*/
static INLINE double features_get_var_of_sub_mean(double* arr_avgLuma, int16_t i_sb0, int16_t i_sb1, int16_t i_sb2, int16_t i_sb3)
{
	double d_var = 0.0;
	double d_avg = (arr_avgLuma[i_sb0] + arr_avgLuma[i_sb1] + arr_avgLuma[i_sb2] + arr_avgLuma[i_sb3]) / 4.0;
	d_var = pow2(arr_avgLuma[i_sb0] - d_avg);
	d_var = pow2(arr_avgLuma[i_sb1] - d_avg) + d_var;
	d_var = pow2(arr_avgLuma[i_sb2] - d_avg) + d_var;
	d_var = pow2(arr_avgLuma[i_sb3] - d_avg) + d_var;
	return d_var / 4.0;
}

/*!
* \brief Build the neighboring variances of four cu's.
*
* \param arr_features   Array of features for current depth.
* \param _x         	X position of the first cu in the array.
* \param _y     		Y position of the first cu in the array.
* \param _depth 		Evaluated depth.
* \return None.
*/
static void features_var_neighbor(features_s* arr_features, int16_t _x, int16_t _y, int16_t _depth)
{
	int16_t i_cu0 = (_x - 1) + ((_y - 1) << _depth);
	int16_t i_cu1 = (_x)+((_y - 1) << _depth);
	int16_t i_cu2 = (_x - 1) + (_y << _depth);
	int16_t i_cu3 = _x + (_y << _depth);

	arr_features[i_cu0].neigh_variance_A = arr_features[i_cu1].variance;
	arr_features[i_cu0].neigh_variance_B = arr_features[i_cu2].variance;
	arr_features[i_cu0].neigh_variance_C = arr_features[i_cu3].variance;


	arr_features[i_cu1].neigh_variance_A = arr_features[i_cu0].variance;
	arr_features[i_cu1].neigh_variance_B = arr_features[i_cu2].variance;
	arr_features[i_cu1].neigh_variance_C = arr_features[i_cu3].variance;


	arr_features[i_cu2].neigh_variance_A = arr_features[i_cu0].variance;
	arr_features[i_cu2].neigh_variance_B = arr_features[i_cu1].variance;
	arr_features[i_cu2].neigh_variance_C = arr_features[i_cu3].variance;


	arr_features[i_cu3].neigh_variance_A = arr_features[i_cu0].variance;
	arr_features[i_cu3].neigh_variance_B = arr_features[i_cu1].variance;
	arr_features[i_cu3].neigh_variance_C = arr_features[i_cu2].variance;
}


/*!
* \brief Extract the features from the pixels for a given different depth.
*
* \param arr_features 		Array of features to be retrieved for the current depth.
* \param i_depth 			Depth to be evaluated.
* \param arr_var 			Array of 16*16 variances.
* \param arr_avg 			Array of 16*16 average lumas.
* \return None.
*/
static void features_compute(features_s* arr_features, uint8_t i_depth, double* arr_var, double* arr_avg)
{
	double d_avgLumaC;

	int8_t i_nbBlock = (1 << i_depth);

	for (int8_t y = 0; y < i_nbBlock; ++y)
	{
		for (int8_t x = 0; x < i_nbBlock; ++x)
		{
			int16_t i_cu = x + (y << i_depth);
			if (i_depth == 4)
			{
				arr_features[i_cu].variance = arr_var[i_cu];
			}
			else
			{
				features_combine_var(arr_var, arr_avg, x, y, i_depth, &arr_features[i_cu].variance, &d_avgLumaC);
				int16_t i_CU_4 = (x << (4 - i_depth)) + (y << (8 - i_depth));
				int16_t i_rows = (16 << (3 - i_depth));
				arr_features[i_cu].var_of_sub_mean = features_get_var_of_sub_mean(arr_avg,
					i_CU_4,
					i_CU_4 + (1 << (3 - i_depth)),
					i_CU_4 + i_rows,
					i_CU_4 + i_rows + (1 << (3 - i_depth)));
				arr_avg[i_CU_4] = d_avgLumaC;
				arr_var[i_CU_4] = arr_features[i_cu].variance;
			}
			if (x % 2 == 1 &&
				y % 2 == 1)
			{
				features_var_neighbor(arr_features, x, y, i_depth);
			}

		}
	}
}



/*!
* \brief Set the features Sub_var from the sub level for a given different depth.
*
* \param arr_features 		Array of features to be retrieved for the current depth.
* \param arr_sub_features	Array of features to be retrieved for the sub depth (depth - 1).
* \param i_rdepth 			Depth to be evaluated.

* \return None.
*/
static void features_sub_var(features_s* arr_features, features_s* arr_sub_features, uint8_t i_depth)
{
	int8_t i_nbBlock = (1 << i_depth);

	for (int8_t y = 0; y < i_nbBlock; ++y)
	{
		for (int8_t x = 0; x < i_nbBlock; ++x)
		{
			int16_t i_cu = x + (y << i_depth);
			int16_t i_sb0 = (x << 1) + (y << (2 + i_depth)); 					/*!< Top left sub block index */
			int16_t i_sb1 = (x << 1) + 1 + (y << (2 + i_depth)); 		    	/*!< Top right sub block index */
			int16_t i_sb2 = (x << 1) + (((y << 1) + 1) << (1 + i_depth)); 		/*!< Bottom left sub block index */
			int16_t i_sb3 = (x << 1) + 1 + (((y << 1) + 1) << (1 + i_depth));  /*!< Bottom right sub block index */


			arr_features[i_cu].sub_variance_0 = arr_sub_features[i_sb0].variance;
			arr_features[i_cu].sub_variance_1 = arr_sub_features[i_sb1].variance;
			arr_features[i_cu].sub_variance_2 = arr_sub_features[i_sb2].variance;
			arr_features[i_cu].sub_variance_3 = arr_sub_features[i_sb3].variance;

		}
	}
}


/*!
* \brief Set the features Merge_var from the up level for a given different depth.
*
* \param arr_features 		Array of features to be retrieved for the current depth.
* \param arr_up_features	Array of features to be retrieved for the upper depth (depth - 1).
* \param i_rdepth 			Depth to be evaluated.

* \return None.
*/
static void features_merge_var(features_s* arr_features, features_s* arr_up_features, uint8_t i_rdepth)
{
	uint8_t i_depth = i_rdepth - 1;
	int8_t 	i_nbBlock = (1 << i_depth);

	for (int8_t y = 0; y < i_nbBlock; ++y)
	{
		for (int8_t x = 0; x < i_nbBlock; ++x)
		{
			int16_t i_cu = x + (y << i_depth);
			int16_t i_sb0 = (x << 1) + (y << (2 + i_depth)); 					/*!< Top left sub block index */
			int16_t i_sb1 = (x << 1) + 1 + (y << (2 + i_depth)); 		    	/*!< Top right sub block index */
			int16_t i_sb2 = (x << 1) + (((y << 1) + 1) << (1 + i_depth)); 		/*!< Bottom left sub block index */
			int16_t i_sb3 = (x << 1) + 1 + (((y << 1) + 1) << (1 + i_depth));  /*!< Bottom right sub block index */

			arr_features[i_sb0].merge_variance = arr_up_features[i_cu].variance;
			arr_features[i_sb1].merge_variance = arr_up_features[i_cu].variance;
			arr_features[i_sb2].merge_variance = arr_up_features[i_cu].variance;
			arr_features[i_sb3].merge_variance = arr_up_features[i_cu].variance;

		}
	}
}


/*!
* \brief Set the features Var_of_sub_var from the sub level for a given different depth.
*
* \param arr_features 		Array of features to be retrieved for the current depth.
* \param i_rdepth 			Depth to be evaluated.

* \return None.
*/
static void features_var_of_sub_var(features_s* arr_features, uint8_t i_depth)
{
	int8_t i_nbBlock = (1 << i_depth);

	for (int8_t y = 0; y < i_nbBlock; ++y)
	{
		for (int8_t x = 0; x < i_nbBlock; ++x)
		{
			int16_t i_cu = x + (y << i_depth);
			double d_var = 0.0;
			double d_avg = (arr_features[i_cu].sub_variance_0 + arr_features[i_cu].sub_variance_1 + arr_features[i_cu].sub_variance_2 + arr_features[i_cu].sub_variance_3) / 4.0;

			d_var = pow2(arr_features[i_cu].sub_variance_0 - d_avg);
			d_var = pow2(arr_features[i_cu].sub_variance_1 - d_avg) + d_var;
			d_var = pow2(arr_features[i_cu].sub_variance_2 - d_avg) + d_var;
			d_var = pow2(arr_features[i_cu].sub_variance_3 - d_avg) + d_var;
			arr_features[i_cu].var_of_sub_var = d_var / 4.0;
		}
	}
}


/*!
* \brief Extract the features from the pixels for all the depth.
*
* \param main_handler 		Pointer to the main high level reduction handler.
* \param p_state      		Pointer to the state of the current LCU.
* \param arr_features_4 	Array of features for level of depth 4.
* \param arr_features_8 	Array of features for level of depth 3.
* \param arr_features_16 	Array of features for level of depth 2.
* \param arr_features_32	Array of features for level of depth 1.
* \param p_features64	    Pointer to the features of depth 0.
* \return None.
*/
static void features_compute_all(features_s* arr_features[5], uint8_t* luma_px)
{

	uint32_t x_px = 0; /*!< Top left X of the lcu */
	uint32_t y_px = 0; /*!< Top left Y of the lcu */
	double variance[256] = { 0.0 };
	double avg_luma[256] = { 0.0 };


	features_s* arr_features_4 = arr_features[4];
	features_s* arr_features_8 = arr_features[3];
	features_s* arr_features_16 = arr_features[2];
	features_s* arr_features_32 = arr_features[1];
	features_s* p_features64 = arr_features[0];

	/*!< Compute the variance for all 4*4 blocs */
	for (int8_t y = 0; y < 8; ++y)
	{
		for (int8_t x = 0; x < 8; ++x)
		{
			int16_t x_blck = (x << 1);
			int16_t y_blck = (y << 1);
			features_var_avg_blck(luma_px, x_px, y_px, x_blck << 2, y_blck << 2, 4, LCU_WIDTH, LCU_WIDTH,
				&avg_luma[CR_GET_CU_D4(x_blck, y_blck, 4)],
				&variance[CR_GET_CU_D4(x_blck, y_blck, 4)]);

			features_var_avg_blck(luma_px, x_px, y_px, (x_blck + 1) << 2, y_blck << 2, 4, LCU_WIDTH, LCU_WIDTH,
				&avg_luma[CR_GET_CU_D4(x_blck + 1, y_blck, 4)],
				&variance[CR_GET_CU_D4(x_blck + 1, y_blck, 4)]);
			features_var_avg_blck(luma_px, x_px, y_px, x_blck << 2, (y_blck + 1) << 2, 4, LCU_WIDTH, LCU_WIDTH,
				&avg_luma[CR_GET_CU_D4(x_blck, y_blck + 1, 4)],
				&variance[CR_GET_CU_D4(x_blck, y_blck + 1, 4)]);
			features_var_avg_blck(luma_px, x_px, y_px, (x_blck + 1) << 2, (y_blck + 1) << 2, 4, LCU_WIDTH, LCU_WIDTH,
				&avg_luma[CR_GET_CU_D4(x_blck + 1, y_blck + 1, 4)],
				&variance[CR_GET_CU_D4(x_blck + 1, y_blck + 1, 4)]);

		}
	}

	/* Compute the generic features of the all depth */
	features_compute(arr_features_4, 4, variance, avg_luma);
	features_compute(arr_features_8, 3, variance, avg_luma);
	features_compute(arr_features_16, 2, variance, avg_luma);
	features_compute(arr_features_32, 1, variance, avg_luma);
	features_compute(p_features64, 0, variance, avg_luma);

	/* Set the Sub_var features for the depth 3, 2, 1, 0*/
	features_sub_var(arr_features_8, arr_features_4, 3);
	features_sub_var(arr_features_16, arr_features_8, 2);
	features_sub_var(arr_features_32, arr_features_16, 1);
	features_sub_var(p_features64, arr_features_32, 0);

	/* Set the Merge_var features for the depth 4, 3, 2, 1*/
	features_merge_var(arr_features_4, arr_features_8, 4);
	features_merge_var(arr_features_8, arr_features_16, 3);
	features_merge_var(arr_features_16, arr_features_32, 2);
	features_merge_var(arr_features_32, p_features64, 1);

	/* Compute the Var_of_sub_var for the depth 3, 2, 1, 0*/
	features_var_of_sub_var(arr_features_8, 3);
	features_var_of_sub_var(arr_features_16, 2);
	features_var_of_sub_var(arr_features_32, 1);
	features_var_of_sub_var(p_features64, 0);

}

/*!
* \brief Compute the constrain on the neighboring depth of a cu for
*        a given depth for a BU approach
*
* \param arr_depthMap 	8*8 depth map.
* \param _x      		X coordinate of the cu in the 8*8 depth map;
* \param _y      		Y coordinate of the cu in the 8*8 depth map;
* \param _depth      	Current depth tested.
* \param _level  		number of depth gap that we want
* \return 1 if the predictions should be tested for this cu, 0 else.
*/
static int neighbor_constrain_bu(uint8_t* arr_depthMap, int _x, int _y, int _depth, int _level)
{
	int nb_block = (8 >> (_depth)) << 1;
	for (int y = _y; y < _y + nb_block; ++y)
	{
		for (int x = _x; x < _x + nb_block; ++x)
		{
			if (arr_depthMap[x + (y << 3)] - _level >= _depth)
				return 0;
		}
	}
	return 1;
}



static int8_t combined_tree_function(int8_t merge_prediction[4], int8_t split_prediction, uint8_t test_id, uint8_t depth)
{
	int8_t prediction;
	int8_t pred_merge_tmp = 0; // NUmber of sub-blocks non merge (=d)
	for (int8_t i = 0; i < 4; i++) {
		pred_merge_tmp += (merge_prediction[i] > 0) ? 1 : 0;
	}
	switch (test_id) {// We don't merge (-1) if :
	case 0: // At least one sub block non merge
		prediction = (pred_merge_tmp >= 1) ? depth : -1;
		break;
	case 1: // At least two sub blocks non merge
		prediction = (pred_merge_tmp >= 2) ? depth : -1;
		break;
	case 2: // At least three sub blocks non merge
		prediction = (pred_merge_tmp >= 3) ? depth : -1;
		break;
	case 3: // All sub blocks non merge
		prediction = (pred_merge_tmp >= 4) ? depth : -1;
		break;
	case 4: // Up bock non merge ( = split)
		prediction = (split_prediction == -1) ? depth : -1;
		break;
	case 5: // (At least one sub block non merge) & Up block non merge
		prediction = ((pred_merge_tmp >= 1) && (split_prediction == -1)) ? depth : -1;
		break;
	case 6: // (At least two sub blocks non merge) & Up block non merge
		prediction = ((pred_merge_tmp >= 2) && (split_prediction == -1)) ? depth : -1;
		break;
	case 7: // (At least three sub blocks non merge) & Up block non merge
		prediction = ((pred_merge_tmp >= 3) && (split_prediction == -1)) ? depth : -1;
		break;
	case 8: // (All sub blocks non merge) & Up block non merge
		prediction = ((pred_merge_tmp >= 4) && (split_prediction == -1)) ? depth : -1;
		break;
	case 9: // (At least one sub block non merge) | Up block non merge
		prediction = ((pred_merge_tmp >= 1) || (split_prediction == -1)) ? depth : -1;
		break;
	case 10: // (At least two sub blocks non merge) | Up block non merge
		prediction = ((pred_merge_tmp >= 2) || (split_prediction == -1)) ? depth : -1;
		break;
	case 11: // (At least three sub blocks non merge) | Up block non merge
		prediction = ((pred_merge_tmp >= 3) || (split_prediction == -1)) ? depth : -1;
		break;
	case 12: // (All sub blocks non merge) | Up block non merge
		prediction = ((pred_merge_tmp >= 4) || (split_prediction == -1)) ? depth : -1;
		break;
	default:
		prediction = 0;
	}

	return prediction;
}


static void fill_depth_matrix_8(uint8_t* matrix, vect_2D* cu, int8_t curr_depth, int8_t val)
{
	//convert cu coordinate
	int32_t x = cu->x;
	int32_t y = cu->y;
	int i = 0;
	int32_t block = (8 >> curr_depth); //nb blocks in 8*8 block
	for (i = y; i < y + block; ++i)
	{
		memset(matrix + x + (i << 3), val, block);
	}
}


/*!
* \brief Generate the PUM depth map in a 8*8 array for a given depth with a Buttom-Up approach.
*
* \param arr_depthMap 		Array of the depth map.
* \param arr_features_cur 	Array of features for current depth (i_depth).
* \param arr_features_up 	Array of features for up depth (i_depth-1).
* \param i_depth      		Current depth tested.
* \param _level             Number of level tested when the algo is Restrained (limited)
* \param limited_flag 		0 to not test that the 4 blocks are at the same depth
* 					  		1 to only merge a bloc if the 4 sub blocks are at the same depth
* \param depth_flag 	    0 to not use depth features
* 							1 to use use depth features
* \return None.
*/
static void ml_os_qt_gen(uint8_t* arr_depthMap, features_s* arr_features_cur, features_s* arr_features_up, uint8_t i_depth, int _level, uint8_t limited_flag)
{
	

		tree_predict predict_func_merge[4] = {
				tree_predict_merge_depth_1,
				tree_predict_merge_depth_2,
				tree_predict_merge_depth_3,
				tree_predict_merge_depth_4
		};

		tree_predict predict_func_split[4] = {
				tree_predict_split_depth_0,
				tree_predict_split_depth_1,
				tree_predict_split_depth_2,
				tree_predict_split_depth_3
		};
	
	tree_predict prediction_function_merge = predict_func_merge[i_depth - 1];
	tree_predict prediction_function_split = predict_func_split[i_depth - 1];

	double d_nb_iter;
	double d_nb_bad;

	uint8_t i_rdepth = i_depth < 4 ? i_depth : 3;

	int16_t i_nbBlocks = 2 << (i_depth - 1);

	int inc = 2;
	for (int16_t y = 0; y < i_nbBlocks; y += inc)
	{
		for (int16_t x = 0; x < i_nbBlocks; x += inc)
		{
			uint8_t check_flag = 1;
			/*!< Check if neighboring blocks are of the same size */
			if ((limited_flag == 1) && (i_depth != 4))
			{
				check_flag = neighbor_constrain_bu(arr_depthMap, x << (3 - i_depth), y << (3 - i_depth), i_depth, _level);
			}

			if (check_flag)
			{
				int16_t i_cu_0 = x + (y << i_depth);
				int16_t i_cu_1 = x + 1 + (y << i_depth);
				int16_t i_cu_2 = x + ((y + 1) << i_depth);
				int16_t i_cu_3 = x + 1 + ((y + 1) << i_depth);
				int16_t i_cu_up = x / 2 + (y / 2 << (i_depth - 1));


				int8_t merge_prediction[4];
				int8_t split_prediction;


				merge_prediction[0] = prediction_function_merge(&arr_features_cur[i_cu_0], &d_nb_iter, &d_nb_bad);
				merge_prediction[1] = prediction_function_merge(&arr_features_cur[i_cu_1], &d_nb_iter, &d_nb_bad);
				merge_prediction[2] = prediction_function_merge(&arr_features_cur[i_cu_2], &d_nb_iter, &d_nb_bad);
				merge_prediction[3] = prediction_function_merge(&arr_features_cur[i_cu_3], &d_nb_iter, &d_nb_bad);
				split_prediction = prediction_function_split(&arr_features_up[i_cu_up], &d_nb_iter, &d_nb_bad);

				int8_t pred = combined_tree_function(merge_prediction, split_prediction, (i_depth >= 4) ? 8 : 9, i_depth);
				int condition = (pred < 0) ? 1 : 0;

				if (condition)
				{
					int16_t i_subCU = CR_GET_CU_D3((i_depth < 4 ? x : x / 2), (i_depth < 4 ? y : y / 2), i_rdepth);
					vect_2D tmp;
					tmp.x = i_subCU % 8;
					tmp.y = i_subCU / 8;
					fill_depth_matrix_8(arr_depthMap, &tmp, i_depth - 1, i_depth - 1);
				}
			}
		}
	}
}



static void os_luma_qt_pred(ml_intra_ctu_pred_t* ml_intra_depth_ctu, uint8_t* luma_px, int8_t qp, uint8_t* arr_CDM)
{
	// Features array per depth
	features_s arr_features_4[256];
	features_s arr_features_8[64];
	features_s arr_features_16[16];
	features_s arr_features_32[4];
	features_s features64;

	// Initialize to 0 all the features
	features_init_array(arr_features_4, 256, qp);
	features_init_array(arr_features_8, 64, qp);
	features_init_array(arr_features_16, 16, qp);
	features_init_array(arr_features_32, 4, qp);
	features_init_array(&features64, 1, qp);

	// Commpute the features for the current CTU for all depth
	features_s* arr_features[5];
	arr_features[0] = &features64;
	arr_features[1] = arr_features_32;
	arr_features[2] = arr_features_16; 
	arr_features[3] = arr_features_8;
	arr_features[4] = arr_features_4;


	features_compute_all(arr_features, luma_px);

	// Generate the CDM for the current CTU
	
	/*!< Set the depth map to 4 by default */
	memset(arr_CDM, 4, 64);
	ml_os_qt_gen(arr_CDM, arr_features_4, arr_features_8, 4, 1, RESTRAINED_FLAG);
	

	ml_os_qt_gen(arr_CDM, arr_features_8, arr_features_16, 3, 1, RESTRAINED_FLAG);
	ml_os_qt_gen(arr_CDM, arr_features_16, arr_features_32, 2, 1, RESTRAINED_FLAG);
	ml_os_qt_gen(arr_CDM, arr_features_32, &features64, 1, 1, RESTRAINED_FLAG);



}

static void fill_matrix_with_depth(uint8_t* matrix, int32_t x, int32_t y, int8_t depth)
{
	int i = 0;
	int32_t block = depth < 4 ? (8 >> depth) : 1; //nb blocks in 8*8 block
	for (i = y; i < y + block; ++i)
	{
		memset(matrix + x + (i << 3), depth, block);
	}
}

/*!
* \brief Merge the depth of the blocks of a depth map if
*        four blocks of the same depths are found.
*
* \param _mat_seed  Array of the depth used as seed for the merge (WARNING: must be the same as arrDepthMerge (tmp)).
* \param _mat_dst   Array of the depth merged.
* \return 1 if blocks have been merged, 0 else.
*/
static uint8_t merge_matrix_64(uint8_t* _mat_seed, uint8_t* _mat_dst)
{
	uint8_t i_depth = 0;
	uint32_t nb_block = 0;
	uint8_t retval = 0;
	uint8_t mat_tmp[64];
	memcpy(mat_tmp, _mat_seed, 64);
	for (uint_fast8_t i_y = 0; i_y < 8; ++i_y)
	{
		for (uint_fast8_t i_x = 0; i_x < 8; ++i_x)
		{
			i_depth = mat_tmp[i_x + (i_y << 3)];

			if (i_depth == 4)
			{
				_mat_dst[i_x + (i_y << 3)] = 3;/*!< All depth 4 blocks are merged by default to depth 3 */
				retval = 1;
				continue; /*!< Skip the modulo operations and conditional tests */
			}

			if (i_depth == 0) /*!< Skip all the loop process, since 0 depth means there will be no other depths tested */
			{
				_mat_dst[i_x + (i_y << 3)] = i_depth;
				memset(_mat_dst, 0, 64);
				goto exit_64;
			}

			nb_block = (16 >> i_depth); /*!< Offset to go check the three other blocks */
										/*!< Check if we are on the fourth block of a depth*/
			if ((i_x % nb_block == (8 >> i_depth)) &&
				(i_y % nb_block == (8 >> i_depth)))
			{
				retval = 1;
				nb_block = (8 >> i_depth); /*!< Generate the real offset for the array */
										   /*
										   *   x 0 1 2 3 4 5 6 7
										   * y
										   * 0   3 3 2 2 1 1 1 1
										   * 1   3 3 2 2 1 1 1 1
										   * 2   2 2 2 2 1 1 1 1
										   * 3   2 2 2 2 1 1 1 1
										   * 4   1 1 1 1 2 2 2 2
										   * 5   1 1 1 1 2 2 2 2
										   * 6   1 1 1 1 2 2 2 2
										   * 7   1 1 1 1 2 2 2 2
										   *
										   * exemple for the first fourth block of depth 2 :
										   * 8 >> 2 = 2
										   * nb_block = 4 -> x % 4 == 2 -> x = 2
										   *              -> y % 4 == 2 -> y = 2
										   * nb_block = 2 -> check blocs[(0,2),(2,0),(0,0)]
										   * all informations are available
										   */
				if (mat_tmp[i_x - nb_block + (i_y << 3)] == i_depth &&
					mat_tmp[i_x + ((i_y - nb_block) << 3)] == i_depth &&
					mat_tmp[i_x - nb_block + ((i_y - nb_block) << 3)] == i_depth)
				{
					fill_matrix_with_depth(_mat_dst, i_x - nb_block, i_y - nb_block, i_depth - 1);
				}
			}
		}
	}
exit_64:
	return retval;
}



/*!
* \brief Perform an in place element wise mask between the two matrix.
*
* \param _mat_mask  Matrix containing result of the mask (input/output).
* \param _mat_src   Matrix used for the mask (input).
* \param _size_w    Width of the matrix.
* \param _size_h    Height of the matrix.
* \return None.
*/
static void matrix_mask(uint8_t* _mat_mask, const uint8_t* _mat_src, size_t _size_w, size_t _size_h)
{
	if (_mat_mask == NULL || _mat_src == NULL)
	{
		fprintf(stderr, "null pointer as parameter.");
		assert(0);
		return;
	}
	size_t i_size = _size_h * _size_w;
	for (size_t i = 0; i < i_size; ++i)
	{
		_mat_mask[i] = (_mat_mask[i] ^ _mat_src[i]) != 0 ? 1 : 0;
	}
}


/*!
* \brief Add 1 depth level to the depth map. If d + 1 > 4 then d - 1 is done.
*        This function use a mask to add level only on selected roi.
*
* \param _mat_sup    	Original upper depth map .
* \param _mat_inf    	Lower depth map.
* \param _mat_sup_dst   Final upper depth map (WARNING: must be a different array as _mat_sup as it can be modified).
* \param _nb_level      The number of level there should be between inf and sup_dst.
* \param _mat_roi   	Mask used to determine which area should be modified on the _mat_inf (convention is 0 for changed area and 1 else).
* \return None.
*/
static void matrix_add_level_roi(const uint8_t* _mat_sup, uint8_t* _mat_inf, uint8_t* _mat_sup_dst, int8_t _nb_level, const uint8_t* _mat_roi)
{
	int8_t x = 0, y = 0;
	int8_t i_depth = 0;
	for (y = 0; y < 8; ++y)
	{
		for (x = 0; x < 8; ++x)
		{
			if ((!_mat_roi[x + (y << 3)]) == 1)
			{
				i_depth = _mat_sup[x + (y << 3)];
				if (i_depth == 4)
				{
					int8_t i_depth_sup = _mat_sup_dst[x + (y << 3)];
					_mat_inf[x + (y << 3)] = 4;
					if (i_depth_sup == 4)
					{
						_mat_sup_dst[x + (y << 3)] = 3;
					}
					else if (i_depth_sup > 0 && abs(i_depth_sup - 4) < _nb_level)
					{
						fill_matrix_with_depth(_mat_sup_dst, (x & (~(8 >> (i_depth_sup)))), (y & (~(8 >> (i_depth_sup)))), i_depth_sup - 1);
					}
					continue;
				}
				else if (i_depth == 3)
				{
					_mat_inf[x + (y << 3)] = 4;
					continue;
				}
				else if (abs(_mat_inf[x + (y << 3)] - _mat_sup[x + (y << 3)]) != _nb_level)
				{
					fill_matrix_with_depth(_mat_inf, x, y, i_depth + 1);
				}
				x += (8 >> (i_depth + 1)) - 1;
			}
		}
	}
}

/*!
* \brief Generate a search interval of controlled level around a MEP seed.
*
* \param _mat_depth_min  Upper depth map (considered as the MEP on call).
* \param _mat_depth_max  Lower depth map (considered initialized with the MEP values).
* \param _nb_level  	 Fixed distance between the two generated depth map.
* \return None.
*/
static void generate_interval_from_os_pred(ml_intra_ctu_pred_t* ml_intra_depth_ctu, uint8_t* _mat_depth_MEP)
{
	uint8_t* _mat_depth_min = ml_intra_depth_ctu->_mat_upper_depth;
	uint8_t* _mat_depth_max = ml_intra_depth_ctu->_mat_lower_depth;
	int8_t _nb_level = ml_intra_depth_ctu->i_nb_addDepth;

	memcpy(_mat_depth_min, _mat_depth_MEP, 64 * sizeof(uint8_t));
	memcpy(_mat_depth_max, _mat_depth_MEP, 64 * sizeof(uint8_t));
	if (_nb_level <= 0)
	{
		return;
	}
	else if (_nb_level >= 4)
	{
		memset(_mat_depth_min, 0, 64 * sizeof(uint8_t));
		memset(_mat_depth_max, 4, 64 * sizeof(uint8_t));
		return;
	}
	uint8_t mat_ref[64];	/*!< Matrix used to store the ref map */
	uint8_t mat_mask[64]; 	/*!< Matrix used as mask */
	uint8_t mat_max[64];	/*!< Matrix used to store current depth map max */

	for (int j = 0; j < _nb_level; ++j)
	{
		/*!< Copy the original map seed */
		memcpy(mat_ref, _mat_depth_min, 64 * sizeof(uint8_t));
		memcpy(mat_mask, _mat_depth_min, 64 * sizeof(uint8_t));
		memcpy(mat_max, _mat_depth_max, 64 * sizeof(uint8_t));

		/*!< Apply the RCDM on the upper map */
		merge_matrix_64(_mat_depth_min, _mat_depth_min);

		/*!< Extract the mask */
		matrix_mask(mat_mask, _mat_depth_min, 8, 8);

		/*!< Add a level only on the masked area */
		matrix_add_level_roi(mat_max, _mat_depth_max, _mat_depth_min, 1, mat_mask);
		
	}
}

/**
*	Generate the interval of depth predictions based on the luma samples
*/
void kvz_lcu_luma_depth_pred(ml_intra_ctu_pred_t* ml_intra_depth_ctu, uint8_t* luma_px, int8_t qp) {

	// Compute the one-shot (OS) Quad-tree prediction (_mat_OS_pred)
	os_luma_qt_pred(ml_intra_depth_ctu, luma_px, qp, ml_intra_depth_ctu->_mat_upper_depth);

	// Generate the interval of QT predictions around the first one
	generate_interval_from_os_pred(ml_intra_depth_ctu, ml_intra_depth_ctu->_mat_upper_depth);

	// Apply the extra Upper Expansion pass
	merge_matrix_64(ml_intra_depth_ctu->_mat_upper_depth, ml_intra_depth_ctu->_mat_upper_depth);
}
