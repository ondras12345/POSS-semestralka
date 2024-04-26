#include "conf.h"

conf_t conf;

#define X_DEFAULT(type, name, default) default,
static const conf_t conf_default = {
    CONF_ITEMS(X_DEFAULT)
};
#undef X_DEFAULT

void conf_init()
{
    conf = conf_default;
}


/**
 * Print conf_t in a format that allows it to be imported by pasting the
 * output to the cli (conf command).
 */
void conf_print(Print *response, conf_t c)
{
#define PRINT_uint8_t(name) c.name
#define PRINT_float(name) c.name
#define PRINT_bool(name) (c.name ? '1' : '0')
#define X_PRINT(type, name, default) \
    response->print("conf " #name " "); \
    response->println( PRINT_##type(name) );

    CONF_ITEMS(X_PRINT)
}
