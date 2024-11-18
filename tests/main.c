#include <criterion/criterion.h>
#include <criterion/logging.h>
#include <criterion/options.h>
#include <stdio.h>

void global_setup(void) {
    // Global setup code
    printf("Global setup\n");
}

void global_teardown(void) {
    // Global cleanup code
    printf("Global teardown\n");
}

int main(int argc, char *argv[]) {
    printf("Starting tests...\n");
    struct criterion_test_set *tests = criterion_initialize();

    // Parse command line arguments
    criterion_handle_args(argc, argv, true);

    // Enable verbose output
    criterion_options.logging_threshold = CRITERION_INFO;
    criterion_options.always_succeed = false;
    criterion_options.no_early_exit = true;

    printf("Running tests...\n");
    int result = criterion_run_all_tests(tests);
    printf("Tests finished with result: %d\n", result);

    criterion_finalize(tests);
    return result;
}
