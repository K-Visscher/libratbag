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
#define MICROSOFT_NUM_BUTTONS 0x05
#define MICROSOFT_NUM_LED 0x01

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

static unsigned int microsoft_pro_intellimouse_raw_to_rate(unsigned int data)
{
    struct microsoft_pro_intellimouse_report_rate_mapping* mapping;
    ARRAY_FOR_EACH(microsoft_pro_intellimouse_report_rate_mapping, mapping) {
	if (mapping->raw == data)
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

struct microsoft_pro_intellimouse_lod_mapping {
    unsigned int raw;
    unsigned int lod;
};

static struct microsoft_pro_intellimouse_lod_mapping microsoft_pro_intellimouse_lod_mapping[] = {
	{ 0x00, 0x02 },
	{ 0x01, 0x03 }
};

static unsigned int microsoft_pro_intellimouse_raw_to_lod(unsigned int data)
{
    struct microsoft_pro_intellimouse_lod_mapping* mapping;
    ARRAY_FOR_EACH(microsoft_pro_intellimouse_lod_mapping, mapping) {
	if (mapping->raw == data)
	    return mapping->lod;
    }
    return 0;
}

static unsigned int microsoft_pro_intellimouse_lod_to_raw(unsigned int lod)
{
    struct microsoft_pro_intellimouse_lod_mapping* mapping;
    ARRAY_FOR_EACH(microsoft_pro_intellimouse_lod_mapping, mapping) {
	if (mapping->lod == lod)
	    return mapping->raw;
    }
    return 0;
}

static unsigned int microsoft_pro_intellimouse_read_property(struct ratbag_device* device, unsigned int property_id)
{
    /* TODO */
    return 0;
}

static unsigned int microsoft_pro_intellimouse_write_property(struct ratbag_device* device, unsigned int property_id, unsigned int value)
{
    /* TODO */
    return 0;
}

static unsigned int microsoft_pro_intellimouse_read_lod(struct ratbag_device* device)
{
    /* TODO */
    return 0;
}

static int microsoft_pro_intellimouse_write_lod(struct ratbag_device* device, unsigned int lod)
{
    /* TODO */
    return 0;
}

static int microsoft_pro_intellimouse_read_led(struct ratbag_device* device)
{
    /* TODO */
    return 0;
}

static int microsoft_pro_intellimouse_write_led(struct ratbag_device* device, unsigned int color)
{
    /* TODO */
    return 0;
}

static unsigned int microsoft_pro_intellimouse_read_dpi(struct ratbag_device* device)
{
    /* TODO */
    return 0;
}

static int microsoft_pro_intellimouse_write_dpi(struct ratbag_device* device, unsigned int dpi)
{
    /* TODO */
    return 0;
}


static int microsoft_pro_intellimouse_read_report_rate(struct ratbag_device* device)
{
    /* TODO */
    return 0;
}

static int microsoft_pro_intellimouse_write_report_rate(struct ratbag_device* device, unsigned int rate)
{
    /* TODO */
    return 0;
}

static int microsoft_pro_intellimouse_write_profile(struct ratbag_profile* profile)
{
    /*TODO*/
    return 0;
}

static int microsoft_pro_intellimouse_read_profile(struct ratbag_profile* profile)
{
    /*TODO*/
    struct ratbag_device* device = profile->device;

    ratbag_profile_set_report_rate_list(profile, microsoft_pro_intellimouse_report_rates, ARRAY_LENGTH(microsoft_pro_intellimouse_report_rates));
    ratbag_profile_set_report_rate(profile, microsoft_pro_intellimouse_read_report_rate(device));

    return 0;
}

static int microsoft_pro_intellimouse_test_hidraw(struct ratbag_device* device)
{
    return ratbag_hidraw_has_report(device, MICROSOFT_WRITE_REPORT_ID);
}

static int microsoft_pro_intellimouse_probe(struct ratbag_device* device)
{
    int rc;
    struct ratbag_profile* profile;

    rc = ratbag_find_hidraw(device, microsoft_pro_intellimouse_test_hidraw);
    if (rc)
	return rc;

    ratbag_device_init_profiles(device,
	MICROSOFT_NUM_PROFILE,
	MICROSOFT_NUM_DPI,
	MICROSOFT_NUM_BUTTONS,
	MICROSOFT_NUM_LED);

    ratbag_device_for_each_profile(device, profile)
	microsoft_pro_intellimouse_read_profile(profile);

    return 0;
}

static int microsoft_pro_intellimouse_commit(struct ratbag_device* device)
{
    struct ratbag_profile* profile;
    int rc = 0;
    list_for_each(profile, &device->profiles, link) {
	if (!profile->dirty)
	    continue;

	log_debug(device->ratbag, "profile %d changed, rewriting\n", profile->index);
	rc = microsoft_pro_intellimouse_write_profile(profile);
	if (rc)
	    return rc;
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
