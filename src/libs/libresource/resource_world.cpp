#include "resource_world.h"
#include "libserver/log4_help.h"

// ResourceWorld 的构造函数，初始化基类 ResourceBase
ResourceWorld::ResourceWorld(std::map<std::string, int>& head) : ResourceBase(head)
{

}

// 获取世界的名称
std::string ResourceWorld::GetName() const
{
    return _name;
}

// 获取世界的类型
ResourceWorldType ResourceWorld::GetType() const
{
    return _worldType;
}

// 检查是否为初始化地图
bool ResourceWorld::IsInitMap() const
{
    return _isInit;
}

// 检查当前世界是否属于指定类型
bool ResourceWorld::IsType(ResourceWorldType iType) const
{
    return _worldType == iType;
}

// 获取初始化位置
Vector3 ResourceWorld::GetInitPosition() const
{
    return _initPosition;
}

// 生成资源结构体，解析头信息
void ResourceWorld::GenStruct()
{
    // 从头信息中提取资源的各个属性
    _name = GetString("name"); // 获取名称
    _isInit = GetBool("init"); // 获取是否初始化的标识
    _worldType = static_cast<ResourceWorldType>(GetInt("type")); // 获取类型

    // 获取初始化位置
    std::string value = GetString("playerinitpos");
    std::vector<std::string> params;
    strutil::split(value, ',', params); // 分割字符串
    if (params.size() == 3) // 确保有三个参数
    {
        _initPosition.X = std::stof(params[0]); // 转换为浮点数并赋值
        _initPosition.Y = std::stof(params[1]);
        _initPosition.Z = std::stof(params[2]);
    }
}

// 检查资源的有效性
bool ResourceWorld::Check()
{
    return true; // 目前始终返回 true，可能需要扩展检查逻辑
}

////////////////////////////////////////////////////////////////////////////////////////////////

// ResourceWorldMgr 的 AfterInit 方法，在初始化后执行
bool ResourceWorldMgr::AfterInit()
{
    auto iter = _refs.begin(); // 遍历所有资源
    while (iter != _refs.end())
    {
        auto pRef = iter->second;

        // 检查是否存在多个初始化地图
        if (pRef->IsInitMap())
        {
            if (_initMapId != 0)
            {
                LOG_ERROR("map has two init Map. id1:" << _initMapId << " id2:" << pRef->GetId());
            }
            _initMapId = pRef->GetId(); // 记录初始化地图的 ID
        }

        // 检查角色选择地图
        if (pRef->IsType(ResourceWorldType::Roles))
        {
            _rolesMapId = pRef->GetId();
        }

        ++iter; // 移动到下一个元素
    }

    return true;
}

// 获取初始化地图
ResourceWorld* ResourceWorldMgr::GetInitMap()
{
    if (_initMapId > 0) // 确保 ID 有效
        return GetResource(_initMapId); // 返回相应的资源

    return nullptr; // 无效则返回 nullptr
}

// 获取角色选择地图
ResourceWorld* ResourceWorldMgr::GetRolesMap()
{
    if (_rolesMapId > 0)
        return GetResource(_rolesMapId); // 返回角色选择地图资源

    return nullptr; // 无效则返回 nullptr
}
