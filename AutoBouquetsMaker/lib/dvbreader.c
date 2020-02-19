#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>

#include <Python.h>

#define HAVE_ALIAUI
#ifdef HAVE_ALIAUI
#include <aui_tsi.h>
#include <aui_nim.h>
#include <ali_basic_common.h>
#include <ali_dmx_common.h>
#include <ali_avsync_common.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdbool.h>

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
static const uint32_t crc32_table[256] = {
	0, 0x4C11DB7, 0x9823B6E, 0xD4326D9,
	0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
	0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
	0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD,
	0x4C11DB70, 0x48D0C6C7, 0x4593E01E, 0x4152FDA9,
	0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75,
	0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011,
	0x791D4014, 0x7DDC5DA3, 0x709F7B7A, 0x745E66CD,
	0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039,
	0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5,
	0xBE2B5B58, 0xBAEA46EF, 0xB7A96036, 0xB3687D81,
	0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D,
	0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49,
	0xC7361B4C, 0xC3F706FB, 0xCEB42022, 0xCA753D95,
	0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1,
	0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D,
	0x34867077, 0x30476DC0, 0x3D044B19, 0x39C556AE,
	0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072,
	0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16,
	0x18AEB13, 0x54BF6A4, 0x808D07D, 0xCC9CDCA,
	0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE,
	0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02,
	0x5E9F46BF, 0x5A5E5B08, 0x571D7DD1, 0x53DC6066,
	0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA,
	0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E,
	0xBFA1B04B, 0xBB60ADFC, 0xB6238B25, 0xB2E29692,
	0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6,
	0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A,
	0xE0B41DE7, 0xE4750050, 0xE9362689, 0xEDF73B3E,
	0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2,
	0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686,
	0xD5B88683, 0xD1799B34, 0xDC3ABDED, 0xD8FBA05A,
	0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637,
	0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB,
	0x4F040D56, 0x4BC510E1, 0x46863638, 0x42472B8F,
	0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53,
	0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47,
	0x36194D42, 0x32D850F5, 0x3F9B762C, 0x3B5A6B9B,
	0x315D626, 0x7D4CB91, 0xA97ED48, 0xE56F0FF,
	0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623,
	0xF12F560E, 0xF5EE4BB9, 0xF8AD6D60, 0xFC6C70D7,
	0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B,
	0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F,
	0xC423CD6A, 0xC0E2D0DD, 0xCDA1F604, 0xC960EBB3,
	0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7,
	0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B,
	0x9B3660C6, 0x9FF77D71, 0x92B45BA8, 0x9675461F,
	0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3,
	0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640,
	0x4E8EE645, 0x4A4FFBF2, 0x470CDD2B, 0x43CDC09C,
	0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8,
	0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24,
	0x119B4BE9, 0x155A565E, 0x18197087, 0x1CD86D30,
	0x29F3D35, 0x65E2082, 0xB1D065B, 0xFDC1BEC,
	0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088,
	0x2497D08D, 0x2056CD3A, 0x2D15EBE3, 0x29D4F654,
	0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0,
	0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C,
	0xE3A1CBC1, 0xE760D676, 0xEA23F0AF, 0xEEE2ED18,
	0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4,
	0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0,
	0x9ABC8BD5, 0x9E7D9662, 0x933EB0BB, 0x97FFAD0C,
	0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668,
	0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4};

static inline uint32_t
crc32(uint32_t val, const void *ss, int len)
{
	const unsigned char *s =(const unsigned char *) ss;
        while (--len >= 0)
//                val = crc32_table[(val ^ *s++) & 0xff] ^ (val >> 8);
                val = (val << 8) ^ crc32_table[(val >> 24) ^ *s++];
        return val;
}

#define DMX_FILTER_SIZE   16
struct ali_dmx_flt {
	uint8_t filter_data [DMX_FILTER_SIZE];
	uint8_t filter_mask [DMX_FILTER_SIZE];
	uint8_t filter_mode [DMX_FILTER_SIZE];
	int timeout;
	int is_oneshot;
};

static bool section_hit(uint8_t *filter_data, uint8_t *filter_mask, uint8_t *filter_mode, uint8_t *buf)
{
	int i;
	uint8_t filt, mask, mode, mode_r, value;
	bool res1, res2, first;

	/*
	 * res1 is the check result of mode bits equal 1
	 * res2 is the check result of mode bits equal 0
	 * first is the check result of "is there any mode bits equal 0?"
	 */
	res1 = true;
	res2 = true;
	first = true;

	for (i = 0; i < DMX_FILTER_SIZE; i++) {
		filt  = filter_data[i];
		mask  = filter_mask[i];
		mode  = filter_mode[i];
		mode_r = ~mode;
		if (i == 0)
			value = buf[i];
		else
			value = buf[i + 2];

		mode  &= mask;
		mode_r &= mask;

		/*
		 * for mode bits equal 1, all bits need to equal,
		 * else section hit fail
		 */
		if ((mode & value) != (mode & filt))
			res1 = false;

		/*
		 * This condition tells us there is at least one mode bit equel 0
		 * else no mode bit equal 0, res2 must be "true"
		 */
		if (mode_r && first) {
			res2 = false;
			first = false;
		}

		/*
		 * for mode bits equal 0, there should be at least one bit un-equal
		 * else section hit fail unless no mode bit equal 0
		 */
		if ((mode_r & value) != (mode_r & filt))
			res2 = true;
	}

	return (res1 && res2);
}

ssize_t ali_read(int fd, void *buf, size_t count, struct ali_dmx_flt *flt)
{
	int r;
	unsigned int c;
	struct timeval start, now;
	int timeout;

	gettimeofday(&start, 0);
	for (;;) {
		gettimeofday(&now, 0);
		timeout = (now.tv_sec - start.tv_sec) * 1000;
		timeout += ((now.tv_usec - start.tv_usec) / 1000);

		if (timeout >= flt->timeout)
			break;

		r = read(fd, buf, count);
		if(r < 0) {
			usleep(1000);
			continue;
		}
		if (!section_hit(flt->filter_data, flt->filter_mask, flt->filter_mode, (uint8_t *)buf))
			continue;

		if ((c = crc32((unsigned)-1, buf, r)))
			continue;

		return r;
	}

	return -1;
}

struct ali_dmx_flt g_ali_filter;

struct ali_dmx_dev {
	int dmx;
	const char *dev;
	const char *feeddev;
};

#define ALI_DMX_SEE_DEV "/dev/ali_m36_dmx_see_0"
static struct ali_dmx_dev dmxdev[] = {
	{0, "/dev/ali_m36_dmx_0", NULL},
	{1, "/dev/ali_m36_dmx_1", NULL},
	{2, "/dev/ali_m36_dmx_2", NULL},
	{3, "/dev/ali_m36_dmx_3", NULL},
	{4, "/dev/ali_m36_dmx_3", NULL},
#define ALI_SW_DMX_IDX 5
	{5, "/dev/ali_dmx_pb_0_out", "/dev/ali_dmx_pb_0_in"},
};

