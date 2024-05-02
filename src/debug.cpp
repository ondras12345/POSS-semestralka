#include "debug.h"

class NullPrint : public Print {
    public:
        size_t write(uint8_t c)
        {
            return 1;
        }

        size_t write(const uint8_t *data, size_t size)
        {
            return size;
        }
};

static NullPrint null = NullPrint();


Print * DEBUG_null = &null;

#define X_def(name) extern Print * DEBUG_##name = DEBUG_null;
DEBUGGERS(X_def)
