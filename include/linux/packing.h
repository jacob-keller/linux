/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2016-2018 NXP
 * Copyright (c) 2018-2019, Vladimir Oltean <olteanv@gmail.com>
 */
#ifndef _LINUX_PACKING_H
#define _LINUX_PACKING_H

#include <linux/types.h>
#include <linux/bitops.h>

#define QUIRK_MSB_ON_THE_RIGHT	BIT(0)
#define QUIRK_LITTLE_ENDIAN	BIT(1)
#define QUIRK_LSW32_IS_FIRST	BIT(2)

int pack(void *pbuf, u64 uval, size_t startbit, size_t endbit, size_t pbuflen,
	 u8 quirks);

int unpack(const void *pbuf, u64 *uval, size_t startbit, size_t endbit,
	   size_t pbuflen, u8 quirks);

void __pack(void *pbuf, u64 uval, size_t startbit, size_t endbit,
	    size_t pbuflen, u8 quirks);

void __unpack(const void *pbuf, u64 *uval, size_t startbit, size_t endbit,
	      size_t pbuflen, u8 quirks);

#define GEN_PACKED_FIELD_MEMBERS(__type) \
	__type startbit; \
	__type endbit; \
	__type offset; \
	__type size;

/* Use with bit offsets < 256, buffers < 32B and unpacked structures < 256B */
struct packed_field_8 {
	GEN_PACKED_FIELD_MEMBERS(u8);
};

/* Use with bit offsets < 65536, buffers < 8KB and unpacked structures < 64KB */
struct packed_field_16 {
	GEN_PACKED_FIELD_MEMBERS(u16);
};

struct packed_field {
	GEN_PACKED_FIELD_MEMBERS(size_t);
};

#define PACKED_FIELD(start, end, struct_name, struct_field) \
	{ \
		(start), \
		(end), \
		offsetof(struct_name, struct_field), \
		sizeof_field(struct_name, struct_field), \
	}

#define CHECK_PACKED_FIELD(field, pbuflen) \
	({ typeof (field) __f = (field); typeof (pbuflen) __len = (pbuflen); \
	BUILD_BUG_ON(__f.startbit < __f.endbit); \
	BUILD_BUG_ON(__f.startbit >= BITS_PER_BYTE * __len); \
	BUILD_BUG_ON(__f.startbit - __f.endbit >= BITS_PER_BYTE * __f.size); \
	BUILD_BUG_ON(__f.size != 1 && __f.size != 2 && __f.size != 4 && __f.size != 8); })

#define CHECK_PACKED_FIELD_OVERLAP(field1, field2) \
	({ typeof (field1) _f1 = (field1); typeof (field2) _f2 = (field2); \
	BUILD_BUG_ON(max(_f1.endbit, _f2.endbit) <=  min(_f1.startbit, _f2.startbit)); })

#include <generated/packing-checks.h>

void pack_fields(void *pbuf, size_t pbuflen, const void *ustruct,
		 const struct packed_field *fields, size_t num_fields,
		 u8 quirks);

void unpack_fields(const void *pbuf, size_t pbuflen, void *ustruct,
		   const struct packed_field *fields, size_t num_fields,
		   u8 quirks);

#define pack_fields_m(pbuf, pbuflen, ustruct, fields, quirks)			\
	({									\
		size_t num_fields = ARRAY_SIZE(fields);				\
										\
		for (size_t i = 0; i < num_fields; i++) {			\
			typeof ((fields)[0]) *field = &(fields)[i];		\
			u64 uval;						\
										\
			switch (field->size) {					\
			case 1:							\
				uval = *((u8 *)((u8 *)ustruct + field->offset)); \
				break;						\
			case 2:							\
				uval = *((u16 *)((u8 *)ustruct + field->offset)); \
				break;						\
			case 4:							\
				uval = *((u32 *)((u8 *)ustruct + field->offset)); \
				break;						\
			default:						\
				uval = *((u64 *)((u8 *)ustruct + field->offset)); \
				break;						\
			}							\
										\
			__pack(pbuf, uval, field->startbit, field->endbit,	\
			       pbuflen, quirks);				\
		}								\
	})

#define unpack_fields_m(pbuf, pbuflen, ustruct, fields, quirks)			\
	({									\
		size_t num_fields = ARRAY_SIZE(fields);				\
										\
		for (size_t i = 0; i < num_fields; i++) {			\
			typeof ((fields)[0]) *field = &fields[i];		\
			u64 uval;						\
										\
			__unpack(pbuf, &uval, field->startbit, field->endbit,	\
				 pbuflen, quirks);				\
										\
			switch (field->size) {					\
			case 1:							\
				*((u8 *)((u8 *)ustruct + field->offset)) = uval; \
				break;						\
			case 2:							\
				*((u16 *)((u8 *)ustruct + field->offset)) = uval; \
				break;						\
			case 4:							\
				*((u32 *)((u8 *)ustruct + field->offset)) = uval; \
				break;						\
			default:						\
				*((u64 *)((u8 *)ustruct + field->offset)) = uval; \
				break;						\
			}							\
		}								\
	})

#endif
