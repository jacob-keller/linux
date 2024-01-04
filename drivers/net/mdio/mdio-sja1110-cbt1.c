// SPDX-License-Identifier: GPL-2.0
/* Copyright 2022 NXP
 *
 * NXP SJA1110 100BASE-T1 MDIO bus driver
 */
#include <linux/module.h>
#include <linux/of_mdio.h>
#include <linux/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

struct sja1110_base_t1_private {
	struct regmap *regmap;
	struct mii_bus *bus;
};

enum sja1110_mdio_opcode {
	SJA1110_C45_ADDR = 0,
	SJA1110_C22 = 1,
	SJA1110_C45_DATA = 2,
	SJA1110_C45_DATA_AUTOINC = 3,
};

static unsigned int sja1110_base_t1_encode_addr(unsigned int phy,
						enum sja1110_mdio_opcode op,
						unsigned int xad)
{
	return (phy << 9) | (op << 7) | (xad << 2);
}

static int sja1110_base_t1_mdio_read_c22(struct mii_bus *bus, int phy, int reg)
{
	struct sja1110_base_t1_private *priv = bus->priv;
	struct regmap *regmap = priv->regmap;
	unsigned int addr, val;
	int err;

	addr = sja1110_base_t1_encode_addr(phy, SJA1110_C22, reg & 0x1f);

	err = regmap_read(regmap, addr, &val);
	if (err)
		return err;

	return val & 0xffff;
}

static int sja1110_base_t1_mdio_read_c45(struct mii_bus *bus, int phy,
					 int mmd, int reg)
{
	struct sja1110_base_t1_private *priv = bus->priv;
	struct regmap *regmap = priv->regmap;
	unsigned int addr, val;
	int err;

	addr = sja1110_base_t1_encode_addr(phy, SJA1110_C45_ADDR, mmd);
	err = regmap_write(regmap, addr, reg);
	if (err)
		return err;

	addr = sja1110_base_t1_encode_addr(phy, SJA1110_C45_DATA, mmd);
	err = regmap_read(regmap, addr, &val);
	if (err)
		return err;

	return val & 0xffff;
}

static int sja1110_base_t1_mdio_write_c22(struct mii_bus *bus, int phy, int reg,
					  u16 val)
{
	struct sja1110_base_t1_private *priv = bus->priv;
	struct regmap *regmap = priv->regmap;
	unsigned int addr;

	addr = sja1110_base_t1_encode_addr(phy, SJA1110_C22, reg & 0x1f);
	return regmap_write(regmap, addr, val & 0xffff);
}

static int sja1110_base_t1_mdio_write_c45(struct mii_bus *bus, int phy,
					  int mmd, int reg, u16 val)
{
	struct sja1110_base_t1_private *priv = bus->priv;
	struct regmap *regmap = priv->regmap;
	unsigned int addr;
	int err;

	addr = sja1110_base_t1_encode_addr(phy, SJA1110_C45_ADDR, mmd);
	err = regmap_write(regmap, addr, reg);
	if (err)
		return err;

	addr = sja1110_base_t1_encode_addr(phy, SJA1110_C45_DATA, mmd);
	return regmap_write(regmap, addr, val & 0xffff);
}

static int sja1110_base_t1_mdio_probe(struct platform_device *pdev)
{
	struct sja1110_base_t1_private *priv;
	struct device *dev = &pdev->dev;
	struct regmap *regmap;
	struct resource *res;
	struct mii_bus *bus;
	int err;

	if (!dev->of_node || !dev->parent)
		return -ENODEV;

	res = platform_get_resource(pdev, IORESOURCE_REG, 0);
	if (!res)
		return -ENODEV;

	regmap = dev_get_regmap(dev->parent, res->name);
	if (!regmap)
		return -ENODEV;

	bus = mdiobus_alloc_size(sizeof(*priv));
	if (!bus)
		return -ENOMEM;

	bus->name = "SJA1110 100base-T1 MDIO bus";
	snprintf(bus->id, MII_BUS_ID_SIZE, "%s", dev_name(dev));
	bus->read = sja1110_base_t1_mdio_read_c22;
	bus->write = sja1110_base_t1_mdio_write_c22;
	bus->read_c45 = sja1110_base_t1_mdio_read_c45;
	bus->write_c45 = sja1110_base_t1_mdio_write_c45;
	bus->parent = dev;
	priv = bus->priv;
	priv->regmap = regmap;

	err = of_mdiobus_register(bus, dev->of_node);
	if (err)
		goto err_free_bus;

	priv->bus = bus;
	platform_set_drvdata(pdev, priv);

	return 0;

err_free_bus:
	mdiobus_free(bus);

	return err;
}

static int sja1110_base_t1_mdio_remove(struct platform_device *pdev)
{
	struct sja1110_base_t1_private *priv = platform_get_drvdata(pdev);

	mdiobus_unregister(priv->bus);
	mdiobus_free(priv->bus);

	return 0;
}

static const struct of_device_id sja1110_base_t1_mdio_match[] = {
	{ .compatible = "nxp,sja1110-base-t1-mdio", },
	{},
};
MODULE_DEVICE_TABLE(of, sja1110_base_t1_mdio_match);

static struct platform_driver sja1110_base_t1_mdio_driver = {
	.probe = sja1110_base_t1_mdio_probe,
	.remove = sja1110_base_t1_mdio_remove,
	.driver = {
		.name = "sja1110-base-t1-mdio",
		.of_match_table = sja1110_base_t1_mdio_match,
	},
};

module_platform_driver(sja1110_base_t1_mdio_driver);

MODULE_DESCRIPTION("NXP SJA1110 100BASE-T1 MDIO bus driver");
MODULE_AUTHOR("Vladimir Oltean <vladimir.oltean@nxp.com>");
MODULE_LICENSE("GPL");
