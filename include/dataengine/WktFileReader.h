#pragma once

#include "dataengine/IReader.h"
#include <string>

namespace dataengine
{
    //! \brief 从文本文件读取几何数据，每行一个 WKT 串
    class WktFileReader : public IReader
    {
    public:
        //! \brief 构造函数
        //! \param filePath 文本文件路径
        explicit WktFileReader(std::string filePath);

        //! \brief 逐行解析 WKT，返回几何对象列表
        std::vector<std::unique_ptr<geo::Geometry>> read() override;

    private:
        std::string filePath_;
    };

} // namespace dataengine
