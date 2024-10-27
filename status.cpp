#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <args.hpp>
#include <vocab.hpp>
#include <status.hpp>
#include <tokeniser.hpp>

int status(struct command_line_args command_line_args) {
    // Open the vocab file
    FILE *vocab_file = fopen(command_line_args.vocab_path, "r");
    if(vocab_file == NULL) {
        fprintf(stderr, "Failed to open vocab file\n");
        return 1;
    }

    // Read the header
    struct vocab_file_header_t header;
    int ret = fread(&header, sizeof(struct vocab_file_header_t), 1, vocab_file);
    if(ret != 1) {
        fprintf(stderr, "Failed to read header\n");
        return 1;
    }

    // Check the preamble
    if(strncmp(header.preamble, "VOCAB", 5)) {
        fprintf(stderr, "File has been corrupted\n");
        return 1;
    }

    // Read the vocab
    struct Token *vocab = (struct Token *)calloc(header.len, sizeof(struct Token));
    if(vocab == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return 1;
    }

    size_t bytes_read = fread(vocab, sizeof(struct Token), header.len, vocab_file);
    if(bytes_read != header.len) {
        fprintf(stderr, "Failed to read vocab\n");
        free(vocab);
        return 1;
    }
    fclose(vocab_file);


    // Print the vocab
    printf("{\"complete\":%s,\"desired_size\":%hu,\"vocab\":[", header.complete ? "true" : "false", header.desired_len);
    for(uint16_t i = 0; i < header.len; i++) {
        printf("{\"b1\":%hu,\"b2\":%hu,\"rep\":%hu}", vocab[i].pair[0], vocab[i].pair[1], vocab[i].token);
        if(i != header.len-1) {
            printf(",");
        }
    }

    fprintf(stdout, "]}\n");
    free(vocab);
    return 0;
}