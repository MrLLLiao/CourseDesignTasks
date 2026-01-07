/**
* @file edit_distance.c
 * @brief 基于 Levenshtein 编辑距离的序列差异度量。
 *
 * “字符”被抽象为字符串 token（StrVec 中的元素），从而可用于 AST 序列的比较。
 */

#include "../include//edit_distance.c.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief 求 3 个 size_t 的最小值（Levenshtein DP 的工具函数）。
 */
static inline size_t min3(const size_t a, const size_t b, const size_t c) {
    return a < b ? a : b < c ? b : c;
}

/**
 * @brief 判断两个字符串是否相等（考虑 NULL）。
 *
 * @return 相等返回 1，否则返回 0。
 */
static inline int eq(const char *x, const char *y) {
    if (x == y) return 1;
    if (!x || !y) return 0;
    return strcmp(x, y) == 0;
}

/**
 * @brief 计算两个字符串序列（StrVec）之间的 Levenshtein（编辑）距离。
 *
 * 这里的“字符”不是单个 char，而是一个“token 字符串”（例如 <IF>、ID、NUM 等）。
 * 采用经典 DP：
 *   dp[i][j] = min( dp[i-1][j]+1, dp[i][j-1]+1, dp[i-1][j-1]+cost )
 *
 * 工程化实现：
 * - 仅保留两行 DP（O(min(n,m)) 空间）；
 * - 若 m > n 则交换，保证按较短序列开辟 DP 行。
 *
 * @param a 序列 A（非 NULL）。
 * @param b 序列 B（非 NULL）。
 * @return 编辑距离（>=0）。
 */
size_t levenshtein_strvec(const StrVec *a, const StrVec *b) {
    if (!a || !b) return 0;

    const StrVec *A = a, *B = b;
    size_t n = a->size, m = b->size;
    if (m > n) { A = b; B = a; n = A->size; m = B->size; }
    if (m == 0) return n;
    if (n == 0) return m;

    size_t *v[2];
    v[0] = (size_t*)malloc((m + 1) * sizeof(size_t));
    v[1] = (size_t*)malloc((m + 1) * sizeof(size_t));
    if (!v[0] || !v[1]) { free(v[0]); free(v[1]); return 0; }

    for (size_t j = 0; j <= m; ++ j) {
        v[0][j] = j;
    }

    int cur = 1;  // 当前行索引
    int prev = 0; // 上一行索引

    for (size_t i = 1; i <= n; ++ i) {
        v[cur][0] = i;
        const char *ai = A->data[i - 1];

        for (size_t j = 1; j <= m; ++ j) {
            const char *bi = B->data[j - 1];
            const size_t cost = eq(ai, bi) ? 0 : 1;

            const size_t del = v[prev][j] + 1;
            const size_t ins = v[cur][j - 1] + 1;
            const size_t sub = v[prev][j - 1] + cost;

            v[cur][j] = min3(del, ins, sub);
        }

        cur ^= 1;
        prev ^= 1;
    }

    const size_t dist = v[prev][m];
    free(v[0]);
    free(v[1]);

    return dist;
}

/**
 * @brief 将编辑距离转换为 [0,1] 相似度（归一化编辑距离的线性映射）。
 *
 * 相似度定义：
 *   sim = 1 - dist / max(lenA, lenB)
 *
 * @param dist 编辑距离。
 * @param lenA 序列 A 长度。
 * @param lenB 序列 B 长度。
 * @return 相似度（0~1）。
 */
double similarity_from_dist(const size_t dist, const size_t lenA, const size_t lenB) {
    const size_t mx = (lenA > lenB) ? lenA : lenB;
    if (mx == 0) return 1.0;
    return 1.0 - (double)dist / (double)mx;
}