/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// This example uses SDMMC peripheral to communicate with SD card.

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "common.h"
// #include "sd_test_io.h"
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif
#define CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4 1
#define CONFIG_EXAMPLE_SDMMC_SPEED_DS 1

#define EXAMPLE_MAX_CHAR_SIZE 64

static const char *TAG = "example";

#define MOUNT_POINT "/sdcard"

int test_spiram(uint8_t *ptr, uint32_t length)
{
    int iError = 0;
    printf("Write %ld bytes patern to %p\n", length, ptr);
    for (uint32_t idx = 0; idx < length; idx++)
        ptr[idx] = (uint8_t)idx;
    printf("Check %ld bytes patern from %p\n", length, ptr);
    for (uint32_t idx = 0; idx < length; idx++)
    {
        if (ptr[idx] != (uint8_t)idx)
        {
            iError++;
            printf("ERROR %p : %d <> %d\n", &ptr[idx], ptr[idx], (uint8_t)idx);
        }
    }

    return 0;
}

void esp_ram_info()
{
    printf("PSRAM Total %u\n", heap_caps_get_total_size(MALLOC_CAP_SPIRAM));
    printf("PSRAM Free  %u\n", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    printf("PSRAM max bloc %u\n", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    printf("SRAM Total %u\n", heap_caps_get_total_size(MALLOC_CAP_8BIT));
    printf("SRAM Free  %u\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    printf("SRAM max bloc %u\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    //    printf("Total PSRAM: %u\n", ESP.getPsramSize());
    //    printf("Free PSRAM: %d\n", esp_spiram_get_size );
    // !! NOT in SDK v3
    //    Serial.printf("spiram size %u\n", esp_spiram_get_size());

    //    Serial.printf("himem free %u\n", esp_himem_get_free_size());
    //    Serial.printf("himem phys %u\n", esp_himem_get_phys_size());
    //    Serial.printf("himem reserved %u\n", esp_himem_reserved_area_size());
}

// uint16_t* audio_file__ptr = 0;
// uint32_t audio_file_smp_count = 0; // size in sample
FILE *f_wav = 0;
uint32_t filesize = 0;
uint32_t filepos = 0;
esp_err_t open_file(const char *path)
{
    printf("Loading file %s\n", path);
    f_wav = fopen(path, "r");
    if (f_wav == NULL)
    {
        ESP_LOGE(TAG, "Failed to open for reading");
        return ESP_FAIL;
    }

    fseek(f_wav, 0, SEEK_END);
    filesize = ftell(f_wav);
    printf("File size %ld \n", filesize);
    fseek(f_wav, 0, SEEK_SET);
    filepos = 0;
    /*
//    w_buf = (uint8_t *)calloc(1, EXAMPLE_BUFF_SIZE);
//    audio_file__ptr = (uint16_t*)calloc(1, filesize);
    esp_ram_info();
    audio_file__ptr = heap_caps_malloc(filesize, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    printf( "File pointer %p\n", audio_file__ptr );
    if(!audio_file__ptr)
    {
//        printf( "Memory allocation fail, try to use manual pointer\n" );
//        audio_file__ptr = (uint16_t*)0x3D800000;
//        audio_file__ptr = (uint16_t*)0x3C0F0000;
//        if ( test_spiram( (uint8_t*)audio_file__ptr, filesize) )
            return ESP_FAIL;
    }
    size_t ret = fread( audio_file__ptr, filesize, 1, f );
    printf( "Read return %d\n", ret );
    fclose(f);
    audio_file_smp_count = filesize/2;
    */
    return ESP_OK;
}

void read_audio_file(uint8_t *ptr, uint32_t u32_length)
{
    if (!f_wav)
        return;
    uint32_t u32_read = filesize - filepos;
    if (u32_read > u32_length)
        u32_read = u32_length;
    fread(ptr, u32_read, 1, f_wav);
    filepos += u32_read;
    if (filepos >= filesize)
    {
        fseek(f_wav, 0, SEEK_SET);
        filepos = 0;
    }
}

void sd_init(void)
{
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024};
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.

    ESP_LOGI(TAG, "Using SDMMC peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
#if CONFIG_EXAMPLE_SDMMC_SPEED_HS
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
#elif CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_SDR50
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_SDR50;
    host.flags &= ~SDMMC_HOST_FLAG_DDR;
#elif CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_DDR50
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_DDR50;
#endif

    // For SoCs where the SD power can be supplied both via an internal or external (e.g. on-board LDO) power supply.
    // When using specific IO pins (which can be used for ultra high-speed SDMMC) to connect to the SD card
    // and the internal LDO power supply, we need to initialize the power supply first.
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
        return;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;
#endif

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
#if EXAMPLE_IS_UHS1
    slot_config.flags |= SDMMC_SLOT_FLAG_UHS1;
#endif

    // Set bus width to use:
#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
    slot_config.width = 4;
#else
    slot_config.width = 1;
#endif
    /* ori
    #define CONFIG_EXAMPLE_PIN_CMD 35
    #define CONFIG_EXAMPLE_PIN_CLK 36
    #define CONFIG_EXAMPLE_PIN_D0 37
    #define CONFIG_EXAMPLE_PIN_D1 38
    #define CONFIG_EXAMPLE_PIN_D2 33
    #define CONFIG_EXAMPLE_PIN_D3 34
    */

    // On chips where the GPIOs used for SD card can be configured, set them in
    // the slot_config structure:
#ifdef CONFIG_SOC_SDMMC_USE_GPIO_MATRIX
	slot_config.clk = (gpio_num_t)CONFIG_EXAMPLE_PIN_CLK;
	slot_config.cmd = (gpio_num_t)CONFIG_EXAMPLE_PIN_CMD;
	slot_config.d0  = (gpio_num_t)CONFIG_EXAMPLE_PIN_D0;
#ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
	slot_config.d1  = (gpio_num_t)CONFIG_EXAMPLE_PIN_D1;
	slot_config.d2  = (gpio_num_t)CONFIG_EXAMPLE_PIN_D2;
	slot_config.d3  = (gpio_num_t)CONFIG_EXAMPLE_PIN_D3;
#endif // CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
#endif // CONFIG_SOC_SDMMC_USE_GPIO_MATRIX

    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                          "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                          "Make sure SD card lines have pull-up resistors in place.",
                     esp_err_to_name(ret));
#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
            check_sd_card_pins(&config, pin_count);
#endif
        }
        return;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files:

#ifdef CONFIG_EXAMPLE_TEST_FILES
    // First create a file.
    const char *file_hello = MOUNT_POINT "/hello.txt";
    char data[EXAMPLE_MAX_CHAR_SIZE];
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Hello", card->cid.name);
    ret = s_example_write_file(file_hello, data);
    if (ret != ESP_OK)
    {
        return;
    }

    const char *file_foo = MOUNT_POINT "/foo.txt";
    // Check if destination file exists before renaming
    struct stat st;
    if (stat(file_foo, &st) == 0)
    {
        // Delete it if it exists
        unlink(file_foo);
    }

    // Rename original file
    ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    if (rename(file_hello, file_foo) != 0)
    {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }

    ret = s_example_read_file(file_foo);
    if (ret != ESP_OK)
    {
        return;
    }
#endif

    // Format FATFS
#ifdef CONFIG_EXAMPLE_FORMAT_SD_CARD
    ret = esp_vfs_fat_sdcard_format(mount_point, card);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
        return;
    }

    if (stat(file_foo, &st) == 0)
    {
        ESP_LOGI(TAG, "file still exists");
        return;
    }
    else
    {
        ESP_LOGI(TAG, "file doesn't exist, formatting done");
    }
#endif // CONFIG_EXAMPLE_FORMAT_SD_CARD

#ifdef CONFIG_EXAMPLE_TEST_FILES

    const char *file_nihao = MOUNT_POINT "/nihao.txt";
    memset(data, 0, EXAMPLE_MAX_CHAR_SIZE);
    snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Nihao", card->cid.name);
    ret = s_example_write_file(file_nihao, data);
    if (ret != ESP_OK)
    {
        return;
    }

    // Open file for reading
    ret = s_example_read_file(file_nihao);
    if (ret != ESP_OK)
    {
        return;
    }

#endif
    //    open_file( MOUNT_POINT"/NTM48k.wav" );
    // open_file(MOUNT_POINT "/anarch.wav");

    // keep filesystem open !
    return;

    // All done, unmount partition and disable SDMMC peripheral
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted");

    // Deinitialize the power control driver if it was used
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    ret = sd_pwr_ctrl_del_on_chip_ldo(pwr_ctrl_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to delete the on-chip LDO power control driver");
        return;
    }
#endif
}
