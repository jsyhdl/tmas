/*#############################################################################
 * �ļ���   : eth_monitor.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2014��01��21��
 * �ļ����� : EthMonitor������
 * ��Ȩ���� : Copyright (c) 2014 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef BROADINTER_ETH_MONITOR
#define BROADINTER_ETH_MONITOR

#include "pkt_processor.hpp"

namespace BroadInter
{

/*******************************************************************************
 * ��  ��: ETH�����ݼ�������
 * ��  ��: teck_zhou
 * ʱ  ��: 2014��01��21��
 ******************************************************************************/
template<class Next, class Succ>
class EthMonitor : public PktProcessor<EthMonitor<Next, Succ>, Next, Succ>
{
public:
	bool Init();

	//--- TODO: ���Ż���������

	// ������Ϣ����
	ProcInfo DoProcessMsg(MsgType msg_type, void* msg_data);

private:

	// ��ӡ��̫����Ϣ
	void PrintEthInfo(const PktMsg* pkt_msg);
};

}

#include "eth_monitor-inl.hpp"

#endif