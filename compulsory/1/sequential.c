#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <omp.h>
#include "mt19937-64.c"
#include <float.h>

#define u64 uint64_t
#define u32 uint32_t

/**
 * 
 */
u64 get_digit(u64 number, u32 b, u32 sort_iteration) {
    // (u64) 1 to create all the necessary 0's to the left of the 1
    // << b move the 1 to the left b times
    // example: 0...0000001 >> 4 -> 0...0010000
    // by subtracting 1, the program replaces the 0's to the right with 1's
    // example: 0...0010000 - 1 -> 0...0001111
    u64 mask = ((u64) 1 << b) - 1;

    // if sort_iteration is 0, we move the number to the right 0 times to get the first digits
    u32 shift_amount = (sort_iteration) * b;

    // the mask isolates the digits most relevant with the & operator
    // the digits to the left after the first b digits after shifting are set to 0
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
 * 
 */
void radix_sort(u32 n, u32 b, double* durations) {
    // initialization
    u64 *input_arr = malloc(n * sizeof(u64));
    u64 *output_arr = malloc(n * sizeof(u64));
    for (long i = 0; i < n; ++i) {
        input_arr[i] = genrand64_int64();
    }

    double tot_counting_duration, tot_prefix_gen_duration, tot_bucketing_duration = 0;

    double sorting_start = omp_get_wtime();
    // sorting starts
    for (int sort_iteration = 0; sort_iteration < 64/b; sort_iteration++) {
        // calculate how many numbers belong to each bucket
        double counting_start = omp_get_wtime();
        
        int num_buckets = ((u64) 1 << b);
        u64 nums_per_bucket[num_buckets];
        for (long i = 0; i < num_buckets; ++i) {
            nums_per_bucket[i] = 0;
        }
        for (u32 i = 0; i < n; i++) {
            int digit = get_digit(input_arr[i], b, sort_iteration);
            nums_per_bucket[digit] += 1;
        }

        double counting_end = omp_get_wtime();
        double counting_duration = counting_end - counting_start;
        tot_counting_duration += counting_duration;

        // calculate prefix sum array
        double prefix_gen_start = omp_get_wtime();

        u32 prefix_sum_array[num_buckets];
        prefix_sum_array[0] = 0;
        for (long i = 1; i < num_buckets; ++i) {
            prefix_sum_array[i] = prefix_sum_array[i-1] + nums_per_bucket[i-1];
        }

        double prefix_gen_end = omp_get_wtime();
        double prefix_gen_duration = prefix_gen_end - prefix_gen_start;
        tot_prefix_gen_duration += prefix_gen_duration;

        // move numbers to bucketed array
        double bucketing_start = omp_get_wtime();
        for (u32 i = 0; i < n; i++) {
            int digit = get_digit(input_arr[i], b, sort_iteration);
            int bucket_i = prefix_sum_array[digit];
            output_arr[bucket_i] = input_arr[i];
            prefix_sum_array[digit]++;
        }
        double bucketing_end = omp_get_wtime();
        double bucketing_duration = bucketing_end - bucketing_start;
        tot_bucketing_duration = bucketing_duration;

        // exchange pointers of the 2 input_arr arrays
        u64* aux = input_arr;
        input_arr = output_arr;
        output_arr = aux;
    }
    double sorting_end = omp_get_wtime();
    double sorting_duration = sorting_end - sorting_start;

    durations[0] = sorting_duration;
    durations[1] = tot_counting_duration;
    durations[2] = tot_prefix_gen_duration;
    durations[3] = tot_bucketing_duration;

    // is_sorted(input_arr, n);

    free(input_arr);
    free(output_arr);
}

int main(int argc, char **argv) {
    double durations[4] = {0};
    if (argc > 1) {
        // n b
        u32 n = (argc > 1) ? atol(argv[1]) : 10;
        u32 b = (argc > 2) ? atol(argv[2]) : 16;

        radix_sort(n, b, durations);
    }
    else if (argc == 1) {
        double best_ratio = DBL_MAX;
        u32 best_n, best_b = 0;

        for (u32 n = 10; n <= 100000000; n *= 10) {
            for (u32 b = 1; b <= 16; b *= 2) {
                radix_sort(n, b, durations);
                
                double numbers_per_second_ratio = n / durations[0];
                if (numbers_per_second_ratio < best_ratio) {
                    best_ratio = numbers_per_second_ratio;
                    best_n = n;
                    best_b = b;
                }
                
                printf("%d, %d, %f, %f, %f, %f\n", n, b, durations[0], durations[1], durations[2], durations[3]);
            }
        }

        u32 lower_bound, upper_bound = 0;
        if (closest_to_10_seconds < 0) {
            lower_bound = best_n / 10;
            upper_bound = best_n;
        } else {
            lower_bound = best_n;
            upper_bound = best_n * 10;
        }

        printf("\n=============\n");
        printf("Chosen configuration: n = %d, b = %d\n", best_n, best_b);
        printf("New lower bound: %d; New upper bound: %d\n", lower_bound, upper_bound);
        printf("=============\n\n");

        closest_to_10_seconds = 10000;

        for (u32 n = lower_bound; n <= upper_bound; n += lower_bound) {
            radix_sort(n, best_b, durations);
            
            double difference_to_10 = 10 - durations[0];
            if (fabs(difference_to_10) < fabs(closest_to_10_seconds)) {
                closest_to_10_seconds = difference_to_10;
                best_n = n;
            }

            printf("%d, %d, %f, %f, %f, %f\n", n, best_b, durations[0], durations[1], durations[2], durations[3]);
        }

        printf("\n=============\n");
        printf("Chosen configuration: n = %d, b = %d\n", best_n, best_b);
        printf("New lower bound: %d; New upper bound: %d\n", lower_bound, upper_bound);
        printf("=============\n\n");

        /*if (closest_to_10_seconds < 0) {
            lower_bound = closest_n / 10;
            upper_bound = closest_n;
        } else {
            lower_bound = closest_n;
            upper_bound = closest_n * 10;
        }

        for (u32 n = 300000000; n <= 400000000; n += 10000000) {
            radix_sort(n, closest_b, durations);
            printf("%d, %d, %f, %f, %f, %f\n", n, closest_b, durations[0], durations[1], durations[2], durations[3]);
        }*/
    }

    return 0;
}
