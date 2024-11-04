#include "mysql_connector.h"
#include "libserver/log4_help.h"
#include "libserver/message_system_help.h"
#include "libserver/message_system.h"
void MysqlConnector::InitMessageComponent()
{
    // 获取消息系统
   auto pMsgSystem = GetSystemManager()->GetMessageSystem();
    

    // 注册处理查询玩家列表消息的回调函数
    pMsgSystem->RegisterFunction(this,Proto::MsgId::L2DB_QueryPlayerList, BindFunP1(this, &MysqlConnector::HandleQueryPlayerList));
    
    // 注册处理创建玩家消息的回调函数
    pMsgSystem->RegisterFunction(this,Proto::MsgId::L2DB_CreatePlayer, BindFunP1(this, &MysqlConnector::HandleCreatePlayer));

    // 注册处理保存玩家消息的回调函数
    pMsgSystem->RegisterFunction(this,Proto::MsgId::G2DB_SavePlayer, BindFunP1(this, &MysqlConnector::HandleSavePlayer));
    
    // 注册处理查询玩家消息的回调函数
    pMsgSystem->RegisterFunction(this, Proto::MsgId::G2DB_QueryPlayer, BindFunP1(this, &MysqlConnector::HandleQueryPlayer));
}

void MysqlConnector::HandleQueryPlayerList(Packet* pPacket)
{
    // 解析从客户端发送的 QueryPlayerList 协议消息
    auto protoQuery = pPacket->ParseToProto<Proto::QueryPlayerList>();
    
    // 查询玩家列表
    QueryPlayerList(protoQuery.account(), pPacket);
}

void MysqlConnector::QueryPlayerList(std::string account, NetIdentify* pIdentify)
{
    my_ulonglong affected_rows;
    
    // 构造查询 SQL 语句，按账号查询玩家列表
    std::string sql = strutil::format("select sn, name, base, item, misc from player where account = '%s'", account.c_str());
    
    // 执行查询
    if (!Query(sql.c_str(), affected_rows))
    {
        // 如果查询失败，记录错误日志
        LOG_ERROR("!!! Failed. MysqlConnector::HandleQueryPlayerList. sql:" << sql.c_str());
        return;
    }

    // 构建返回的 PlayerList 协议消息
    Proto::PlayerList protoRs;
    protoRs.set_account(account.c_str());

    Proto::PlayerBase protoBase;
    if (affected_rows > 0)
    {
        std::string tempStr;
        MYSQL_ROW row;

        // 遍历查询结果
        while ((row = Fetch()))
        {
            // 将每个玩家信息添加到返回的 PlayerList 中
            auto pProtoPlayer = protoRs.add_player();
            pProtoPlayer->set_sn(GetUint64(row, 0));
            pProtoPlayer->set_name(GetString(row, 1));

            // 解析玩家的基础信息
            GetBlob(row, 2, tempStr);
            protoBase.ParseFromString(tempStr);
            pProtoPlayer->set_level(protoBase.level());
            pProtoPlayer->set_gender(protoBase.gender());
        }
    }

    // 发送玩家列表的查询结果回客户端
    MessageSystemHelp::SendPacket(Proto::MsgId::L2DB_QueryPlayerListRs, pIdentify, protoRs);
}

void MysqlConnector::HandleQueryPlayer(Packet* pPacket)
{
    // 解析从客户端发送的 QueryPlayer 协议消息
    auto protoQuery = pPacket->ParseToProto<Proto::QueryPlayer>();

    my_ulonglong affected_rows;
    
    // 构造查询 SQL 语句，按玩家唯一识别号（SN）查询
    std::string sql = strutil::format("select sn, name, account, base, item, misc from player where sn = %llu", protoQuery.player_sn());
    
    // 执行查询
    if (!Query(sql.c_str(), affected_rows))
    {
        // 如果查询失败，记录错误日志
        LOG_ERROR("!!! Failed. MysqlConnector::HandleQueryPlayer. sql:" << sql.c_str());
        return;
    }

    // 构建返回的 QueryPlayerRs 协议消息
    Proto::QueryPlayerRs protoRs;
    if (affected_rows > 0)
    {
        std::string tempStr;
        MYSQL_ROW row;

        // 获取查询结果并填充协议消息
        if ((row = Fetch()))
        {
            Proto::Player* pProtoPlayer = protoRs.mutable_player();
            pProtoPlayer->set_sn(GetUint64(row, 0));
            pProtoPlayer->set_name(GetString(row, 1));
            protoRs.set_account(GetString(row, 2));

            // 解析玩家基础信息
            GetBlob(row, 3, tempStr);
            pProtoPlayer->mutable_base()->ParseFromString(tempStr);

            // 解析玩家附加信息
            GetBlob(row, 5, tempStr);
            pProtoPlayer->mutable_misc()->ParseFromString(tempStr);
        }
    }

    // 发送查询玩家的结果回客户端
    MessageSystemHelp::SendPacket(Proto::MsgId::G2DB_QueryPlayerRs, pPacket, protoRs);
}

