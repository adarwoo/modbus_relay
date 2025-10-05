/**
 * Configuration file for ULOG
 */

// Define the project preferred log level
#ifdef NDEBUG
#   define ULOG_LEVEL ULOG_LEVEL_DEBUG3
#else
#   define ULOG_LEVEL ULOG_LEVEL_DEBUG3
#endif

// Override the default queue size (number of messages that can be buffered)
// #define ULOG_QUEUE_SIZE 64