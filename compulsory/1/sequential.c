#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <omp.h>
#include "mt19937-64.c"

#define u64 u_int64_t
#define u32 u_int32_t

/**
 * 
 */
u64 get_digit(u64 number, u32 b, u32 sort_iteration) {
    // 1. Create a constant mask of exactly 'b' bits (e.g., if b=8, mask is 0xFF)
    // Using 1ULL ensures the literal '1' is treated as a 64-bit unsigned integer
    u64 mask = ((u64) 1 << b) - 1;

    // 2. Calculate the shift offset. 
    // Subtracting 1 assumes sort_iteration starts at 1. 
    // Iteration 1: shift 0 bits. Iteration 2: shift 'b' bits.
    u32 shift_amount = (sort_iteration - 1) * b;

    // 3. Shift the target bits to the rightmost position, then isolate them with the mask
    u64 y = (number >> shift_amount) & mask;

    return y;
}

/**
 * 
 */
bool is_sorted(u64 *arr, u32 size) {
    bool sorted = true;
    for (int i = 0; i < size - 1 && sorted; i++) {
        if (arr[i] > arr[i+1]) {
            sorted = false;
        }
    }
    if (sorted) {
        printf("IT IS SORTED!\n");
    } else {
        printf("IT IS NOT SORTED!\n");
    }
    return sorted;
}

/**
 * @brief a sequential implementation of radix sort
 *
 * @param [in] n  Description of an input parameter, including its type and purpose.
 * @param [in] b  Description of an input parameter, including its type and purpose.
 * @param [out] out_param  Description of an output parameter (a pointer to a modified value).
 *
 * @returns description   Description of the value returned by the function.
 * @retval value          An alternative to @returns for specific return values (e.g., error codes).
 *
 * @throws error_type     Documents any error conditions or exceptions.
 * @see related_function  Links to related functions or resources.
 */
void radix_sort(u32 n, u32 b) {
    u64 *input_arr = malloc(n * sizeof(u64));
    u64 *output_arr = malloc(n * sizeof(u64));
    for (long i = 0; i < n; ++i) {
        input_arr[i] = genrand64_int64();
    }

    double start = omp_get_wtime();
    for (int sort_iteration = 1; sort_iteration <= 64/b; sort_iteration++) {
        // calculate how many numbers belong to each bucket
        int num_buckets = ((u64) 1 << b);
        u64 nums_per_bucket[num_buckets];
        for (long i = 0; i < num_buckets; ++i) {
            nums_per_bucket[i] = 0;
        }
        for (u32 i = 0; i < n; i++) {
            int digit = get_digit(input_arr[i], b, sort_iteration);
            nums_per_bucket[digit] += 1;
        }

        // generate prefix sum array
        u32 prefix_sum_array[num_buckets];
        prefix_sum_array[0] = 0;
        for (long i = 1; i < num_buckets; ++i) {
            prefix_sum_array[i] = prefix_sum_array[i-1] + nums_per_bucket[i-1];
        }

        // move numbers to bucket separated array
        for (u32 i = 0; i < n; i++) {
            int digit = get_digit(input_arr[i], b, sort_iteration);
            int bucket_i = prefix_sum_array[digit];
            output_arr[bucket_i] = input_arr[i];
            prefix_sum_array[digit]++;
        }

        // exchange pointers of the 2 input_arr arrays
        u64* aux = input_arr;
        input_arr = output_arr;
        output_arr = aux;
    }
    double end = omp_get_wtime();
    double duration = end - start;

    print("Sorting duration: %f", duration);

    is_sorted(input_arr, n);

    free(input_arr);
    free(output_arr);
}

int main(int argc, char **argv) {
    // n b
    u32 n = (argc > 1) ? atol(argv[1]) : 10;
    u32 b = (argc > 2) ? atol(argv[2]) : 16;

    radix_sort(n, b);

    return 0;
}
