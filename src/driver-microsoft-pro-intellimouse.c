#include "config.h"
#include <assert.h>
#include <errno.h>
#include <libevdev/libevdev.h>
#include <linux/input.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "libratbag-private.h"
#include "libratbag-hidraw.h"

#define MICROSOFT_NUM_PROFILE 0x01
#define MICROSOFT_NUM_DPI 0x01
#define MICROSOFT_NUM_BUTTONS 0x00 /* TODO: IMPLEMENT BUTTON REMAPPING */
#define MICROSOFT_NUM_LED 0x01
#define MICROSOFT_MIN_DPI 0xC8
#define MICROSOFT_MAX_DPI 0x3E80

#define MICROSOFT_WRITE_REPORT_ID 0x24
#define MICROSOFT_WRITE_REPORT_SIZE 0x49
#define MICROSOFT_READ_REPORT_ID 0x27
#define MICROSOFT_READ_REPORT_SIZE 0x29

#define MICROSOFT_LOD_WRITE_PROPERTY_ID 0xB8
#define MICROSOFT_LOD_READ_PROPERTY_ID MICROSOFT_LOD_WRITE_PROPERTY_ID - 0x02
#define MICROSOFT_REPORT_RATE_WRITE_PROPERTY_ID 0x83
#define MICROSOFT_REPORT_RATE_READ_PROPERTY_ID MICROSOFT_REPORT_RATE_WRITE_PROPERTY_ID + 0x01
#define MICROSOFT_LED_WRITE_PROPERTY_ID 0xB2
#define MICROSOFT_LED_READ_PROPERTY_ID MICROSOFT_LED_WRITE_PROPERTY_ID + 0x01
#define MICROSOFT_DPI_WRITE_PROPERTY_ID 0x96
#define MICROSOFT_DPI_READ_PROPERTY_ID MICROSOFT_DPI_WRITE_PROPERTY_ID + 0x01


struct microsoft_pro_intellimouse_report_rate_mapping {
    unsigned int raw;
    unsigned int rate;
};

unsigned int microsoft_pro_intellimouse_report_rates[] = { 125, 500, 1000 };

static struct microsoft_pro_intellimouse_report_rate_mapping microsoft_pro_intellimouse_report_rate_mapping[] = {
	{ 0x00, 1000 },
	{ 0x01, 500 },
	{ 0x02, 125}
};

static unsigned int microsoft_pro_intellimouse_raw_to_rate(unsigned int raw)
{
    struct microsoft_pro_intellimouse_report_rate_mapping* mapping;
    ARRAY_FOR_EACH(microsoft_pro_intellimouse_report_rate_mapping, mapping) {
	if (mapping->raw == raw)
	    return mapping->rate;
    }
    return 0;
}

static unsigned int microsoft_pro_intellimouse_rate_to_raw(unsigned int rate)
{
    struct microsoft_pro_intellimouse_report_rate_mapping* mapping;
    ARRAY_FOR_EACH(microsoft_pro_intellimouse_report_rate_mapping, mapping) {
	if (mapping->rate == rate)
	    return mapping->raw;
    }
    return 0;
}

static int microsoft_pro_intellimouse_read_property(struct ratbag_device* device, unsigned int property_id, uint8_t* buf, size_t len)
{
    uint8_t write_buf[MICROSOFT_WRITE_REPORT_SIZE] = { MICROSOFT_WRITE_REPORT_ID, property_id };
    int ret;

    ret = ratbag_hidraw_set_feature_report(device, write_buf[0], write_buf, sizeof(write_buf));
    if (ret < 0)
	return ret;
    if (ret != sizeof(write_buf))
	return -EIO;

    uint8_t read_buf[MICROSOFT_READ_REPORT_SIZE] = { MICROSOFT_READ_REPORT_ID };
    ret = ratbag_hidraw_read_input_report(device, read_buf, sizeof(read_buf));
    if (ret < 0)
	return ret;
    if (ret != sizeof(read_buf))
	return -EIO;

    if (len < read_buf[3])
	return -EINVAL;
    
    for (int i = 4, j = 0; i < 4 + read_buf[3]; i++, j++)
    {
	*(buf + j) = read_buf[i];
    }

    return 0;
}

