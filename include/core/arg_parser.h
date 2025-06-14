#ifndef ARG_PARSE_H
#define ARG_PARSE_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX_OPTIONS      64
#define MAX_PARSED       64
#define MAX_HELP_MESSAGE 4096

typedef struct _Option {
    char short_name;
    char *long_name;
    bool required;
    void (*callback)(char *value);
} Option;

typedef struct _ParsedShortOption {
    char key;
    char *value;
} ParsedShortOption;

typedef struct _ParsedLongOption {
    char *key;
    char *value;
} ParsedLongOption;

typedef struct _ArgParser {
    Option options[MAX_OPTIONS];
    int option_count;

    ParsedShortOption parsed_short[MAX_PARSED];
    int parsed_short_count;

    ParsedLongOption parsed_long[MAX_PARSED];
    int parsed_long_count;
    char *proc_exec;
} ArgParser;

ArgParser *args_parser_create() {
    ArgParser *parser = (ArgParser *)calloc(1, sizeof(ArgParser));
    return parser;
}

void args_parser_parse_args(ArgParser *parser, int argc, char **argv) {
    parser->proc_exec = argv[0];

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 0; j < parser->option_count; j++) {
                Option *opt = &parser->options[j];

                if (argv[i][1] != '-' && opt->short_name == argv[i][1]) {
                    char *val = NULL;
                    if (argv[i][2] != '\0') {
                        val = &argv[i][2];
                    } else if (i + 1 < argc) {
                        val = argv[++i];
                    }

                    parser->parsed_short[parser->parsed_short_count++] =
                        (ParsedShortOption){opt->short_name, val};
                } else if (argv[i][1] == '-' &&
                           strcmp(opt->long_name, &argv[i][2]) == 0) {
                    char *val = NULL;
                    if (i + 1 < argc) {
                        val = argv[++i];
                    }

                    parser->parsed_long[parser->parsed_long_count++] =
                        (ParsedLongOption){opt->long_name, val};
                    if (opt->short_name != '\0') {
                        parser->parsed_short[parser->parsed_short_count++] =
                            (ParsedShortOption){opt->short_name, val};
                    }
                }
            }
        }
    }
}

void args_parser_dispatch_callbacks(ArgParser *parser) {
    for (int i = 0; i < parser->parsed_short_count; i++) {
        ParsedShortOption *opt = &parser->parsed_short[i];
        for (int j = 0; j < parser->option_count; j++) {
            Option *option = &parser->options[j];
            if (option->short_name == opt->key && option->callback) {
                option->callback(opt->value);
            }
        }
    }

    for (int i = 0; i < parser->parsed_long_count; i++) {
        ParsedLongOption *opt = &parser->parsed_long[i];
        for (int j = 0; j < parser->option_count; j++) {
            Option *option = &parser->options[j];
            if (option->long_name && strcmp(option->long_name, opt->key) == 0 &&
                option->callback) {
                option->callback(opt->value);
            }
        }
    }
}

void arg_parser_add_option(ArgParser *parser, char short_name, char *long_name,
                           bool required, void (*callback)(char *value)) {
    if (parser->option_count >= MAX_OPTIONS)
        return;

    parser->options[parser->option_count++] =
        (Option){short_name, long_name, required, callback};
}

const char *arg_parser_get_option_short(ArgParser *parser, char short_name) {
    for (int i = 0; i < parser->parsed_short_count; i++) {
        if (parser->parsed_short[i].key == short_name) {
            return parser->parsed_short[i].value;
        }
    }
    return NULL;
}

const char *arg_parser_get_option_long(ArgParser *parser,
                                       const char *long_name) {
    for (int i = 0; i < parser->parsed_long_count; i++) {
        if (strcmp(parser->parsed_long[i].key, long_name) == 0) {
            return parser->parsed_long[i].value;
        }
    }
    return NULL;
}

bool arg_parser_has_option_short(ArgParser *parser, char short_name) {
    for (int i = 0; i < parser->parsed_short_count; i++) {
        if (parser->parsed_short[i].key == short_name) {
            return true;
        }
    }
    return false;
}

bool arg_parser_has_option_long(ArgParser *parser, const char *long_name) {
    for (int i = 0; i < parser->parsed_long_count; i++) {
        if (strcmp(parser->parsed_long[i].key, long_name) == 0) {
            return true;
        }
    }
    return false;
}

void args_parser_free(ArgParser *parser) {
    if (parser) {
        free(parser);
    }
}

#endif // ARG_PARSE_H
