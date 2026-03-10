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
    // example: 0...0000001 << 4 -> 0...0010000
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
    return sorted;
}

/**
 * 
 */
void radix_sort(u32 n, u32 b, double* durations, bool print_sorted) {
    // initialization
    durations[0] = DBL_MAX;
    u64 *input_arr = malloc(n * sizeof(u64));
    u64 *output_arr = malloc(n * sizeof(u64));
    for (long i = 0; i < n; ++i) {
        input_arr[i] = genrand64_int64();
    }

    for (int i = 0; i < 3; i++) {
        double tot_counting_duration = 0.0;
        double tot_prefix_gen_duration = 0.0;
        double tot_bucketing_duration = 0.0;

        __asm__ __volatile__("" ::: "memory");
        double sorting_start = omp_get_wtime();
        __asm__ __volatile__("" ::: "memory");

        // sorting starts
        for (int sort_iteration = 0; sort_iteration < 64/b; sort_iteration++) {
            // calculate how many numbers belong to each bucket
            __asm__ __volatile__("" ::: "memory");
            double counting_start = omp_get_wtime();
            __asm__ __volatile__("" ::: "memory");
            
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
            __asm__ __volatile__("" ::: "memory");
            double prefix_gen_start = omp_get_wtime();
            __asm__ __volatile__("" ::: "memory");

            u32 prefix_sum_array[num_buckets];
            prefix_sum_array[0] = 0;
            for (long i = 1; i < num_buckets; ++i) {
                prefix_sum_array[i] = prefix_sum_array[i-1] + nums_per_bucket[i-1];
            }

            __asm__ __volatile__("" ::: "memory");
            double prefix_gen_end = omp_get_wtime();
            double prefix_gen_duration = prefix_gen_end - prefix_gen_start;
            tot_prefix_gen_duration += prefix_gen_duration;

            // move numbers to bucketed array
            double bucketing_start = omp_get_wtime();
            __asm__ __volatile__("" ::: "memory");
            for (u32 i = 0; i < n; i++) {
                int digit = get_digit(input_arr[i], b, sort_iteration);
                int bucket_i = prefix_sum_array[digit];
                output_arr[bucket_i] = input_arr[i];
                prefix_sum_array[digit]++;
            }
            __asm__ __volatile__("" ::: "memory");
            double bucketing_end = omp_get_wtime();
            double bucketing_duration = bucketing_end - bucketing_start;
            tot_bucketing_duration += bucketing_duration;
            __asm__ __volatile__("" ::: "memory");

            // exchange pointers of the 2 input_arr arrays
            u64* aux = input_arr;
            input_arr = output_arr;
            output_arr = aux;
        }

        __asm__ __volatile__("" ::: "memory");
        double sorting_end = omp_get_wtime();
        double sorting_duration = sorting_end - sorting_start;
        __asm__ __volatile__("" ::: "memory");

        // check output
        bool sorted = is_sorted(input_arr, n);
        if (print_sorted && sorted) {
            printf("IT IS SORTED!\n");
        } else if (!sorted) {
            printf("IT IS NOT SORTED!\n");
        }

        if (sorting_duration < durations[0]) {
            durations[0] = sorting_duration;
            durations[1] = tot_counting_duration;
            durations[2] = tot_prefix_gen_duration;
            durations[3] = tot_bucketing_duration;
        }
    }

    // is_sorted(input_arr, n);

    free(input_arr);
    free(output_arr);
}

void print_stats_heading() {
    printf("n\tb\ttotal_time\tcounting_time\tprefix_time\tbucket_time\n");
}

int main(int argc, char **argv) {
    double durations[4] = {0};
    if (argc == 3) {
        // n b
        u32 n = (argc > 1) ? atol(argv[1]) : 10;
        u32 b = (argc > 2) ? atol(argv[2]) : 16;

        radix_sort(n, b, durations, true);

        printf("%d\t%d\t%f\t%f\t%f\t%f\n", n, b, durations[0], durations[1], durations[2], durations[3]);
    }
    else if (argc == 1) {
        printf("\n=============\n");
        printf("New loop: n from %d to %d and b from %d to %d", 10, 100000000, 1, 16);
        printf("\n=============\n\n");

        print_stats_heading();
        for (u32 n = 10; n <= 100000000; n *= 10) {
            for (u32 b = 1; b <= 16; b *= 2) {
                radix_sort(n, b, durations, false);
                
                printf("%d\t%d\t%f\t%f\t%f\t%f\n", n, b, durations[0], durations[1], durations[2], durations[3]);
            }
        }

        u32 lower_bound = 100000000;
        printf("\n=============\n");
        printf("Chosen configuration: n = %d, b = %d", lower_bound, 16);
        printf("\nNew loop: n from %d to %d and b = %d", lower_bound, lower_bound * 10, 16);
        printf("\n=============\n\n");

        print_stats_heading();
        u32 best_b = 8;
        durations[0] = 0;
        for (u32 n = lower_bound; durations[0] < 10; n += lower_bound) {
            radix_sort(n, best_b, durations, false);

            printf("%d\t%d\t%f\t%f\t%f\t%f\n", n, best_b, durations[0], durations[1], durations[2], durations[3]);
        }

        u32 fixed_n = 400000000;
        printf("\n=============\n");
        printf("Chosen configuration: n = %d, b = %d", fixed_n, 16);
        printf("\nNew loop: n = %d and b from %d to %d", fixed_n, 1, 16);
        printf("\n=============\n\n");

        print_stats_heading();
        for (u32 b = 1; b <= 16; b *= 2) {
            radix_sort(fixed_n, b, durations, false);
            printf("%d\t%d\t%f\t%f\t%f\t%f\n", fixed_n, b, durations[0], durations[1], durations[2], durations[3]);
        }
    }

    return 0;
}