static int microsoft_pro_intellimouse_write_property(struct ratbag_device* device, unsigned int property_id, uint8_t* buf, size_t len)
{
    int ret;
    uint8_t write_buf[MICROSOFT_WRITE_REPORT_SIZE] = { MICROSOFT_WRITE_REPORT_ID, property_id, len };

    if (len > MICROSOFT_WRITE_REPORT_SIZE - 3)
	return -EINVAL;

    for (unsigned int i = 0; i < len; i++)
    {
	write_buf[3 + i] = *(buf + i);
    }

    ret = ratbag_hidraw_set_feature_report(device, write_buf[0], write_buf, sizeof(write_buf));
    if (ret < 0)
	    return ret;
    if (ret != sizeof(write_buf))
	return -EIO;

    return 0;
}

static int microsoft_pro_intellimouse_read_led(struct ratbag_led* led)
{
    struct ratbag_profile* profile = led->profile;
    struct ratbag_device* device = profile->device;

    int ret;
    uint8_t read_buf[3] = { 0 };

    ret = microsoft_pro_intellimouse_read_property(device, MICROSOFT_LED_READ_PROPERTY_ID, read_buf, sizeof(read_buf));
    if (ret < 0)
	return ret;

    led->type = RATBAG_LED_TYPE_LOGO;
    led->colordepth = RATBAG_LED_COLORDEPTH_RGB_888;
    ratbag_led_set_mode_capability(led, RATBAG_LED_ON);

    led->mode = RATBAG_LED_ON;

    led->color.red = read_buf[0];
    led->color.green = read_buf[1];
    led->color.blue = read_buf[2];

    return 0;
}

static int microsoft_pro_intellimouse_write_led(struct ratbag_led* led)
{
    int ret;
    struct ratbag_profile* profile = led->profile;
    struct ratbag_device* device = profile->device;

    uint8_t write_buf[3] =
    {
	(uint8_t) led->color.red,
	(uint8_t) led->color.green,
	(uint8_t) led->color.blue
    };

    ret = microsoft_pro_intellimouse_write_property(device, MICROSOFT_LED_WRITE_PROPERTY_ID, write_buf, sizeof(write_buf));
    if (ret < 0)
	return ret;

    return 0;
}

static int microsoft_pro_intellimouse_read_dpi(struct ratbag_device* device, uint16_t* dpi)
{
    int ret;
    uint8_t read_buf[2] = { 0 };

    ret = microsoft_pro_intellimouse_read_property(device, MICROSOFT_DPI_READ_PROPERTY_ID, read_buf, sizeof(read_buf));
    if (ret < 0)
	return ret;

    *dpi = 0;
    *dpi = read_buf[1] << 8 | read_buf[0];

    return 0;
}

static int microsoft_pro_intellimouse_write_dpi(struct ratbag_device* device, uint16_t dpi)
{
    int ret;
    uint8_t write_buf[2] = { (uint8_t) dpi, (uint8_t) dpi >> 8 };

    ret = microsoft_pro_intellimouse_write_property(device, MICROSOFT_DPI_WRITE_PROPERTY_ID, write_buf, sizeof(write_buf));
    if (ret < 0)
	return ret;

    return 0;
}


static int microsoft_pro_intellimouse_read_report_rate(struct ratbag_device* device, uint16_t* rate)
{
    int ret;
    uint8_t read_buf[1] = { 0 };

    ret = microsoft_pro_intellimouse_read_property(device, MICROSOFT_REPORT_RATE_READ_PROPERTY_ID, read_buf, sizeof(read_buf));
    if (ret < 0)
	return ret;

    *rate = microsoft_pro_intellimouse_raw_to_rate(read_buf[0]);

    return 0;
}

static int microsoft_pro_intellimouse_write_report_rate(struct ratbag_device* device, uint16_t rate)
{
    int ret;
    uint8_t write_buf[1] = { (uint8_t) microsoft_pro_intellimouse_rate_to_raw(rate) };

    ret = microsoft_pro_intellimouse_write_property(device, MICROSOFT_REPORT_RATE_WRITE_PROPERTY_ID, write_buf, sizeof(write_buf));
    if (ret < 0)
	return ret;

    return 0;
}

