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
            printf("%s", color);
        }

        ~scope_output_color()
        {
            printf("\033[0m");
        }
    };

    inline std::string to_string(const component_type_index& index)
    {
        int off = std::string_view(index.name()).find("::");
        return std::string(index.name() + off + 2);
    }

    inline std::string to_string(const archetype_index& index)
    {
        std::string result;
        const auto& arch = index.get_info();
        for (size_t i = 0; i < arch.component_count(); i++)
        {
            const auto& comp = arch[i];
            result += to_string(comp);
            if (i != arch.component_count() - 1)
            {
                result += " ";
            }
        }
        return result;
    }

    inline std::string to_string(const query_condition& condition)
    {
        std::string result;
        if (!condition.all().empty())
        {
            result += "[";
            for (size_t i = 0; i < condition.all().size(); i++)
            {
                auto comp = condition.all()[i];
                result += to_string(comp);
                if (i == condition.all().size() - 1) break;
                result += " ";
            }
            result += "]";
        }
        if (!condition.anys().empty())
        {
            result += "( ";
            for (size_t i = 0; i < condition.anys().size(); i++)
            {
                auto& any = condition.anys()[i];
                result += "(";
                for (size_t j = 0; j < any.size(); j++)
                {
                    auto comp = any[j];
                    result += to_string(comp);
                    if (j == any.size() - 1) break;
                    result += "|";
                }
                result += ")";
                if (i == condition.anys().size() - 1) break;
                result += "|";
            }
            result += " )";
        }
        if (!condition.none().empty())
        {
            result += " - (";
            for (size_t i = 0; i < condition.none().size(); i++)
            {
                auto comp = condition.none()[i];
                result += to_string(comp);
                if (i == condition.none().size() - 1) break;
                result += " ";
            }
            result += ")";
        }
        return result;
    }
}
