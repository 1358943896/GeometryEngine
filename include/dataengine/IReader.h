#pragma once

#include "geometry/Geometry.h"
#include <memory>
#include <vector>

namespace dataengine
{
    //! \brief 几何数据读取器抽象接口
    //! 所有具体读取器必须继承此接口
    class IReader
    {
    public:
        virtual ~IReader() = default;

        //! \brief 从数据源读取所有几何对象
        //! \return 几何对象列表（调用方拥有所有权）
        virtual std::vector<std::unique_ptr<geo::Geometry>> read() = 0;
    };

} // namespace dataengine