static int microsoft_pro_intellimouse_write_profile(struct ratbag_profile* profile)
{
    int ret;
    struct ratbag_device* device = profile->device;
    struct ratbag_resolution* resolution;
    struct ratbag_led* led;

    ret = microsoft_pro_intellimouse_write_report_rate(device, profile->hz);
    if (ret < 0)
	return ret;

    ratbag_profile_for_each_resolution(profile, resolution) {
	ret = microsoft_pro_intellimouse_write_dpi(device, (uint16_t) resolution->dpi_x);
	if (ret < 0)
	    return ret;
    }

    ratbag_profile_for_each_led(profile, led) {
	ret = microsoft_pro_intellimouse_write_led(led);
	if (ret < 0)
	    return ret;
    }

    return 0;
}

static int microsoft_pro_intellimouse_read_profile(struct ratbag_profile* profile)
{
    int ret;
    struct ratbag_device* device = profile->device;
    struct ratbag_resolution* resolution;
    struct ratbag_led* led;

    ratbag_profile_set_report_rate_list(profile, microsoft_pro_intellimouse_report_rates, ARRAY_LENGTH(microsoft_pro_intellimouse_report_rates));

    uint16_t rate = 0;
    ret = microsoft_pro_intellimouse_read_report_rate(device, &rate);
    if (ret < 0)
	return ret;

    ratbag_profile_set_report_rate(profile, rate);


    ratbag_profile_for_each_resolution(profile, resolution) {
	uint16_t dpi = 0;
	ret = microsoft_pro_intellimouse_read_dpi(device, &dpi);
	if (ret < 0)
	    return ret;

	resolution->dpi_x = dpi;
	resolution->dpi_y = dpi;
	resolution->is_default = true;
	resolution->is_active = true;

	ratbag_resolution_set_dpi_list_from_range(resolution, MICROSOFT_MIN_DPI, MICROSOFT_MAX_DPI);
    }

    ratbag_profile_for_each_led(profile, led) {
	ret = microsoft_pro_intellimouse_read_led(led);
	if (ret < 0)
	    return ret;
    }

    profile->name = "default";
    profile->is_active = true;

    return 0;
}

static int microsoft_pro_intellimouse_test_hidraw(struct ratbag_device* device)
{
    return ratbag_hidraw_has_report(device, MICROSOFT_WRITE_REPORT_ID);
}

static int microsoft_pro_intellimouse_probe(struct ratbag_device* device)
{
    int ret;
    struct ratbag_profile* profile;

    ret = ratbag_find_hidraw(device, microsoft_pro_intellimouse_test_hidraw);
    if (ret)
	return ret;

    ratbag_device_init_profiles(device,
	MICROSOFT_NUM_PROFILE,
	MICROSOFT_NUM_DPI,
	MICROSOFT_NUM_BUTTONS, /* TODO: IMPLEMENT BUTTON REMAPPING */
	MICROSOFT_NUM_LED);

    ratbag_device_for_each_profile(device, profile)
	microsoft_pro_intellimouse_read_profile(profile);

    return 0;
}

static int microsoft_pro_intellimouse_commit(struct ratbag_device* device)
{
    struct ratbag_profile* profile;
    int ret;
    list_for_each(profile, &device->profiles, link) {
	if (!profile->dirty)
	    continue;

	log_debug(device->ratbag, "profile %d changed, rewriting\n", profile->index);
	ret = microsoft_pro_intellimouse_write_profile(profile);
	if (ret)
	    return ret;
    }
    return 0;
}

static void microsoft_pro_intellimouse_remove(struct ratbag_device* device)
{
    ratbag_close_hidraw(device);
}


struct ratbag_driver microsoft_pro_intellimouse_driver =
{
	.name = "Microsoft Pro Intellimouse",
	.id = "microsoft-pro-intellimouse",
	.probe = microsoft_pro_intellimouse_probe,
	.commit = microsoft_pro_intellimouse_commit,
	.remove = microsoft_pro_intellimouse_remove
};