void MysqlConnector::HandleCreatePlayer(Packet* pPacket)
{
    // 解析从客户端发送的 CreatePlayerToDB 协议消息
    auto protoCreate = pPacket->ParseToProto<Proto::CreatePlayerToDB>();

    auto protoPlayer = protoCreate.player();

    // 获取创建玩家的语句对象
    DatabaseStmt* stmt = GetStmt(DatabaseStmtKey::StmtCreate);
    if (stmt == nullptr)
        return;

    // 记录创建玩家的日志
    LOG_INFO("HandlePlayerCreate sn:" << protoPlayer.sn() << " account:" << protoCreate.account().c_str() << " name:" << protoPlayer.name().c_str());

    // 清空语句中的参数
    ClearStmtParam(stmt);
    
    // 添加创建玩家的参数
    AddParamUint64(stmt, protoPlayer.sn());
    AddParamStr(stmt, protoCreate.account().c_str());
    AddParamStr(stmt, protoPlayer.name().c_str());

    // 构建返回的 CreatePlayerToDBRs 协议消息
    Proto::CreatePlayerToDBRs protoRs;
    protoRs.set_account(protoCreate.account().c_str());
    protoRs.set_return_code(Proto::CreatePlayerReturnCode::CPR_Unkonwn);

    // 执行创建玩家的语句
    if (ExecuteStmt(stmt))
    {
        // 创建成功，设置返回码
        protoRs.set_return_code(Proto::CreatePlayerReturnCode::CPR_Create_OK);
    }

    // 如果创建成功，查询玩家列表并返回
    if (protoRs.return_code() == Proto::CreatePlayerReturnCode::CPR_Create_OK)
    {
        QueryPlayerList(protoCreate.account(), pPacket);
    }
    else
    {
        // 创建失败，返回失败信息
        MessageSystemHelp::SendPacket(Proto::MsgId::L2DB_CreatePlayerRs, pPacket, protoRs);
    }
}

void MysqlConnector::HandleSavePlayer(Packet* pPacket)
{
    // 解析从客户端发送的 SavePlayer 协议消息
    auto proto = pPacket->ParseToProto<Proto::SavePlayer>();

    // 获取保存玩家的语句对象
    DatabaseStmt* stmt = GetStmt(DatabaseStmtKey::StmtSave);
    if (stmt == nullptr)
        return;

    // 记录保存玩家的日志
    //LOG_INFO("HandleSavePlayer sn:" << proto.player_sn());

    // 保存玩家信息
    Proto::Player protoPlayer = proto.player();
    OnSavePlayer(stmt, protoPlayer);
}

bool MysqlConnector::OnSavePlayer(DatabaseStmt* stmtSave, Proto::Player& protoPlayer)
{
    // 清空语句中的参数
    ClearStmtParam(stmtSave);

    // 序列化玩家基础信息为字符串
    std::string baseStr;
    protoPlayer.base().SerializeToString(&baseStr);

    // 添加玩家基础信息的 BLOB 参数
    AddParamBlob(stmtSave, (void*)baseStr.c_str(), (int)baseStr.size());

    // 序列化玩家附加信息为字符串
    std::string miscStr;
    protoPlayer.misc().SerializeToString(&miscStr);

    // 添加玩家附加信息的 BLOB 参数
    AddParamBlob(stmtSave, (void*)miscStr.c_str(), (int)miscStr.size());

    // 添加玩家的唯一识别号（SN）
    AddParamUint64(stmtSave, protoPlayer.sn());

    // 执行保存语句
    if (!ExecuteStmt(stmtSave))
    {
        // 如果保存失败，记录错误日志
        LOG_ERROR("save player failed. player sn:" << protoPlayer.sn());
        return false;
    }

    return true;
}
