#ifndef ERROR_H
#define ERROR_H

/**
 * @brief Error definitions. Allows for intelligent error passing.
 */
typedef enum {
    ERROR_OK = 0,
    ERROR_BAD_INPUT,
    ERROR_HAL,
} error_t;

#endif //ERROR_H