#endif




/*
	DMX_SET_SOURCE no longer exists. For more info check the following:
	https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/include/uapi/linux/dvb/dmx.h?h=v4.17&id=13adefbe9e566c6db91579e4ce17f1e5193d6f2c
*/
#ifndef DMX_SET_SOURCE
typedef enum dmx_source {
	DMX_SOURCE_FRONT0 = 0,
	DMX_SOURCE_FRONT1,
	DMX_SOURCE_FRONT2,
	DMX_SOURCE_FRONT3,
	DMX_SOURCE_DVR0   = 16,
	DMX_SOURCE_DVR1,
	DMX_SOURCE_DVR2,
	DMX_SOURCE_DVR3
} dmx_source_t;
#define DMX_SET_SOURCE	_IOW('o', 49, dmx_source_t)
#endif

PyObject *ss_open(PyObject *self, PyObject *args) {
	int fd, pid;
	const char *demuxer;
	char filter, mask, frontend;
	struct dmx_sct_filter_params sfilter;
	dmx_source_t ssource = DMX_SOURCE_FRONT0;

	if (!PyArg_ParseTuple(args, "sibbb", &demuxer, &pid, &filter, &mask, &frontend))
		return Py_BuildValue("i", -1);

	memset(&sfilter, 0, sizeof(sfilter));
	sfilter.pid = pid & 0xffff;
	sfilter.filter.filter[0] = filter & 0xff;
	sfilter.filter.mask[0] = mask & 0xff;
	sfilter.timeout = 0;
	sfilter.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

	ssource = DMX_SOURCE_FRONT0 + frontend;

#ifndef HAVE_ALIAUI
	if ((fd = open(demuxer, O_RDWR | O_NONBLOCK)) < 0) {
		printf("Cannot open demuxer '%s'", demuxer);
		return Py_BuildValue("i", -1);
	}

	if (ioctl(fd, DMX_SET_SOURCE, &ssource) == -1) {
		printf("ioctl DMX_SET_SOURCE failed");
		close(fd);
		return Py_BuildValue("i", -1);
	}

	if (ioctl(fd, DMX_SET_FILTER, &sfilter) == -1) {
		printf("ioctl DMX_SET_FILTER failed");
		close(fd);
		return Py_BuildValue("i", -1);
	}
#else
	char *str_dmx;
	int dvb_demux = 0;
	str_dmx = demuxer + strlen(demuxer) -1;
	dvb_demux = atoi(str_dmx);

	if ((fd = open(dmxdev[dvb_demux].dev, O_RDONLY | O_NONBLOCK)) < 0) {
		printf("Cannot open demuxer '%d'", dvb_demux);
		return Py_BuildValue("i", -1);
	}

	aui_attr_tsi attr_tsi;
	enum aui_tsi_input_id input_src;
	unsigned long init_param;
	enum aui_tsi_output_id tsi_output_id;
	enum aui_tsi_channel_id tsi_channel_id;
	int fenum, demux, is_sourcepvr;
	aui_hdl hdl_tsi = NULL;

	fenum = ssource;
	demux = dvb_demux;
	is_sourcepvr = 0;

	memset(&attr_tsi, 0, sizeof(aui_attr_tsi));

	if (fenum == 0) {
	    init_param =  AUI_TSI_IN_CONF_SPI_ENABLE
		| AUI_TSI_IN_CONF_SSI_BIT_ORDER
		| AUI_TSI_IN_CONF_SYNC_SIG_POL
		| AUI_TSI_IN_CONF_VALID_SIG_POL;
	    input_src = AUI_TSI_INPUT_SPI_3;
	    printf("[eDVBDemux] fenum 0 \n");
	} else if (fenum == 1) {
	    init_param = AUI_TSI_IN_CONF_SSI2B_ENABLE
		| AUI_TSI_IN_CONF_SSI_CLOCK_POL
		| AUI_TSI_IN_CONF_SSI_BIT_ORDER
		| AUI_TSI_IN_CONF_ERR_SIG_POL
		| AUI_TSI_IN_CONF_SYNC_SIG_POL
		| AUI_TSI_IN_CONF_VALID_SIG_POL;
	    input_src = AUI_TSI_INPUT_SSI2B_3;
	    printf("[eDVBDemux] fenum 1 \n");
	}

	if (demux == 0)
	{
	    tsi_output_id = AUI_TSI_OUTPUT_DMX_0;
	    tsi_channel_id = AUI_TSI_CHANNEL_0;
	}
	else if (demux == 1)
	{
	    tsi_output_id = AUI_TSI_OUTPUT_DMX_1;
	    tsi_channel_id = AUI_TSI_CHANNEL_1;
	}
	else if (demux == 2)
	{
	    tsi_output_id = AUI_TSI_OUTPUT_DMX_2;
	    tsi_channel_id = AUI_TSI_CHANNEL_2;
	}
	else if (demux == 3)
	{
	    tsi_output_id = AUI_TSI_OUTPUT_DMX_3;
	    tsi_channel_id = AUI_TSI_CHANNEL_3;
	}
	else if (demux == 4)
	{
	    tsi_output_id = AUI_TSI_OUTPUT_DMX_3;
	    tsi_channel_id = AUI_TSI_CHANNEL_3;
	}

	attr_tsi.ul_init_param = init_param;
	if (aui_find_dev_by_idx(AUI_MODULE_TSI, 0, &hdl_tsi))
	{
	    if (aui_tsi_open(&hdl_tsi))
	    {
		printf("[eDVBDemux] aui_tsi_open error \n");
		return Py_BuildValue("i", -1);
	    }
	}

	if (aui_tsi_src_init(hdl_tsi, input_src, &attr_tsi)) {
	    printf("[eDVBDemux] aui_tsi_src_init error \n");
	    return Py_BuildValue("i", -1);
	}

	if (aui_tsi_route_cfg(hdl_tsi, input_src, tsi_channel_id, tsi_output_id)) {
	    printf("[eDVBDemux] aui_tsi_route_cfg error \n");
	    return Py_BuildValue("i", -1);
	}
	printf("[eDVBDemux] DMX_SET_SOURCE Frontend%d success \n", fenum);

	struct dmx_channel_param param;
	memset(&param, 0, sizeof(param));
	param.output_space = DMX_OUTPUT_SPACE_USER;
	param.output_format = DMX_CHANNEL_OUTPUT_FORMAT_SEC;
	param.sec_param.pid = pid & 0xffff;
	param.sec_param.mask_len = 1;
	param.sec_param.mask[0] = 0;
	param.sec_param.value[0] = 0;
	param.sec_param.timeout = 5000;
	param.sec_param.option = 0;
	param.sec_param.needdiscramble = 0; /* Clear stream */

	memset(&g_ali_filter, 0, sizeof(g_ali_filter));
	g_ali_filter.filter_data[0] = filter & 0xff;;
	g_ali_filter.filter_mask[0] = mask & 0xff;
	g_ali_filter.timeout = 0;
	g_ali_filter.is_oneshot = 1;
	for (int i = 0; i < DMX_FILTER_SIZE; i++) {
		g_ali_filter.filter_mode[i] = ~(g_ali_filter.filter_mode[i]);
	}

	ioctl(fd, ALI_DMX_CHANNEL_START, &param);
#endif

	return Py_BuildValue("i", fd);
}

