#pragma once
#include "core/hyecs_core.h"
#include "ecs/query/query.h"

namespace hyecs
{
    struct scope_output_color
    {
        inline static const char* red = "\033[0;31m";
        inline static const char* green = "\033[0;32m";
        inline static const char* yellow = "\033[0;33m";
        inline static const char* blue = "\033[0;34m";
        inline static const char* magenta = "\033[0;35m";
        inline static const char* cyan = "\033[0;36m";
        inline static const char* white = "\033[0;37m";

        scope_output_color(const char* color)
        {
            std::cout << color;
        }
        ~scope_output_color()
        {
            std::cout << "\033[0m";
            std::cout.flush();
        }
    };

    inline std::ostream& operator<<(std::ostream& os, const component_type_index& index)
    {
        std::string_view name = index.name();
        os << name.substr(7);
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const archetype_index& index)
    {
        const auto& arch = index.get_info();
        for (size_t i = 0; i < arch.component_count(); i++)
        {
            const auto& comp = arch[i];
            os << comp;
            if (i != arch.component_count() - 1)
            {
                os << " ";
            }
        }
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const query_condition& condition)
    {
        if (!condition.all().empty())
        {
            os << "[";
            for (size_t i = 0; i < condition.all().size(); i++) {
                auto comp = condition.all()[i];
                os << comp;
                if (i == condition.all().size() - 1) break;
                os << " ";
            }
            os << "]";
        }
        if (!condition.anys().empty())
        {
            os << "( ";
            for (size_t i = 0; i < condition.anys().size(); i++)
            {
                auto& any = condition.anys()[i];
                os << "(";
                for (size_t j = 0; j < any.size(); j++)
                {
                    auto comp = any[j];
                    os << comp;
                    if (j == any.size() - 1) break;
                    os << "|";
                }
                os << ")";
                if (i == condition.anys().size() - 1) break;
                os << "|";
            }
            os << " )";
        }
        if (!condition.none().empty())
        {
            os << " - (";
            for (size_t i = 0; i < condition.none().size(); i++) {
                auto comp = condition.none()[i];
                os << comp;
                if (i == condition.none().size() - 1) break;
                os << " ";
            }
            os << ")";
        }
        return os;
    }


}