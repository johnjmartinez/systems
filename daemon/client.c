#include "client.h"

/*
CLIENT = yash <SERVER_IP_ADDR> (port is implicit = 3826)
    protocol:
        CMD <cmd_string> \n
        CTL <[c|z|d]> \n
    end: 
        ctrl+d (EOF) or 'quit'
    signal handling:
        ctrl+c will send "CTL c\n" (stop cmd on server)
        ctrl+x will send "CTL z\n" (suspend cmd on server)
 */

int main(int argc, char* argv[]) {

    return (EXIT_SUCCESS);
}

