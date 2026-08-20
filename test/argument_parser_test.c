#include "argument_parser.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Athrill's Windows compatibility headers declare assert as a function.
 * Keep this standalone test independent of that runtime implementation.
 */
#undef assert
#define assert(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "assertion failed at line %d: %s\\n", __LINE__, #condition); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

static void test_clustered_flags(void)
{
    const char *argv[] = {"athrill2", "-irb", "program.elf"};
    ArgumentParserState state;

    argument_parser_init(&state);
    assert(argument_parser_next(&state, 3, argv, "irb") == 'i');
    assert(argument_parser_next(&state, 3, argv, "irb") == 'r');
    assert(argument_parser_next(&state, 3, argv, "irb") == 'b');
    assert(argument_parser_next(&state, 3, argv, "irb") == -1);
    assert(state.index == 2);
}

static void test_required_arguments(void)
{
    const char *argv[] = {"athrill2", "-t", "100", "-c3", "program.elf"};
    ArgumentParserState state;

    argument_parser_init(&state);
    assert(argument_parser_next(&state, 5, argv, "t:c:") == 't');
    assert(state.argument != NULL);
    assert(state.argument[0] == '1');
    assert(argument_parser_next(&state, 5, argv, "t:c:") == 'c');
    assert(state.argument != NULL);
    assert(state.argument[0] == '3');
    assert(argument_parser_next(&state, 5, argv, "t:c:") == -1);
    assert(state.index == 4);
}

static void test_end_marker(void)
{
    const char *argv[] = {"athrill2", "-i", "--", "-program.elf"};
    ArgumentParserState state;

    argument_parser_init(&state);
    assert(argument_parser_next(&state, 4, argv, "i") == 'i');
    assert(argument_parser_next(&state, 4, argv, "i") == -1);
    assert(state.index == 3);
}

static void test_unknown_option(void)
{
    const char *argv[] = {"athrill2", "-x", "program.elf"};
    ArgumentParserState state;

    argument_parser_init(&state);
    assert(argument_parser_next(&state, 3, argv, "i") == '?');
    assert(state.error_option == 'x');
    assert(state.index == 2);
}

static void test_missing_argument(void)
{
    const char *argv[] = {"athrill2", "-t"};
    ArgumentParserState state;

    argument_parser_init(&state);
    assert(argument_parser_next(&state, 2, argv, "t:") == '?');
    assert(state.error_option == 't');
    assert(state.argument == NULL);
    assert(state.index == 2);
}

static void test_non_option_stops_parser(void)
{
    const char *argv[] = {"athrill2", "program.elf", "-i"};
    ArgumentParserState state;

    argument_parser_init(&state);
    assert(argument_parser_next(&state, 3, argv, "i") == -1);
    assert(state.index == 1);
}

static void test_long_options(void)
{
    const char *argv[] = {
        "athrill_remote",
        "--verbose",
        "--athrill-listen-port=1234",
        "--remote-client-listen-port",
        "5678",
        "status"
    };
    const ArgumentParserLongOption options[] = {
        {"athrill-listen-port", 1, 'a'},
        {"remote-client-listen-port", 1, 'r'},
        {"verbose", 0, 'v'},
        {NULL, 0, 0}
    };
    ArgumentParserState state;

    argument_parser_init(&state);
    assert(argument_parser_next_long(&state, 6, argv, "a:r:v", options) == 'v');
    assert(argument_parser_next_long(&state, 6, argv, "a:r:v", options) == 'a');
    assert(state.argument[0] == '1');
    assert(argument_parser_next_long(&state, 6, argv, "a:r:v", options) == 'r');
    assert(state.argument[0] == '5');
    assert(argument_parser_next_long(&state, 6, argv, "a:r:v", options) == -1);
    assert(state.index == 5);
}

static void test_invalid_long_option(void)
{
    const char *unknown_argv[] = {"athrill_remote", "--unknown"};
    const char *missing_argv[] = {"athrill_remote", "--port"};
    const char *unexpected_argv[] = {"athrill_remote", "--verbose=yes"};
    const ArgumentParserLongOption options[] = {
        {"port", 1, 'p'},
        {"verbose", 0, 'v'},
        {NULL, 0, 0}
    };
    ArgumentParserState state;

    argument_parser_init(&state);
    assert(argument_parser_next_long(&state, 2, unknown_argv, "p:v", options) == '?');
    argument_parser_init(&state);
    assert(argument_parser_next_long(&state, 2, missing_argv, "p:v", options) == '?');
    assert(state.error_option == 'p');
    argument_parser_init(&state);
    assert(argument_parser_next_long(&state, 2, unexpected_argv, "p:v", options) == '?');
    assert(state.error_option == 'v');
}

int main(void)
{
    test_clustered_flags();
    test_required_arguments();
    test_end_marker();
    test_unknown_option();
    test_missing_argument();
    test_non_option_stops_parser();
    test_long_options();
    test_invalid_long_option();
    return 0;
}
