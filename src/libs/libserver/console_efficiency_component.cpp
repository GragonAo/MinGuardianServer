#include "console_efficiency_component.h"
#include "message_system.h"
#include "global.h"
// 组件初始化
void ConsoleEfficiencyComponent::Awake()
{
    auto pMsgSystem = GetSystemManager()->GetMessageSystem();

    // 注册消息处理函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_Efficiency, BindFunP1(this, &ConsoleEfficiencyComponent::HandleEfficiency));
    pMsgSystem->RegisterFunction(this, Proto::MsgId::MI_CmdEfficiency, BindFunP1(this, &ConsoleEfficiencyComponent::HandleCmdEfficiency));
}

// 返回对象池时的清理工作
void ConsoleEfficiencyComponent::BackToPool()
{
    _threads.clear();
}

// 处理来自数据包的效率信息
void ConsoleEfficiencyComponent::HandleEfficiency(Packet* pPacket)
{
    auto proto = pPacket->ParseToProto<Proto::Efficiency>();
    auto threadId = proto.thread_id();
    auto iter = _threads.find(threadId);
    
    // 如果未找到线程信息，则初始化
    if (iter == _threads.end())
    {
        ThreadEfficiencyInfo info{};
        info.ThreadTypeKey = (ThreadType)proto.thread_type();
        _threads[threadId] = info;
    }

    auto time = proto.time();
    // 更新最大更新时间
    if (time > _threads[threadId].UpdateTimeMax)
    {
        _threads[threadId].UpdateTimeMax = time;
    }

    // 记录最后接收时间和当前更新时间
    _threads[threadId].LastRecvTime = Global::GetInstance()->TimeTick;
    _threads[threadId].UpdateTime = time;
}

// 处理效率命令并记录结果
void ConsoleEfficiencyComponent::HandleCmdEfficiency(Packet* pPacket)
{
    std::stringstream log;
    log << "\n*************************** " << "\n";
    log << "| id\t| type\t| avg time\t| max time\t| frame\t| log time";
    std::cout << log.str().c_str() << std::endl;

    for (const auto& pair : _threads)
    {
        log.clear();
        log.seekg(std::ios::beg);
        log.seekp(std::ios::beg);

        log << pair.first << "\t";
        log << GetThreadTypeName(pair.second.ThreadTypeKey) << "\t";
        log << pair.second.UpdateTime << "\t";
        log << pair.second.UpdateTimeMax << "\t";
        log << (pair.second.UpdateTime > 0 ? 1000 / pair.second.UpdateTime : 1000) << "\t";
        log << timeutil::ToString(pair.second.LastRecvTime);
        std::cout << log.str().c_str() << std::endl;
    }
}