PyObject *ss_close(PyObject *self, PyObject *args) {
	int fd;
	if (PyArg_ParseTuple(args, "i", &fd))
		close(fd);

	return Py_None;
}

PyObject *ss_parse_bat(unsigned char *data, int length) {
	PyObject* list = PyList_New(0);

	int bouquet_id = (data[3] << 8) | data[4];
	int bouquet_descriptors_length = ((data[8] & 0x0f) << 8) | data[9];
	int transport_stream_loop_length = ((data[bouquet_descriptors_length + 10] & 0x0f) << 8) | data[bouquet_descriptors_length + 11];
	int offset1 = 10;

	while (bouquet_descriptors_length > 0)
	{
		unsigned char descriptor_tag = data[offset1];
		unsigned char descriptor_length = data[offset1 + 1];
		int offset2 = offset1 + 2;

		if (descriptor_tag == 0xd4) // Freesat regions, for freesat regions extractor
		{
			int size = descriptor_length;
			while (size > 0)
			{
				char lang[4];
				char description[256];
				memset(lang, '\0', 4);
				memset(description, '\0', 256);

				int region_id = (data[offset2] << 8) | data[offset2 + 1];
				memcpy(lang, data + offset2 + 2, 3);
				unsigned char description_size = data[offset2 + 5];
				if (description_size == 255)
					description_size--;
				memcpy(description, data + offset2 + 6, description_size);

				PyObject *item = Py_BuildValue("{s:i,s:i,s:s,s:s}",
							"descriptor_tag", descriptor_tag,
							"region_id", region_id,
							"language", lang,
							"description", description);

				PyList_Append(list, item);
				Py_DECREF(item);

				offset2 += (6 + description_size) ;
				size -= (6 + description_size);
			}
		}
		else if (descriptor_tag == 0xd5) // Freesat, links channel ID to category description
		{
			int size = descriptor_length;
			while (size > 2)
			{
				unsigned short int category_group = data[offset2];
				unsigned short int category_id = data[offset2 + 1];
				short int size2 = data[offset2 + 2];

				offset2 += 3;
				size -= 3;
				while (size2 > 1)
				{
					unsigned short int channel_id = ((data[offset2] << 8) | data[offset2 + 1]) & 0x0fff;

					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i}",
							"descriptor_tag", descriptor_tag,
							"category_group", category_group,
							"category_id", category_id,
							"channel_id", channel_id);

					PyList_Append(list, item);
					Py_DECREF(item);

					offset2 += 2;
					size2 -= 2;
					size -= 2;
				}
			}
		}
		else if ((descriptor_tag == 0xd8)) // Freesat category description
		{
			int size = descriptor_length;
			while (size > 0)
			{
				char description[256];
				memset(description, '\0', 256);

				unsigned short int category_group = data[offset2];
				unsigned short int category_id = data[offset2 + 1];
				unsigned char description_size = data[offset2 + 6];
				if (description_size == 255)
					description_size--;

				memcpy(description, data + offset2 + 7, description_size);

				PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:s}",
						"descriptor_tag", descriptor_tag,
						"category_group", category_group,
						"category_id", category_id,
						"description", description);

				PyList_Append(list, item);
				Py_DECREF(item);

				offset2 += (description_size + 7);
				size -= (description_size + 7);
			}
		}
		else if (descriptor_tag == 0x47) // Bouquet name descriptor
		{
			char description[descriptor_length + 1];
			memset(description, '\0', descriptor_length + 1);
			memcpy(description, data + offset1 + 2, descriptor_length);
			char *description_ptr = description;
			if (strlen(description) == 0)
				strcpy(description, "Unknown");
			else if (description[0] == 0x05)
				description_ptr++;

			PyObject *item = Py_BuildValue("{s:i,s:i,s:s}",
						"descriptor_tag", descriptor_tag,
						"bouquet_id", bouquet_id,
						"description", description_ptr);

			PyList_Append(list, item);
			Py_DECREF(item);
		}
		else if (descriptor_tag == 0x83)	// LCN descriptor (Fransat, 5W)
		{
			int size = descriptor_length;
			while (size > 0)
			{
				int original_network_id = (data[offset2] << 8) | data[offset2 + 1];
				int transport_stream_id = (data[offset2 + 2] << 8) | data[offset2 + 3];
				int service_id = (data[offset2 + 4] << 8) | data[offset2 + 5];
				int logical_channel_number = (data[offset2 + 6] << 4) | (data[offset2 + 7] >> 4);

				PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i}",
						"bouquet_id", bouquet_id,
						"original_network_id", original_network_id,
						"transport_stream_id", transport_stream_id,
						"service_id", service_id,
						"logical_channel_number", logical_channel_number,
						"descriptor_tag", descriptor_tag);

				PyList_Append(list, item);
				Py_DECREF(item);

				offset2 += 8;
				size -= 8;
			}
		}
		else  // unknown descriptors
		{
			char description[2 * descriptor_length + 5];
			memset(description, '\0', 2 * descriptor_length + 5);
			int length = descriptor_length + 2;
			int i = 0, j = 0;
			while (length > 0)
			{
				int decimalNumber = data[offset2 + i - 2];
				int quotient, n=0, temp;
				char hextemp[3] = {'0','0','\0'};
				quotient = decimalNumber;
				while(quotient!=0)
				{
					temp = quotient % 16;
					if (temp < 10)
						temp = temp + 48;
					else
						temp = temp + 55;
					hextemp[n]= temp;
					n += 1;
					quotient = quotient / 16;
				}
				//swap result
				description[j] = hextemp[1];
				j += 1;
				description[j] = hextemp[0];
				j += 1;
				i += 1;
				length -= 1;
			}
			if (strlen(description) == 0)
				strcpy(description, "Empty");

			PyObject *item = Py_BuildValue("{s:i,s:i,s:s}",
						"descriptor_tag", descriptor_tag,
						"descriptor_length", descriptor_length,
						"hexcontent", description);

			PyList_Append(list, item);
			Py_DECREF(item);
		}

		offset1 += (descriptor_length + 2);
		bouquet_descriptors_length -= (descriptor_length + 2);
	}

	offset1 += 2;

	while (transport_stream_loop_length > 0)
	{
		int transport_stream_id = (data[offset1] << 8) | data[offset1 + 1];
		int original_network_id = (data[offset1 + 2] << 8) | data[offset1 + 3];
		int transport_descriptor_length = ((data[offset1 + 4] & 0x0f) << 8) | data[offset1 + 5];
		int offset2 = offset1 + 6;

		offset1 += (transport_descriptor_length + 6);
		transport_stream_loop_length -= (transport_descriptor_length + 6);

		while (transport_descriptor_length > 0)
		{
			unsigned char descriptor_tag = data[offset2];
			unsigned char descriptor_length = data[offset2 + 1];
			int offset3 = offset2 + 2;

			offset2 += (descriptor_length + 2);
			transport_descriptor_length -= (descriptor_length + 2);

			if (descriptor_tag == 0xb1) // User defined Sky
			{
				unsigned char region_id;
				region_id = data[offset3 + 1];

				offset3 += 2;
				descriptor_length -= 2;
				while (descriptor_length > 0)
				{
					unsigned short int channel_id;
					unsigned short int sky_id;
					unsigned short int service_id;
					unsigned char service_type;

					channel_id = (data[offset3 + 3] << 8) | data[offset3 + 4];
					sky_id = ( data[offset3 + 5] << 8 ) | data[offset3 + 6];
					service_id = (data[offset3] << 8) | data[offset3 + 1];
					service_type = data[offset3 + 2];

					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
							"descriptor_tag", descriptor_tag,
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"service_id", service_id, "number", sky_id,
							"service_type", service_type, "region_id", region_id,
							"channel_id", channel_id);

					PyList_Append(list, item);
					Py_DECREF(item);

					offset3 += 9;
					descriptor_length -= 9;
				}
			}
			else if (descriptor_tag == 0x41) // Service list descriptor
			{
				while (descriptor_length > 0)
				{
					unsigned short int service_id;
					unsigned char service_type;
					service_id = (data[offset3] << 8) | data[offset3 + 1];
					service_type = data[offset3 + 2];

					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i}",
							"descriptor_tag", descriptor_tag,
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"service_id", service_id, "service_type", service_type);

					PyList_Append(list, item);
					Py_DECREF(item);

					descriptor_length -= 3;
				}
			}
			else if (descriptor_tag == 0x81)	// LCN descriptor (UPC, 0.8W)
			{
				while (descriptor_length > 0)
				{
					int service_id = (data[offset3] << 8) | data[offset3 + 1];
					int logical_channel_number = (data[offset3 + 2] << 8) | data[offset3 + 3];

					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i}",
							"bouquet_id", bouquet_id,
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"service_id", service_id,
							"logical_channel_number", logical_channel_number,
							"descriptor_tag", descriptor_tag);

					PyList_Append(list, item);
					Py_DECREF(item);

					offset3 += 4;
					descriptor_length -= 4;
				}
			}
			else if (descriptor_tag == 0x83)	// LCN descriptor (NC+, 13E)
			{
				while (descriptor_length > 0)
				{
					int service_id = (data[offset3] << 8) | data[offset3 + 1];
					int visible_service_flag = (data[offset3 + 2] >> 7) & 0x01;
					int logical_channel_number = ((data[offset3 + 2] & 0x03) << 8) | data[offset3 + 3];

					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
							"bouquet_id", bouquet_id,
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"service_id", service_id,
							"visible_service_flag", visible_service_flag,
							"logical_channel_number", logical_channel_number,
							"descriptor_tag", descriptor_tag);

					PyList_Append(list, item);
					Py_DECREF(item);

					offset3 += 4;
					descriptor_length -= 4;
				}
			}
			else if (descriptor_tag == 0x86)	// LCN descriptor (DIGI 0.8W)
			{
				while (descriptor_length > 0)
				{
					int service_id = (data[offset3] << 8) | data[offset3 + 1];
					int logical_channel_number = (data[offset3 + 2] << 8) | data[offset3 + 3];

					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i}",
							"bouquet_id", bouquet_id,
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"service_id", service_id,
							"logical_channel_number", logical_channel_number,
							"descriptor_tag", descriptor_tag);

					PyList_Append(list, item);
					Py_DECREF(item);

					offset3 += 4;
					descriptor_length -= 4;
				}
			}
			else if (descriptor_tag == 0x93)	// LCN descriptor (NOVA, 13E)
			{
				while (descriptor_length > 0)
				{
					int service_id = (data[offset3] << 8) | data[offset3 + 1];
					int logical_channel_number = (data[offset3 + 2] << 8) | data[offset3 + 3];

					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i}",
							"bouquet_id", bouquet_id,
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"service_id", service_id,
							"logical_channel_number", logical_channel_number,
							"descriptor_tag", descriptor_tag);

					PyList_Append(list, item);
					Py_DECREF(item);

					offset3 += 4;
					descriptor_length -= 4;
				}
			}
			else if (descriptor_tag == 0xd0) // User defined UPC RoI cable LCN
			{
				// skip 2 bytes
				offset3 += 2;
				descriptor_length -= 2;
				while (descriptor_length > 0)
				{
					int service_id = (data[offset3] << 8) | data[offset3 + 1];
					int logical_channel_number = (data[offset3 + 7] << 8) | data[offset3 + 8];

					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i}",
							"bouquet_id", bouquet_id,
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"service_id", service_id,
							"logical_channel_number", logical_channel_number,
							"descriptor_tag", descriptor_tag);

					PyList_Append(list, item);
					Py_DECREF(item);

					offset3 += 9;
					descriptor_length -= 9;
				}
			}
			else if (descriptor_tag == 0xd3) // Freesat main descriptor
			{
				while (descriptor_length > 0)
				{
					unsigned short int service_id;
					unsigned short int channel_id;
					unsigned char size;

					service_id = (data[offset3] << 8) | data[offset3 + 1];
					channel_id = (data[offset3 + 2] << 8) | data[offset3 + 3];
					size = data[offset3 + 4];

					offset3 += 5;
					descriptor_length -= 5;
					while (size > 0)
					{
						unsigned short int region_id;
						unsigned short int channel_number;
						channel_number = ((data[offset3] << 8) | data[offset3 + 1]) & 0x0fff;
						region_id = (data[offset3 + 2] << 8) | data[offset3 + 3];

						PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
								"descriptor_tag", descriptor_tag,
								"transport_stream_id", transport_stream_id,
								"original_network_id", original_network_id,
								"service_id", service_id, "number", channel_number,
								"region_id", region_id, "channel_id", channel_id);

						PyList_Append(list, item);
						Py_DECREF(item);

						offset3 += 4;
						size -= 4;
						descriptor_length -= 4;
					}
				}
			}
			else if (descriptor_tag == 0xe2) // LCN descriptor (Viasat, 4.8E)
			{
				while (descriptor_length > 0)
				{
					int service_id = (data[offset3] << 8) | data[offset3 + 1];
					int logical_channel_number = ((data[offset3 + 2] & 0x03) << 8) | data[offset3 + 3];

					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i}",
							"descriptor_tag", descriptor_tag,
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"bouquet_id", bouquet_id,
							"service_id", service_id,
							"logical_channel_number", logical_channel_number);

					PyList_Append(list, item);
					Py_DECREF(item);

					offset3 += 4;
					descriptor_length -= 4;
				}
			}
			else  // unknown descriptors
			{
				char description[2 * descriptor_length + 5];
				memset(description, '\0', 2 * descriptor_length + 5);
				int length = descriptor_length + 2;
				int i = 0, j = 0;
				while (length > 0)
				{
					int decimalNumber = data[offset3 + i - 2];
					int quotient, n=0, temp;
					char hextemp[3] = {'0','0','\0'};
					quotient = decimalNumber;
					while(quotient!=0)
					{
						temp = quotient % 16;
						if (temp < 10)
							temp = temp + 48;
						else
							temp = temp + 55;
						hextemp[n]= temp;
						n += 1;
						quotient = quotient / 16;
					}
					description[j] = hextemp[1];
					j += 1;
					description[j] = hextemp[0];
					j += 1;
					i += 1;
					length -= 1;
				}
				if (strlen(description) == 0)
					strcpy(description, "Empty");

				PyObject *item = Py_BuildValue("{s:i,s:i,s:s}",
							"descriptor_tag", descriptor_tag,
							"descriptor_length", descriptor_length,
							"hexcontent", description);

				PyList_Append(list, item);
				Py_DECREF(item);
			}
		}
	}

	return list;
}

