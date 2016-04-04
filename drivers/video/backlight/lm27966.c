/*
 * TI LM27966 LED Driver
 *
 * adapted from lv5207lp.c
 * Copyright (C) 2013 Ideas on board SPRL
 *
 * Contact: Laurent Pinchart <laurent.pinchart@ideasonboard.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/fb.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/platform_data/lm27966.h>
#include <linux/slab.h>

#define LM27966_ENABLE_REG                  0x10
#define LM27966_DISP_BRIGHTNESS_REG         0xA0
#define LM27966_AUX_BRIGHTNESS_REG          0xC0

#define LM27966_ENABLE_MASK                 0x01
#define LM27966_ENABLE_DEFAULT              0x20

#define LM27966_DISP_BRIGHTNESS_MASK        0x1F
#define LM27966_DISP_BRIGHTNESS_DEFAULT     0xE0

#define LM27966_AUX_BRIGHTNESS_MASK         0x03
#define LM27966_AUX_BRIGHTNESS_DEFAULT      0xFC

#define LM27966_MAX_BRIGHTNESS		32

struct lm27966 {
	struct i2c_client *client;
	struct backlight_device *backlight;
	struct lm27966_platform_data *pdata;
};

static int lm27966_write(struct lm27966 *lv, u8 reg, u8 data)
{
	return i2c_smbus_write_byte_data(lv->client, reg, data);
}

static int lm27966_backlight_update_status(struct backlight_device *backlight)
{
	struct lm27966 *lv = bl_get_data(backlight);
	int brightness = backlight->props.brightness;

	if (backlight->props.power != FB_BLANK_UNBLANK ||
	    backlight->props.fb_blank != FB_BLANK_UNBLANK ||
	    backlight->props.state & (BL_CORE_SUSPENDED | BL_CORE_FBBLANK))
		brightness = 0;

	if (brightness) {
		lm27966_write(lv, lm27966_CTRL1,
			       lm27966_CPSW | lm27966_C10 | lm27966_CKSW);
		lm27966_write(lv, lm27966_CTRL2,
			       lm27966_MSW | lm27966_MLED4 |
			       (brightness - 1));
	} else {
		lm27966_write(lv, lm27966_CTRL1, 0);
		lm27966_write(lv, lm27966_CTRL2, 0);
	}

	return 0;
}

static int lm27966_backlight_get_brightness(struct backlight_device *backlight)
{
	return backlight->props.brightness;
}

static int lm27966_backlight_check_fb(struct backlight_device *backlight,
				       struct fb_info *info)
{
	struct lm27966 *lv = bl_get_data(backlight);

	return lv->pdata->fbdev == NULL || lv->pdata->fbdev == info->dev;
}

static const struct backlight_ops lm27966_backlight_ops = {
	.options	= BL_CORE_SUSPENDRESUME,
	.update_status	= lm27966_backlight_update_status,
	.get_brightness	= lm27966_backlight_get_brightness,
	.check_fb	= lm27966_backlight_check_fb,
};

static int lm27966_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	struct lm27966_platform_data *pdata = dev_get_platdata(&client->dev);
	struct backlight_device *backlight;
	struct backlight_properties props;
	struct lm27966 *lv;

	if (pdata == NULL) {
		dev_err(&client->dev, "No platform data supplied\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_warn(&client->dev,
			 "I2C adapter doesn't support I2C_FUNC_SMBUS_BYTE\n");
		return -EIO;
	}

	lv = devm_kzalloc(&client->dev, sizeof(*lv), GFP_KERNEL);
	if (!lv)
		return -ENOMEM;

	lv->client = client;
	lv->pdata = pdata;

	memset(&props, 0, sizeof(props));
	props.type = BACKLIGHT_RAW;
	props.max_brightness = min_t(unsigned int, pdata->max_value,
				     LM27966_MAX_BRIGHTNESS);
	props.brightness = clamp_t(unsigned int, pdata->def_value, 0,
				   props.max_brightness);

	backlight = devm_backlight_device_register(&client->dev,
				dev_name(&client->dev), &lv->client->dev,
				lv, &lm27966_backlight_ops, &props);
	if (IS_ERR(backlight)) {
		dev_err(&client->dev, "failed to register backlight\n");
		return PTR_ERR(backlight);
	}

	backlight_update_status(backlight);
	i2c_set_clientdata(client, backlight);

	return 0;
}

static int lm27966_remove(struct i2c_client *client)
{
	struct backlight_device *backlight = i2c_get_clientdata(client);

	backlight->props.brightness = 0;
	backlight_update_status(backlight);

	return 0;
}

static const struct i2c_device_id lm27966_ids[] = {
	{ "lm27966", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lm27966_ids);

static struct i2c_driver lm27966_driver = {
	.driver = {
		.name = "lm27966",
	},
	.probe = lm27966_probe,
	.remove = lm27966_remove,
	.id_table = lm27966_ids,
};

module_i2c_driver(lm27966_driver);

MODULE_DESCRIPTION("Texas Instruments LM27966 Backlight Driver");
MODULE_AUTHOR("Joerg Rothe <rothe@venneos.com>");
MODULE_AUTHOR("Laurent Pinchart <laurent.pinchart@ideasonboard.com>");
MODULE_LICENSE("GPL");
