#pragma once

#define APP_NAME "servidorftp"
#define VERSION "1.0"
#define OS_NAME "Linux"
#define BANNER APP_NAME " version " VERSION " Ready "
#define BUG_EMAIL "danielvinzia@gmail.com"

#define FTP_PORT 21
#define LOCALHOST "127.0.0.1"
#define BUFSIZE 512
#define USERNAME_MAX 64

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define PORT_DOC "Port number (default: " STR(FTP_PORT) ")"
#define ADDR_DOC "Local IP address (default: " LOCALHOST ")"