PyObject *ss_parse_nit(unsigned char *data, int length) {
	PyObject* list = PyList_New(0);

	int network_descriptors_length = ((data[8] & 0x0f) << 8) | data[9];
	int transport_stream_loop_length = ((data[network_descriptors_length + 10] & 0x0f) << 8) | data[network_descriptors_length + 11];
	int offset1 = network_descriptors_length + 12;

	while (transport_stream_loop_length > 0)
	{
		int transport_stream_id = (data[offset1] << 8) | data[offset1 + 1];
		int original_network_id = (data[offset1 + 2] << 8) | data[offset1 + 3];
		int transport_descriptor_length = ((data[offset1 + 4] & 0x0f) << 8) | data[offset1 + 5];
		int offset2 = offset1 + 6;

		offset1 += (transport_descriptor_length + 6);
		transport_stream_loop_length -= (transport_descriptor_length + 6);

		while (transport_descriptor_length > 0)
		{
			unsigned char descriptor_tag = data[offset2];
			unsigned char descriptor_length = data[offset2 + 1];

			if (descriptor_tag == 0x43)	// Satellite delivery system descriptor
			{
				int frequency = (data[offset2 + 2] >> 4) * 10000000;
				frequency += (data[offset2 + 2] & 0x0f) * 1000000;
				frequency += (data[offset2 + 3] >> 4) * 100000;
				frequency += (data[offset2 + 3] & 0x0f) * 10000;
				frequency += (data[offset2 + 4] >> 4) * 1000;
				frequency += (data[offset2 + 4] & 0x0f) * 100;
				frequency += (data[offset2 + 5] >> 4) * 10;
				frequency += data[offset2 + 5] & 0x0f;

				int orbital_position = (data[offset2 + 6] << 8) | data[offset2 + 7];
				int west_east_flag = (data[offset2 + 8] >> 7) & 0x01;
				int polarization = (data[offset2 + 8] >> 5) & 0x03;
				int roll_off = (data[offset2 + 8] >> 3) & 0x03;
				int modulation_system = (data[offset2 + 8] >> 2) & 0x01;
				int modulation_type = data[offset2 + 8] & 0x03;

				int symbol_rate = (data[offset2 + 9] >> 4) * 1000000;
				symbol_rate += (data[offset2 + 9] & 0xf) * 100000;
				symbol_rate += (data[offset2 + 10] >> 4) * 10000;
				symbol_rate += (data[offset2 + 10] & 0xf) * 1000;
				symbol_rate += (data[offset2 + 11] >> 4) * 100;
				symbol_rate += (data[offset2 + 11] & 0xf) * 10;
				symbol_rate += data[offset2 + 11] >> 4;

				int fec_inner = data[offset2 + 12] & 0xf;

				PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
						"transport_stream_id", transport_stream_id,
						"original_network_id", original_network_id,
						"frequency", frequency,
						"orbital_position", orbital_position,
						"west_east_flag", west_east_flag,
						"polarization", polarization,
						"roll_off", roll_off,
						"modulation_system", modulation_system,
						"modulation_type", modulation_type,
						"symbol_rate", symbol_rate,
						"fec_inner", fec_inner,
						"descriptor_tag", descriptor_tag);

				PyList_Append(list, item);
				Py_DECREF(item);
			}
			else if (descriptor_tag == 0x44)	// Cable delivery system descriptor
			{
				int frequency = (data[offset2 + 2] >> 4) * 10000000;
				frequency += (data[offset2 + 2] & 0x0f) * 1000000;
				frequency += (data[offset2 + 3] >> 4) * 100000;
				frequency += (data[offset2 + 3] & 0x0f) * 10000;
				frequency += (data[offset2 + 4] >> 4) * 1000;
				frequency += (data[offset2 + 4] & 0x0f) * 100;
				frequency += (data[offset2 + 5] >> 4) * 10;
				frequency += data[offset2 + 5] & 0x0f;

				int fec_outer = data[offset2 + 7] & 0xf;
				int modulation_type = data[offset2 + 8];

				int symbol_rate = (data[offset2 + 9] >> 4) * 1000000;
				symbol_rate += (data[offset2 + 9] & 0xf) * 100000;
				symbol_rate += (data[offset2 + 10] >> 4) * 10000;
				symbol_rate += (data[offset2 + 10] & 0xf) * 1000;
				symbol_rate += (data[offset2 + 11] >> 4) * 100;
				symbol_rate += (data[offset2 + 11] & 0xf) * 10;
				symbol_rate += data[offset2 + 12] >> 4;

				int fec_inner = data[offset2 + 12] & 0xf;

				PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
						"transport_stream_id", transport_stream_id,
						"original_network_id", original_network_id,
						"frequency", frequency,
						"fec_outer", fec_outer,
						"modulation_type", modulation_type,
						"symbol_rate", symbol_rate,
						"fec_inner", fec_inner,
						"descriptor_tag", descriptor_tag);

				PyList_Append(list, item);
				Py_DECREF(item);
			}
			else if (descriptor_tag == 0x5A)	// Terrestrial delivery system descriptor
			{
				int frequency = ((data[offset2 + 2] << 24) | (data[offset2 + 3] << 16) | (data[offset2 + 4] << 8) | (data[offset2 + 5]));

				int bandwidth = (data[offset2 + 6] >> 5 & 0x07);
				int priority = (data[offset2 + 6] >> 4 & 0x01);
				int time_slicing = (data[offset2 + 6] >> 3 & 0x01);
				int mpe_fec = (data[offset2 + 6] >> 2 & 0x01);

				int modulation = (data[offset2 + 7] >> 6 & 0x03);
				int hierarchy = (data[offset2 + 7] >> 3 & 0x07);
				int code_rate_hp = (data[offset2 + 7] & 0x07);

				int code_rate_lp = (data[offset2 + 8] >> 5 & 0x07);
				int guard_interval = (data[offset2 + 8] >> 3 & 0x03);
				int transmission_mode = (data[offset2 + 8] >> 1 & 0x03);
				int other_frequency_flag = (data[offset2 + 8] & 0x01);

				PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
						"transport_stream_id", transport_stream_id,
						"original_network_id", original_network_id,
						"frequency", frequency,
						"bandwidth", bandwidth,
						"priority", priority,
						"time_slicing", time_slicing,
						"mpe_fec", mpe_fec,
						"modulation", modulation,
						"hierarchy", hierarchy,
						"code_rate_hp", code_rate_hp,
						"code_rate_lp", code_rate_lp,
						"guard_interval", guard_interval,
						"transmission_mode", transmission_mode,
						"other_frequency_flag", other_frequency_flag,
						"descriptor_tag", descriptor_tag);

				PyList_Append(list, item);
				Py_DECREF(item);
			}
			else if (descriptor_tag == 0x7f)	// DVB-T2 delivery system descriptor when descriptor_tag_extension == 4
			{
				unsigned char descriptor_tag_extension = data[offset2 + 2];
				if (descriptor_tag_extension == 0x04)
				{
					int system = 1;
					int inversion = 0;
					int plp_id = data[offset2 + 3];
					int T2_system_id = (data[offset2 + 4] << 8) | data[offset2 + 5];

					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:s,s:i,s:i,s:i}",
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"plp_id", plp_id,
							"T2_system_id", T2_system_id,
							"delivery_system_type", "DVB-T2",
							"system", system,
							"inversion", inversion,
							"descriptor_tag", descriptor_tag);

					PyList_Append(list, item);
					Py_DECREF(item);
				}

			}
			else if (descriptor_tag == 0x41)	// Service list descriptor
			{
				int offset3 = offset2 + 2;
				while (offset3 < (offset2 + descriptor_length + 2))
				{
					int service_id = (data[offset3] << 8) | data[offset3 + 1];
					int service_type = data[offset3 + 2];

					offset3 += 3;
					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i}",
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"service_id", service_id,
							"service_type", service_type,
							"descriptor_tag", descriptor_tag);

					PyList_Append(list, item);
					Py_DECREF(item);
				}
			}
			else if (descriptor_tag == 0x83)	// LCN descriptor
			{
				int offset3 = offset2 + 2;
				while (offset3 < (offset2 + descriptor_length + 2))
				{
					int service_id = (data[offset3] << 8) | data[offset3 + 1];
					int visible_service_flag = (data[offset3 + 2] >> 7) & 0x01;
					int logical_channel_number = ((data[offset3 + 2] & 0x03) << 8) | data[offset3 + 3];

					offset3 += 4;
					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i}",
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"service_id", service_id,
							"visible_service_flag", visible_service_flag,
							"logical_channel_number", logical_channel_number,
							"descriptor_tag", descriptor_tag);

					PyList_Append(list, item);
					Py_DECREF(item);
				}
			}
			else if (descriptor_tag == 0x87)	// LCN V2 descriptor (Canal Digital Nordic 0.8W)
			{
				int offset3 = offset2 + 2;
				int channel_list_id = data[offset3];
				int channel_list_name_length = data[offset3 + 1];

				char channel_list_name[channel_list_name_length + 1];
				memset(channel_list_name, '\0', channel_list_name_length + 1);
				memcpy(channel_list_name, data + offset3 + 2, channel_list_name_length);
				char *channel_list_name_ptr = channel_list_name;

				char country_code[3];
				memset(country_code, '\0', 3);
				memcpy(country_code, data + offset3 + 2 + channel_list_name_length, 3);
				char *country_code_ptr = country_code;

				int descriptor_length_2 = offset3 + 2 + channel_list_name_length + 3;
				int offset4 = offset3 + 2 + channel_list_name_length + 3 + 1;

				while (offset4 < (offset3 + descriptor_length_2 + 2))
				{
					int service_id = (data[offset4] << 8) | data[offset4 + 1];
					int visible_service_flag = (data[offset4 + 2] >> 7) & 0x01;
					int logical_channel_number = ((data[offset4 + 2] & 0x03) << 8) | data[offset4 + 3];

					offset4 += 4;
					PyObject *item = Py_BuildValue("{s:i,s:s,s:s,s:i,s:i,s:i,s:i,s:i,s:i}",
							"channel_list_id", channel_list_id,
							"channel_list_name", channel_list_name_ptr,
							"country_code", country_code_ptr,
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"service_id", service_id,
							"visible_service_flag", visible_service_flag,
							"logical_channel_number", logical_channel_number,
							"descriptor_tag", descriptor_tag);

					PyList_Append(list, item);
					Py_DECREF(item);
				}
			}
			else if (descriptor_tag == 0x88)	// HD simulcast LCN descriptor
			{
				int offset3 = offset2 + 2;
				while (offset3 < (offset2 + descriptor_length + 2))
				{
					int service_id = (data[offset3] << 8) | data[offset3 + 1];
					int visible_service_flag = (data[offset3 + 2] >> 7) & 0x01;
					int hd_logical_channel_number = ((data[offset3 + 2] & 0x03) << 8) | data[offset3 + 3];

					offset3 += 4;
					PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i}",
							"transport_stream_id", transport_stream_id,
							"original_network_id", original_network_id,
							"service_id", service_id,
							"visible_service_flag", visible_service_flag,
							"logical_channel_number", hd_logical_channel_number,
							"descriptor_tag", descriptor_tag);

					PyList_Append(list, item);
					Py_DECREF(item);
				}
			}

			offset2 += (descriptor_length + 2);
			transport_descriptor_length -= (descriptor_length + 2);
		}
	}

	return list;
}

