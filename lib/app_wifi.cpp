#include <string.h>
#include "common.h"
#ifdef USE_WIFI
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_log.h"

static const char *TAG = "wifi softAP";
#define EXAMPLE_ESP_WIFI_SSID   "Gamebiuno_AKA"
#define EXAMPLE_ESP_WIFI_CHANNEL 7
#define EXAMPLE_ESP_WIFI_PASS   "12345678"
#define EXAMPLE_MAX_STA_CONN 4
#define EXAMPLE_GTK_REKEY_INTERVAL 600

#if 0
    // http
#include "esp_http_server.h"
//#include "dns_server.h"



// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 13;
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    return server;
}

static void dhcp_set_captiveportal_url(void) {
    // get the IP of the access point to redirect to
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    // turn the IP into a URI
    char* captiveportal_uri = (char*) malloc(32 * sizeof(char));
    assert(captiveportal_uri && "Failed to allocate captiveportal_uri");
    strcpy(captiveportal_uri, "http://");
    strcat(captiveportal_uri, ip_addr);

    // get a handle to configure DHCP with
    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");

    // set the DHCP option 114
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(netif));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_CAPTIVEPORTAL_URI, captiveportal_uri, strlen(captiveportal_uri)));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(netif));
}
#endif

IRAM_ATTR static void wifi_event_handler_ap(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d, reason=%d",
                 MAC2STR(event->mac), event->aid, event->reason);
    }
}

void wifi_init_softap(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler_ap,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                    .required = true,
            },
#ifdef CONFIG_ESP_WIFI_BSS_MAX_IDLE_SUPPORT
            .bss_max_idle_cfg = {
                .period = WIFI_AP_DEFAULT_MAX_IDLE_PERIOD,
                .protected_keep_alive = 1,
            },
#endif
//            .gtk_rekey_interval = EXAMPLE_GTK_REKEY_INTERVAL,
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);

#if 0
    dhcp_set_captiveportal_url();

    // Start the server for the first time
    start_webserver();
#endif    
}

#define WIFI_STA_SSID   #eror need populate SSID as "ssid_xyz"
#define WIFI_STA_PASS   #eror need populate KEY as "12345678"

#define GOT_IP_EVENT             (1)
#define WIFI_DISCONNECT_EVENT    (1<<1)
#define WIFI_STA_CONNECTED       (1<<2)
#define WIFI_AP_STA_CONNECTED    (1<<3)
#define CONNECT_TIMEOUT_MS   (10000)

#define EVENT_HANDLER_FLAG_DO_NOT_AUTO_RECONNECT 0x00000001
static uint32_t wifi_event_handler_flag;

static EventGroupHandle_t wifi_events;

IRAM_ATTR static void wifi_event_handler_sta(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "wifi event handler: %"PRIi32, event_id);
    switch (event_id) {
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START");
        break;
    case WIFI_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED");
        if (wifi_events) {
            xEventGroupSetBits(wifi_events, WIFI_AP_STA_CONNECTED);
        }
        break;
    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");
        if (wifi_events) {
            xEventGroupSetBits(wifi_events, WIFI_STA_CONNECTED);
        }
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
        wifi_event_sta_disconnected_t *event = (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGI(TAG, "disconnect reason: %u", event->reason);
        if (!(EVENT_HANDLER_FLAG_DO_NOT_AUTO_RECONNECT & wifi_event_handler_flag)) {
            ESP_ERROR_CHECK(esp_wifi_connect());
        }
        if (wifi_events) {
            xEventGroupSetBits(wifi_events, WIFI_DISCONNECT_EVENT);
        }
        break;
    default:
        break;
    }
    return;
}


IRAM_ATTR static void ip_event_handler_sta(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    ip_event_got_ip_t *event;

    ESP_LOGI(TAG, "ip event handler");
    switch (event_id) {
    case IP_EVENT_STA_GOT_IP:
        event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP");
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        if (wifi_events) {
            xEventGroupSetBits(wifi_events, GOT_IP_EVENT);
        }
        break;
    default:
        break;
    }
    return;
}

int wifi_connect_sta(void)
{
EventBits_t bits;

    wifi_config_t w_config = {
        .sta.ssid = WIFI_STA_SSID,
        .sta.password = WIFI_STA_PASS,
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &w_config));
    ESP_ERROR_CHECK(esp_wifi_connect());
    ESP_LOGI(TAG, "start esp_wifi_connect: %s", WIFI_STA_SSID);
    bits = xEventGroupWaitBits(wifi_events, GOT_IP_EVENT, 1, 0, CONNECT_TIMEOUT_MS / portTICK_PERIOD_MS);
    //TEST_ASSERT(bits & GOT_IP_EVENT);
    if ( bits & GOT_IP_EVENT )
        return 0; // success
    return -1;  // fail
}


