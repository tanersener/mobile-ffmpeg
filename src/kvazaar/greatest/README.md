# greatest

A unit testing system for C, contained in 1 file. It doesn't use dynamic
allocation or depend on anything beyond ANSI C89, and the test scaffolding
should build without warnings under `-Wall -pedantic`.

To use, just `#include` greatest.h in your project. 

There are some compile time options, and slightly nicer syntax for
parametric testing (running tests with arguments) is available if
compiled with -std=c99.

Also, I wrote a
[blog post](http://spin.atomicobject.com/2013/07/31/greatest-c-testing-embedded/)
with more information.

[theft][], a related project, adds [property-based testing][pbt].

[1]: http://spin.atomicobject.com/2013/07/31/greatest-c-testing-embedded/
[theft]: https://github.com/silentbicycle/theft
[pbt]: https://spin.atomicobject.com/2014/09/17/property-based-testing-c/

## Available Assertions

### `ASSERT(COND)` and `ASSERTm(MSG, COND)`

Assert that `COND` evaluates to a true value.

### `ASSERT_FALSE(COND)` and `ASSERT_FALSEm(MSG, COND)`

Assert that `COND` evaluates to a false value.

### `ASSERT_EQ(EXPECTED, ACTUAL)` and `ASSERT_EQm(MSG, EXPECTED, ACTUAL)`

Assert that `EXPECTED == ACTUAL`. To compare with a custom equality test
function, use `ASSERT_EQUAL_T` instead. To print the values if they
differ, use `ASSERT_EQ_FMT`.

### `ASSERT_EQ_FMT(EXPECTED, ACTUAL, FORMAT)` and `ASSERT_EQ_FMTm(MSG, EXPECTED, ACTUAL, FORMAT)`

Assert that `EXPECTED == ACTUAL`. If they are not equal, print their values using
FORMAT as the `printf` format string.

For example: `ASSERT_EQ_FMT(123, result, "%d");`

### `ASSERT_IN_RANGE(EXPECTED, ACTUAL, TOLERANCE)` and `ASSERT_IN_RANGEm(MSG, EXPECTED, ACTUAL, TOLERANCE)`

Assert that ACTUAL is within EXPECTED +/- TOLERANCE, once the values
have been converted to a configurable floating point type
(`GREATEST_FLOAT`).

### `ASSERT_STR_EQ(EXPECTED, ACTUAL)` and `ASSERT_STR_EQm(MSG, EXPECTED, ACTUAL)`

Assert that `strcmp(EXPECTED, ACTUAL) == 0`.

### `ASSERT_EQUAL_T(EXPECTED, ACTUAL, TYPE_INFO, UDATA)` and `ASSERT_EQUAL_Tm(MSG, EXPECTED, ACTUAL, TYPE_INFO, UDATA)`

Assert that EXPECTED and ACTUAL are equal, using the `greatest_equal_cb`
function pointed to by `TYPE_INFO->equal` to compare them. The
function's UDATA argument can be used to pass in arbitrary user data (or
NULL). If the values are not equal and the `TYPE_INFO->print` function
is defined, it will be used to print an "Expected: X, Got: Y" message.

### `ASSERT_OR_LONGJMP(COND)` and `ASSERT_OR_LONGJMPm(MSG, COND)`

Assert that `COND` evaluates to a true value. If not, then use
longjmp(3) to immediately return from the test case and any intermediate
function calls. (If built with `GREATEST_USE_LONGJMP` set to 0, then all
setjmp/longjmp-related functionality will be compiled out.)


In all cases, the `m` version allows you to pass in a customized failure
message. If an assertion without a custom message fails, `greatest` uses C
preprocessor stringification to simply print the assertion's parameters.

## Basic Usage

```c
#include "greatest.h"

TEST x_should_equal_1() {
    int x = 1;
    ASSERT_EQ(1, x);                              /* default message */
    ASSERT_EQm("yikes, x doesn't equal 1", 1, x); /* custom message */
    /* printf expected and actual values as "%d" if they differ */
    ASSERT_EQ_FMT(1, x, "%d");
    PASS();
}

SUITE(the_suite) {
    RUN_TEST(x_should_equal_1);
}

/* Add definitions that need to be in the test runner's main file. */
GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();      /* command-line arguments, initialization. */
    RUN_SUITE(the_suite);
    GREATEST_MAIN_END();        /* display results */
}
```

Output:

```sh
$ make simple && ./simple
cc -g -Wall -Werror -pedantic    simple.c   -o simple

* Suite the_suite:
.
1 tests - 1 pass, 0 fail, 0 skipped (5 ticks, 0.000 sec)

Total: 1 tests (47 ticks, 0.000 sec), 3 assertions
Pass: 1, fail: 0, skip: 0.
```

Test cases should call assertions and then end in PASS(), SKIP(),
FAIL(), or one of their message variants (e.g. `SKIPm("TODO");`).
If there are any test failures, the test runner will return 1,
otherwise it will return 0. (Skips do not cause a test runner to
report failure.)

Tests and suites are just functions, so normal C scoping rules apply.

(For more examples, look at example.c and example-suite.c.)


## Sub-Functions

Because of how `PASS()`, `ASSERT()`, `FAIL()`, etc. are implemented
(returning a test result enum value), calls to functions that use them
directly from test functions must be wrapped in `CHECK_CALL`:

    TEST example_using_subfunctions(void) {
        CHECK_CALL(less_than_three(5));
        PASS();
    }

This is only necessary if the called function can cause test failures.


## Command Line Options

Test runners build with the following command line options:

    Usage: (test_runner) [-hlfv] [-s SUITE] [-t TEST]
      -h        print this Help
      -l        List suites and their tests, then exit
      -f        Stop runner after first failure
      -v        Verbose output
      -s SUITE  only run suite w/ name containing SUITE substring
      -t TEST   only run test w/ name containing TEST substring

If you want to run multiple test suites in parallel, look at
[parade](https://github.com/silentbicycle/parade).


## Aliases

All the Macros exist with the unprefixed notation and with the prefixed notation, for example:

`SUITE` is the same as `GREATEST_SUITE` 

Checkout the [source][1] for the entire list.

These aliases can be disabled by `#define`-ing `GREATEST_USE_ABBREVS` to 0.

[1]: https://github.com/silentbicycle/greatest/blob/87530d9ce56b98e2efc6105689dc411e9863190a/greatest.h#L582-L603


## Color Output

If you want color output (`PASS` in green, `FAIL` in red, etc.), you can
pipe the output through the included `greenest` script:

```sh
$ ./example -v | greenest
```

greatest itself doesn't have built-in coloring to stay small and portable.