PyObject *ss_parse_sdt(unsigned char *data, int length) {
	PyObject* list = PyList_New(0);

	int transport_stream_id = (data[3] << 8) | data[4];
	int original_network_id = (data[8] << 8) | data[9];
	int offset = 11;
	length -= 11;

	while (length >= 5)
	{
		int service_id = (data[offset] << 8) | data[offset + 1];
		int free_ca = (data[offset + 3] >> 4) & 0x01;
		int descriptors_loop_length = ((data[offset + 3] & 0x0f) << 8) | data[offset + 4];
		char service_name[256];
		char provider_name[256];
		int service_type = 0;
		int region_code = 0;
		int city_code = 0;
		int lcn_id = 0;
		int bouquets_id = 0;
		int service_group_id = 0;
		int category_id = 0;
		memset(service_name, '\0', 256);
		memset(provider_name, '\0', 256);

		length -= 5;
		offset += 5;

		int offset2 = offset;

		length -= descriptors_loop_length;
		offset += descriptors_loop_length;

		while (descriptors_loop_length >= 2)
		{
			int tag = data[offset2];
			int size = data[offset2 + 1];

			if (tag == 0x48)	// Service descriptor
			{
				service_type = data[offset2 + 2];
				int service_provider_name_length = data[offset2 + 3];
				if (service_provider_name_length == 255)
					service_provider_name_length--;

				int service_name_length = data[offset2 + 4 + service_provider_name_length];
				if (service_name_length == 255)
					service_name_length--;

				memset(service_name, '\0', 256);
				memcpy(provider_name, data + offset2 + 4, service_provider_name_length);
				memcpy(service_name, data + offset2 + 5 + service_provider_name_length, service_name_length);
			}
			if (tag == 0x81)	// UPC RoI cable LCN.
			{
				region_code = data[offset2 + 3];
				city_code = (data[offset2 + 4] << 8) | data[offset2 + 5];
				lcn_id = (data[offset2 + 6] << 8) | data[offset2 + 7];
			}
			if (tag == 0xb2 && size > 5)	//User defined. SKY category
			{
				category_id = (data[offset2 + 4] << 8) | data[offset2 + 5];
			}
			if (tag == 0xc0)	// Sky protocol. Used for NVOD service names.
			{
				memset(service_name, '\0', 256);
				memcpy(service_name, data + offset2 + 2, size);
			}
			if (tag == 0xca)	//User defined. Virgin LCN and Bouquets id
			{
				lcn_id = ((data[offset2 + 2] & 0x03) << 8) | data[offset2 + 3];
				int name_length = data[offset2 + 4];
				//service name is taken from descriptor 48
				bouquets_id = data[offset2 + 5 + name_length];
				service_group_id = data[offset2 + 6 + name_length];
			}
			descriptors_loop_length -= (size + 2);
			offset2 += (size + 2);
		}

		char *provider_name_ptr = provider_name;
		if (strlen(provider_name) == 0)
			strcpy(provider_name, "Unknown");
		else if (provider_name[0] == 0x05)
				provider_name_ptr++;

		char *service_name_ptr = service_name;
		if (strlen(service_name) == 0)
			strcpy(service_name, "Unknown");
		else if (service_name[0] == 0x05)
				service_name_ptr++;

		PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:s,s:s,s:i,s:i,s:i,s:i,s:i,s:i}",
					"transport_stream_id", transport_stream_id,
					"original_network_id", original_network_id,
					"service_id", service_id,
					"service_type", service_type,
					"free_ca", free_ca,
					"service_name", service_name_ptr,
					"provider_name", provider_name_ptr,
					"logical_channel_number", lcn_id,
					"bouquets_id", bouquets_id,
					"service_group_id", service_group_id,
					"category_id", category_id,
					"region_code", region_code,
					"city_code", city_code);
		PyList_Append(list, item);
		Py_DECREF(item);
	}

	return list;
}

