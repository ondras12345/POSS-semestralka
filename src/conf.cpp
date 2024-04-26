#include "conf.h"

conf_t conf;

conf_t conf_default = {
    0 // all zeros, TODO
};


void conf_init()
{
    conf = conf_default;  // TODO
}


/**
 * Print conf_t in a format that allows it to be imported by pasting the
 * output to the cli (conf command).
 */
void conf_print(Print *response, conf_t c)
{
    response->print("conf test_uint ");
    response->println(c.test_uint);

    response->print("conf test_bool ");
    response->println(c.test_bool ? '1' : '0');

    response->print("conf test_float ");
    response->println(c.test_float);  // TODO better float print
}
