#include "dataengine/WktFileReader.h"
#include "geometry/GeoPoint.h"
#include "geometry/GeoLine.h"
#include "geometry/GeoRegion.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace dataengine
{

// ── 内部 WKT 解析工具 ─────────────────────────────────────────────────────────

namespace
{

// 去除首尾空白
static std::string trim(const std::string &s)
{
    std::size_t a = s.find_first_not_of(" \t\r\n");
    std::size_t b = s.find_last_not_of(" \t\r\n");
    return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
}

// 解析 "x y" 为 Coord
static geo::Coord parseCoord(const std::string &s)
{
    std::istringstream ss(s);
    double x, y;
    ss >> x >> y;
    return {x, y};
}

// 解析括号内的坐标序列 "x1 y1, x2 y2, ..."
static geo::GeoLine::VertexList parseRing(const std::string &s)
{
    geo::GeoLine::VertexList verts;
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ','))
        verts.push_back(parseCoord(trim(token)));
    return verts;
}

// 提取最外层括号内的内容
static std::string innerContent(const std::string &s)
{
    std::size_t a = s.find('(');
    std::size_t b = s.rfind(')');
    if (a == std::string::npos || b == std::string::npos)
        throw std::runtime_error("WKT: missing parentheses in: " + s);
    return s.substr(a + 1, b - a - 1);
}

// 按顶层逗号分割（忽略括号内的逗号）
static std::vector<std::string> splitTopLevel(const std::string &s)
{
    std::vector<std::string> parts;
    int depth = 0;
    std::string cur;
    for (char c : s)
    {
        if      (c == '(') { ++depth; cur += c; }
        else if (c == ')') { --depth; cur += c; }
        else if (c == ',' && depth == 0)
        {
            parts.push_back(trim(cur));
            cur.clear();
        }
        else cur += c;
    }
    if (!trim(cur).empty()) parts.push_back(trim(cur));
    return parts;
}

// 解析单个 WKT 串
static std::unique_ptr<geo::Geometry> parseWkt(const std::string &wkt)
{
    std::string s = trim(wkt);
    // 转大写以便匹配类型关键字
    std::string upper(s.size(), ' ');
    std::transform(s.begin(), s.end(), upper.begin(), ::toupper);

    if (upper.rfind("MULTIPOLYGON", 0) == 0)
    {
        // MULTIPOLYGON (((x y,...),(x y,...)),((x y,...)))
        geo::GeoRegion::PartList parts;
        for (const auto &polyStr : splitTopLevel(innerContent(s)))
        {
            // 每个 polygon 取第一个环（外环）
            std::string ringStr = innerContent(trim(polyStr));
            auto rings = splitTopLevel(ringStr);
            if (!rings.empty())
                parts.push_back(parseRing(innerContent(rings[0])));
        }
        return std::make_unique<geo::GeoRegion>(std::move(parts));
    }
    else if (upper.rfind("POLYGON", 0) == 0)
    {
        // POLYGON ((x y,...),(x y,...)) — 取第一个环（外环）
        geo::GeoRegion::PartList parts;
        for (const auto &ringStr : splitTopLevel(innerContent(s)))
            parts.push_back(parseRing(innerContent(trim(ringStr))));
        return std::make_unique<geo::GeoRegion>(std::move(parts));
    }
    else if (upper.rfind("MULTILINESTRING", 0) == 0)
    {
        geo::GeoLine::PartList parts;
        for (const auto &lineStr : splitTopLevel(innerContent(s)))
            parts.push_back(parseRing(innerContent(trim(lineStr))));
        return std::make_unique<geo::GeoLine>(std::move(parts));
    }
    else if (upper.rfind("LINESTRING", 0) == 0)
    {
        return std::make_unique<geo::GeoLine>(parseRing(innerContent(s)));
    }
    else if (upper.rfind("MULTIPOINT", 0) == 0)
    {
        geo::GeoPoint::PartList coords;
        for (const auto &ptStr : splitTopLevel(innerContent(s)))
            coords.push_back(parseCoord(innerContent(trim(ptStr))));
        return std::make_unique<geo::GeoPoint>(std::move(coords));
    }
    else if (upper.rfind("POINT", 0) == 0)
    {
        return std::make_unique<geo::GeoPoint>(parseCoord(innerContent(s)));
    }
    throw std::runtime_error("WKT: unsupported geometry type: " + s);
}

} // anonymous namespace

// ── WktFileReader ─────────────────────────────────────────────────────────────

WktFileReader::WktFileReader(std::string filePath)
    : filePath_(std::move(filePath))
{
}

std::vector<std::unique_ptr<geo::Geometry>> WktFileReader::read()
{
    std::ifstream file(filePath_);
    if (!file.is_open())
        throw std::runtime_error("WktFileReader: cannot open file: " + filePath_);

    std::vector<std::unique_ptr<geo::Geometry>> result;
    std::string line;
    // 逐行读取，跳过空行和注释行（#）
    while (std::getline(file, line))
    {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        result.push_back(parseWkt(line));
    }
    return result;
}

} // namespace dataengine