PyObject *ss_parse_fastscan(unsigned char *data, int length) {
	PyObject* list = PyList_New(0);

	int offset = 8;
	length -= 8;

	while (length >= 5)
	{
		char service_name[256];
		char provider_name[256];
		int service_type = 0;
		memset(service_name, '\0', 256);
		memset(provider_name, '\0', 256);

		int original_network_id = (data[offset] << 8) | data[offset + 1];
		int transport_stream_id = (data[offset + 2] << 8) | data[offset + 3];
		int service_id = (data[offset + 4] << 8) | data[offset + 5];
		int descriptors_loop_length = ((data[offset + 16] & 0x0f) << 8) | data[offset + 17];

		length -= 18;
		offset += 18;

		int offset2 = offset;

		length -= descriptors_loop_length;
		offset += descriptors_loop_length;

		while (descriptors_loop_length >= 2)
		{
			int tag = data[offset2];
			int size = data[offset2 + 1];

			if (tag == 0x48)	// Service descriptor
			{
				service_type = data[offset2 + 2];
				int service_provider_name_length = data[offset2 + 3];
				if (service_provider_name_length == 255)
					service_provider_name_length--;

				int service_name_length = data[offset2 + 4 + service_provider_name_length];
				if (service_name_length == 255)
					service_name_length--;

				memcpy(provider_name, data + offset2 + 4, service_provider_name_length);
				memcpy(service_name, data + offset2 + 5 + service_provider_name_length, service_name_length);
			}

			descriptors_loop_length -= (size + 2);
			offset2 += (size + 2);
		}

		char *provider_name_ptr = provider_name;
		if (strlen(provider_name) == 0)
			strcpy(provider_name, "Unknown");
		else if (provider_name[0] == 0x05)
				provider_name_ptr++;

		char *service_name_ptr = service_name;
		if (strlen(service_name) == 0)
			strcpy(service_name, "Unknown");
		else if (service_name[0] == 0x05)
				service_name_ptr++;

		PyObject *item = Py_BuildValue("{s:i,s:i,s:i,s:i,s:s,s:s}",
					"transport_stream_id", transport_stream_id,
					"original_network_id", original_network_id,
					"service_id", service_id,
					"service_type", service_type,
					"service_name", service_name_ptr,
					"provider_name", provider_name_ptr);

		PyList_Append(list, item);
		Py_DECREF(item);
	}

	return list;
}

