/*#############################################################################
 * 文件名   : tcp_conn_reorder.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2014年01月09日
 * 文件描述 : TcpConnReorder类实现
 * 版权声明 : Copyright (c) 2014 BroadInter. All rights reserved.
 * ##########################################################################*/
#ifndef BROADINTER_TCP_CONN_REORDER_INL
#define BROADINTER_TCP_CONN_REORDER_INL

#include <glog/logging.h>

#include "tcp_conn_reorder.hpp"
#include "tmas_assert.hpp"
#include "pkt_resolver.hpp"
#include "tcp_monitor.hpp"
#include "tmas_cfg.hpp"
#include "tmas_config_parser.hpp"
#include "pkt_dispatcher.hpp"

namespace BroadInter
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数: [in] tcp_monitor 所属TcpMonitor
 * 返回值: 
 * 修  改: 
 *   时间 2014年01月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
TcpConnReorder<Next, Succ>::TcpConnReorder(TcpMonitorType* tcp_monitor)
	: tcp_monitor_(tcp_monitor), conn_closed_(false)
{
	GET_TMAS_CONFIG_INT("global.tcp.max-cached-unordered-pkt", 
		                max_cached_unordered_pkt_);

	if (max_cached_unordered_pkt_ > 1024 * 1024)
	{
		max_cached_unordered_pkt_ = 16;

		LOG(WARNING) << "Invalid max-cached-unordered-pkt value " 
			         << max_cached_unordered_pkt_;
	}

	GET_TMAS_CONFIG_INT("global.tcp.max-cached-pkt-before-handshake", 
		                max_cached_pkt_before_handshake_);

	if (max_cached_pkt_before_handshake_ > 1024 * 1024)
	{
		max_cached_pkt_before_handshake_ = 16;

		LOG(WARNING) << "Invalid max_cached_pkt_before_handshake_ value " 
			         << max_cached_pkt_before_handshake_;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 析构函数
 * 参  数: 
 * 返回值: 
 * 修  改: 
 *   时间 2014年01月14日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
TcpConnReorder<Next, Succ>::~TcpConnReorder()
{
	// 释放乱序缓存的报文
}

/*-----------------------------------------------------------------------------
 * 描  述: 重新初始化
 * 参  数: 
 * 返回值: 
 * 修  改: 
 *   时间 2014年03月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline void TcpConnReorder<Next, Succ>::Reinitialize()
{
	conn_closed_ = false;
	conn_info_.started = false;

	ReInitHalfConn(conn_info_.half_conn[PKT_DIR_S2B]);
	ReInitHalfConn(conn_info_.half_conn[PKT_DIR_B2S]);	
}

/*-----------------------------------------------------------------------------
 * 描  述: 重新初始化半连接信息
 * 参  数: [in] half_conn 半连接
 * 返回值: 
 * 修  改: 
 *   时间 2014年03月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline void TcpConnReorder<Next, Succ>::ReInitHalfConn(HalfConnInfo& half_conn)
{
	half_conn.next_ack_num = 0;
	half_conn.next_seq_num = 0;
	half_conn.sent_ack_num = 0;
	half_conn.sent_seq_num = 0;
	half_conn.send_pkt_num = 0;

	half_conn.cached_pkt.clear();
}

/*-----------------------------------------------------------------------------
 * 描  述: 消息处理
 * 参  数: [in] msg_type 消息类型
 *         [in] msg_data 消息数据
 * 返回值: 
 * 修  改: 
 *   时间 2014年03月24日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline ProcInfo TcpConnReorder<Next, Succ>::DoProcessMsg(MsgType msg_type, void* data)
{
	if (msg_type == MSG_REINITIALIZE)
	{
		Reinitialize();
	}

	// 继续传给Analyzer处理
	this->PassMsgToSuccProcessor(msg_type, data);

	return PI_HAS_PROCESSED;
}

/*-----------------------------------------------------------------------------
 * 描  述: 报文处理
 * 参  数: [in] pkt_msg 报文消息
 * 返回值: 
 * 修  改: 
 *   时间 2014年01月10日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline ProcInfo TcpConnReorder<Next, Succ>::DoProcessPkt(PktMsgSP& pkt_msg)
{
	boost::mutex::scoped_lock lock(reorder_mutex_);

	// 多线程场景，一个线程关闭了连接，其他线程还可能持有连接
	if (conn_closed_) return PI_HAS_PROCESSED;

	PktOrderType order_type = GetPktOrderType(pkt_msg);
	if (order_type == POT_IN_ORDER)
	{
		TryProcOrderedPkt(pkt_msg);
	}
	else if (order_type == POT_OUT_OF_ORDER)
	{
		AddUnorderedPkt(pkt_msg);
	}
	else if (order_type == POT_RETRANSMIT)
	{
		DLOG(WARNING) << "Tcp retransmission | seq: " << SEQ_NUM(pkt_msg);
	}
	else // POT_IGNORE
	{
		DLOG(WARNING) << "Ignored tcp packet | seq: " << SEQ_NUM(pkt_msg);
	}

	return PI_HAS_PROCESSED;
}

/*-----------------------------------------------------------------------------
 * 描  述: 对报文进行预处理，确定报文的状态，是否有序以及是否合法
 * 参  数: [in] pkt_msg 报文消息
 * 返回值: 报文状态
 * 修  改:
 *   时间 2014年01月27日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
typename TcpConnReorder<Next, Succ>::PktOrderType 
inline TcpConnReorder<Next, Succ>::GetPktOrderType(const PktMsgSP& pkt_msg)
{
	uint8 direction = pkt_msg->l3_pkt_info.direction;
	TMAS_ASSERT(direction < PKT_DIR_BUTT);

	HalfConnInfo& send_half_conn = conn_info_.half_conn[direction];

	// 连接的交互还未开始，则下一个期望的报文为三次握手的第一个报文
	if (!conn_info_.started)
	{
		if (IsFirstPktOf3WHS(pkt_msg))
		{
			return POT_IN_ORDER;
		}
		else
		{
			return POT_OUT_OF_ORDER;
		}
	}

	// 走到这说明三次握手第一个报文已经到达，如果发送端还没有发送过报文，
	// 则说明发送端是server端，下一个期望的报文应该是三次握手第二个报文
	if (send_half_conn.send_pkt_num == 0)
	{
		return IsSecondPktOf3WHS(pkt_msg) ? POT_IN_ORDER : POT_OUT_OF_ORDER;
	}

	//--- 走到这，说明三次握手的前两次报文交互都已经完成

	// TODO: 事实上，seq_num是可能后退的，比如在关闭nagle算法并发生重传时。
	// 一般来说，重传的新旧报文之间数据应该是一致的，但是也不能绝对保证，
	// 这里为了保持处理的简单，不处理重传报文，留待后续优化。
	if (SEQ_NUM(pkt_msg) < send_half_conn.next_seq_num)
	{
		// 纯ACK报文，如果乱序，并不会影响连接的重建
		if (L4_DATA_LEN(pkt_msg) == 0 && ACK_FLAG(pkt_msg) 
			&& !SYN_FLAG(pkt_msg) && !FIN_FLAG(pkt_msg) && !RST_FLAG(pkt_msg))
		{
			return POT_IGNORE;
		}
		else
		{
			return POT_RETRANSMIT;
		}
	}
	else if (SEQ_NUM(pkt_msg) > send_half_conn.next_seq_num)
	{
		return POT_OUT_OF_ORDER;
	}
	else // SEQ_NUM(pkt_msg) == send_conn.next_seq_num
	{
		if (ACK_NUM(pkt_msg) > send_half_conn.next_ack_num)
		{
			return POT_OUT_OF_ORDER;
		}
		else
		{
			return POT_IN_ORDER;
		}
	}
} 

/*-----------------------------------------------------------------------------
 * 描  述: 是否是三次握手的第一个报文。3WHS: 3 way handshake
 * 参  数: [in] pkt_msg 报文消息
 * 返回值: 是/否
 * 修  改:
 *   时间 2014年01月27日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline bool TcpConnReorder<Next, Succ>::IsFirstPktOf3WHS(const PktMsgSP& pkt_msg)
{
	return pkt_msg->l4_pkt_info.l4_data_len == 0
		&& pkt_msg->l4_pkt_info.syn_flag
		&& !pkt_msg->l4_pkt_info.ack_flag;
}

/*-----------------------------------------------------------------------------
 * 描  述: 是否是三次握手的第二个报文。3WHS: 3 way handshake
 * 参  数: [in] pkt_msg 报文消息
 * 返回值: 是/否
 * 修  改:
 *   时间 2014年01月27日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline bool TcpConnReorder<Next, Succ>::IsSecondPktOf3WHS(const PktMsgSP& pkt_msg)
{
	return pkt_msg->l4_pkt_info.l4_data_len == 0
		&& pkt_msg->l4_pkt_info.syn_flag
		&& pkt_msg->l4_pkt_info.ack_flag;
}

/*-----------------------------------------------------------------------------
 * 描  述: 添加乱序报文
 * 参  数: [in] conn_info 连接信息
 *         [in] pkt_msg 报文消息
 * 返回值: 成功/失败
 * 修  改:
 *   时间 2014年01月27日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline void TcpConnReorder<Next, Succ>::AddUnorderedPkt(const PktMsgSP& pkt_msg)
{
	DLOG(INFO) << "Add unordered tcp packet";

	TMAS_ASSERT(SEND_DIR < PKT_DIR_BUTT);
	HalfConnInfo& send_half_conn = conn_info_.half_conn[SEND_DIR];

	// 缓存这么多报文还未开始握手，删除连接
	if (!conn_info_.started 
		&& (GetTotalCachedPktNum() > max_cached_pkt_before_handshake_))
	{
		LOG(WARNING) << "Cached too many packets before handshake";
		return HandshakeFailed(pkt_msg);
	}
	
	// 将报文插入到缓存报文列表
	InsertUnorderedPktToList(pkt_msg, send_half_conn.cached_pkt);

	// 如果缓存的乱序报文超出限制（可能是丢包所致），此时需要人工干预
	// 连接的运行，通过人为修正连接状态参数，使得缓存报文变为有序。
	if (send_half_conn.cached_pkt.size() > max_cached_unordered_pkt_)
	{
		LOG(WARNING) << "Refactor tcp connection ";
		return RefactorTcpConn();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取总缓存报文个数
 * 参  数: 
 * 返回值: 缓存报文个数
 * 修  改:
 *   时间 2014年03月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline uint32 TcpConnReorder<Next, Succ>::GetTotalCachedPktNum()
{
	HalfConnInfo& s2b_half = conn_info_.half_conn[PKT_DIR_S2B];
	HalfConnInfo& b2s_half = conn_info_.half_conn[PKT_DIR_B2S];

	return s2b_half.cached_pkt.size() + b2s_half.cached_pkt.size();
}

/*-----------------------------------------------------------------------------
 * 描  述: 重构TCP连接。偶发性丢包可能导致TCP连接数据流中断，为了提高系统容错
 *         性，需要自动重构连接，避免偶发性丢包导致的连接中断。
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2014年03月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline void TcpConnReorder<Next, Succ>::RefactorTcpConn()
{
	MakeChacedPktOrdered();

	TryProcCachedPkt();
}

/*-----------------------------------------------------------------------------
 * 描  述: 重构TCP连接。偶发性丢包可能导致TCP连接数据流中断，为了提高系统容错
 *         性，需要自动重构连接，避免偶发性丢包导致的连接中断。
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2014年03月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline void TcpConnReorder<Next, Succ>::MakeChacedPktOrdered()
{
	HalfConnInfo& s2b_half = conn_info_.half_conn[PKT_DIR_S2B];
	HalfConnInfo& b2s_half = conn_info_.half_conn[PKT_DIR_B2S];

	if (!s2b_half.cached_pkt.empty())
	{
		if (s2b_half.next_seq_num < SEQ_NUM(s2b_half.cached_pkt.front()))
		{		
			LOG(WARNING) << "S2B missing tcp data length " 
				<< SEQ_NUM(s2b_half.cached_pkt.front()) - s2b_half.next_seq_num;

			s2b_half.next_seq_num = SEQ_NUM(s2b_half.cached_pkt.front());
		}
		
		if (ACK_NUM(s2b_half.cached_pkt.front()) > s2b_half.next_ack_num)
		{
			s2b_half.next_ack_num = ACK_NUM(s2b_half.cached_pkt.front());
		}
	}

	if (!b2s_half.cached_pkt.empty())
	{
		if (b2s_half.next_seq_num < SEQ_NUM(b2s_half.cached_pkt.front()))
		{
			LOG(WARNING) << "B2S missing tcp data length " 
				<< SEQ_NUM(b2s_half.cached_pkt.front()) - b2s_half.next_seq_num;

			b2s_half.next_seq_num = SEQ_NUM(b2s_half.cached_pkt.front());	
		}

		if (SEQ_NUM(b2s_half.cached_pkt.front()) > b2s_half.next_ack_num)
		{
			b2s_half.next_ack_num = SEQ_NUM(b2s_half.cached_pkt.front());
		}
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 将乱序报文插入到队列
 * 参  数: [in] pkt_msg 报文消息
 *         [out] pkt_list 报文队列
 * 返回值: 
 * 修  改:
 *   时间 2014年01月27日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline void TcpConnReorder<Next, Succ>::InsertUnorderedPktToList(
	const PktMsgSP& pkt_msg, PktMsgList& pkt_list)
{
	auto iter = pkt_list.begin();
	for (; iter != pkt_list.end(); ++iter)
	{
		// 首先根据发送序号排序
		if (SEQ_NUM(pkt_msg) < SEQ_NUM(*iter))
		{
			break;
		}
		else if (SEQ_NUM(pkt_msg) == SEQ_NUM(*iter))
		{
			// 发送序号相等，则根据确认序号排序
			if (ACK_NUM(pkt_msg) < ACK_NUM(*iter))
			{
				break;
			}
			else if (ACK_NUM(pkt_msg) == ACK_NUM(*iter))
			{
				// 如果两个报文都携带数据，且seq_num和ack_num相等，则肯定是重传。
				if ((L4_DATA_LEN(pkt_msg) > 0 && L4_DATA_LEN(*iter) > 0))
				{
					DLOG(WARNING) << "Unordered tcp retransmission with data | "
						<< (*iter)->l4_pkt_info << "->" << pkt_msg->l4_pkt_info;
					return;
				}
				// 如果都没携带数据，且FIN和SYN标志也相同，则也是属于重传。
				else if (L4_DATA_LEN(pkt_msg) == 0 && L4_DATA_LEN(*iter) == 0)
				{
					if (FIN_FLAG(pkt_msg) == FIN_FLAG(*iter) 
						&& SYN_FLAG(pkt_msg) == SYN_FLAG(*iter))
					{
						DLOG(WARNING) << "Unordered tcp retransmission without data | "
							<< (*iter)->l4_pkt_info << "->" << pkt_msg->l4_pkt_info;
						return;
					}
					else
					{
						// 未置标志位的排在前面(SYN和FIN只能置其一)
						if (!SYN_FLAG(pkt_msg) && !FIN_FLAG(pkt_msg))
						{
							break;
						}
					}
				}
				else // 走到这，则肯定有一个报文的长度为0，另一个报文长度不为0
				{
					// 首先按照数据长度排序，payload为0的报文排在前面
					if (L4_DATA_LEN(pkt_msg) < L4_DATA_LEN(*iter))
					{
						TMAS_ASSERT(L4_DATA_LEN(pkt_msg) == 0);
						break;
					}
					// 数据长度一致，则按照SYN/FIN标志位排序
					else
					{
						; // 否则，继续寻找发送号和确认号都相等的报文
					}
				}
			}
			else
			{
				; // 继续寻找发送号相等情况下，
				  // 第一个确认号大于当前确认号的报文
			}
		}
		else
		{
			; // 继续寻找第一个发送序号大于当前发送序号的报文位
		}
	}

	pkt_list.insert(iter, pkt_msg);
}

/*-----------------------------------------------------------------------------
 * 描  述: 处理有序报文，处理完后，看下缓存报文是否也是有序
 * 参  数: [in] pkt_msg 报文消息
 * 返回值: 
 * 修  改:
 *   时间 2014年01月27日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline void TcpConnReorder<Next, Succ>::TryProcOrderedPkt(PktMsgSP& pkt_msg)
{
	ProcessOrderedPkt(pkt_msg);

	TryProcCachedPkt();
}

/*-----------------------------------------------------------------------------
 * 描  述: 处理有序报文
 * 参  数: [in] pkt_msg 报文消息
 * 返回值: 
 * 修  改:
 *   时间 2014年01月27日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline void TcpConnReorder<Next, Succ>::ProcessOrderedPkt(PktMsgSP& pkt_msg)
{
	RefreshTcpConn(pkt_msg);

	// 将报文传递给TcpConnAnalyzer处理
	this->PassPktToSuccProcessor(pkt_msg);
}

/*-----------------------------------------------------------------------------
 * 描  述: 处理半连接的缓存报文
 * 参  数: [in][out] pkt_list 缓存报文队列
 *         [in][out] pkt_iter 报文在容器中位置
 *         [in][out] local_half_flag 本半连接控制标志
 *         [out] other_half_flag 另半连接控制标志
 * 返回值: 
 * 修  改:
 *   时间 2014年02月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline void TcpConnReorder<Next, Succ>::ProcHalfConnCachedPkt(PktMsgList& pkt_list, 
	PktMsgIter& pkt_iter, bool& local_half_flag, bool& other_half_flag)
{
	if (pkt_iter == pkt_list.end())
	{
		local_half_flag = false;
	}
	else
	{
		if (!ProcOneCachedPkt(pkt_list, pkt_iter))
		{
			local_half_flag = false;
		}
		else
		{
			// 本端处理完一个有序报文后，可能另一端
			// 缓存报文变得有序，需要唤醒另一端队列
			other_half_flag = true;
		}
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 处理缓存报文
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2014年01月27日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline void TcpConnReorder<Next, Succ>::TryProcCachedPkt()
{
	PktMsgList& s2b_list = conn_info_.half_conn[PKT_DIR_S2B].cached_pkt;
	PktMsgList& b2s_list = conn_info_.half_conn[PKT_DIR_B2S].cached_pkt;

	auto s2b_iter = s2b_list.begin(); 
	auto b2s_iter = b2s_list.begin();

	bool s2b_proc_flag = true;
	bool b2s_proc_flag = true;

	// 来回切换队列处理有序报文，如果两个队列都已经没有有序报文则退出
	while (s2b_proc_flag || b2s_proc_flag)
	{
		if (s2b_proc_flag)
		{
			ProcHalfConnCachedPkt(s2b_list, s2b_iter, s2b_proc_flag, b2s_proc_flag);
		}

		if (b2s_proc_flag)
		{
			ProcHalfConnCachedPkt(b2s_list, b2s_iter, b2s_proc_flag, s2b_proc_flag);
		}
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 处理一个缓存报文
 * 参  数: [in] pkt_list 报文列表
 *         [in][out] pkt_iter 报文游标
 * 返回值: true: 有序或者非法
 *         false: 无序
 * 修  改:
 *   时间 2014年01月27日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline bool TcpConnReorder<Next, Succ>::ProcOneCachedPkt(PktMsgList& pkt_list, PktMsgIter& pkt_iter)
{
	PktOrderType order_type = GetPktOrderType(*pkt_iter);
	if (order_type == POT_IN_ORDER)
	{
		ProcessOrderedPkt(*pkt_iter);

		pkt_iter = pkt_list.erase(pkt_iter);
	}
	else if (order_type == POT_OUT_OF_ORDER)
	{
		return false;
	}
	else // retransmit or ignore
	{
		DLOG(WARNING) << "Invalid cached packet | " << order_type;

		pkt_iter = pkt_list.erase(pkt_iter);
	}

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 刷新TCP连接相关信息
 * 参  数: [in] pkt_msg 报文消息
 * 返回值: 
 * 修  改:
 *   时间 2014年01月27日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline void TcpConnReorder<Next, Succ>::RefreshTcpConn(const PktMsgSP& pkt_msg)
{
	// 这里只会处理有序报文，无论如何都需要刷新此标志
	if (!conn_info_.started)
	{
		conn_info_.started = true;
	}

	uint8 direction = pkt_msg->l3_pkt_info.direction;
	TMAS_ASSERT(direction < PKT_DIR_BUTT);

	HalfConnInfo& send_half_conn = conn_info_.half_conn[direction];
	HalfConnInfo& recv_half_conn = conn_info_.half_conn[(direction + 1) % 2];

	// 更新发送端连接参数

	send_half_conn.sent_seq_num = pkt_msg->l4_pkt_info.seq_num;
	send_half_conn.sent_ack_num = pkt_msg->l4_pkt_info.ack_num;

	send_half_conn.next_seq_num = send_half_conn.sent_seq_num + L4_DATA_LEN(pkt_msg);
	
	// SYN和FIN会占用一个发送序号
	if (SYN_FLAG(pkt_msg) || FIN_FLAG(pkt_msg))
	{
		send_half_conn.next_seq_num++;
	}

	send_half_conn.send_pkt_num++;

	// 更新接收端连接参数

	recv_half_conn.next_ack_num = send_half_conn.next_seq_num;

	DLOG(INFO) << "TCP send half state | [" 
		       << send_half_conn.sent_seq_num 
			   << ":"
			   << send_half_conn.sent_ack_num
			   << "]["
			   << send_half_conn.next_seq_num
			   << ":"
			   << send_half_conn.next_ack_num
		       << "] | " 
			   << send_half_conn.send_pkt_num;
}

/*-----------------------------------------------------------------------------
 * 描  述: 握手失败处理
 * 参  数: [in] pkt_msg 报文消息
 * 返回值: 
 * 修  改:
 *   时间 2014年03月28日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
template<class Next, class Succ>
inline void TcpConnReorder<Next, Succ>::HandshakeFailed(const PktMsgSP& pkt_msg)
{
	// 握手失败后，连接会等待接收报文超时后才删除，但是，连接中缓存
	// 的报文可以不用等连接删除后再清除。
	conn_info_.half_conn[PKT_DIR_S2B].cached_pkt.clear();
	conn_info_.half_conn[PKT_DIR_B2S].cached_pkt.clear();

	tcp_monitor_->AbandonTcpConnection(pkt_msg->l4_pkt_info.conn_id);
}

}

#endif