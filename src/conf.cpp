#include "conf.h"

conf_t conf;

void conf_init()
{
    memset(&conf, 0, sizeof(conf_t));
    conf.base_speed = 50;
    conf.Ki = 0.5;
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
