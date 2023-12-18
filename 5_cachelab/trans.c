/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transSpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    REQUIRES(M > 0);
    REQUIRES(N > 0);

    int i, j, k, a0, a1, a2, a3, a4, a5, a6, a7;
    // 32 * 32
    if (M == 32 && N == 32) {
        for (i = 0; i < N; i += 8) {
            for (j = 0; j < M; j += 8) {
                for (k = i; k < i + 8; k++) {
                    // just divide the matrix into 8 * 8
                    a0 = A[k][j];
                    a1 = A[k][j + 1];
                    a2 = A[k][j + 2];
                    a3 = A[k][j + 3];
                    a4 = A[k][j + 4];
                    a5 = A[k][j + 5];
                    a6 = A[k][j + 6];
                    a7 = A[k][j + 7];
                    B[j][k] = a0;
                    B[j + 1][k] = a1;
                    B[j + 2][k] = a2;
                    B[j + 3][k] = a3;
                    B[j + 4][k] = a4;
                    B[j + 5][k] = a5;
                    B[j + 6][k] = a6;
                    B[j + 7][k] = a7;
                }
            }
        }
    }

    // 64 * 64

    if (M == 64 && N == 64) {
        for (i = 0; i < N; i += 8) {
            for (j = 0; j < M; j += 8) {
                /*
                    8 * 8 -> (4 * 4) * 4
                        j          j+8        i              i+8
                    i   -------------     j   -----------------
                        |  P  |  Q  |         |  P^T  |  R^T  |
                        -------------  ->     -----------------
                        |  R  |  S  |         |  Q^T  |  S^T  |
                    i+8 -------------     j+8 -----------------
                    */

                for (k = i; k < i + 4; k++) {
                    a0 = A[k][j];
                    a1 = A[k][j + 1];
                    a2 = A[k][j + 2];
                    a3 = A[k][j + 3];
                    a4 = A[k][j + 4];
                    a5 = A[k][j + 5];
                    a6 = A[k][j + 6];
                    a7 = A[k][j + 7];
                    // P -> P^T
                    B[j][k] = a0;
                    B[j + 1][k] = a1;
                    B[j + 2][k] = a2;
                    B[j + 3][k] = a3;
                    // Q -> Q^T
                    B[j][k + 4] = a4;
                    B[j + 1][k + 4] = a5;
                    B[j + 2][k + 4] = a6;
                    B[j + 3][k + 4] = a7;
                }

                for (k = j; k < j + 4; k++) {
                    a0 = A[i + 4][k];
                    a1 = A[i + 5][k];
                    a2 = A[i + 6][k];
                    a3 = A[i + 7][k];
                    a4 = B[k][i + 4];
                    a5 = B[k][i + 5];
                    a6 = B[k][i + 6];
                    a7 = B[k][i + 7];
                    // R -> Q^T
                    B[k][i + 4] = a0;
                    B[k][i + 5] = a1;
                    B[k][i + 6] = a2;
                    B[k][i + 7] = a3;
                    // move Q^T to the correct position
                    B[k + 4][i] = a4;
                    B[k + 4][i + 1] = a5;
                    B[k + 4][i + 2] = a6;
                    B[k + 4][i + 3] = a7;
                }

                for (k = i + 4; k < i + 8; k++) {
                    a0 = A[k][j + 4];
                    a1 = A[k][j + 5];
                    a2 = A[k][j + 6];
                    a3 = A[k][j + 7];
                    // S -> S^T
                    B[j + 4][k] = a0;
                    B[j + 5][k] = a1;
                    B[j + 6][k] = a2;
                    B[j + 7][k] = a3;
                }
            }
        }
    }

    // 60 * 68
    if (M == 60 && N == 68) {
        for (int i = 0; i < N; i += 20) {
            for (int j = 0; j < M; j += 4) {
                for (int k = i; k < (i + 20) && (k < N); k++) {
                    // just divide the matrix into 4 * 4
                    a0 = A[k][j];
                    a1 = A[k][j + 1];
                    a2 = A[k][j + 2];
                    a3 = A[k][j + 3];
                    B[j][k] = a0;
                    B[j + 1][k] = a1;
                    B[j + 2][k] = a2;
                    B[j + 3][k] = a3;
                }
            }
        }
    }

    ENSURES(is_transpose(M, N, A, B));
}
/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the
 * cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, a;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            a = A[i][j];
            B[j][i] = a;
        }
    }

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
