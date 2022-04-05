/*
 * @auther      : Parkertao
 * @Date        : 2022-3-27
 * @copyright Apache 2.0
*/

#include <unistd.h>
#include "server/webserver.h"

int main() {
    WebServer server(
        9006, 3, 60000, false,
        9006, "tau", "tau", "mydb",
        1, 1, true, 0, 1024
    );
    server.Start();
}