/**
* @file edit_distance.c.h
 * @brief 序列编辑距离与相似度计算接口。
 */


#pragma once

#ifndef COURSEDESIGNTASKS_EDIT_DISTANCE_C_H
#define COURSEDESIGNTASKS_EDIT_DISTANCE_C_H

#pragma once
#include <stddef.h>
#include "ast_serial.h"

/**
 * @brief 计算两条字符串序列的 Levenshtein 编辑距离。
 */
size_t levenshtein_strvec(const StrVec *a, const StrVec *b);

/**
 * @brief 由编辑距离计算相似度（归一化）。
 */
double similarity_from_dist(size_t dist, size_t lenA, size_t lenB);

#endif //COURSEDESIGNTASKS_EDIT_DISTANCE_C_H