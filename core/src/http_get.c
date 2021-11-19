#include <string.h>

#include "mgos.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

static void http_get_task(int field, float value, int field2, float value2, int field3, float value3)
{
    char *web_server = "api.thingspeak.com";
    char *web_port = "80";
    char *web_path;
    asprintf(&web_path, "/update?api_key=ZKVAX1H28KORXNXB&field%d=%lf&field%d=%lf&field%d=%lf", field, value, field2, value2, field3, value3);

    char *REQUEST;
    asprintf(&REQUEST, "GET %s  HTTP/1.0\r\nHost: %s:%s\r\nUser-Agent: esp-idf/1.0 esp32\r\n\r\n", web_path, web_server, web_port);


    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

        int err = getaddrinfo(web_server, web_port, &hints, &res);

        if(err != 0 || res == NULL) {
            //ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            LOG(LL_INFO, ("DNS lookup failed!"));
            return;
        }

        /* Code to print the resolved IP.
           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        //ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            //ESP_LOGE(TAG, "... Failed to allocate socket.");
            LOG(LL_INFO, ("... Failed to allocate socket."));
            freeaddrinfo(res);
            return;
        }
        //ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            //ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            LOG(LL_INFO, ("... socket connect failed."));
            close(s);
            freeaddrinfo(res);
            return;
        }

        //ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

        if (write(s, REQUEST, strlen(REQUEST)) < 0) {
            //ESP_LOGE(TAG, "... socket send failed");
            LOG(LL_INFO, ("... socket send failed."));
            close(s);
            return;
        }
        //ESP_LOGI(TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            //ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            LOG(LL_INFO, ("... failed to set socket receiving timeout."));
            close(s);
            return;
        }
        //ESP_LOGI(TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        //ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            //ESP_LOGI(TAG, "%d... ", countdown);
        }
        //ESP_LOGI(TAG, "Starting again!");
}