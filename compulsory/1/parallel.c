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
void radix_sort(u32 n, u32 b, u32 num_threads, double* durations, bool print_sorted) {
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
            u64 *per_thread_counter = malloc(num_buckets * num_threads * sizeof(u64));
            u32 *prefix_sum_array = malloc(num_buckets * num_threads * sizeof(u32));

            if (!per_thread_counter || !prefix_sum_array) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
            }

            u64 nums_per_bucket[num_buckets];

            #pragma omp parallel
            {
                int tid = omp_get_thread_num();
                // Pre-calculate the base pointer for this thread to simplify the inner loop
                u64* my_counts = &per_thread_counter[tid * num_buckets];

                // Initialize only this thread's portion
                for (int j = 0; j < num_buckets; j++) {
                    my_counts[j] = 0;
                }

                #pragma omp for schedule(static)
                for (u32 i = 0; i < n; i++) {
                    u64 digit = get_digit(input_arr[i], b, sort_iteration);
                    my_counts[digit]++; // No false sharing here!
                }
            }

            double counting_end = omp_get_wtime();
            double counting_duration = counting_end - counting_start;
            tot_counting_duration += counting_duration;

            // calculate prefix sum array
            __asm__ __volatile__("" ::: "memory");
            double prefix_gen_start = omp_get_wtime();
            __asm__ __volatile__("" ::: "memory");

            u32 running_sum = 0;
            // We iterate through buckets first, then threads, to build the global order
            for (u32 bucket = 0; bucket < num_buckets; bucket++) {
                for (int t = 0; t < num_threads; t++) {
                    u32 count = per_thread_counter[t * num_buckets + bucket];
                    // Store the starting position for this specific thread's items for this bucket
                    prefix_sum_array[t * num_buckets + bucket] = running_sum;
                    running_sum += count;
                }
            }

            __asm__ __volatile__("" ::: "memory");
            double prefix_gen_end = omp_get_wtime();
            double prefix_gen_duration = prefix_gen_end - prefix_gen_start;
            tot_prefix_gen_duration += prefix_gen_duration;

            // move numbers to bucketed array
            double bucketing_start = omp_get_wtime();
            __asm__ __volatile__("" ::: "memory");

            #pragma omp parallel
            {
                int tid = omp_get_thread_num();
                u32* my_offsets = &prefix_sum_array[tid * num_buckets];

                #pragma omp for schedule(static)
                for (u32 i = 0; i < n; i++) {
                    u64 digit = get_digit(input_arr[i], b, sort_iteration);
                    
                    // Get the current available index for this thread/digit combo
                    u32 target_idx = my_offsets[digit];
                    output_arr[target_idx] = input_arr[i];
                    
                    // Increment the offset so the next item for this digit goes to the next slot
                    my_offsets[digit]++; 
                }
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

            free(per_thread_counter);
            free(prefix_sum_array);
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

    free(input_arr);
    free(output_arr);
}

void print_stats_heading() {
    printf("n\tb\tp\ttotal_time\tcounting_time\tprefix_time\tbucket_time\n");
}

int main(int argc, char **argv) {
    double durations[4] = {0};
    if (argc == 4) {
        // n b p
        u32 n = (argc > 1) ? atol(argv[1]) : 10;
        u32 b = (argc > 2) ? atol(argv[2]) : 16;
        u32 p = (argc > 3) ? atol(argv[3]) : 1;
        omp_set_num_threads(p);

        radix_sort(n, b, p, durations, true);
        print_stats_heading();
        printf("%d\t%d\t%d\t%f\t%f\t%f\t%f\n", n, b, p, durations[0], durations[1], durations[2], durations[3]);
    }
    else if (argc == 1) {
        u32 fixed_n = 3000000;
        int max_b = 8;
        int max_p = 16;

        printf("\n=============\n");
        printf("Fixed value for n: %d", fixed_n);
        printf("\nNew loop: b from %d to %d and p from %d to %d", 1, max_b, 1, max_p);
        printf("\n=============\n\n");

        print_stats_heading();
        for (int p = 1; p <= max_p; p++) {
            for (u32 b = max_b; b <= max_b; b *= 2) {
                omp_set_num_threads(p);
                radix_sort(fixed_n, b, p, durations, false);
                printf("%d\t%d\t%d\t%f\t%f\t%f\t%f\n", fixed_n, b, p, durations[0], durations[1], durations[2], durations[3]);
            }
        }

        int fixed_b = 8;
        int n = 100000;
        int n_rate = 50000;

        printf("\n=============\n");
        printf("Fixed value for b: %d", fixed_b);
        printf("\nNew loop: n from %d to %d and p from %d to %d", n, n + n_rate * (max_p - 1), 1, max_p);
        printf("\n=============\n\n");

        print_stats_heading();
        for (int p = 1; p <= max_p; p++) {
            omp_set_num_threads(p);
            radix_sort(n, fixed_b, p, durations, false);
            printf("%d\t%d\t%d\t%f\t%f\t%f\t%f\n", n, fixed_b, p, durations[0], durations[1], durations[2], durations[3]);
            n += n_rate;
        }
    }

    return 0;
}
