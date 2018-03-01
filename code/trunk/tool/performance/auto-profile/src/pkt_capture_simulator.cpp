#include <boost/thread.hpp>
#include "pkt_capture_simulator.hpp"
#include "prof_typedef.hpp"
#include "tmas_util.hpp"
#include "tmas_typedef.hpp"
#include "pkt_processor.hpp"
#include "pkt_dispatcher.hpp"
#include "prof_stat_processor.hpp"

namespace BroadInter
{

extern ProfStat g_prof_stat;
extern CmdLinePara g_cmdline_para;

#pragma GCC diagnostic ignored "-Wnarrowing"

static char pkt1[] =
{
	0x00, 0x24, 0xe8, 0x79, 0x35, 0xed, 0x00, 0x24, 0xe8, 0x75, 0xf5, 0x96, 0x08, 0x00, 0x45, 0x00,
	0x00, 0x3c, 0x63, 0x0c, 0x40, 0x00, 0x40, 0x06, 0x54, 0xe2, 0xc0, 0xa8, 0x00, 0xbe, 0xc0, 0xa8,
	0x00, 0xbf, 0x8c, 0xc3, 0x23, 0x82, 0xbe, 0x37, 0x49, 0x21, 0x00, 0x00, 0x00, 0x00, 0xa0, 0x02,
	0x39, 0x08, 0x57, 0x08, 0x00, 0x00, 0x02, 0x04, 0x05, 0xb4, 0x04, 0x02, 0x08, 0x0a, 0x53, 0x56,
	0x2a, 0x2d, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x03, 0x07
};

static char pkt2[] =
{
	0x00, 0x24, 0xe8, 0x75, 0xf5, 0x96, 0x00, 0x24, 0xe8, 0x79, 0x35, 0xed, 0x08, 0x00, 0x45, 0x00,
	0x00, 0x3c, 0x00, 0x00, 0x40, 0x00, 0x40, 0x06, 0xb7, 0xee, 0xc0, 0xa8, 0x00, 0xbf, 0xc0, 0xa8,
	0x00, 0xbe, 0x23, 0x82, 0x8c, 0xc3, 0x3e, 0x0f, 0x28, 0xd1, 0xbe, 0x37, 0x49, 0x22, 0xa0, 0x12,
	0x38, 0x90, 0xef, 0xce, 0x00, 0x00, 0x02, 0x04, 0x05, 0xb4, 0x04, 0x02, 0x08, 0x0a, 0x53, 0x56,
	0xad, 0x69, 0x53, 0x56, 0x2a, 0x2d, 0x01, 0x03, 0x03, 0x07
};

static char pkt3[] =
{
	0x00, 0x24, 0xe8, 0x79, 0x35, 0xed, 0x00, 0x24, 0xe8, 0x75, 0xf5, 0x96, 0x08, 0x00, 0x45, 0x00,
	0x00, 0x34, 0x63, 0x0d, 0x40, 0x00, 0x40, 0x06, 0x54, 0xe9, 0xc0, 0xa8, 0x00, 0xbe, 0xc0, 0xa8,
	0x00, 0xbf, 0x8c, 0xc3, 0x23, 0x82, 0xbe, 0x37, 0x49, 0x22, 0x3e, 0x0f, 0x28, 0xd2, 0x80, 0x10,
	0x00, 0x73, 0x56, 0xb8, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x53, 0x56, 0x2a, 0x2d, 0x53, 0x56,
	0xad, 0x69
};

static char pkt4[] =
{
	0x00, 0x24, 0xe8, 0x79, 0x35, 0xed, 0x00, 0x24, 0xe8, 0x75, 0xf5, 0x96, 0x08, 0x00, 0x45, 0x00,
	0x00, 0x94, 0x63, 0x0e, 0x40, 0x00, 0x40, 0x06, 0x54, 0x88, 0xc0, 0xa8, 0x00, 0xbe, 0xc0, 0xa8,
	0x00, 0xbf, 0x8c, 0xc3, 0x23, 0x82, 0xbe, 0x37, 0x49, 0x22, 0x3e, 0x0f, 0x28, 0xd2, 0x80, 0x18,
	0x00, 0x73, 0xf7, 0xb2, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x53, 0x56, 0x2a, 0x2d, 0x53, 0x56,
	0xad, 0x69, 0x47, 0x45, 0x54, 0x20, 0x2f, 0x69, 0x6e, 0x64, 0x65, 0x78, 0x2e, 0x68, 0x74, 0x6d,
	0x6c, 0x20, 0x48, 0x54, 0x54, 0x50, 0x2f, 0x31, 0x2e, 0x31, 0x0d, 0x0a, 0x48, 0x6f, 0x73, 0x74,
	0x3a, 0x20, 0x77, 0x77, 0x77, 0x2e, 0x62, 0x61, 0x69, 0x64, 0x75, 0x2e, 0x63, 0x6f, 0x6d, 0x0d,
	0x0a, 0x55, 0x73, 0x65, 0x72, 0x2d, 0x41, 0x67, 0x65, 0x6e, 0x74, 0x3a, 0x20, 0x77, 0x65, 0x69,
	0x67, 0x68, 0x74, 0x74, 0x70, 0x2f, 0x31, 0x2e, 0x34, 0x2e, 0x34, 0x0d, 0x0a, 0x43, 0x6f, 0x6e,
	0x6e, 0x65, 0x63, 0x74, 0x69, 0x6f, 0x6e, 0x3a, 0x20, 0x63, 0x6c, 0x6f, 0x73, 0x65, 0x0d, 0x0a,
	0x0d, 0x0a
};

static char pkt5[] =
{
	0x00, 0x24, 0xe8, 0x75, 0xf5, 0x96, 0x00, 0x24, 0xe8, 0x79, 0x35, 0xed, 0x08, 0x00, 0x45, 0x00,
	0x00, 0x34, 0x85, 0xbb, 0x40, 0x00, 0x40, 0x06, 0x32, 0x3b, 0xc0, 0xa8, 0x00, 0xbf, 0xc0, 0xa8,
	0x00, 0xbe, 0x23, 0x82, 0x8c, 0xc3, 0x3e, 0x0f, 0x28, 0xd2, 0xbe, 0x37, 0x49, 0x82, 0x80, 0x10,
	0x00, 0x72, 0x56, 0x59, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x53, 0x56, 0xad, 0x69, 0x53, 0x56,
	0x2a, 0x2d
};

static char pkt6[] =
{
	0x00, 0x24, 0xe8, 0x75, 0xf5, 0x96, 0x00, 0x24, 0xe8, 0x79, 0x35, 0xed, 0x08, 0x00, 0x45, 0x00,
	0x01, 0x1c, 0x85, 0xbc, 0x40, 0x00, 0x40, 0x06, 0x31, 0x52, 0xc0, 0xa8, 0x00, 0xbf, 0xc0, 0xa8,
	0x00, 0xbe, 0x23, 0x82, 0x8c, 0xc3, 0x3e, 0x0f, 0x28, 0xd2, 0xbe, 0x37, 0x49, 0x82, 0x80, 0x18,
	0x00, 0x72, 0x83, 0xdc, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x53, 0x56, 0xad, 0x6a, 0x53, 0x56,
	0x2a, 0x2d, 0x48, 0x54, 0x54, 0x50, 0x2f, 0x31, 0x2e, 0x31, 0x20, 0x32, 0x30, 0x30, 0x20, 0x4f,
	0x4b, 0x0d, 0x0a, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72, 0x3a, 0x20, 0x6e, 0x67, 0x69, 0x6e, 0x78,
	0x2f, 0x31, 0x2e, 0x34, 0x2e, 0x34, 0x0d, 0x0a, 0x44, 0x61, 0x74, 0x65, 0x3a, 0x20, 0x54, 0x75,
	0x65, 0x2c, 0x20, 0x31, 0x38, 0x20, 0x4d, 0x61, 0x72, 0x20, 0x32, 0x30, 0x31, 0x34, 0x20, 0x30,
	0x39, 0x3a, 0x33, 0x32, 0x3a, 0x33, 0x31, 0x20, 0x47, 0x4d, 0x54, 0x0d, 0x0a, 0x43, 0x6f, 0x6e,
	0x74, 0x65, 0x6e, 0x74, 0x2d, 0x54, 0x79, 0x70, 0x65, 0x3a, 0x20, 0x74, 0x65, 0x78, 0x74, 0x2f,
	0x68, 0x74, 0x6d, 0x6c, 0x0d, 0x0a, 0x43, 0x6f, 0x6e, 0x74, 0x65, 0x6e, 0x74, 0x2d, 0x4c, 0x65,
	0x6e, 0x67, 0x74, 0x68, 0x3a, 0x20, 0x36, 0x33, 0x35, 0x0d, 0x0a, 0x4c, 0x61, 0x73, 0x74, 0x2d,
	0x4d, 0x6f, 0x64, 0x69, 0x66, 0x69, 0x65, 0x64, 0x3a, 0x20, 0x54, 0x68, 0x75, 0x2c, 0x20, 0x31,
	0x33, 0x20, 0x46, 0x65, 0x62, 0x20, 0x32, 0x30, 0x31, 0x34, 0x20, 0x30, 0x34, 0x3a, 0x33, 0x30,
	0x3a, 0x30, 0x36, 0x20, 0x47, 0x4d, 0x54, 0x0d, 0x0a, 0x43, 0x6f, 0x6e, 0x6e, 0x65, 0x63, 0x74,
	0x69, 0x6f, 0x6e, 0x3a, 0x20, 0x63, 0x6c, 0x6f, 0x73, 0x65, 0x0d, 0x0a, 0x45, 0x54, 0x61, 0x67,
	0x3a, 0x20, 0x22, 0x35, 0x32, 0x66, 0x63, 0x34, 0x61, 0x34, 0x65, 0x2d, 0x32, 0x37, 0x62, 0x22,
	0x0d, 0x0a, 0x41, 0x63, 0x63, 0x65, 0x70, 0x74, 0x2d, 0x52, 0x61, 0x6e, 0x67, 0x65, 0x73, 0x3a,
	0x20, 0x62, 0x79, 0x74, 0x65, 0x73, 0x0d, 0x0a, 0x0d, 0x0a
};

static char pkt7[] =
{
	0x00, 0x24, 0xe8, 0x75, 0xf5, 0x96, 0x00, 0x24, 0xe8, 0x79, 0x35, 0xed, 0x08, 0x00, 0x45, 0x00,
	0x02, 0xaf, 0x85, 0xbd, 0x40, 0x00, 0x40, 0x06, 0x2f, 0xbe, 0xc0, 0xa8, 0x00, 0xbf, 0xc0, 0xa8,
	0x00, 0xbe, 0x23, 0x82, 0x8c, 0xc3, 0x3e, 0x0f, 0x29, 0xba, 0xbe, 0x37, 0x49, 0x82, 0x80, 0x19,
	0x00, 0x72, 0x85, 0x6f, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x53, 0x56, 0xad, 0x6a, 0x53, 0x56,
	0x2a, 0x2d, 0x3c, 0x21, 0x44, 0x4f, 0x43, 0x54, 0x59, 0x50, 0x45, 0x20, 0x68, 0x74, 0x6d, 0x6c,
	0x3e, 0x0a, 0x3c, 0x68, 0x74, 0x6d, 0x6c, 0x3e, 0x0a, 0x3c, 0x68, 0x65, 0x61, 0x64, 0x3e, 0x0a,
	0x3c, 0x74, 0x69, 0x74, 0x6c, 0x65, 0x3e, 0x57, 0x65, 0x6c, 0x63, 0x6f, 0x6d, 0x65, 0x20, 0x74,
	0x6f, 0x20, 0x6e, 0x67, 0x69, 0x6e, 0x78, 0x21, 0x3c, 0x2f, 0x74, 0x69, 0x74, 0x6c, 0x65, 0x3e,
	0x0a, 0x3c, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3e, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x62, 0x6f, 0x64,
	0x79, 0x20, 0x7b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x77, 0x69, 0x64, 0x74,
	0x68, 0x3a, 0x20, 0x33, 0x35, 0x65, 0x6d, 0x3b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x6d, 0x61, 0x72, 0x67, 0x69, 0x6e, 0x3a, 0x20, 0x30, 0x20, 0x61, 0x75, 0x74, 0x6f, 0x3b,
	0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x66, 0x6f, 0x6e, 0x74, 0x2d, 0x66, 0x61,
	0x6d, 0x69, 0x6c, 0x79, 0x3a, 0x20, 0x54, 0x61, 0x68, 0x6f, 0x6d, 0x61, 0x2c, 0x20, 0x56, 0x65,
	0x72, 0x64, 0x61, 0x6e, 0x61, 0x2c, 0x20, 0x41, 0x72, 0x69, 0x61, 0x6c, 0x2c, 0x20, 0x73, 0x61,
	0x6e, 0x73, 0x2d, 0x73, 0x65, 0x72, 0x69, 0x66, 0x3b, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x7d, 0x0a,
	0x3c, 0x2f, 0x73, 0x74, 0x79, 0x6c, 0x65, 0x3e, 0x0a, 0x3c, 0x2f, 0x68, 0x65, 0x61, 0x64, 0x3e,
	0x0a, 0x3c, 0x62, 0x6f, 0x64, 0x79, 0x3e, 0x0a, 0x3c, 0x68, 0x31, 0x3e, 0x57, 0x65, 0x6c, 0x63,
	0x6f, 0x6d, 0x65, 0x20, 0x74, 0x6f, 0x20, 0x6e, 0x67, 0x69, 0x6e, 0x78, 0x21, 0x3c, 0x2f, 0x68,
	0x31, 0x3e, 0x0a, 0x3c, 0x70, 0x3e, 0x49, 0x66, 0x20, 0x79, 0x6f, 0x75, 0x20, 0x73, 0x65, 0x65,
	0x20, 0x74, 0x68, 0x69, 0x73, 0x20, 0x70, 0x61, 0x67, 0x65, 0x2c, 0x20, 0x74, 0x68, 0x65, 0x20,
	0x6e, 0x67, 0x69, 0x6e, 0x78, 0x20, 0x77, 0x65, 0x62, 0x20, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72,
	0x20, 0x69, 0x73, 0x20, 0x73, 0x75, 0x63, 0x63, 0x65, 0x73, 0x73, 0x66, 0x75, 0x6c, 0x6c, 0x79,
	0x20, 0x69, 0x6e, 0x73, 0x74, 0x61, 0x6c, 0x6c, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x0a, 0x77,
	0x6f, 0x72, 0x6b, 0x69, 0x6e, 0x67, 0x2e, 0x20, 0x46, 0x75, 0x72, 0x74, 0x68, 0x65, 0x72, 0x20,
	0x63, 0x6f, 0x6e, 0x66, 0x69, 0x67, 0x75, 0x72, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x69, 0x73,
	0x20, 0x72, 0x65, 0x71, 0x75, 0x69, 0x72, 0x65, 0x64, 0x2e, 0x3c, 0x2f, 0x70, 0x3e, 0x0a, 0x0a,
	0x3c, 0x70, 0x3e, 0x46, 0x6f, 0x72, 0x20, 0x6f, 0x6e, 0x6c, 0x69, 0x6e, 0x65, 0x20, 0x64, 0x6f,
	0x63, 0x75, 0x6d, 0x65, 0x6e, 0x74, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x61, 0x6e, 0x64, 0x20,
	0x73, 0x75, 0x70, 0x70, 0x6f, 0x72, 0x74, 0x20, 0x70, 0x6c, 0x65, 0x61, 0x73, 0x65, 0x20, 0x72,
	0x65, 0x66, 0x65, 0x72, 0x20, 0x74, 0x6f, 0x0a, 0x3c, 0x61, 0x20, 0x68, 0x72, 0x65, 0x66, 0x3d,
	0x22, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x6e, 0x67, 0x69, 0x6e, 0x78, 0x2e, 0x6f, 0x72,
	0x67, 0x2f, 0x22, 0x3e, 0x6e, 0x67, 0x69, 0x6e, 0x78, 0x2e, 0x6f, 0x72, 0x67, 0x3c, 0x2f, 0x61,
	0x3e, 0x2e, 0x3c, 0x62, 0x72, 0x2f, 0x3e, 0x0a, 0x43, 0x6f, 0x6d, 0x6d, 0x65, 0x72, 0x63, 0x69,
	0x61, 0x6c, 0x20, 0x73, 0x75, 0x70, 0x70, 0x6f, 0x72, 0x74, 0x20, 0x69, 0x73, 0x20, 0x61, 0x76,
	0x61, 0x69, 0x6c, 0x61, 0x62, 0x6c, 0x65, 0x20, 0x61, 0x74, 0x0a, 0x3c, 0x61, 0x20, 0x68, 0x72,
	0x65, 0x66, 0x3d, 0x22, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x6e, 0x67, 0x69, 0x6e, 0x78,
	0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x22, 0x3e, 0x6e, 0x67, 0x69, 0x6e, 0x78, 0x2e, 0x63, 0x6f, 0x6d,
	0x3c, 0x2f, 0x61, 0x3e, 0x2e, 0x3c, 0x2f, 0x70, 0x3e, 0x0a, 0x0a, 0x3c, 0x68, 0x32, 0x3e, 0x20,
	0x31, 0x39, 0x30, 0x20, 0x53, 0x65, 0x72, 0x76, 0x65, 0x72, 0x20, 0x3c, 0x2f, 0x68, 0x32, 0x3e,
	0x0a, 0x0a, 0x3c, 0x70, 0x3e, 0x3c, 0x65, 0x6d, 0x3e, 0x54, 0x68, 0x61, 0x6e, 0x6b, 0x20, 0x79,
	0x6f, 0x75, 0x20, 0x66, 0x6f, 0x72, 0x20, 0x75, 0x73, 0x69, 0x6e, 0x67, 0x20, 0x6e, 0x67, 0x69,
	0x6e, 0x78, 0x2e, 0x3c, 0x2f, 0x65, 0x6d, 0x3e, 0x3c, 0x2f, 0x70, 0x3e, 0x0a, 0x3c, 0x2f, 0x62,
	0x6f, 0x64, 0x79, 0x3e, 0x0a, 0x3c, 0x2f, 0x68, 0x74, 0x6d, 0x6c, 0x3e, 0x0a
};

static char pkt8[] =
{
	0x00, 0x24, 0xe8, 0x79, 0x35, 0xed, 0x00, 0x24, 0xe8, 0x75, 0xf5, 0x96, 0x08, 0x00, 0x45, 0x00,
	0x00, 0x34, 0x63, 0x0f, 0x40, 0x00, 0x40, 0x06, 0x54, 0xe7, 0xc0, 0xa8, 0x00, 0xbe, 0xc0, 0xa8,
	0x00, 0xbf, 0x8c, 0xc3, 0x23, 0x82, 0xbe, 0x37, 0x49, 0x82, 0x3e, 0x0f, 0x29, 0xba, 0x80, 0x10,
	0x00, 0x7b, 0x55, 0x67, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x53, 0x56, 0x2a, 0x2d, 0x53, 0x56,
	0xad, 0x6a
};

static char pkt9[] =
{
	0x00, 0x24, 0xe8, 0x79, 0x35, 0xed, 0x00, 0x24, 0xe8, 0x75, 0xf5, 0x96, 0x08, 0x00, 0x45, 0x00,
	0x00, 0x34, 0x63, 0x10, 0x40, 0x00, 0x40, 0x06, 0x54, 0xe6, 0xc0, 0xa8, 0x00, 0xbe, 0xc0, 0xa8,
	0x00, 0xbf, 0x8c, 0xc3, 0x23, 0x82, 0xbe, 0x37, 0x49, 0x82, 0x3e, 0x0f, 0x2c, 0x36, 0x80, 0x10,
	0x00, 0x85, 0x52, 0xe1, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x53, 0x56, 0x2a, 0x2d, 0x53, 0x56,
	0xad, 0x6a
};

static char pkt10[] =
{
	0x00, 0x24, 0xe8, 0x79, 0x35, 0xed, 0x00, 0x24, 0xe8, 0x75, 0xf5, 0x96, 0x08, 0x00, 0x45, 0x00,
	0x00, 0x34, 0x63, 0x11, 0x40, 0x00, 0x40, 0x06, 0x54, 0xe5, 0xc0, 0xa8, 0x00, 0xbe, 0xc0, 0xa8,
	0x00, 0xbf, 0x8c, 0xc3, 0x23, 0x82, 0xbe, 0x37, 0x49, 0x82, 0x3e, 0x0f, 0x2c, 0x36, 0x80, 0x11,
	0x00, 0x85, 0x52, 0xe0, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x53, 0x56, 0x2a, 0x2d, 0x53, 0x56,
	0xad, 0x6a
};

static char pkt11[] =
{
	0x00, 0x24, 0xe8, 0x75, 0xf5, 0x96, 0x00, 0x24, 0xe8, 0x79, 0x35, 0xed, 0x08, 0x00, 0x45, 0x00,
	0x00, 0x34, 0x85, 0xbe, 0x40, 0x00, 0x40, 0x06, 0x32, 0x38, 0xc0, 0xa8, 0x00, 0xbf, 0xc0, 0xa8,
	0x00, 0xbe, 0x23, 0x82, 0x8c, 0xc3, 0x3e, 0x0f, 0x2c, 0x36, 0xbe, 0x37, 0x49, 0x83, 0x80, 0x10,
	0x00, 0x72, 0x52, 0xf3, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0x53, 0x56, 0xad, 0x6a, 0x53, 0x56,
	0x2a, 0x2d
};

#pragma GCC diagnostic warning "-Wnarrowing"

//-------------------------- 压力测试相关宏 -------------------------------

#define PT_SEND_PACKET(pkt)													\
	pkt_entry.buf = pkt;													\
	pkt_entry.len = sizeof(pkt);											\
	pkt_dispatcher_->ProcessPktEntry(pkt_entry);							\
	sent_count++;															\
	if (g_cmdline_para.micro_sleep_time != 0)								\
	{																		\
		boost::this_thread::sleep(boost::posix_time::microseconds(			\
			g_cmdline_para.micro_sleep_time));								\
	}																		\
	else if (g_cmdline_para.nano_sleep_time != 0)							\
	{																		\
		if (sent_count % sleep_micro_after_send_count == 0)					\
		{																	\
			boost::this_thread::sleep(boost::posix_time::microseconds(1));	\
		}																	\
	}																		\
	g_prof_stat.captured_pkt_num++;											\
	g_prof_stat.total_captured_pkt_num++;									\
	if (g_cmdline_para.send_count											\
		&& sent_count >= g_cmdline_para.send_count)							\
	{																		\
		g_prof_stat.stop_time = MicroTimeNow();								\
		return;																\
	}																		

#define PT_SEND_PKT_C2S(pkt)													\
	ip_addr_tmp = ip_addr;														\
	for (uint32 i = 0; i < g_cmdline_para.concurrent_conns; i++, ip_addr_tmp++)	\
	{																			\
		*(reinterpret_cast<uint32*>(pkt + 26)) = ip_addr_tmp;					\
		PT_SEND_PACKET(pkt);													\
	}

#define PT_SEND_PKT_S2C(pkt)													\
	ip_addr_tmp = ip_addr;														\
	for (uint32 i = 0; i < g_cmdline_para.concurrent_conns; i++, ip_addr_tmp++)	\
	{																			\
		*(reinterpret_cast<uint32*>(pkt + 30)) = ip_addr_tmp;					\
		PT_SEND_PACKET(pkt);													\
	}

PressureTestSimulator::PressureTestSimulator(const PktDispatcherTestTypeSP& dispatcher)
	: stop_flag_(false), pkt_dispatcher_(dispatcher)
{

}

void PressureTestSimulator::GenPktThreadFunc()
{
	g_prof_stat.start_time = MicroTimeNow();

	PktEntry pkt_entry;
	uint32 sent_count = 0;

	uint32 ip_addr = 0;
	uint32 ip_addr_tmp = 0;

	uint32 sleep_micro_after_send_count = 1;
	if (g_cmdline_para.nano_sleep_time != 0)
	{
		sleep_micro_after_send_count = (1000 / g_cmdline_para.nano_sleep_time);
	}

	while (!stop_flag_)
	{
		PT_SEND_PKT_C2S(pkt1);
		PT_SEND_PKT_S2C(pkt2);
		PT_SEND_PKT_C2S(pkt3);
		PT_SEND_PKT_C2S(pkt4);
		PT_SEND_PKT_S2C(pkt5);
		PT_SEND_PKT_S2C(pkt6);
		PT_SEND_PKT_S2C(pkt7);
		PT_SEND_PKT_C2S(pkt8);
		PT_SEND_PKT_C2S(pkt9);
		PT_SEND_PKT_C2S(pkt10);
		PT_SEND_PKT_S2C(pkt11);

		ip_addr += 11;
	}

	g_prof_stat.stop_time = MicroTimeNow();
}

void PressureTestSimulator::Start()
{
	GenPktThreadFunc();
}

void PressureTestSimulator::Stop()
{
	stop_flag_ = true;
}

////////////////////////////////////////////////////////////////////////////////

static void CalcTimeUseOfAllLayers(uint32 pkt_index)
{
	time_duration duration;

	if (g_prof_stat.l3_start_proc_time > g_prof_stat.start_time)
	{
		duration = g_prof_stat.l3_start_proc_time - g_prof_stat.start_time;
		g_prof_stat.time_use_of_layer[pkt_index - 1][0] = duration.total_microseconds();
	}
	else
	{
		g_prof_stat.time_use_of_layer[pkt_index - 1][0] = 0;
	}
	
	if (g_prof_stat.l4_start_proc_time > g_prof_stat.l3_start_proc_time 
		&& g_prof_stat.l3_start_proc_time > g_prof_stat.start_time)
	{
		duration = g_prof_stat.l4_start_proc_time - g_prof_stat.l3_start_proc_time;
		g_prof_stat.time_use_of_layer[pkt_index - 1][1] = duration.total_microseconds();
	}
	else
	{
		g_prof_stat.time_use_of_layer[pkt_index - 1][1] = 0;
	}
	
	if (g_prof_stat.l7_start_proc_time > g_prof_stat.l4_start_proc_time 
		&& g_prof_stat.l4_start_proc_time > g_prof_stat.start_time)
	{
		duration = g_prof_stat.l7_start_proc_time - g_prof_stat.l4_start_proc_time;
		g_prof_stat.time_use_of_layer[pkt_index - 1][2] = duration.total_microseconds();
	}
	else
	{
		g_prof_stat.time_use_of_layer[pkt_index - 1][2] = 0;
	}

	if (g_prof_stat.stop_time > g_prof_stat.l7_start_proc_time 
		&& g_prof_stat.l7_start_proc_time > g_prof_stat.start_time)
	{
		duration = g_prof_stat.stop_time - g_prof_stat.l7_start_proc_time;
		g_prof_stat.time_use_of_layer[pkt_index - 1][3] = duration.total_microseconds();
	}
	else
	{
		g_prof_stat.time_use_of_layer[pkt_index - 1][3] = 0;
	}	
}

//------------------------ 性能测量相关宏 -----------------------
#define PM_SEND_PACKET(packet, pkt_index)						\
{																\
	PktInfoSP pkt_info(new PktInfo);							\
	std::memcpy(pkt_info->pkt.buf, packet, sizeof(packet));		\
	pkt_info->pkt.len = sizeof(packet);							\
	g_prof_stat.start_time = MicroTimeNow();					\
	eth_monitor_->ProcessPkt(pkt_info);							\
	g_prof_stat.stop_time = MicroTimeNow();						\
	CalcTimeUseOfAllLayers(pkt_index);							\
	boost::this_thread::sleep(boost::posix_time::millisec(1));	\
}


ProfMeasureSimulator::ProfMeasureSimulator(const EthMonitorTestTypeSP& eth_monitor)
	: eth_monitor_(eth_monitor)
{

}

void ProfMeasureSimulator::Start()
{
	PM_SEND_PACKET(pkt1, 1);
	PM_SEND_PACKET(pkt2, 2);
	PM_SEND_PACKET(pkt3, 3);
	PM_SEND_PACKET(pkt4, 4);
	PM_SEND_PACKET(pkt5, 5);
	PM_SEND_PACKET(pkt6, 6);
	PM_SEND_PACKET(pkt7, 7);
	PM_SEND_PACKET(pkt8, 8);
	PM_SEND_PACKET(pkt9, 9);
	PM_SEND_PACKET(pkt10, 10);
	PM_SEND_PACKET(pkt11, 11);
}

}