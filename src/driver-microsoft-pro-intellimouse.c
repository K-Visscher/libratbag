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

#define STEELSERIES_NUM_PROFILE 1

#define MICROSOFT_WRITE_REPORT_ID 0x24
#define MICROSOFT_WRITE_REPORT_SIZE 0x49
#define MICROSOFT_READ_REPORT_ID 0x27
#define MICROSOFT_READ_REPORT_SIZE 0x29

#define MICROSOFT_LOD_WRITE_PROPERTY_ID 0xB8
#define MICROSOFT_LOD_READ_PROPERTY_ID MICROSOFT_LOD_WRITE_PROPERTY_ID - 0x02
#define MICROSOFT_POLLING_RATE_WRITE_PROPERTY_ID 0x83
#define MICROSOFT_POLLING_RATE_READ_PROPERTY_ID MICROSOFT_POLLING_RATE_WRITE_PROPERTY_ID + 0x01
#define MICROSOFT_LED_WRITE_PROPERTY_ID 0xB2
#define MICROSOFT_LED_READ_PROPERTY_ID MICROSOFT_LED_WRITE_PROPERTY_ID + 0x01
#define MICROSOFT_DPI_WRITE_PROPERTY_ID 0x96
#define MICROSOFT_DPI_READ_PROPERTY_ID MICROSOFT_DPI_WRITE_PROPERTY_ID + 0x01

/*
static const unsigned int lod_mapping[] =
{
    [0x00] = 0x02,
    [0x01] = 0x03
};

static const unsigned int lod_mapping_inverse[] =
{
    [0x02] = 0x00,
    [0x03] = 0x01
};

static const unsigned int polling_rate_mapping[] =
{
    [0x00] = 0x03E8,
    [0x01] = 0x01F4,
    [0x02] = 0x007D
};

static const unsigned int polling_rate_mapping_inverse[] =
{
    [0x03E8] = 0x00,
    [0x01F4] = 0x01,
    [0x007D] = 0x02
};
*/

static int microsoft_pro_intellimouse_write_profile(struct ratbag_profile* profile)
{
    /*TODO*/
    return 0;
}

static int microsoft_pro_intellimouse_test_hidraw(struct ratbag_device* device)
{
    return ratbag_hidraw_has_report(device, MICROSOFT_WRITE_REPORT_ID);
}

static int microsoft_pro_intellimouse_probe(struct ratbag_device* device)
{
    int return_code;

    return_code = ratbag_find_hidraw(device, microsoft_pro_intellimouse_test_hidraw);
    if (return_code)
	return return_code;

    return 0;
}

static int microsoft_pro_intellimouse_commit(struct ratbag_device* device)
{
    struct ratbag_profile* profile;
    int rc = 0;
    list_for_each(profile, &device->profiles, link)
    {
	if (!profile->dirty) continue;
	log_debug(device->ratbag, "profile %d changed, rewriting\n", profile->index);
	rc = microsoft_pro_intellimouse_write_profile(profile);
	if (rc) return rc;
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
