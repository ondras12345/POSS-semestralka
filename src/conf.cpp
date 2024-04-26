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
    response->print("conf base_speed ");
    response->println(c.base_speed);

    response->print("conf Kp ");
    response->println(c.Kp);

    response->print("conf Ki ");
    response->println(c.Ki);
}