//static esp_netif_t* s_ap_netif = NULL;
static esp_netif_t* s_sta_netif = NULL;

static esp_err_t event_init(void)
{
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler_sta, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler_sta, NULL));
    s_sta_netif = esp_netif_create_default_wifi_sta();
//    s_ap_netif = esp_netif_create_default_wifi_ap();
    return ESP_OK;
}

static esp_err_t event_deinit(void)
{
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler_sta));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler_sta));
    esp_netif_destroy_default_wifi(s_sta_netif);
//    esp_netif_destroy_default_wifi(s_ap_netif);
    ESP_ERROR_CHECK(esp_event_loop_delete_default());
    return ESP_OK;
}


static void start_wifi_as_sta(void)
{

    event_init();

    if (wifi_events == NULL) {
        wifi_events = xEventGroupCreate();
    }
    xEventGroupClearBits(wifi_events, 0x00ffffff);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

}

static void stop_wifi_sta(void)
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    event_deinit();
    if (wifi_events) {
        vEventGroupDelete(wifi_events);
        wifi_events = NULL;
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);
}

#include "LCD.h"
#include "esp_http_client.h"

size_t size_download = 0;
IRAM_ATTR esp_err_t http_event_handle(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
        break;
    case HTTP_EVENT_ON_DATA:
//        esp_rom_md5_update(&md5_context, evt->data, evt->data_len);
        size_download += evt->data_len;
        lcd_printf( "get %d bytes (%d total)\n", evt->data_len, size_download );
        if ( lcd_refresh_completed() )
            lcd_refresh();
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    case HTTP_EVENT_REDIRECT:
        ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

void test_wifi_download_file()
{
    
    esp_http_client_config_t config = {
//        .url = "https://dl.espressif.com/dl/misc/2MB.bin",
        .url = "http://www.jmpapillon.fr/AKA/outrun.bmp",
//          .url = "http://www.jmpapillon.fr/AKA/Piskel-0.14.0.zip",
//        .cert_pem = dl_espressif_com_root_cert_pem_start,
        .event_handler = http_event_handle,
        .buffer_size = 5120,
//        .is_async = true
    };
    lcd_printf( "GET %s\n", config.url );
    lcd_refresh();
    esp_http_client_handle_t client = esp_http_client_init(&config);
//    TEST_ASSERT_NOT_NULL(client);

    lcd_printf( "esp_http_client_init return %d \n", client );
    lcd_refresh();

    ESP_ERROR_CHECK(esp_http_client_perform(client));
    /*
    int finish = 0;
    while (!finish)
    {
        esp_err_t ret = esp_http_client_perform(client);
        switch (ret)
        {
            case EAGAIN:
//            case EWOULDBLOCK:
            case EINPROGRESS:
                lcd_printf( "get %d total\n", size_download );
                lcd_refresh();
                break;

            case ESP_OK:
                lcd_printf( "Download SUCCESS\n" );
                lcd_refresh();
                finish = 1;
                lcd_refresh();
                break;
            case ESP_FAIL:
                lcd_printf( "Download FAIL\n" );
                finish = 1;
                lcd_refresh();
                break;
            default:
                lcd_printf( "esp_http_client_perform return %d\n", ret );
                lcd_refresh();
                break;

        }
       if ( lcd_refresh_completed() )
           lcd_refresh();
    }
    */

    ESP_ERROR_CHECK(esp_http_client_cleanup(client));
//    esp_rom_md5_final(digest, &md5_context);
}



void test_wifi_connection_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
//    ESP_ERROR_CHECK(esp_event_loop_create_default());
//    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    EventBits_t bits;
    start_wifi_as_sta();

    // make sure softap has started
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    int ret = wifi_connect_sta();
    lcd_printf( "Wifi connect return %d\n",  ret );
    lcd_refresh();
    if ( ret == 0 ) // download
    {
        lcd_printf( "CONNECTED !!!\n" );
        lcd_refresh();
        test_wifi_download_file();
    }

    // connected !
    lcd_printf( "Wait 6 seconds...\n" );
    lcd_refresh();
    // do not auto reconnect after connected
    xEventGroupClearBits(wifi_events, WIFI_DISCONNECT_EVENT);
    wifi_event_handler_flag |= EVENT_HANDLER_FLAG_DO_NOT_AUTO_RECONNECT;

    // disconnect event not triggered after 60s
    bits = xEventGroupWaitBits(wifi_events, WIFI_DISCONNECT_EVENT, 1, 0, 6000 / portTICK_PERIOD_MS);
//    TEST_ASSERT((bits & WIFI_DISCONNECT_EVENT) == 0);

    stop_wifi_sta();
}

#endif // USE_WIFI