PyObject *ss_parse_header_nit(unsigned char *data, int length, const char *variable_key_name)
{
	int table_id = data[0];
	int variable_id = (data[3] << 8) | data[4];
	int version_number = (data[5] >> 1) & 0x1f;
	int current_next_indicator = data[5] & 0x01;
	int section_number = data[6];
	int last_section_number = data[7];
	int network_descriptors_length = ((data[8] & 0x0f) << 8) | data[9];
	int original_network_id = (data[network_descriptors_length + 9 + 5] << 8) | data[network_descriptors_length + 9 + 6];
	int offset = 10;
	
	char network_name[256];
	memset(network_name, '\0', 256);
	strcpy(network_name, "Unknown");
	
	while (network_descriptors_length > 0)
	{
		int descriptor_tag = data[offset];
		int descriptor_length = data[offset + 1];
		
		if (descriptor_tag == 0x40)
		{
			unsigned network_name_length = data[offset + 1];
			memset(network_name, '\0', 256);
			if (network_name_length == 255)
				network_name_length--;
			memcpy(network_name, data + offset + 2, network_name_length);
		}

		offset += (descriptor_length + 1);
		network_descriptors_length -= (descriptor_length + 1);
	}
	
	return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:s}",
		"table_id", table_id, variable_key_name, variable_id,
		"version_number", version_number, "current_next_indicator", current_next_indicator,
		"section_number", section_number, "last_section_number", last_section_number,
		"original_network_id", original_network_id,
		"network_name", network_name);
}

PyObject *ss_parse_header_bat(unsigned char *data, int length, const char *variable_key_name)
{
	int table_id = data[0];
	int variable_id = (data[3] << 8) | data[4];
	int version_number = (data[5] >> 1) & 0x1f;
	int current_next_indicator = data[5] & 0x01;
	int section_number = data[6];
	int last_section_number = data[7];
	int network_descriptors_length = ((data[8] & 0x0f) << 8) | data[9];
	int original_network_id = (data[network_descriptors_length + 9 + 5] << 8) | data[network_descriptors_length + 9 + 6];

	return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
		"table_id", table_id, variable_key_name, variable_id,
		"version_number", version_number, "current_next_indicator", current_next_indicator,
		"section_number", section_number, "last_section_number", last_section_number,
		"original_network_id", original_network_id);
}

