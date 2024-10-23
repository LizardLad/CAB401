#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstdint>

#define VOCAB_DTYPE uint16_t //These two are related to each other
#define SKIP_TOKEN UINT16_MAX

#define CHUNK_SIZE 1<<19 //2MB (L2 cache per P-Core on the i5-13600k), meaning 1/2 of the data on the E cores will be in the L3 cache

#define VOCAB_START 256U

#endif