PyObject *ss_parse_header(unsigned char *data, int length, const char *variable_key_name) //SDT and Fastscan
{
	int table_id = data[0];
	int variable_id = (data[3] << 8) | data[4];
	int version_number = (data[5] >> 1) & 0x1f;
	int current_next_indicator = data[5] & 0x01;
	int section_number = data[6];
	int last_section_number = data[7];
	int original_network_id = (data[8] << 8) | data[9];

	return Py_BuildValue("{s:i,s:i,s:i,s:i,s:i,s:i,s:i}",
		"table_id", table_id, variable_key_name, variable_id,
		"version_number", version_number, "current_next_indicator", current_next_indicator,
		"section_number", section_number, "last_section_number", last_section_number,
		"original_network_id", original_network_id);
}

PyObject *ss_parse_table(unsigned char *data, int length) {
	PyObject* list = PyList_New(0);
	int i = 0;
	while (length > 0)
	{
		int value = data[i];
		PyObject *item = Py_BuildValue("i", value);
		PyList_Append(list, item);
		Py_DECREF(item);
		i += 1;
		length -= 1;
	}
	return list;
}

PyObject *ss_read_ts(PyObject *self, PyObject *args) { // for table dump
	PyObject *content = NULL;
	unsigned char buffer[4096], table_id_current, table_id_other;
	int fd;

	if (!PyArg_ParseTuple(args, "ibb", &fd, &table_id_current, &table_id_other))
		return Py_None;

#ifndef HAVE_ALIAUI
	int size = read(fd, buffer, sizeof(buffer));
#else
	int size = ali_read(fd, buffer, sizeof(buffer), &g_ali_filter);
#endif



	if (size < 3)
		return Py_None;

	if (buffer[0] != table_id_current && buffer[0] != table_id_other)
		return Py_None;

	int section_length = ((buffer[1] & 0x0f) << 8) | buffer[2];

	if (size != section_length + 3)
		return Py_None;

	content = ss_parse_table(buffer, section_length);

	PyObject *ret = Py_BuildValue("O", content);
	Py_DECREF(content);
	return ret;
}

PyObject *ss_read_bat(PyObject *self, PyObject *args) {
	PyObject *content = NULL, *header = NULL;
	unsigned char buffer[4096], table_id;
	int fd;

	if (!PyArg_ParseTuple(args, "ib", &fd, &table_id))
		return Py_None;

#ifndef HAVE_ALIAUI
	int size = read(fd, buffer, sizeof(buffer));
#else
	int size = ali_read(fd, buffer, sizeof(buffer), &g_ali_filter);
#endif
	if (size < 3)
		return Py_None;

	if (buffer[0] != table_id)
		return Py_None;

	int section_length = ((buffer[1] & 0x0f) << 8) | buffer[2];

	if (size != section_length + 3)
		return Py_None;

	header = ss_parse_header_bat(buffer, section_length, "bouquet_id");
	content = ss_parse_bat(buffer, section_length);

	if (!header || !content)
		return Py_None;

	PyObject *ret = Py_BuildValue("{s:O,s:O}", "header", header, "content", content);
	Py_DECREF(header);
	Py_DECREF(content);
	return ret;
}

PyObject *ss_read_sdt(PyObject *self, PyObject *args) {
	PyObject *content = NULL, *header = NULL;
	unsigned char buffer[4096], table_id_current, table_id_other;
	int fd;

	if (!PyArg_ParseTuple(args, "ibb", &fd, &table_id_current, &table_id_other))
		return Py_None;
#ifndef HAVE_ALIAUI
	int size = read(fd, buffer, sizeof(buffer));
#else
	int size = ali_read(fd, buffer, sizeof(buffer), &g_ali_filter);
#endif
	if (size < 3)
		return Py_None;

	if (buffer[0] != table_id_current && buffer[0] != table_id_other)
		return Py_None;

	int section_length = ((buffer[1] & 0x0f) << 8) | buffer[2];

	if (size != section_length + 3)
		return Py_None;

	header = ss_parse_header(buffer, section_length, "transport_stream_id");
	content = ss_parse_sdt(buffer, section_length);

	if (!header || !content)
		return Py_None;

	PyObject *ret = Py_BuildValue("{s:O,s:O}", "header", header, "content", content);
	Py_DECREF(header);
	Py_DECREF(content);
	return ret;
}

PyObject *ss_read_fastscan(PyObject *self, PyObject *args) {
	PyObject *content = NULL, *header = NULL;
	unsigned char buffer[4096], table_id;
	int fd;

	if (!PyArg_ParseTuple(args, "ib", &fd, &table_id))
		return Py_None;
#ifndef HAVE_ALIAUI
	int size = read(fd, buffer, sizeof(buffer));
#else
	int size = ali_read(fd, buffer, sizeof(buffer), &g_ali_filter);
#endif
	if (size < 3)
		return Py_None;

	if (buffer[0] != table_id)
		return Py_None;

	int section_length = ((buffer[1] & 0x0f) << 8) | buffer[2];

	if (size != section_length + 3)
		return Py_None;

	header = ss_parse_header(buffer, section_length, "fastscan_id");
	content = ss_parse_fastscan(buffer, section_length);

	if (!header || !content)
		return Py_None;

	PyObject *ret = Py_BuildValue("{s:O,s:O}", "header", header, "content", content);
	Py_DECREF(header);
	Py_DECREF(content);
	return ret;
}

PyObject *ss_read_nit(PyObject *self, PyObject *args) {
	PyObject *content = NULL, *header = NULL;
	unsigned char buffer[4096], table_id_current, table_id_other;
	int fd;

	if (!PyArg_ParseTuple(args, "ibb", &fd, &table_id_current, &table_id_other))
		return Py_None;
#ifndef HAVE_ALIAUI
	int size = read(fd, buffer, sizeof(buffer));
#else
	int size = ali_read(fd, buffer, sizeof(buffer), &g_ali_filter);
#endif
	if (size < 3)
		return Py_None;

	if (buffer[0] != table_id_current && buffer[0] != table_id_other)
		return Py_None;

	int section_length = ((buffer[1] & 0x0f) << 8) | buffer[2];

	if (size != section_length + 3)
		return Py_None;

	header = ss_parse_header_nit(buffer, section_length, "network_id");
	content = ss_parse_nit(buffer, section_length);

	if (!header || !content)
		return Py_None;

	PyObject *ret = Py_BuildValue("{s:O,s:O}", "header", header, "content", content);
	Py_DECREF(header);
	Py_DECREF(content);
	return ret;
}

static PyMethodDef dvbreaderMethods[] = {
		{ "open", ss_open, METH_VARARGS },
		{ "close", ss_close, METH_VARARGS },
		{ "read_bat", ss_read_bat, METH_VARARGS },
		{ "read_nit", ss_read_nit, METH_VARARGS },
		{ "read_sdt", ss_read_sdt, METH_VARARGS },
		{ "read_fastscan", ss_read_fastscan, METH_VARARGS },
		{ "read_ts", ss_read_ts, METH_VARARGS },
		{ NULL, NULL }
};

void initdvbreader() {
	PyObject *m;
	m = Py_InitModule("dvbreader", dvbreaderMethods);
}
